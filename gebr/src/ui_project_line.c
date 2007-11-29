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

#include "ui_project_line.h"

/*
 * Function: project_line_setup_ui
 * Assembly the project/lines widget.
 *
 * Return:
 * The structure containing relevant data.
 *
 * TODO:
 * Add an info summary about the project/line.
 */
struct ui_project_line
project_line_setup_ui(void)
{
	struct ui_project_line	ui_project_line;
	GtkWidget *		ui_project_line.widget;
	GtkWidget *		scrolledwin;

	GtkTreeSelection *selection;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* Create projects/lines ui_project_line.widget */
	ui_project_line.widget = gtk_vbox_new (FALSE, 0);

	/* Project and line tree */
	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add (GTK_CONTAINER (ui_project_line.widget), scrolledwin);

	gebr.proj_line_store = gtk_tree_store_new(PL_N_COLUMN,
						G_TYPE_STRING,  /* Name (title for libgeoxml) */
						G_TYPE_STRING); /* Filename */

	gebr.proj_line_view = gtk_tree_view_new_with_model
	(GTK_TREE_MODEL (gebr.proj_line_store));

	gtk_container_add (GTK_CONTAINER (scrolledwin), gebr.proj_line_view);

	/* Projects/lines column */
	renderer = gtk_cell_renderer_text_new ();

	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (GTK_OBJECT (renderer), "edited",
			GTK_SIGNAL_FUNC (proj_line_rename), NULL );

	col = gtk_tree_view_column_new_with_attributes ("Projects/lines index", renderer, NULL);
	gtk_tree_view_column_set_sort_column_id (col, PL_NAME);
	gtk_tree_view_column_set_sort_indicator (col, TRUE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (gebr.proj_line_view), col);
	gtk_tree_view_column_add_attribute (col, renderer, "text", PL_NAME);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.proj_line_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (GTK_OBJECT (gebr.proj_line_view), "cursor-changed",
			GTK_SIGNAL_FUNC (line_load_flows), NULL );

	gebr.proj_line_selection_path = NULL;
}
