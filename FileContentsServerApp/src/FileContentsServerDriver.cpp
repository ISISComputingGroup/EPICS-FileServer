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

FileContentsServerDriver::FileContentsServerDriver(const char *portName, const char *fileDir, const char *inputFile, const char *outputFile)
	: asynPortDriver(portName,
					 0,																												/* maxAddr */
					 asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
					 asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,					/* Interrupt mask */
					 ASYN_CANBLOCK,																									/* asynFlags.  This driver can block but it is not multi-device */
					 1,																												/* Autoconnect */
					 0,																												/* Default priority */
					 0),																											/* Default stack size*/
	  m_fileDir(fileDir)
{
	const char *functionName = "FileContentsServerDriver";

	createParam(P_inputFileNameString, asynParamOctet, &P_inputFileName);
	createParam(P_outputFileNameString, asynParamOctet, &P_outputFileName);
	createParam(P_fileContentsString, asynParamOctet, &P_fileContents);
	createParam(P_saveFileString, asynParamInt32, &P_saveFile);
	createParam(P_resetString, asynParamInt32, &P_reset);
	createParam(P_fileDirString, asynParamOctet, &P_fileDir);
	createParam(P_logString, asynParamOctet, &P_log);
	createParam(P_unsavedChangesString, asynParamInt32, &P_unsavedChanges);

	setStringParam(P_fileDir, fileDir);
	setStringParam(P_inputFileName, inputFile);

	// Read the original contents in first.
	readFile();

	if (outputFile && *outputFile)
	{
		setStringParam(P_outputFileName, outputFile);
		// Save the file so that the output file has the contents of the input file initially.
		saveFile();
	}
    else
	{
		// If the output file is not specified, default to saving output to the input file.
		setStringParam(P_outputFileName, inputFile);
	}

	const int defaultSaveFile = 0;
	setIntegerParam(P_saveFile, defaultSaveFile);
	const int defaultReset = 0;
	setIntegerParam(P_reset, defaultReset);
	const int defaultUnsavedChanges = 0;
	setIntegerParam(P_unsavedChanges, defaultUnsavedChanges);
	setStringParam(P_log, "");
	logMessage("Editor initialized");
	callParamCallbacks();
}

asynStatus FileContentsServerDriver::saveFile()
{
	logMessage("Triggering save");

	std::string buffer;

	// Get the value of the parameter we just set
	getStringParam(P_fileContents, buffer);

	std::string fileName;
	getStringParam(P_outputFileName, fileName);
	// join the file name with the file directory
	std::string fullFileName = m_fileDir + "/" + fileName;
	// Write the contents of buffer to a file
	try
	{
		std::ofstream outfile(fullFileName, std::ios::trunc | std::ios::binary); // Open in  write mode
		if (outfile.is_open())
		{
			outfile << buffer;
			outfile.close();
			logMessage("File saved successfully");
			setIntegerParam(P_saveFile, 0);
			setIntegerParam(P_unsavedChanges, 0);
		}
		else
		{
			logMessage(std::string("Failed to open file (") + fullFileName + ") for writing");
			return asynError;
		}
	}
	catch (const std::exception &e)
	{
		logMessage(e.what());
		return asynError;
	}
	callParamCallbacks();
	return asynSuccess;
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
			return saveFile();
		}
		return asynSuccess;
	}
	else if (pasynUser->reason == P_reset)
	{
		if (value == 1)
		{
			logMessage("Reloading from disk");
			setStringParam(P_fileContents, m_original_lines_array);
			saveFile();
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

void FileContentsServerDriver::logMessage(const std::string &message)
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

	std::cout << log_message << std::endl;
	setStringParam(P_log, log_message);
	callParamCallbacks();
}

void FileContentsServerDriver::readFile()
{
	std::cout << "Calling readFile" << std::endl;
	std::ifstream f;
	std::string fileName;
	getStringParam(P_inputFileName, fileName);
	std::string fullFileName = m_fileDir + "/" + fileName;

	logMessage(std::string("Reading file ") + fullFileName);
	try
	{
		f.open(fullFileName, std::ios::binary);
		if (!f.is_open())
		{
			m_original_lines_array = "";
			setStringParam(P_fileContents, "");
			if (errno == ENOENT) // File not found
			{
				logMessage(std::string("File not found: ") + fullFileName);
				throw std::runtime_error("File not found");
			}
			else // Other errors, e.g., permission denied
			{
				logMessage(std::string("Unable to open file: ") + fullFileName + ", error " + std::string(strerror(errno)));
				throw std::runtime_error(std::string("Unable to open file: ") + std::string(strerror(errno)));
			}
		}
		int i = 0;

		std::stringstream buffer;
		buffer << f.rdbuf();
		setStringParam(P_fileContents, buffer.str());
		m_original_lines_array = buffer.str();
		setIntegerParam(P_unsavedChanges, 0);

		logMessage("File read successfully");

		callParamCallbacks();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
		logMessage(e.what());
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
	if (function == P_fileContents)
	{

		setStringParam(P_fileContents, value);
		if (m_original_lines_array != value)
		{
			setIntegerParam(P_unsavedChanges, 1);
		}
		else
		{
			setIntegerParam(P_unsavedChanges, 0);
		}
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
	int FileContentsServerConfigure(const char *portName, const char *fileDir, const char *inputFile, const char *outputFile)
	{
		try
		{
			FileContentsServerDriver *driver = new FileContentsServerDriver(portName, fileDir, inputFile, outputFile);
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

	static const iocshArg initArg0 = {"portName", iocshArgString};	 ///< The name of the asyn driver port we will create
	static const iocshArg initArg1 = {"fileDir", iocshArgString};	 ///< The directory to open/save files into
	static const iocshArg initArg2 = {"inputFile", iocshArgString};	 ///< input file name to read and initially serve contents of
	static const iocshArg initArg3 = {"outputFile", iocshArgString}; ///< file name to save output to when a user hits save. If not set this defaults to inputFile.

	static const iocshArg *const initArgs[] = {&initArg0,
											   &initArg1,
											   &initArg2,
											   &initArg3};

	static const iocshFuncDef initFuncDef = {"FileContentsServerConfigure", sizeof(initArgs) / sizeof(iocshArg *), initArgs};

	static void initCallFunc(const iocshArgBuf *args)
	{
		FileContentsServerConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].sval);
	}

	static void FileContentsServerRegister(void)
	{
		iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(FileContentsServerRegister);
}
