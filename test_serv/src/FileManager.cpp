/*
 * FileManeger.cpp
 *
 *  Created on: Apr 26, 2019
 *      Author: dombran
 */

#include "FileManager.h"

	STATEErrNotif *STATEErrorNotif;

	STATEErrNotif::STATEErrNotif (const std::string linkEN, boost::asio::io_context	&m_Ioc):
													 m_ioc(m_Ioc),
													 m_strand(boost::asio::make_strand(m_Ioc)){

		linkErrNtf = linkEN;

		}
	STATEErrNotif::~STATEErrNotif () {

		}


		// добавить запись в конце файла
		void STATEErrNotif::AddRecordInFile (const std::string fileName, const std::string message) {
			try{
			  std::ofstream ofs;
			  ofs.open (fileName, std::ofstream::out | std::ofstream::app);
			  ofs << message << std::endl;
			  ofs.close();
			}catch (std::exception& e)			{

			}
		}

		void STATEErrNotif::WriteToFile_Ntf(std::string message) {
		    if (m_ioc.stopped())
			    return;

				std::ofstream ofs;
				ofs.open (linkErrNtf, std::ofstream::out | std::ofstream::app);

				ofs << message << std::endl;
				ofs.close();

		}


		void STATEErrNotif::priCreateNewRecord_Ntf(std::string message){
			boost::asio::dispatch(m_strand, 	std::bind(&STATEErrNotif::WriteToFile_Ntf, this, message));
		}
		void STATEErrNotif::CreateNewRec_Ntf(std::string message){
			boost::asio::dispatch(m_strand, 	std::bind(&STATEErrNotif::priCreateNewRecord_Ntf, this, message));
		}



