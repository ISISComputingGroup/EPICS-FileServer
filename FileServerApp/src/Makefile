TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_INCLUDES += -I"$(TOP)/../../libraries/boost/include"
USR_CXXFLAGS += /EHa /DNOMINMAX

LIBRARY_IOC += FileServer
FileServer_SRCS += FileServerDriver.cpp

FileServer_LIBS += asyn oncrpc utilities zlib pcrecpp pcre
FileServer_LIBS += $(EPICS_BASE_IOC_LIBS)

# FileServer.dbd will be created and installed
# see FileServerInclude.dbd
DBD += FileServer.dbd

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

