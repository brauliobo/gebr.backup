# libgebr - GeBR Library
# Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
include $(top_srcdir)/Makefile.decl

if ENABLE_TESTS
TESTSDIR=tests
endif
SUBDIRS = . data validate upgrade menu-diff menu-edit menu-query flow-dump ${TESTSDIR}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr-geoxml.pc

man_MANS = gebr-geoxml.1
EXTRA_DIST =			\
	gebr-geoxml.pc		\
	gebr-geoxml.1

libgebr_geoxmlincludedir=$(includedir)/libgebr
libgebr_geoxmlinclude_HEADERS=geoxml.h

lib_LTLIBRARIES = libgebr_geoxml.la
libgebr_geoxml_la_SOURCES =		\
	gebr-geoxml-clipboard.c		\
	gebr-geoxml-document.c		\
	gebr-geoxml-enum-option.c	\
	gebr-geoxml-error.c		\
	gebr-geoxml-flow.c		\
	gebr-geoxml-line.c		\
	gebr-geoxml-object.c		\
	gebr-geoxml-parameter.c		\
	gebr-geoxml-parameter-group.c	\
	gebr-geoxml-parameters.c	\
	gebr-geoxml-program-parameter.c	\
	gebr-geoxml-program.c		\
	gebr-geoxml-project.c		\
	gebr-geoxml-sequence.c		\
	gebr-geoxml-tmpl.c		\
	gebr-geoxml-validate.c		\
	gebr-geoxml-value-sequence.c	\
	priv-gebr-geoxml-xml.c		\
	$(NULL)

libgebr_geoxmlsubincludedir=$(includedir)/libgebr/geoxml
libgebr_geoxmlsubinclude_HEADERS =	\
	gebr-geoxml-clipboard.h		\
	gebr-geoxml-document.h		\
	gebr-geoxml-enum-option.h	\
	gebr-geoxml-error.h		\
	gebr-geoxml-flow.h		\
	gebr-geoxml-line.h		\
	gebr-geoxml-object.h		\
	gebr-geoxml-parameter-group.h 	\
	gebr-geoxml-parameter.h		\
	gebr-geoxml-parameters.h	\
	gebr-geoxml-program-parameter.h	\
	gebr-geoxml-program.h 		\
	gebr-geoxml-project.h		\
	gebr-geoxml-sequence.h		\
	gebr-geoxml-tmpl.h		\
	gebr-geoxml-validate.h		\
	gebr-geoxml-value-sequence.h	\
	$(NULL)

noinst_HEADERS =				\
	priv-gebr-geoxml-document.h		\
	priv-gebr-geoxml-parameter.h		\
	priv-gebr-geoxml-parameter-group.h	\
	priv-gebr-geoxml-parameters.h		\
	priv-gebr-geoxml-program.h		\
	priv-gebr-geoxml-program-parameter.h	\
	priv-gebr-geoxml-sequence.h		\
	priv-gebr-geoxml-types.h		\
	priv-gebr-geoxml-xml.h			\
	$(NULL)

AM_CPPFLAGS = -I$(top_srcdir)		\
	      -I$(top_srcdir)/libgebr	\
	      -I..			\
	      $(GLIB_CFLAGS)		\
	      $(GDOME2_CFLAGS)		\
	      @DEBUG_CFLAGS@

AM_CFLAGS = $(COMMON_CFLAGS) -fPIC
libgebr_geoxml_la_LIBADD = ../libgebr.la $(GLIB_LIBS) $(GDOME2_LIBS)
libgebr_geoxml_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@
