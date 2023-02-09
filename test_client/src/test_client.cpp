//============================================================================
// Name        : test_client.cpp
// Author      : dombran
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "stdafx.h"
#include "StackTracer.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/write.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include <thread>
#include <chrono>
#include <fstream>

#include <boost/thread.hpp>

using boost::asio::ip::tcp;

//#define link_to_file "../f_read"
#define LINK_TO_FILE "../output.log"
#define HOST "127.0.0.1"
#define PORT "8080"

class client {
public:
  void connect(std::string host, std::string port,//boost::asio::ip::tcp::endpoint& ep,
      std::chrono::steady_clock::duration timeout)  {

    auto endpoints = tcp::resolver(io_context_).resolve(host, port);

    boost::system::error_code error;
    boost::asio::async_connect(socket_, endpoints,
        [&](const boost::system::error_code& result_error,
            const tcp::endpoint& /*result_endpoint*/)
        {
          error = result_error;
        });


    run(timeout);

    if (error)
      throw std::system_error(error);
  }

  std::string read_line(std::chrono::steady_clock::duration timeout)  {

    boost::system::error_code error;
    std::size_t n = 0;
    boost::asio::async_read_until(socket_,
        boost::asio::dynamic_buffer(input_buffer_), '\n',
        [&](const boost::system::error_code& result_error,
            std::size_t result_n)
        {
          error = result_error;
          n = result_n;
        });

    run(timeout);

    if (error)
      throw std::system_error(error);

    std::string line(input_buffer_.substr(0, n - 1));
    input_buffer_.erase(0, n);
    return line;
  }

  void write_line(std::vector<uint8_t> sv,
      std::chrono::steady_clock::duration timeout)  {

    boost::system::error_code error;
    boost::asio::async_write(socket_, boost::asio::buffer(sv, sv.size()),
        [&](const boost::system::error_code& result_error,
            std::size_t /*result_n*/)
        {
          error = result_error;
        });

    run(timeout);


    if (error)
      throw std::system_error(error);
  }

private:
  void run(std::chrono::steady_clock::duration timeout)  {

    io_context_.restart();

    io_context_.run_for(timeout);

    if (!io_context_.stopped())
    {
      socket_.close();

      io_context_.run();
    }
  }

  boost::asio::io_context io_context_;
  tcp::socket socket_{io_context_};
  std::string input_buffer_;
};

void WriteToFile_Ntf(std::string link, std::string message) {

		std::ofstream ofs;
		ofs.open (link, std::ofstream::out | std::ofstream::app);

		ofs << message << std::endl;
		ofs.close();

}

inline void handler(int sig) {
	void *array[20];
   int size, i;

      //STATEErrorNotif->SEN_WriteToFile(std::chrono::milliseconds(0));

   WriteToFile_Ntf(LINK_TO_FILE, "!=== Custom StackTracer ===!");
   std::string str = StackTracer::i().getStackTrace();
   WriteToFile_Ntf(LINK_TO_FILE,str.c_str());
   WriteToFile_Ntf(LINK_TO_FILE,"!=== Custom StackTracer END ===!");


   size = backtrace(array, 20);
   char **strings = backtrace_symbols(array, size);

   std::stringstream strstr;
   str = " Размер backtrace = " + std::to_string(size);
   WriteToFile_Ntf(LINK_TO_FILE,str.c_str());
   strstr << "Signal Number = " << sig << "   Backtrace: " << '\n';
   for (i = 0; i < size; i++) {
	   strstr << "| "<< strings[i] << "0x" << std::hex << *(int*)(array[i]) << '\n';
   }
   WriteToFile_Ntf(LINK_TO_FILE,strstr.str().c_str());

   free(strings);

   exit(EXIT_SUCCESS);
}

int main( int argc, char* argv[] ) {
	signal(SIGABRT, handler);/* Abnormal termination.  */
	signal(SIGSEGV, handler);/* Invalid access to storage.  */
	signal(SIGILL, handler);/* Illegal instruction.  */
	signal(SIGFPE, handler);/* Erroneous arithmetic operation.  */
	signal(SIGKILL, handler);/* Interactive attention signal.  */
	signal(SIGINT, handler);
	signal(SIGTERM, handler);/* Termination request.  */

	  try
	  {
	    //if (argc < 2)
	    //{
	    //  std::cerr << "Usage: file name for sending <link to file>\n";
	    //  return -1;
	    //}

	    std::ifstream file(argv[1], std::ios::in | std::ios::binary);

		if(!file.is_open()){
			std::cerr << "File not found. \n";
			return -2;
		}

		std::vector<uint8_t> array((std::istreambuf_iterator<char>(file)),
								std::istreambuf_iterator<char>());


	    boost::asio::io_context io_context;
	    //boost::asio::ip::tcp::endpoint ep( boost::asio::ip::address::from_string("127.0.0.1"), 8080);

	    client c;
	    c.connect(HOST, PORT, std::chrono::seconds(10));

	    auto time_sent = std::chrono::steady_clock::now();
        c.write_line(array, std::chrono::seconds(10));


        //std::this_thread::sleep_for (std::chrono::seconds(1));


        std::string line = c.read_line(std::chrono::seconds(1));

        auto time_received = std::chrono::steady_clock::now();

        std::cout << "Return num: " << line << std::endl;

	    file.close();
	  }
	  catch (std::exception& e)
	  {
	    std::cerr << "Exception: " << e.what() << "\n";
	  }

	return 0;
}



