/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UI_SERVERS_H
#define __UI_SERVERS_H

#include <gtk/gtk.h>

/* Store field */
enum {
	SERVER_ADDRESS = 0,
	SERVER_POINTER,
	SERVER_N_COLUMN
};

struct ui_server_list {
	GtkWidget *		dialog;

	GtkListStore *		store;
	GtkWidget *		view;
};

struct ui_server_list
server_list_setup_ui(void);

#endif //__UI_SERVERS_H
