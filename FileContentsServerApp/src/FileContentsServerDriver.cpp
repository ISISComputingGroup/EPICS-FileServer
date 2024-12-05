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
#include <boost/algorithm/string/join.hpp>
#include <sstream>

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

#include "FileContentsServerDriver.h"

#include <epicsExport.h>

static const char *driverName = "FileContentsServerDriver";

FileContentsServerDriver::FileContentsServerDriver(const char *portName, const char *fileDir)
	: asynPortDriver(portName,
					 0, /* maxAddr */
					 asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
					 asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,					/* Interrupt mask */
					 ASYN_CANBLOCK,																									/* asynFlags.  This driver can block but it is not multi-device */
					 1,																												/* Autoconnect */
					 0,																												/* Default priority */
					 0),																											/* Default stack size*/
	  m_fileDir(fileDir)
{
	const char *functionName = "FileContentsServerDriver";
	createParam(P_fileNameString, asynParamOctet, &P_fileName);
	createParam(P_linesArrayString, asynParamOctet, &P_linesArray);
	createParam(P_saveFileString, asynParamInt32, &P_saveFile);
	createParam(P_resetString, asynParamInt32, &P_reset);
	createParam(P_fileDirString, asynParamOctet, &P_fileDir);
	createParam(P_logString, asynParamOctet, &P_log);
	createParam(P_newFileWarningString, asynParamInt32, &P_newFileWarning);
	createParam(P_unsavedChangesString, asynParamInt32, &P_unsavedChanges);

	setStringParam(P_fileDir, fileDir);
	const int defaultSaveFile = 0;
	setIntegerParam(P_saveFile, defaultSaveFile);
	const int defaultReset = 0;
	setIntegerParam(P_reset, defaultReset);
	const int defaultNewFileWarning = 0;
	setIntegerParam(P_newFileWarning, defaultNewFileWarning);
	const int defaultUnsavedChanges = 0;
	setIntegerParam(P_unsavedChanges, defaultUnsavedChanges);
	setStringParam(P_linesArray, "");
	setStringParam(P_log, "");
	logMessage("Editor initialized");
	callParamCallbacks();
}

// Override the method to handle writes to parameters
asynStatus FileContentsServerDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int hasChanged = 0;
	getIntegerParam(P_unsavedChanges, &hasChanged);
	if (pasynUser->reason == P_saveFile)
	{
		if (value == 1 && hasChanged == 1)
		{
			std::cout << "Triggering save" << std::endl;
			std::cout << "Value received by asyn: " << value << std::endl;

			char buffer[5000]; // Assuming the response will fit within 256 characters

			// Get the value of the parameter we just set
			getStringParam(P_linesArray, sizeof(buffer), buffer);

			char fileName[512];
			getStringParam(P_fileName, sizeof(fileName), fileName);
			// join the file name with the file directory
			std::string m_fullFileName = m_fileDir + "/" + fileName;
			// Write the contents of buffer to a file
            try
            {
                std::ofstream outfile(m_fullFileName, std::ios::trunc | std::ios::binary); // Open in  write mode
                if (outfile.is_open())
                {
                    outfile << buffer;
                    outfile.close();
                    std::cout << "Contents written to file successfully." << std::endl;
                    logMessage("File saved successfully");
                    setIntegerParam(P_saveFile, 0);
                    setIntegerParam(P_newFileWarning, 0);
                    setIntegerParam(P_unsavedChanges, 0);
                }
                else
                {
                    std::cerr << "Failed to open file for writing." << std::endl;
                    logMessage("Failed to open file for writing");
                    return asynError;
                }
            }
	        catch (const std::exception &e)
	        {
		        std::cerr << "Exception caught: " << e.what() << std::endl;
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
			setIntegerParam(P_unsavedChanges, 0);
			callParamCallbacks();
		}
		return asynSuccess;
	}
	else
	{
		return asynPortDriver::writeInt32(pasynUser, value);
	}
}

