/*
 * FileManeger.h
 *
 *  Created on: Apr 26, 2019
 *      Author: dombran
 */

#ifndef FILEMANAGER_H_
#define FILEMANAGER_H_



#include "stdafx.h"
#include "StackTracer.h"
#include <vector>
#include <algorithm>
#include <fstream>

#include <map>
#include <chrono>
#include <list>
#include <string>

#include <unistd.h>


	class STATEErrNotif {
	public:

		boost::asio::io_context			&m_ioc;
		boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
		std::string linkErrNtf;

		//boost::signals2::signal<void()> sgnl_handler_StoppeingProcess;


		STATEErrNotif (const std::string linkEN, boost::asio::io_context	&m_Ioc);
		~STATEErrNotif ();


		void priCreateNewRecord_Err(std::string message);
		void CreateNewRec_Err(std::string message);/*===============*/

		void priCreateNewRecord_Ntf(std::string message);
		void CreateNewRec_Ntf(std::string message);/*===============*/

		void bStop_ioc();
		void pri_bStop_ioc();

		std::string getThisFileName();

		void WriteToFile_Err(std::string message);
		void WriteToFile_Ntf(std::string message);

		std::string getThisTimeString();

	protected:
			std::string createNameFile_err(int tm_mday, int tm_mon, int tm_year);
			std::string createNameFile_ntf(int tm_mday, int tm_mon, int tm_year);

			enum class ERRNOT{
				ERROR,
				NOTIFY
			};


			std::list<std::string> listFilter(std::list<std::string> &v_str, const char *format);
			int filesSize(ERRNOT en, std::list<std::string> &v_str);
			std::list<std::string> GetDirectoryFiles(const std::string dir);
			void AddRecordInFile (const std::string fileName, const std::string message);

	};

	extern STATEErrNotif *STATEErrorNotif;



#endif /* FILEMANAGER_H_ */
