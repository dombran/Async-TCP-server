/*
 * test_proj.h
 *
 *  Created on: Feb 7, 2023
 *      Author: dombran
 */

#ifndef TEST_PROJ_H_
#define TEST_PROJ_H_

#include "stdafx.h"

class MainClass{
	boost::thread_group				m_thrdGr;
	boost::asio::io_service			m_Ios;
	boost::asio::steady_timer		m_WorkerThread;

	void WorkThread(size_t Number);

	template<class Rep, class Period>
	void TimerFunc(const std::chrono::duration<Rep, Period>& dur) {
		m_WorkerThread.expires_from_now(dur);
		m_WorkerThread.async_wait(boost::bind(&MainClass::StartSecFunc, this, boost::asio::placeholders::error));
	}

	void StartSecFunc(const boost::system::error_code& error);

public:

	MainClass();
	~MainClass();

	void WaitStopped();

	boost::asio::io_service& get_io_context();

};

#endif /* TEST_PROJ_H_ */
