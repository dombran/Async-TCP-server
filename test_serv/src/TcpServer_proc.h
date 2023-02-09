/*
 * TcpServer_proc.h
 *
 *  Created on: Feb 7, 2023
 *      Author: dombran
 */

#ifndef TCPSERVER_PROC_H_
#define TCPSERVER_PROC_H_

#include "stdafx.h"

#include "TcpServerS2.h"
#include "TcpConnectionS2.h"

class TcpServer_proc{
	boost::asio::io_context&	m_ios;
	boost::asio::ip::tcp::endpoint& ep_;

	boost::asio::io_context::strand m_strand;

	typedef TcpServerS2<TcpConnectionS2> TcpServerType;
	std::map<boost::asio::ip::tcp::endpoint, std::shared_ptr<TcpServerType> > 	m_TCPSvcPnts;

	std::map<boost::asio::ip::address, shpTcpConnectionS2> m_connect;

	void AsyncStop();
	void priAsyncStop();

	void init_process();

	void OnNewIncTcpConnection(shpTcpConnectionS2 pNewConn);
	void priOnNewIncTcpConnection(shpTcpConnectionS2 pNewConn);

	void UpdateTcpConnection(shpTcpConnectionS2 pNewConn);
	void priUpdateTcpConnection(shpTcpConnectionS2 pNewConn);

	void OnTcpDataRecive(shpTcpConnectionS2 pNewConn, const std::vector<uint8_t>& dp);
	void priOnTcpDataRecive(shpTcpConnectionS2 pNewConn, const std::vector<uint8_t>& dp);

	void OnConnectionBroken(shpTcpConnectionS2 pConn);
	void priOnConnectionBroken(shpTcpConnectionS2 pConn);

	void ForgotOldClient(boost::asio::ip::address addr);
	bool IsOurTcpConnection(shpTcpConnectionS2 pConn);

	void WhenLogicClientTcpConnClosed(const boost::asio::ip::address& addr);
	void priWhenLogicClientTcpConnClosed(const boost::asio::ip::address& addr);

	void Send(shpTcpConnectionS2 pNewConn, const std::vector<uint8_t>& dt);

	uint32_t decide_file(std::vector<uint8_t> vec);

	void tokenize(std::string str, const char delim, std::vector<std::string> &out) {
	    // строим поток из строки
	    std::stringstream ss(str);

	    std::string s;
	    while (std::getline(ss, s, delim)) {
	        out.push_back(s);
	    }
	}
	// dd.mm.yyyy hh:mm:ss float float
	long int string2timestamp(std::string &str) {
	    //time_t t;
	    struct tm tm;
	    setlocale(LC_ALL,"/QSYS.LIB/EN_US.LOCALE");
	    if(strptime(str.c_str(), "%d.%m.%Y %H:%M:%S",&tm) == NULL)
	          return -1;

	    return mktime(&tm);
	}
public:

	TcpServer_proc(boost::asio::io_context&	m_Ios, boost::asio::ip::tcp::endpoint &ep);
	~TcpServer_proc();

};




#endif /* TCPSERVER_PROC_H_ */
