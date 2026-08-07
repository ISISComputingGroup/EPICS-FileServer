#ifndef FILECONTENTSSERVERDRIVER_H
#define FILECONTENTSSERVERDRIVER_H

#include "asynPortDriver.h"

class FileContentsServerDriver : public asynPortDriver
{
public:
	FileContentsServerDriver(const char *portName, const char *fileDir, const char *inputFile, const char *outputFile);

	// These are the methods that we override from asynPortDriver
	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

private:
	int P_inputFileName;
	int P_outputFileName;
	int P_fileContents;
	int P_saveFile;
	int P_fileDir;
	int P_reset;
	int P_log;
	int P_unsavedChanges;

	std::string m_fileDir;
	std::string m_original_lines_array;

	void readFile();
	asynStatus saveFile();
	void logMessage(const std::string &message);
};

#define P_inputFileNameString "INPUT_FILE"
#define P_outputFileNameString "OUTPUT_FILE"
#define P_fileContentsString "LINES_ARRAY"
#define P_saveFileString "SAVE_FILE"
#define P_fileDirString "FILE_DIR"
#define P_logString "LOG"
#define P_resetString "RESET"
#define P_unsavedChangesString "UNSAVED_CHANGES"

#endif /* FILECONTENTSSERVERDRIVER_H */
