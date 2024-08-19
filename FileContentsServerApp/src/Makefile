TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_INCLUDES += -I"$(TOP)/../../libraries/boost/include"
#USR_CXXFLAGS += /EHa /DNOMINMAX

LIBRARY_IOC += FileContentsServer
FileContentsServer_SRCS += FileContentsServerDriver.cpp

FileContentsServer_LIBS += asyn utilities zlib pcrecpp pcre
FileContentsServer_LIBS += $(EPICS_BASE_IOC_LIBS)

# FileContentsServer.dbd will be created and installed
# see FileContentsServerInclude.dbd
DBD += FileContentsServer.dbd

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

