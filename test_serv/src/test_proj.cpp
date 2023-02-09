//============================================================================
// Name        : test_proj.cpp
// Author      : bandackiy
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;

#include "test_proj.h"
#include "TcpServer_proc.h"
#include "FileManager.h"

#define NUM_THREADS 3
#define LINK_TO_FILE "../output.log"
#define BIND_TO_IP "127.0.0.1"


inline void handler(int sig) {
	void *array[20];
   int size, i;

      //STATEErrorNotif->SEN_WriteToFile(std::chrono::milliseconds(0));

   STATEErrorNotif->CreateNewRec_Ntf("!=== Custom StackTracer ===!");
   std::string str = StackTracer::i().getStackTrace();
   STATEErrorNotif->CreateNewRec_Ntf(str.c_str());
   STATEErrorNotif->CreateNewRec_Ntf("!=== Custom StackTracer END ===!");


   size = backtrace(array, 20);
   char **strings = backtrace_symbols(array, size);

   std::stringstream strstr;
   str = " Размер backtrace = " + std::to_string(size);
   STATEErrorNotif->CreateNewRec_Ntf(str.c_str());
   strstr << "Signal Number = " << sig << "   Backtrace: " << '\n';
   for (i = 0; i < size; i++) {
	   strstr << "| "<< strings[i] << "0x" << std::hex << *(int*)(array[i]) << '\n';
   }
   STATEErrorNotif->CreateNewRec_Ntf(strstr.str().c_str());

   free(strings);

   exit(EXIT_SUCCESS);
}

int main() {

	signal(SIGABRT, handler);/* Abnormal termination.  */
	signal(SIGSEGV, handler);/* Invalid access to storage.  */
	signal(SIGILL, handler);/* Illegal instruction.  */
	signal(SIGFPE, handler);/* Erroneous arithmetic operation.  */
	signal(SIGKILL, handler);/* Interactive attention signal.  */
	signal(SIGINT, handler);
	signal(SIGTERM, handler);/* Termination request.  */

	MainClass mc;

	STATEErrorNotif = new STATEErrNotif(LINK_TO_FILE, mc.get_io_context());

	STATEErrorNotif->CreateNewRec_Ntf("<=== Custom StackTracer ===>");

	boost::system::error_code ec;
	boost::asio::ip::address ip_address = boost::asio::ip::address::from_string(BIND_TO_IP, ec);

	boost::asio::ip::tcp::endpoint ep( ip_address, 8080 );
	TcpServer_proc tc_p(mc.get_io_context(), ep);


	mc.WaitStopped();
	return 0;
}


MainClass::MainClass():m_WorkerThread(m_Ios) {

	TimerFunc(std::chrono::milliseconds(1000));

	for(size_t i=0; i < NUM_THREADS; ++i) // +1 поток из-за spi_sync
		m_thrdGr.create_thread(	std::bind(&MainClass::WorkThread, this, m_thrdGr.size())	);

}

MainClass::~MainClass() {
	m_Ios.stop();
	m_thrdGr.join_all();
}

void MainClass::WorkThread(size_t Number){
	std::string strFuncName = "BKK_Main::WorkThread";

	boost::system::error_code ec;

	while (true) {
		try {
			m_Ios.run(ec);

			if (ec)
				syslog(LOG_CRIT, "%s, %s\n", strFuncName.c_str(), ec.message().c_str() );

			break;
		} catch (std::exception& ex) {
			syslog(LOG_CRIT, "%s, %s\n", strFuncName.c_str(), ex.what() );
		}
	}
}

void MainClass::WaitStopped()	{
	m_thrdGr.join_all();
}

boost::asio::io_service& MainClass::get_io_context() {
	return m_Ios;
}
void MainClass::StartSecFunc(const boost::system::error_code& error){
	TimerFunc(std::chrono::milliseconds(1000));
}


