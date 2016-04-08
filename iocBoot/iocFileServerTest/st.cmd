#!../../bin/windows-x64-debug/FileServerTest

## You may have to change FileServerTest to something else
## everywhere it appears in this file

# Increase this if you get <<TRUNCATED>> or discarded messages warnings in your errlog output
errlogInit2(65536, 256)

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/FileServerTest.dbd"
FileServerTest_registerRecordDeviceDriver pdbbase

FileServerConfigure("fserv", "${TOP}/iocBoot/${IOC}/test.txt")

## Load record instances

## Load our record instances
dbLoadRecords("db/FileServerTest.db","P=TEST:")

cd ${TOP}/iocBoot/${IOC}
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"

