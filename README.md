# EPICS-fileserver

This support module contains two EPICS modules:
- FileServerApp: used for parsing and reading key-value-comment files to generate asyn parameters and PVs based on the key.
- FileContentsServerApp: used for reading and writing the raw contents of a file to/from an EPICS PV served as a waveform record.


## EPICS iocsh commands 

### FileServerConfigure(portName, fileName)

* `portName`: The name of the asyn driver port to use
* `fileName`: file name


### FileContentsServerConfigure(portName, fileName, fileType)

* `portName`: The name of the asyn driver port to use
* `fileName`: file name
* `fileType`: file type (see `FileType` enum)

