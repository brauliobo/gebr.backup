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

#include "ui_flow_browse.h"

/*
 * Function: add_flow_browse
 * Assembly the flow browse page.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_flow_browse
flow_browse_setup_ui(void)
{
	GtkWidget         *page;
	GtkWidget         *hpanel;
	GtkWidget         *scrolledwin;
	GtkTreeSelection  *selection;
	GtkTreeViewColumn *col;
	GtkCellRenderer   *renderer;
	static const char *label = "Flows";

	/* Create flow browse page */
	page = gtk_vbox_new (FALSE, 0);

	hpanel = gtk_hpaned_new ();
	gtk_container_add (GTK_CONTAINER (page), hpanel);

	/*
	 * Left side
	 */
	/* Flow list */
	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_paned_pack1 (GTK_PANED (hpanel), scrolledwin, FALSE, FALSE);
	gtk_widget_set_size_request (scrolledwin, 180, -1);

	gebr.flow_store = gtk_list_store_new (FB_N_COLUMN,
					G_TYPE_STRING,  /* Name (title for libgeoxml) */
					G_TYPE_STRING); /* Filename */

	gebr.flow_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gebr.flow_store));

	renderer = gtk_cell_renderer_text_new ();

	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (GTK_OBJECT (renderer), "edited",
			GTK_SIGNAL_FUNC  (flow_rename), NULL );

	g_signal_connect (GTK_OBJECT (renderer), "edited",
			GTK_SIGNAL_FUNC  (flow_info_update), NULL );

	col = gtk_tree_view_column_new_with_attributes  (label, renderer, NULL);
	gtk_tree_view_column_set_sort_column_id  (col, FB_NAME);
	gtk_tree_view_column_set_sort_indicator  (col, TRUE);
	gtk_tree_view_append_column  (GTK_TREE_VIEW (gebr.flow_view), col);
	gtk_tree_view_column_add_attribute  (col, renderer, "text", FB_NAME);

	selection = gtk_tree_view_get_selection  (GTK_TREE_VIEW (gebr.flow_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	gtk_container_add (GTK_CONTAINER (scrolledwin), gebr.flow_view);

	g_signal_connect (GTK_OBJECT (gebr.flow_view), "cursor-changed",
			GTK_SIGNAL_FUNC (flow_load), NULL );

	g_signal_connect (GTK_OBJECT (gebr.flow_view), "cursor-changed",
			GTK_SIGNAL_FUNC (flow_info_update), NULL );

	/* Right side */
	/* Flow info  */
	{
	GtkWidget *frame;
	GtkWidget *infopage;

	frame = gtk_frame_new ("Details");
	gtk_paned_pack2 (GTK_PANED (hpanel), frame, TRUE, FALSE);

	infopage = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), infopage);

	/* Title */
	gebr.flow_info.title = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.title), 0, 0);
	gtk_box_pack_start (GTK_BOX (infopage), gebr.flow_info.title, FALSE, TRUE, 0);

	/* Description */
	gebr.flow_info.description = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.description), 0, 0);
	gtk_box_pack_start (GTK_BOX (infopage), gebr.flow_info.description, FALSE, TRUE, 10);

	/* I/O */
	GtkWidget *table;
	table = gtk_table_new (3, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (infopage), table, FALSE, TRUE, 0);

	gebr.flow_info.input_label = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.input_label), 0, 0);
	gtk_table_attach (GTK_TABLE (table), gebr.flow_info.input_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	gebr.flow_info.input = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.input), 0, 0);
	gtk_table_attach (GTK_TABLE (table), gebr.flow_info.input, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	gebr.flow_info.output_label = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.output_label), 0, 0);
	gtk_table_attach (GTK_TABLE (table), gebr.flow_info.output_label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	gebr.flow_info.output = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.output), 0, 0);
	gtk_table_attach (GTK_TABLE (table), gebr.flow_info.output, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	gebr.flow_info.error_label = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.error_label), 0, 0);
	gtk_table_attach (GTK_TABLE (table), gebr.flow_info.error_label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

	gebr.flow_info.error = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.error), 0, 0);
	gtk_table_attach (GTK_TABLE (table), gebr.flow_info.error, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

	/* Help */
	gebr.flow_info.help = gtk_button_new_from_stock ( GTK_STOCK_INFO );
	gtk_box_pack_end (GTK_BOX (infopage), gebr.flow_info.help, FALSE, TRUE, 0);
	g_signal_connect (GTK_OBJECT (gebr.flow_info.help), "clicked",
			GTK_SIGNAL_FUNC (flow_show_help), flow );

	/* Author */
	gebr.flow_info.author = gtk_label_new("");
	gtk_misc_set_alignment ( GTK_MISC(gebr.flow_info.author), 0, 0);
	gtk_box_pack_end (GTK_BOX (infopage), gebr.flow_info.author, FALSE, TRUE, 0);

	}
}
