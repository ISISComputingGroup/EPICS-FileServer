#ifndef FILESERVERDRIVER_H
#define FILESERVERDRIVER_H
 
#include "asynPortDriver.h"

class FileServerDriver : public asynPortDriver 
{
public:
    enum FileType { FileTypeTextKVC=1 };
    FileServerDriver(const char *portName, const char* fileName, int fileType);
                
    // These are the methods that we override from asynPortDriver
	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
	
private:

	int P_fileName; // string
	int P_fileType; // int
	#define FIRST_FILESERV_PARAM P_fileName
	#define LAST_FILESERV_PARAM P_fileType

	std::string m_fileName;
	FileType m_fileType;
	struct KV
	{
	    int line;
		int param;
	    std::string value;
		KV(int l, int p, const std::string& v) : line(l), param(p), value(v) { }
		KV() : line(-1), param(-1), value("<NONE>") { }
	};
	std::map<std::string,KV> m_kv;
	std::vector<std::string> m_lines;
	
	void readFile();
	void updateKey(const std::string& key, const std::string& value);
	void updateFile();

	
};

#define NUM_FILESERV_PARAMS (&LAST_FILESERV_PARAM - &FIRST_FILESERV_PARAM + 1)

// use _ in name to avoid clash with k,v from file
#define P_fileNameString	"_FILENAME_"
#define P_fileTypeString	"_FILETYPE_"

#endif /* FILESERVERDRIVER_H */
