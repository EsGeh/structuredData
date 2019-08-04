# @configure_input@

# this is only needed for recursive Makefiles, to point to Makefiles
# in other source dirs
# SUBDIRS = src

PATH_GLOBAL = src/global
PATH_CODE = src/structuredData
PATH_SCRIPT = src/sdScript
PATH_PD_OBJS = pd_objs

AM_CFLAGS = -Wall -fpic -std=c99 -Winline -fgnu89-inline -I$(top_srcdir)/$(PATH_GLOBAL) -I$(top_srcdir)/$(PATH_SCRIPT)

# by default, copy all to the prefix dir, no subdirs
libdir = $(prefix)
datadir = $(prefix)

# list of all products:
lib_LTLIBRARIES = structuredDataC.la
data_DATA = \
	$(PATH_PD_OBJS)/sdAnyBang.pd \
	$(PATH_PD_OBJS)/sdFloatBang.pd \
	$(PATH_PD_OBJS)/sdSymbolBang.pd \
	$(PATH_PD_OBJS)/sdObjSaveAll.pd \
	$(PATH_PD_OBJS)/sdObjConnectTo.pd \
	$(PATH_PD_OBJS)/sdObjTrack.pd \
	$(PATH_PD_OBJS)/sdObjGet.pd \
	$(PATH_PD_OBJS)/sdObjExample.pd \
	$(PATH_PD_OBJS)/sdObjSndExample.pd \
	$(PATH_PD_OBJS)/sdObjDuplexExample.pd \
	$(PATH_PD_OBJS)/sdObjState_ui.pd \
	$(PATH_PD_OBJS)/sdObjRcvExample.pd \
	$(PATH_PD_OBJS)/sdObjInterface_obj.pd \
	$(PATH_PD_OBJS)/sdObjGroupsInterface_obj.pd \
	$(PATH_PD_OBJS)/sdTest.pd \
  $(PATH_PD_OBJS)/doc/structuredDataC_doc.pd \
  $(PATH_PD_OBJS)/doc/modules/sdData-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdEvent-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdList-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdObjState-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdObjState_example.pd \
  $(PATH_PD_OBJS)/doc/modules/sdPack-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdUtils-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdPackMatch-help.pd \
	$(PATH_PD_OBJS)/doc/sdScript-help.pd

structuredDataC_la_SOURCES = \
  $(PATH_CODE)/structuredData.c $(PATH_CODE)/structuredData.h \
  $(PATH_CODE)/sdList.c $(PATH_CODE)/sdList.h \
  $(PATH_CODE)/sdPack.c $(PATH_CODE)/sdPack.h \
  $(PATH_CODE)/sdPackMatch.c $(PATH_CODE)/sdPackMatch.h \
  $(PATH_CODE)/sdData.c $(PATH_CODE)/sdData.h \
  $(PATH_CODE)/sdEvent.c $(PATH_CODE)/sdEvent.h \
  $(PATH_CODE)/sdProperties.c $(PATH_CODE)/sdProperties.h \
  $(PATH_CODE)/sdObjState.c $(PATH_CODE)/sdObjState.h \
	$(PATH_SCRIPT)/sdScriptObj.c $(PATH_SCRIPT)/sdScriptObj.h \
	$(PATH_SCRIPT)/sdScript.c $(PATH_SCRIPT)/sdScript.h \
	$(PATH_SCRIPT)/SymbolTable.c $(PATH_SCRIPT)/SymbolTable.h \
	$(PATH_SCRIPT)/Commands.c $(PATH_SCRIPT)/Commands.h

structuredDataC_la_LDFLAGS = -module
