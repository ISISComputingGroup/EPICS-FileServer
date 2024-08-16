#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sys/timeb.h>
#include <boost/algorithm/string.hpp>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <shareLib.h>
#include <iocsh.h>

#include <macLib.h>
#include <epicsGuard.h>
#include "pcrecpp.h"

#include "envDefs.h"
#include "errlog.h"

#include "utilities.h"

#include "FileServerDriver.h"

#include <epicsExport.h>

static const char *driverName = "FileServerDriver";

/// Constructor for the isisdaeDriver class.
/// Calls constructor for the asynPortDriver base class.
/// \param[in] dcomint DCOM interface pointer created by lvDCOMConfigure()
/// \param[in] portName @copydoc initArg0
FileServerDriver::FileServerDriver(const char *portName, const char *fileDir, int fileType)
	: asynPortDriver(portName,
					 0, /* maxAddr */
					 NUM_FILESERV_PARAMS + 100,
					 asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
					 asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,					/* Interrupt mask */
					 ASYN_CANBLOCK,																									/* asynFlags.  This driver can block but it is not multi-device */
					 1,																												/* Autoconnect */
					 0,																												/* Default priority */
					 0),																											/* Default stack size*/
	  m_fileDir(fileDir), m_fileType(static_cast<FileType>(fileType))
{
	const char *functionName = "FileServerDriver";
	createParam(P_fileNameString, asynParamOctet, &P_fileName);
	createParam(P_fileTypeString, asynParamInt32, &P_fileType);
	createParam(P_linesArrayString, asynParamOctet, &P_linesArray);
	createParam(P_saveFileString, asynParamInt32, &P_saveFile);
	createParam(P_resetString, asynParamInt32, &P_reset);
	createParam(P_fileDirString, asynParamOctet, &P_fileDir);
	createParam(P_logString, asynParamOctet, &P_log);

	setIntegerParam(P_fileType, fileType);
	setStringParam(P_fileDir, fileDir);
	const int defaultSaveFile = 0;
	setIntegerParam(P_saveFile, defaultSaveFile);
	const int defaultReset = 0;
	setIntegerParam(P_reset, defaultReset);
	logMessage("Editor initialized");
	callParamCallbacks();
}

// Override the method to handle writes to parameters
asynStatus FileServerDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	if (pasynUser->reason == P_saveFile)
	{
		if (value == 1)
		{
			// int param = pasynUser->reason;
			std::cout << "Triggering save" << std::endl;
			std::cout << "Value received by asyn: " << value << std::endl;

			char buffer[5000]; // Assuming the response will fit within 256 characters

			// Get the value of the parameter we just set
			getStringParam(P_linesArray, sizeof(buffer), buffer);
			std::cout << "Value of P_linesArray after setStringParam: " << buffer << std::endl;
			// replace newlines with ""
			std::string buffer_str = buffer;
			boost::replace_all(buffer_str, "\n", "");

			char fileName[256];
			getStringParam(P_fileName, sizeof(fileName), fileName);
			// join the file name with the file directory
			std::string m_fullFileName = m_fileDir + fileName;
			// Write the contents of buffer to a file
			std::ofstream outfile(m_fullFileName, std::ios::out | std::ios::trunc); // Open in  write mode
			if (outfile.is_open())
			{
				outfile << buffer_str << std::endl;
				outfile.close();
				std::cout << "Contents written to file successfully." << std::endl;
				logMessage("File saved successfully");
				setIntegerParam(P_saveFile, 0);
			}
			else
			{
				std::cerr << "Failed to open file for writing." << std::endl;
				logMessage("Failed to open file for writing");
				return asynError;
			}

			callParamCallbacks();
		}
		return asynSuccess;
	}
	else if (pasynUser->reason == P_reset)
	{
		if (value == 1)
		{
			std::cout << "Resetting" << std::endl;
			setStringParam(P_linesArray, m_original_lines_array.c_str());
			logMessage("Reset successful.");
			setIntegerParam(P_reset, 0);
			callParamCallbacks();
		}
		return asynSuccess;
	}
	else
	{
		return asynPortDriver::writeInt32(pasynUser, value);
	}
}

