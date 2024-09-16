#ifndef FILECONTENTSSERVERDRIVER_H
#define FILECONTENTSSERVERDRIVER_H

#include "asynPortDriver.h"

class FileContentsServerDriver : public asynPortDriver
{
public:
	FileContentsServerDriver(const char *portName, const char *fileDir);

	// These are the methods that we override from asynPortDriver
	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

private:
	int P_fileName;
	int P_linesArray;
	int P_saveFile;
	int P_fileDir;
	int P_reset;
	int P_log;
	int P_newFileWarning;
	int P_unsavedChanges;
#define FIRST_FILESERV_PARAM P_fileName
#define SECOND_FILESERV_PARAM P_fileType
#define THIRD_FILESERV_PARAM P_linesArray
#define FOURTH_FILESERV_PARAM P_fileDir
#define FIFTH_FILESERV_PARAM P_log
#define SIXTH_FILESERV_PARAM P_reset
#define SEVENTH_FILESERV_PARAM P_newFileWarning
#define EIGHTH_FILESERV_PARAM P_unsavedChanges
#define LAST_FILESERV_PARAM P_saveFile

	std::string m_fileName;
	std::string m_fileDir;

	std::vector<std::string> m_linesArray;
	std::string m_original_lines_array;

	void readFile();
	void updateLinesArray();
	void logMessage(std::string message);
};

#define NUM_FILESERV_PARAMS (&LAST_FILESERV_PARAM - &FIRST_FILESERV_PARAM + 1)

#define P_fileNameString "FILE_NAME"
#define P_linesArrayString "LINES_ARRAY"
#define P_saveFileString "SAVE_FILE"
#define P_fileDirString "FILE_DIR"
#define P_logString "LOG"
#define P_resetString "RESET"
#define P_newFileWarningString "NEW_FILE_WARNING"
#define P_unsavedChangesString "UNSAVED_CHANGES"

#endif /* FILECONTENTSSERVERDRIVER_H */
