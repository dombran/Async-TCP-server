/*
 * TcpServer_proc.cpp
 *
 *  Created on: Feb 8, 2023
 *      Author: dombran
 */

#include "TcpServer_proc.h"

#include <time.h>

#include "FileManager.h"

TcpServer_proc::TcpServer_proc(boost::asio::io_context&	m_Ios,
			 boost::asio::ip::tcp::endpoint &ep)	:m_ios(m_Ios),
			 ep_(ep),
			 m_strand(m_Ios){

	init_process();
}
TcpServer_proc::~TcpServer_proc(){
	AsyncStop();
}

void TcpServer_proc::AsyncStop(){

	m_strand.dispatch( std::bind(&TcpServer_proc::priAsyncStop, this)	);
}

// асинхронный стоп
void TcpServer_proc::priAsyncStop(){
	if (m_ios.stopped())
		return;

	for (auto it = begin(m_TCPSvcPnts); it != end(m_TCPSvcPnts); ++it)	{
		auto pServ = it->second;
		pServ->AsyncStop();
	}

}

void TcpServer_proc::init_process()	{
	try
	{
		auto pSrv = std::make_shared<TcpServerType>( m_ios );
		auto isOk=pSrv->LocalBind(ep_);

		if(!isOk)		{
			std::stringstream strm;
			strm<<"локальный биндинг к "<<ep_<<" неуспешен";
			auto str=strm.str();
			throw std::runtime_error(str);
		}

		pSrv->StartAccept();

		pSrv->m_OnNewConnection.connect(
			std::bind(&TcpServer_proc::OnNewIncTcpConnection, this, std::placeholders::_1)
		);


		m_TCPSvcPnts[ep_] = pSrv;


	} catch (const std::exception& ex) {
		syslog(LOG_ERR, "%s: Ошибка создания TCP-сервера - %s\n", BOOST_CURRENT_FUNCTION, ex.what());
	}

}

// прилетело входящее подключение
void TcpServer_proc::OnNewIncTcpConnection(shpTcpConnectionS2 pNewConn){

	m_strand.dispatch(std::bind(&TcpServer_proc::priOnNewIncTcpConnection, this, pNewConn)	);

}
// прилетело входящее подключение
void TcpServer_proc::priOnNewIncTcpConnection(shpTcpConnectionS2 pNewConn) {
	if ( m_ios.stopped() )
		return;

	auto addr = pNewConn->RemoteEndpoint().address();

	syslog(LOG_DEBUG, "Новое TCP-подключение от %s\n", addr.to_string().c_str());

	UpdateTcpConnection(pNewConn);

}

// обновление TCP-подключения
void TcpServer_proc::UpdateTcpConnection(shpTcpConnectionS2 pNewConn){
	boost::asio::dispatch(m_strand,
		std::bind(&TcpServer_proc::priUpdateTcpConnection, this, pNewConn)
	);
}
// обновление TCP-подключения
void TcpServer_proc::priUpdateTcpConnection(shpTcpConnectionS2 pNewConn) {
	if (m_ios.stopped())
		return;

	ForgotOldClient(pNewConn->LocalEndpoint().address());
//	gSignals.sgnlEth_sendResetParser();
	m_connect[pNewConn->RemoteEndpoint().address()] = pNewConn;

	pNewConn->m_OnDataReaded.connect(
		std::bind(&TcpServer_proc::OnTcpDataRecive, this, pNewConn, std::placeholders::_1)
	);

	pNewConn->m_OnDisconnected.connect(
		std::bind(&TcpServer_proc::OnConnectionBroken, this, pNewConn)
	);

	pNewConn->BeginAsyncRead();

	//auto addr = pNewConn->RemoteEndpoint().address();

	//	m_intfCli[addr] = (ETH_cli){newId(), time(0), CLIENT_TYPE::TCP, addr} ;
	// регистрируем новое соединение в интерфейсах
	//	gSignals.sgnl_newIntfConnect( m_intfCli[addr]);
}

// забыть старого клиента
void TcpServer_proc::ForgotOldClient(boost::asio::ip::address addr) {
	// забыть старые подключения

	if( m_connect.find(addr) != m_connect.end() ) {
		auto pTcpConn = boost::get<shpTcpConnectionS2>(m_connect[addr]);
		pTcpConn->Close();

		// так надо, потому что сигналы содержат в себе shared_pt на коннект
		// делая это мы уменьшаем счётчик ссылок и TcpConnectionS2 грохается
		pTcpConn->m_OnDataReaded.disconnect_all_slots();
		pTcpConn->m_OnDisconnected.disconnect_all_slots();
	}

	//m_connect[addr] = boost::blank{};
	//m_intfCli.erase(addr);
}