void FileContentsServerDriver::updateLinesArray(const std::vector<std::string>& linesArray)
{
	
}

void FileContentsServerDriver::logMessage(const std::string& message)
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

void FileContentsServerDriver::readFile()
{
	std::cout << "Calling readFile" << std::endl;
	std::fstream f;
	std::string line;
	std::vector<std::string> linesArray;
	char fileName[256];
	getStringParam(P_fileName, sizeof(fileName), fileName);
	std::string m_fullFileName = m_fileDir + "/" + fileName;
	std::cout << "FileContentsServerDriver: Reading file " << m_fullFileName << std::endl;
	try
	{
		f.open(m_fullFileName.c_str(), std::ios::in);
		if (!f.is_open())
		{
			m_original_lines_array = "";
			if (errno == ENOENT) // File not found
			{
				logMessage("File not found");
				std::cout << "File not found" << std::endl;
				setIntegerParam(P_newFileWarning, 1);
				setStringParam(P_linesArray, "");
			}
			else // Other errors, e.g., permission denied
			{
				logMessage("Unable to open file: " + std::string(strerror(errno)));
			}

			throw std::runtime_error("Unable to open file: " + m_fullFileName);
		}
		setIntegerParam(P_newFileWarning, 0);
		int i = 0;

		std::stringstream buffer;
		buffer << f.rdbuf();
		setStringParam(P_linesArray, buffer.str().c_str());
		m_original_lines_array = buffer.str().c_str();
		setIntegerParam(P_unsavedChanges, 0);

		logMessage("File read successfully");

		callParamCallbacks();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
	}

	callParamCallbacks();
}

asynStatus FileContentsServerDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
	int function = pasynUser->reason; // Function to call
	const char *functionName = "writeOctet";
	const char *paramName = NULL;
	getParamName(function, &paramName);
	std::cout << "writeOctet " << paramName << std::endl;
	if (function == P_linesArray)
	{

		setStringParam(P_linesArray, value);
		if (m_original_lines_array != value)
		{
			setIntegerParam(P_unsavedChanges, 1);
		}
		else
		{
			setIntegerParam(P_unsavedChanges, 0);
		}
	}
	else if (function == P_fileName)
	{
		std::cout << "Setting file name to " << value << std::endl;
		setStringParam(P_fileName, value);
		readFile();
	}
	else
	{
		return asynPortDriver::writeOctet(pasynUser, value, maxChars, nActual);
	}

	// Set nActual to the length of the string that was written
	*nActual = strlen(value);

	// Update the parameter
	callParamCallbacks();
	return asynSuccess;
}

extern "C"
{

	/// \param[in] portName @copydoc initArg0
	/// \param[in] fileDir @copydoc initArg1
	int FileContentsServerConfigure(const char *portName, const char *fileDir)
	{
		try
		{
			FileContentsServerDriver *driver = new FileContentsServerDriver(portName, fileDir);
			if (driver == NULL)
			{
				errlogSevPrintf(errlogMajor, "FileContentsServerConfigure failed (NULL)\n");
				return (asynError);
			}
			else
			{
				return (asynSuccess);
			}
		}
		catch (const std::exception &ex)
		{
			errlogSevPrintf(errlogMajor, "FileContentsServerConfigure failed: %s\n", ex.what());
			return (asynError);
		}
	}

	// EPICS iocsh shell commands

	static const iocshArg initArg0 = {"portName", iocshArgString}; ///< The name of the asyn driver port we will create
	static const iocshArg initArg1 = {"fileDir", iocshArgString};  ///< file name

	static const iocshArg *const initArgs[] = {&initArg0,
											   &initArg1};

	static const iocshFuncDef initFuncDef = {"FileContentsServerConfigure", sizeof(initArgs) / sizeof(iocshArg *), initArgs};

	static void initCallFunc(const iocshArgBuf *args)
	{
		FileContentsServerConfigure(args[0].sval, args[1].sval);
	}

	static void FileContentsServerRegister(void)
	{
		iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(FileContentsServerRegister);
}
