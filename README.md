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

### FileServer KV format
The first column is the KEYWORD. The keyword names may be up to 8 characters long and can only contain uppercase letters A to Z, the digits 0 to 9, the hyphen, and the underscore character.

The second column is the value - currently it is saved as a string

The third column is the comment

The entire record should be less than 80 characters long, therefore the comment field will be truncated if needed.

> [!TIP]
> - Empty lines are allowed and ignored
> - "//" - means a comment string, is also ignored
> - columns are separated by either a TAB or SPACE characters


Example: 
```
EXP_RB  1234567 Proposal RB number
EXP_Run 234567 Run number
STINDX 0 Image stack index
...
```
