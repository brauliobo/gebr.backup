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

#ifndef __UI_FLOW_H
#define __UI_FLOW_H

#include <gui/gtkfileentry.h>

struct ui_flow_io {
	GtkWidget *		dialog;

	GtkFileEntry *		input;
	GtkFileEntry *		output;
	GtkFileEntry *		error;
};

struct ui_flow_io
flow_io_setup_ui(void);

#endif //__UI_FLOW_H
