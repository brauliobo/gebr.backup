# Debr - GeBR Designer
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

bin_PROGRAMS = debr

debr_SOURCES = \
	main.cc \
	interface.cc \
	callbacks.cc \
	debr.cc \
	menu.cc \
	program.cc \
	parameter.cc \
	parametergroup.cc \
	enumoptionedit.cc \
	validate.cc \
	help.cc \
	preferences.cc
noinst_HEADERS = \
	interface.h \
	callbacks.h \
	debr.h \
	menu.h \
	program.h \
	parameter.h \
	parametergroup.h \
	enumoptionedit.h \
	validate.h \
	help.h \
	preferences.h
EXTRA_DIST = $(noinst_HEADERS)

INCLUDES = -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" $(WEBKIT_ENABLED)
AM_CXXFLAGS = -Wall -std=c++0x @DEBUG_CFLAGS@ $(GTK_CFLAGS) \
	$(GEBR_CFLAGS) $(GEBR_GEOXML_CFLAGS) $(GEBR_GUI_CFLAGS)
debr_LDADD = $(INTLLIBS) $(GTK_LIBS) \
	$(GEBR_LIBS) $(GEBR_GEOXML_LIBS) $(GEBR_GUI_LIBS)
