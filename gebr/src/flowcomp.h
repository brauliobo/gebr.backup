/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This flow_component is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This flow_component is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this flow_component.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GEBR_CB_FLOWCOMP_H_
#define _GEBR_CB_FLOWCOMP_H_

#include <gtk/gtk.h>

void
flow_component_properties_set      (void);

void flow_component_set_status	(GtkMenuItem *menuitem);

void
flow_component_add_to_flow(GtkButton *button,
			  gpointer user_data);

void
flow_component_remove_from_flow      (GtkButton *button,
			       gpointer user_data);
void
flow_component_move_down    (GtkButton *button,
		      gpointer user_data);
void
flow_component_move_up    (GtkButton *button,
		    gpointer user_data);

#endif //_GEBR_CB_FLOWCOMP_H_