// по приёму данных
void TcpServer_proc::OnTcpDataRecive(shpTcpConnectionS2 pNewConn, const std::vector<uint8_t>& dp){
	boost::asio::dispatch(m_strand,
		std::bind(&TcpServer_proc::priOnTcpDataRecive, this, pNewConn, dp)
	);
}
// по приёму данных
void TcpServer_proc::priOnTcpDataRecive(shpTcpConnectionS2 pNewConn, const std::vector<uint8_t>& dp){
	//static const char* strFuncName = "priOnTcpDataRecive";

	if (m_ios.stopped())
		return;

	if(!IsOurTcpConnection(pNewConn))
		return;			// игнорить "летящие" остатки из закрытого ранее соединения

	uint32_t ui = decide_file(dp);

	std::string tmp = std::to_string(ui);
	std::vector<uint8_t> vec(tmp.begin(), tmp.end());
	vec.push_back('\n');

	Send(pNewConn, vec);

	//size_t Offset = 0;

	//auto it = m_connect.find(pNewConn->RemoteEndpoint().address());

	//if( m_intfCli.find(pNewConn->RemoteEndpoint().address()) == m_intfCli.end() )
	//	syslog(LOG_DEBUG, "Соединение TCP %s отсутствует \n", pNewConn->RemoteEndpoint().address().to_string().c_str());
	//else
	//	gSignals.sgnlItf_getLogicData(m_intfCli[pNewConn->RemoteEndpoint().address()], dt);
}

void TcpServer_proc::Send(shpTcpConnectionS2 pNewConn, const std::vector<uint8_t>& dt) {

	auto pConn = boost::get<shpTcpConnectionS2>(m_connect.find( pNewConn->RemoteEndpoint().address() )->second);

	pConn->Send(&dt[0], dt.size());
}


// когда связь оборвалась
void TcpServer_proc::OnConnectionBroken(shpTcpConnectionS2 pConn){

	boost::asio::dispatch(m_strand,
		std::bind(&TcpServer_proc::priOnConnectionBroken, this, pConn)
	);

}

// когда связь оборвалась
void TcpServer_proc::priOnConnectionBroken(shpTcpConnectionS2 pConn) {
	if (m_ios.stopped())
		return;

	if (IsOurTcpConnection(pConn))
		ForgotOldClient(pConn->LocalEndpoint().address());

	WhenLogicClientTcpConnClosed(pConn->LocalEndpoint().address());
}

bool TcpServer_proc::IsOurTcpConnection(shpTcpConnectionS2 pConn) {

	auto pOldConn = boost::get<shpTcpConnectionS2>(m_connect.find(pConn->RemoteEndpoint().address())->second);

	return pOldConn == pConn;
}

// когда у клиента отваливается TCP-канал связи
void TcpServer_proc::WhenLogicClientTcpConnClosed(const boost::asio::ip::address& addr) {
	boost::asio::dispatch(m_strand, std::bind(&TcpServer_proc::priWhenLogicClientTcpConnClosed, this, addr)
	);
}

// когда у клиента отваливается TCP-канал связи
void TcpServer_proc::priWhenLogicClientTcpConnClosed(const boost::asio::ip::address& addr) {
	if (m_ios.stopped())
		return;

}
// поиск записи с максимальной датой
// запись в протокол найденого поля и частного двух полей
// возвращает кол-во записей
// dd.mm.yyyy hh:mm:ss float float
uint32_t TcpServer_proc::decide_file(std::vector<uint8_t> vec){
	std::vector<std::string> v_str;

	if( vec.size() == 0 )
		return 0;

	// поиск всех строк
	uint32_t prev_0A = 0;
	for(uint32_t i=0; i<vec.size(); i++){
		if(vec[i] == 0x0A) {
			std::string str(&vec[prev_0A], &vec[i-1]);
			v_str.push_back(str);
			prev_0A = i+1;

			std::cout << str << std::endl;
		}
	}

	// поиск записи с максимальной датой
	std::string str_max;
	long int int_max = 0;
	for(std::string &it:v_str){

		std::vector<std::string> out;
		tokenize(it, ' ', out);

		std::string sss = out[0] + " " + out[1];
		auto i_max = string2timestamp(sss);

		if(i_max > int_max){
			int_max = i_max;
			str_max = it;
		}
	}

	// поиск частного
	std::vector<std::string> out;
	float fl = 0;
	tokenize(str_max, ' ', out);
	if( std::atof(out[3].c_str()) > 0 ) {
		fl = std::atof( out[2].c_str()) / std::atof(out[3].c_str());
	}else{ // деление на ноль

	}

	// запись в файл

	STATEErrorNotif->CreateNewRec_Ntf( out[0] + " " + out[1] + " : " + std::to_string(fl) );



	return v_str.size();
}