void FileServerDriver::updateLinesArray()
{
	std::string concatenatedLines;
	for (const auto &line : m_linesArray)
	{
		concatenatedLines += line + "\n";
	}

	setStringParam(P_linesArray, concatenatedLines.c_str());
	m_original_lines_array = concatenatedLines;

	logMessage("File read successfully");

	callParamCallbacks();
}

void FileServerDriver::logMessage(std::string message)
{
	// add the time to the front of the log message
	// current time
	// Get the current time
	time_t now = time(0);
	tm *ltm = localtime(&now);

	// Format the time into a string (e.g., "2024-08-15 12:34:56")
	char time_str[20];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ltm);

	// Create the log message
	std::string log_message = std::string(time_str) + ": " + message;

	setStringParam(P_log, log_message.c_str());
	callParamCallbacks();
}

void FileServerDriver::readFile()
{
	std::cout << "Calling readFile" << std::endl;
	std::fstream f;
	std::string line, key, value;
	m_linesArray.clear();
	int param;
	char fileName[256];
	getStringParam(P_fileName, sizeof(fileName), fileName);
	std::string m_fullFileName = m_fileDir + fileName;
	std::cout << "FileServerDriver: Reading file " << m_fullFileName << std::endl;
	try
	{
		f.open(m_fullFileName.c_str(), std::ios::in);
		if (!f.is_open())
		{
			logMessage("Unable to open file");
			throw std::runtime_error("Unable to open file: " + m_fullFileName);
		}
		int i = 0;
		while (f.good())
		{
			std::getline(f, line);
			m_linesArray.push_back(line);
			i++;
		}
		std::cout << "FileServerDriver: Read " << i << " lines " << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
	}

	updateLinesArray(); // Update the PV with the new content of m_linesArray
	callParamCallbacks();
}

asynStatus FileServerDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
	int function = pasynUser->reason; // Function to call
	const char *functionName = "writeOctet";
	asynStatus status = asynSuccess; // Return status
	const char *paramName = NULL;
	getParamName(function, &paramName);
	std::string paramNameStr = paramName;
	std::cout << paramName << std::endl;
	if (paramNameStr == P_linesArrayString)
	{

		std::cout << "Calling writeOctet" << std::endl;
		setStringParam(P_linesArray, value);
	}
	else if (paramNameStr == P_fileNameString)
	{
		std::cout << paramName << std::endl;
		std::cout << "Setting file name" << std::endl;
		setStringParam(P_fileName, value);
		readFile();
	}
	else
	{
		std::cout << "Unknown parameter" << std::endl;
	}

	// Set nActual to the length of the string that was written
	*nActual = strlen(value);

	// Update the parameter
	callParamCallbacks();
	return status;
}

extern "C"
{

	/// \param[in] portName @copydoc initArg0
	/// \param[in] fileDir @copydoc initArg1
	/// \param[in] fileType @copydoc initArg2
	int FileServerConfigure(const char *portName, const char *fileDir, int fileType)
	{
		try
		{
			FileServerDriver *driver = new FileServerDriver(portName, fileDir, fileType);
			if (driver == NULL)
			{
				errlogSevPrintf(errlogMajor, "FileServerConfigure failed (NULL)\n");
				return (asynError);
			}
			else
			{
				return (asynSuccess);
			}
		}
		catch (const std::exception &ex)
		{
			errlogSevPrintf(errlogMajor, "FileServerConfigure failed: %s\n", ex.what());
			return (asynError);
		}
	}

	// EPICS iocsh shell commands

	static const iocshArg initArg0 = {"portName", iocshArgString}; ///< The name of the asyn driver port we will create
	static const iocshArg initArg1 = {"fileDir", iocshArgString};  ///< file name
	static const iocshArg initArg2 = {"fileType", iocshArgInt};	   ///< file type enum

	static const iocshArg *const initArgs[] = {&initArg0,
											   &initArg1,
											   &initArg2};

	static const iocshFuncDef initFuncDef = {"FileServerConfigure", sizeof(initArgs) / sizeof(iocshArg *), initArgs};

	static void initCallFunc(const iocshArgBuf *args)
	{
		FileServerConfigure(args[0].sval, args[1].sval, args[2].ival);
	}

	static void FileServerRegister(void)
	{
		iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(FileServerRegister);
}
