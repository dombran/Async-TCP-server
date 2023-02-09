#pragma once

#include "stdafx.h"
		// TCP-сервер с разеляемым io_context
template<class ConnectionClass>
class TcpServerS2 : public std::enable_shared_from_this< TcpServerS2<ConnectionClass> >
{
	typedef std::weak_ptr< TcpServerS2<ConnectionClass> > wkpServerType;

	boost::asio::io_service&			m_ioc;
	boost::asio::io_service::strand		m_strand;

	// аксептилка подключений
	boost::asio::ip::tcp::acceptor		m_acceptor;

	// текущий сокет на подключение
	boost::asio::ip::tcp::socket		m_CurSocket;

	// флаг останова
	bool							m_bStopped;

	static void HandleAcceptInternal(wkpServerType wkp, const boost::system::error_code& error) {
		auto shpServer = wkp.lock();

		if (!shpServer)
			return;

		shpServer->HandleAccept(error);
	}

	void HandleAccept(const boost::system::error_code& error) {
		static const char* strFuncName = "TcpServerS2<T>::HandleAccept";

		if (m_ioc.stopped())
			return;

		if (m_bStopped)
			return;

		if (!error) {
			auto shpNewConn = std::make_shared<ConnectionClass>(m_ioc, std::move(m_CurSocket));
			m_OnNewConnection(shpNewConn);
		} else
			syslog(LOG_ERR, "%s, err=%s\n", strFuncName, error.message().c_str());

		StartAccept();
	}

	// запрос асинхронного останова
	void priAsyncStop() {
		if (m_ioc.stopped())
			return;

		CancelAccept();

		m_bStopped = true;
	}

public:

	// сигнал на поступление нового входящего подключения
	boost::signals2::signal<void(std::shared_ptr<ConnectionClass>)>	m_OnNewConnection;

	TcpServerS2(const TcpServerS2&) = delete;
	TcpServerS2(TcpServerS2&&) = default;

	// первым параметром идёт размер приёмного буфера для каждого подключения
	TcpServerS2(	boost::asio::io_service&
					ioc):	m_ioc(ioc),
							m_strand(m_ioc),
							m_acceptor(ioc),
							m_CurSocket(ioc),
							m_bStopped(false)	{ 	}
	~TcpServerS2() 	{ 	}

	TcpServerS2& operator=(const TcpServerS2&) = delete;
	TcpServerS2& operator=(TcpServerS2&&) = default;

	// локальный бинд
	// true если успешно
	bool LocalBind(const boost::asio::ip::tcp::endpoint& ep, bool bReuse = true) {
		static const char* strFuncName = "TcpServerS2<T>::LocalBind";

		try {
			m_acceptor.close();

			m_acceptor.open(ep.protocol());

			if (bReuse)
				m_acceptor.set_option(boost::asio::socket_base::reuse_address(true));

			m_acceptor.bind(ep);

			m_acceptor.listen();
		} catch (std::exception& ex) {
			syslog(LOG_ERR, "%s, error %s\n", strFuncName, ex.what());
			return false;
		}

		return true;
	}

	// начать принимать асинхронные подключения
	void StartAccept() {
		// за каким-то чёртом надо указать явно
		wkpServerType wkp = this->shared_from_this();


		m_acceptor.async_accept(m_CurSocket,
			m_strand.wrap(
				boost::bind(&TcpServerS2::HandleAcceptInternal, wkp, boost::asio::placeholders::error)
			)
		);

	}

	// запретить новые подключения
	void CancelAccept() {
		boost::system::error_code ec;
		m_acceptor.cancel(ec);
	}

	// выдаёт локальный адрес биндинга
	boost::asio::ip::tcp::endpoint GetLocalEndpoint() const {
		return m_acceptor.local_endpoint();
	}

	// запрос асинхронного останова
	void AsyncStop() 	{
		// без указания this, g++ ругается что не может найти эту функцию
		auto shp=this->shared_from_this();


		m_strand.dispatch(
			std::bind(&TcpServerS2::priAsyncStop, shp)
		);

	}

};


