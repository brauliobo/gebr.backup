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

#include "ui_job_control.h"

/*
 * Function: ui_job_control
 * Assembly the job control page.
 *
 * Return:
 * The structure containing relevant data.
 *
 */
struct ui_job_control
job_control_setup_ui(void)
{
	struct ui_job_control	ui_job_control;
	GtkWidget *page;
	GtkWidget *vbox;

	GtkWidget *toolbar;
	GtkIconSize tmp_toolbar_icon_size;
	GtkWidget *toolitem;
	GtkWidget *button;

	GtkWidget *hpanel;
	GtkWidget *scrolledwin;
	GtkWidget *frame;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *text_view;

	/* Create flow edit page */
	page = gtk_vbox_new (FALSE, 0);

	/* Vbox to hold toolbar and main content */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (page), vbox);

	/* Toolbar */
	toolbar = gtk_toolbar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	/* FIXME ! */
	/* g_object_set_property(G_OBJECT(toolbar), "shadow-type", GTK_SHADOW_NONE); */

	tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar));

	/* Cancel button = END */
	toolitem = (GtkWidget*) gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (toolitem), button);

	g_signal_connect (GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_cancel), NULL);

	/* Close button */
	toolitem = (GtkWidget*) gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (toolitem), button);

	g_signal_connect (GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_close), NULL);

	/* Clear button */
	toolitem = (GtkWidget*) gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

	button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
	gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (toolitem), button);

	g_signal_connect (GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_clear), NULL);

	/* Stop button = KILL */
	toolitem = (GtkWidget*) gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

	button = gtk_button_new_from_stock (GTK_STOCK_STOP);
	gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (toolitem), button);

	g_signal_connect (GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_stop), NULL);

	hpanel = gtk_hpaned_new ();
	gtk_box_pack_start (GTK_BOX (vbox), hpanel, TRUE, TRUE, 0);

	/* Left side */
	frame = gtk_frame_new ("Jobs");
	gtk_paned_pack1 (GTK_PANED (hpanel), frame, FALSE, FALSE);

	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (frame), scrolledwin);

	gebr.job_store = gtk_list_store_new(JC_N_COLUMN,
					GDK_TYPE_PIXBUF,	/* Icon		*/
					G_TYPE_STRING,		/* Title	*/
					G_TYPE_POINTER);	/* struct job	*/

	gebr.job_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gebr.job_store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(gebr.job_view), FALSE);

	g_signal_connect (GTK_OBJECT (gebr.job_view), "cursor-changed",
		GTK_SIGNAL_FUNC (job_clicked), NULL);

	renderer = gtk_cell_renderer_pixbuf_new ();
	col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (gebr.job_view), col);
	gtk_tree_view_column_add_attribute (col, renderer, "pixbuf", JC_ICON);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (gebr.job_view), col);
	gtk_tree_view_column_add_attribute (col, renderer, "text", JC_TITLE);

	gtk_container_add (GTK_CONTAINER (scrolledwin), gebr.job_view);
	gtk_widget_set_size_request (GTK_WIDGET (scrolledwin), 180, 30);

	/* Right side */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2 (GTK_PANED (hpanel), vbox, TRUE, TRUE);

	gebr.job_ctrl.job_label = gtk_label_new("");
	gtk_box_pack_start (GTK_BOX (vbox), gebr.job_ctrl.job_label, FALSE, TRUE, 0);

	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_end (GTK_BOX (vbox), scrolledwin, TRUE, TRUE, 0);

	gebr.job_ctrl.text_buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(gebr.job_ctrl.text_buffer);
	g_object_set(G_OBJECT(text_view),
		"editable", FALSE,
		"cursor-visible", FALSE,
		NULL);
	gebr.job_ctrl.text_view = text_view;
	gtk_container_add (GTK_CONTAINER (scrolledwin), text_view);
}
