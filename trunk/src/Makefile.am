lib_LTLIBRARIES = libgeoxml.la

libgeoxml_la_SOURCES=flow.c program.c
# libgeoxml_la_HEADERS=geoxml.h flow.h program.c

libgeoxml_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@

#->gdome2 stuff
AM_CFLAGS=$(GDOME2_CFLAGS)
libgeoxml_la_LIDADD=$(GDOME2_LIBS)
