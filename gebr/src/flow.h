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

#ifndef __FLOW_H
#define __FLOW_H

#include <gtk/gtk.h>
#include <geoxml.h>

void
flow_load(void);

int
flow_save   (void);

void
flow_export(void);

void
flow_free(void);

void
flow_new(void);

void
flow_delete(void);

void
flow_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text);

void
flow_program_move_down(void);

void
flow_run(void);

#endif //__FLOW_H
