# @configure_input@

# this is only needed for recursive Makefiles, to point to Makefiles
# in other source dirs
# SUBDIRS = src

PATH_GLOBAL=src/global
PATH_CODE=src/structuredData

AM_CFLAGS = -Wall -fpic -std=c99 -Winline -fgnu89-inline -I$(top_srcdir)/$(PATH_GLOBAL)

# list of all products:
lib_LTLIBRARIES = structuredDataC.la

structuredDataC_la_SOURCES = \
  $(PATH_CODE)/structuredData.c $(PATH_CODE)/structuredData.h \
  $(PATH_CODE)/sdList.c $(PATH_CODE)/sdList.h \
  $(PATH_CODE)/sdPack.c $(PATH_CODE)/sdPack.h \
  $(PATH_CODE)/sdData.c $(PATH_CODE)/sdData.h \
  $(PATH_CODE)/sdEvent.c $(PATH_CODE)/sdEvent.h \
  $(PATH_CODE)/sdObjState.c $(PATH_CODE)/sdObjState.h

structuredDataC_la_LDFLAGS = -module
