#pragma once

#include "DataPortion.h"
#include "DataPortionT.h"

		// подключение по TCP
		// с разделяемым io_context и потоком соответственно
		// с возможностью уведомления о том, что конкретный пакет был уложен в очередь отправки
		// служит для индикации того что конкретный блок данных "ушёл"
		// это конечно плохой критерий, но уж лучше такой чем никакой вовсе
		template<class SendNotifyType>
		class TcpConnectionT : public std::enable_shared_from_this< TcpConnectionT<SendNotifyType> >
		{
			typedef TcpConnectionT<SendNotifyType> Self;

			typedef std::shared_ptr<Self> shpTcpConnection;
			typedef std::weak_ptr<Self> wkpTcpConnection;

			typedef DataPortionT<SendNotifyType> CustomDataPortionType;

			enum
			{
				// нет правильного дефолтного значения. правильным может быть как:
				// 64k (max IP packet size)-sizeof(TCPheader) так и 
				// MTU - sizeof(TCPheader), либо что-то другое
				RequiredBufferSize = 1500
			};

			// для удобства

			boost::asio::io_service&

				m_ioc;

			boost::asio::ip::tcp::socket		m_socket;

			// Если нафантазировать то будет всё очень печально.
			// Даже один поток - поставщик порций данных может накидать несколько задач сюда
			// и не факт что они будут выполнены последовательно.
			// технически операции рид/врайт независимы - можно повесить на разные странды.
			// да только вот практика показывает что close желательно делать в едином контексте с операциями read/write

			boost::asio::io_service::strand

				m_strand;

			// размер буфера приёма
			size_t								m_InBufSize;

			// буфер приёма
			boost::asio::streambuf				m_InBuff;

			// пакеты для отправки
			std::deque<CustomDataPortionType>	m_OutMessages;

			// флаг подключения
			bool								m_bConnected;

			// обработка записи данных
			static void HandleSendInternal(std::weak_ptr<TcpConnectionT> wkp, const boost::system::error_code& error)
			{
				auto shpConn = wkp.lock();

				if (!shpConn)
					return;

				shpConn->HandleSend(error);
			}

			// обработка записи данных
			void HandleSend(const boost::system::error_code& error)
			{
				std::string strFuncName = "TcpConnectionT::HandleSend";

				if (error == boost::asio::error::operation_aborted)
					return;

				if (m_ioc.stopped())
					return;

				if (error)
				{
					const auto remEp = RemoteEndpoint();

					syslog(LOG_ERR, "%s, remEp=%s. error=%s\n", strFuncName.c_str()
															, remEp.address().to_string().c_str()
															, error.message().c_str());

					if (!m_OutMessages.empty())
						ErrorSendPrimitive();

					return;
				}

				if (!m_bConnected)
					return;

				// удалять по условию не-пустоты. Да-да. Бывает так что данные успевают уйти. Затем возникает сбой. Вызывается функция реконнекта в которой грохается дека.
				// Затем вызывается этот хэндл врайт и пытается удалить элемент из пустой деки
				if (!m_OutMessages.empty())
					GoodSendPrimitive();

				// write next message

				if (m_OutMessages.empty())
					return;

				rawSend();
			}

			// выполняется в контексте strand
			void priSend(const CustomDataPortionType& dp)
			{
				if (m_ioc.stopped())
					return;

				if (!m_bConnected)
					return;

				// готов к отправке
				bool bSendReady = m_OutMessages.empty();

				m_OutMessages.push_back(std::move(dp));

				if (!bSendReady)
					return;

				rawSend();
			}

			// голая отправка данных
			void rawSend()
			{
				wkpTcpConnection wkp = Self::shared_from_this();

				// первый элемент в очереди
				const auto& first = m_OutMessages.front();

				// используем async_write вместо async_send. async_send может не передать все данные за раз

				boost::asio::async_write(m_socket, first.GetBuffer(),
					m_strand.wrap(
						boost::bind(&TcpConnectionT::HandleSendInternal, wkp, boost::asio::placeholders::error)
						)
					);

			}

			// обработка чтения данных
			static void HandleReadInternal(wkpTcpConnection wkp, const boost::system::error_code& error,
				size_t BytesTransferred)
			{
				auto shpConn = wkp.lock();

				if (!shpConn)
					return;

				shpConn->HandleRead(error, BytesTransferred);
			}

			// обработка чтения данных
			void HandleRead(const boost::system::error_code& error, size_t BytesTransferred)
			{
				std::string strFuncName = "TcpConnectionT::HandleRead";

				if (error == boost::asio::error::operation_aborted)
					return;

				if (m_ioc.stopped())
					return;

				if (!error)
				{
					m_InBuff.commit(BytesTransferred);

					std::vector<uint8_t> target(m_InBuff.size());
					boost::asio::buffer_copy(boost::asio::buffer(target), m_InBuff.data());

					m_InBuff.consume(BytesTransferred);

					// wait for the next message
					BeginAsyncRead();

					m_OnDataReaded(target);
				}
				else
				{
					const auto remEp = RemoteEndpoint();

					syslog(LOG_ERR, "%s, rem=%s, err=%s\n", strFuncName.c_str(), remEp.address().to_string().c_str(), error.message().c_str());

					m_bConnected = false;

					while (!m_OutMessages.empty())
						ErrorSendPrimitive();

					m_OnDisconnected(error);
				}
			}

			// обработка подключения
			static void HandleConnectInternal(wkpTcpConnection wkp, const boost::asio::ip::tcp::endpoint& remEp, const boost::system::error_code& error)
			{
				auto shpConn = wkp.lock();

				if (!shpConn)
					return;

				shpConn->HandleConnect(remEp, error);
			}

			// обработка подключения
			void HandleConnect(const boost::asio::ip::tcp::endpoint& remEp, const boost::system::error_code& error)
			{
				// даже тут надо обрабатывать эту ошибку
				if (error == boost::asio::error::operation_aborted)
					return;

				if (m_ioc.stopped())
					return;

				//m_log.Trace(LOG_ERR, "Handle connect\n");

				static const char* strFuncName = "TcpConnectionT::HandleConnect";

				if (error)
				{
					syslog(LOG_ERR, "%s, remEp=%s, error: %s\n", strFuncName, remEp.address().to_string(), error.message());
					m_OnNotConnected(error);
					return;
				}

				m_bConnected = true;

				m_OnConnected();
			}

			// примитив ошибочной отправки
			void ErrorSendPrimitive()
			{
				const auto& fstEl = m_OutMessages.front();

				auto opt = fstEl.GetOptMark();

				if (opt)
				{
					m_OnDataErrorSend(*opt);
				}

				m_OutMessages.pop_front();
			}

			// примитив удачной отправки
			void GoodSendPrimitive()
			{
				const auto& fstEl= m_OutMessages.front();

				auto opt = fstEl.GetOptMark();

				if (opt)
				{
					m_OnDataSended(*opt);
				}

				m_OutMessages.pop_front();
			}

			// закрыть соединение
			void priClose()
			{
				//if (m_ioc.stopped())
				//	return;

				//m_log.Trace(LOG_ERR, "Close\n");

				boost::system::error_code ec;
				m_socket.cancel(ec);
				m_socket.close();
			}

		public:

			// установлено подключение
			boost::signals2::signal<void()>	m_OnConnected;

			// подключение разорвано
			boost::signals2::signal<void(const boost::system::error_code&)> m_OnDisconnected;

			// подключение Не было установлено
			boost::signals2::signal<void(const boost::system::error_code&)> m_OnNotConnected;

			// сигнал прочитанных данных
			boost::signals2::signal<void(const std::vector<uint8_t>&)>	m_OnDataReaded;

			// данные поступили на обработку (легли во внутренний буфер ОС)
			// очень грубый признак того, что данные наверно отправились
			boost::signals2::signal<void(const SendNotifyType&)>	m_OnDataSended;

			// возникла ошибка в трансляции данных
			boost::signals2::signal<void(const SendNotifyType&)>	m_OnDataErrorSend;



			// специальный изъёб с move-семантикой
			// для вызова на стороне сервера
			TcpConnectionT(

				boost::asio::io_service&

				ioc, boost::asio::ip::tcp::socket&& mvsock) :
				m_ioc(ioc),
				m_socket(std::move(mvsock)),

				m_strand(m_ioc),

				m_InBufSize(RequiredBufferSize),
				m_bConnected(true)
			{
				static const char* strFuncName = "TcpConnectionS2::TcpConnectionS2";

				syslog(LOG_DEBUG, "%s\n", strFuncName);
			}

			// клиентский конструктор
			TcpConnectionT(

				boost::asio::io_service&

				ioc):
				m_ioc(ioc),
				m_socket(m_ioc),


				m_strand(m_ioc),

				m_InBufSize(RequiredBufferSize),
				m_bConnected(false)
			{
			}

			~TcpConnectionT()
			{
				static const char* strFuncName = "TcpConnectionS2::~TcpConnectionS2";

				priClose();

				syslog(LOG_DEBUG, "%s\n", strFuncName);
			}

			// начало асинхронного чтения
			void BeginAsyncRead()
			{
				//m_log.Trace(LOG_ERR, "BAR\n");

				const auto buf = m_InBuff.prepare(m_InBufSize);

				wkpTcpConnection wkp = Self::shared_from_this();

				m_socket.async_read_some(
					buf,
					boost::bind(&TcpConnectionT::HandleReadInternal, wkp, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
					);
			}

			// закрыть соединение (асинхронная операция)
			void Close()
			{

				m_strand.dispatch(
					std::bind(&TcpConnectionT::priClose, this)
					);

			}

			void Send(const CustomDataPortionType& dp)
			{

				m_strand.dispatch(
					std::bind(&TcpConnectionT::priSend, Self::shared_from_this(), dp)
					);

			}

			// эта функция безопасна в многопоточной среде, т.к. содержимое pData длиной Lenth копируется в момент вызова
			void Send(const void* pData, size_t Lenth, boost::optional<SendNotifyType> opt = boost::none)
			{
				Send(CustomDataPortionType(pData, Lenth, opt));
			}

			void Send(const std::string& str, boost::optional<SendNotifyType> opt = boost::none)
			{
				CustomDataPortionType dp(str.data(), str.size(), opt);

				Send(dp);
			}

			// true - успешно назначена локальная точка подключения
			bool AssignLocalEndpoint(const boost::asio::ip::tcp::endpoint& ep)
			{
				try
				{
					m_socket.close();
					m_socket.open(ep.protocol());
					m_socket.bind(ep);
				}
				catch (const std::exception& ex)
				{
					syslog(LOG_ERR, "%s: socket manipulation error. what: %s\n", BOOST_CURRENT_FUNCTION, ex.what());
					return false;
				}

				return true;
			}

			// асинхронное подключение к удалённому хосту
			void AsyncConnect(const boost::asio::ip::tcp::endpoint& remEp)
			{
				wkpTcpConnection wkp = Self::shared_from_this();

				// try to reconnect, then call handle_connect
				m_socket.async_connect(remEp,
					boost::bind(&TcpConnectionT::HandleConnectInternal, wkp, remEp, boost::asio::placeholders::error));
			}

			// получить локальную точку подключения 
			boost::asio::ip::tcp::endpoint LocalEndpoint() const
			{
				return m_socket.local_endpoint();
			}

			// получить удалённую точку подключения 
			boost::asio::ip::tcp::endpoint RemoteEndpoint() const
			{
				return m_socket.remote_endpoint();
			}

			// переинициализировать сокет
			// true - успешно
			bool ReinitSocket(const boost::asio::ip::tcp::endpoint::protocol_type& protType)
			{
				try
				{
					m_socket.close();
					m_socket.open(protType);
				}
				catch (const std::exception& ex)
				{
					syslog(LOG_ERR, "%s: socket manipulation error. what: %s\n", BOOST_CURRENT_FUNCTION, ex.what());
					return false;
				}

				return true;
			}
		};

