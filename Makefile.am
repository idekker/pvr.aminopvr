#
# Makefile for the AminoPVR add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_AminoPVR
ADDONNAME       = pvr.aminopvr
LIBNAME         = libaminopvr-addon
lib_LTLIBRARIES = libaminopvr-addon.la

LIBS            = @abs_top_builddir@/lib/jsoncpp/libjsoncpp.la

include ../Makefile.include.am

INCLUDES+=-Isrc -Isrc/lib/filesystem -I@abs_top_srcdir@/lib/jsoncpp/include

libaminopvr_addon_la_SOURCES = src/client.cpp \
                               src/categories.cpp \
                               src/AminoPVRData.cpp
libaminopvr_addon_la_LDFLAGS = @TARGET_LDFLAGS@

