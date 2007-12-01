/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team(http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or(at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/*
 * File: callbacks.c
 * Callbacks
 */

#include "callbacks.h"
#include "gebr.h"
#include "ui_document.h"

/*
 * Function: switch_page
 * Hide/Show the corresponding menu to the selected page
 *
 */
void
switch_page(GtkNotebook * notebook, GtkNotebookPage * page, guint page_num)
{
	switch (page_num) {
	case 0: /* Project page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
		break;
	case 1: /* Flow browse page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
		break;
	case 2: /* Flow edit page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", TRUE, NULL);
		break;
	case 3: /* Job control page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
		break;
	default:
		break;
	}
}
