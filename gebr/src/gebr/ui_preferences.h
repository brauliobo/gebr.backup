/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifndef __UI_PREFERENCES_H
#define __UI_PREFERENCES_H

#include <gtk/gtk.h>

struct ui_preferences {
	GtkWidget *		dialog;

	gboolean		first_run;

	GtkWidget *		username;
	GtkWidget *		email;
	GtkWidget *		usermenus;
	GtkWidget *		editor;
	GtkWidget *		browser;
};

struct ui_preferences *
preferences_setup_ui(gboolean first_run);

#endif //__UI_PREFERENCES_H
