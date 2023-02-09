#pragma once

#include "TcpConnectionT.h"

		// подключение по TCP
		// с разделяемым io_context и потоком соответственно

		typedef TcpConnectionT<boost::blank> TcpConnectionS2;

		typedef std::shared_ptr<TcpConnectionS2> shpTcpConnectionS2;
		typedef std::weak_ptr<TcpConnectionS2> wkpTcpConnectionS2;

