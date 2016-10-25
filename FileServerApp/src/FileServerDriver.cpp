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
#include <iocsh.h>

#include <macLib.h>
#include <epicsGuard.h>
#include "pcrecpp.h"

#include "envDefs.h"
#include "errlog.h"

#include "utilities.h"

#include "FileServerDriver.h"

#include <epicsExport.h>

static const char *driverName="FileServerDriver";


/// Constructor for the isisdaeDriver class.
/// Calls constructor for the asynPortDriver base class.
/// \param[in] dcomint DCOM interface pointer created by lvDCOMConfigure()
/// \param[in] portName @copydoc initArg0
FileServerDriver::FileServerDriver(const char *portName, const char* fileName, int fileType) 
   : asynPortDriver(portName, 
                    0, /* maxAddr */ 
                    NUM_FILESERV_PARAMS + 100,
                    asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
                    asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
                    ASYN_CANBLOCK, /* asynFlags.  This driver can block but it is not multi-device */
                    1, /* Autoconnect */
                    0, /* Default priority */
                    0),	/* Default stack size*/
					m_fileName(fileName), m_fileType(static_cast<FileType>(fileType))
{
    const char *functionName = "FileServerDriver";
    createParam(P_fileNameString, asynParamOctet, &P_fileName);
    createParam(P_fileTypeString, asynParamInt32, &P_fileType);

    setStringParam(P_fileName, fileName);
    setIntegerParam(P_fileType, fileType);
	
    readFile();	

}

void FileServerDriver::readFile()
{
    const std::string comment = "//";
    std::fstream f;
	std::string line, key, value;
	pcrecpp::RE re("(\\w+)(\\s+)(\\S+)(.*)");
	m_lines.clear();
	m_kv.clear();
	int param;
	f.open(m_fileName.c_str(), std::ios::in);
	std::getline(f, line);
	int i = 0, n = 0;
	while(f.good())
	{
		m_lines.push_back(line);
		if (line.substr(0,comment.size()) == comment)
		{
	        ;
		}
        else if (re.FullMatch(line.c_str(), &key, (void*)0, &value))
		{
            createParam(key.c_str(), asynParamOctet, &param);
		    setStringParam(param, value.c_str());
		    m_kv[key] = KV(i, param, value);
	        std::cout << "FileServerDriver: Found key \"" << key << "\" = \"" << value << "\"" << std::endl;	
			++n;
		}
		++i;
	    std::getline(f, line);
	}
	std::cout << "FileServerDriver: Read " << i << " lines and " << n << " parameters" << std::endl;
}

asynStatus FileServerDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName = NULL;
	getParamName(function, &paramName);
    const char* functionName = "writeOctet";
	std::string value_s;
    // we might get an embedded NULL from channel access char waveform records
    if ( (maxChars > 0) && (value[maxChars-1] == '\0') )
    {
        value_s.assign(value, maxChars-1);
    }
    else
    {
        value_s.assign(value, maxChars);
    }
	try
	{
	    updateKey(paramName, value_s);
		status = asynPortDriver::writeOctet(pasynUser, value_s.c_str(), value_s.size(), nActual);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%s\n", 
              driverName, functionName, function, paramName, value_s.c_str());
        if (status == asynSuccess)
        {
		    *nActual = maxChars;   // to keep result happy in case we skipped an embedded trailing NULL
        }
		return status;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%s, error=%s", 
                  driverName, functionName, status, function, paramName, value_s.c_str(), ex.what());
		*nActual = 0;
		return asynError;
	}
}

asynStatus FileServerDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    char value_s[32];
    size_t nActual = 0;
	sprintf(value_s, "%d", static_cast<int>(value));
    return writeOctet(pasynUser, value_s, strlen(value_s), &nActual);
}

asynStatus FileServerDriver::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    char value_s[32];
    int param = pasynUser->reason;
    getStringParam(param, sizeof(value_s), value_s); 
	*value = atol(value_s);
	return asynSuccess;
}

asynStatus FileServerDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    char value_s[32];
    size_t nActual = 0;
	sprintf(value_s, "%g", static_cast<double>(value));
    return writeOctet(pasynUser, value_s, strlen(value_s), &nActual);
}

asynStatus FileServerDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    char value_s[32];
    int param = pasynUser->reason;
    getStringParam(param, sizeof(value_s), value_s); 
	*value = atof(value_s);
	return asynSuccess;
}


void FileServerDriver::updateKey(const std::string& key, const std::string& value)
{
	pcrecpp::RE re("(\\w+)(\\s+)(\\w+)(.*)");
    std::map<std::string,KV>::iterator it = m_kv.find(key);
    if ( it == m_kv.end() )
	{
	    errlogSevPrintf(errlogMajor, "Unknown file key \"%s\"", key.c_str());
	    return;
	}
	it->second.value = value;
	std::string& s = m_lines[it->second.line];
	std::string newline = key + "\\2" + value + "\\4";
	re.Replace(newline.c_str(), &s);
	std::cout << "New line " << it->second.line + 1 << ": " << s << std::endl;
	updateFile();
}

void FileServerDriver::updateFile()
{
    std::fstream f;
	f.open(m_fileName.c_str(), std::ios::out);
	for(int i=0; i<m_lines.size(); ++i)
	{
	    f << m_lines[i] << std::endl;
	}
	f.close();
}
  
		
extern "C" {

/// \param[in] portName @copydoc initArg0
/// \param[in] fileName @copydoc initArg1
/// \param[in] fileType @copydoc initArg2
int FileServerConfigure(const char *portName, const char* fileName, int fileType)
{
	try
	{
		FileServerDriver* driver = new FileServerDriver(portName, fileName, fileType);
		if (driver == NULL)
		{
			errlogSevPrintf(errlogMajor, "FileServerConfigure failed (NULL)\n");
			return(asynError);
		}
		else
		{
			return(asynSuccess);
		}
	}
	catch(const std::exception& ex)
	{
		errlogSevPrintf(errlogMajor, "FileServerConfigure failed: %s\n", ex.what());
		return(asynError);
	}
}

// EPICS iocsh shell commands 

static const iocshArg initArg0 = { "portName", iocshArgString};			///< The name of the asyn driver port we will create
static const iocshArg initArg1 = { "fileName", iocshArgString};			///< file name
static const iocshArg initArg2 = { "fileType", iocshArgInt};			///< file type enum

static const iocshArg * const initArgs[] = { &initArg0,
                                             &initArg1,
											 &initArg2
};

static const iocshFuncDef initFuncDef = {"FileServerConfigure", sizeof(initArgs) / sizeof(iocshArg*), initArgs};

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
