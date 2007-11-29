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

#include "ui_flowcomp.h"
#include "gebr.h"
#include "support.h"
#include "flow.h"

gchar * no_flow_comp_selected_error = "No flow component selected";

/* Function: flow_edition_setup_ui
 * Assembly the flow edit ui_flow_edition.widget.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_flow_edition
flow_edition_setup_ui(void)
{
	struct ui_flow_edition		ui_flow_edition;
	GtkWidget *			hpanel;

	/* Create flow edit ui_flow_edition.widget */
	ui_flow_edition.widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(ui_flow_edition.widget), hpanel);

	/* Left side */
	{
		GtkWidget *		hpanel;
		GtkWidget *		scrolledwin;
		GtkWidget *		frame;
		GtkTreeViewColumn *	col;
		GtkCellRenderer *	renderer;

		frame = gtk_frame_new(_("Flow sequence"));
		gtk_paned_pack1 (GTK_PANED (hpanel), frame, FALSE, FALSE);

		scrolledwin = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER(frame), scrolledwin);

		ui_flow_edition.fseq_store = gtk_list_store_new(FSEQ_N_COLUMN,
						GDK_TYPE_PIXBUF,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_ULONG);

		ui_flow_edition.fseq_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ui_flow_edition.fseq_store));
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(ui_flow_edition.fseq_view), FALSE);

		renderer = gtk_cell_renderer_pixbuf_new ();
		col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (ui_flow_edition.fseq_view), col);
		gtk_tree_view_column_add_attribute (col, renderer, "pixbuf", FSEQ_STATUS_COLUMN);

		renderer = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (ui_flow_edition.fseq_view), col);
		gtk_tree_view_column_add_attribute (col, renderer, "text", FSEQ_TITLE_COLUMN);

		/* Double click on flow component open its parameter window */
		g_signal_connect (ui_flow_edition.fseq_view, "row-activated",
				(GCallback) progpar_config_window, NULL );
		g_signal_connect (GTK_OBJECT (ui_flow_edition.fseq_view), "cursor-changed",
				GTK_SIGNAL_FUNC (flow_edition_selected), NULL );

		gtk_container_add (GTK_CONTAINER (scrolledwin), ui_flow_edition.fseq_view);
		gtk_widget_set_size_request (GTK_WIDGET (scrolledwin), 180, 30);
	}

	/* Right side */
	{
		GtkWidget *	hbox;
		GtkWidget *	button;
		GtkWidget *	vbox;

		frame = gtk_frame_new(_("Flow components"));
		gtk_paned_pack2 (GTK_PANED (hpanel), frame, TRUE, TRUE);

		hbox = gtk_hbox_new (FALSE, 1);
		gtk_container_add (GTK_CONTAINER (frame), hbox);

		/*
		 * Up, Down, Right and Left buttons
		 */
		vbox = gtk_vbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

		button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_ADD, 1));
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (program_add_to_flow), NULL );

		button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_REMOVE, 1));
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (program_remove_from_flow), NULL );

		button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_HELP, 1));
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (menu_show_help), NULL );

		button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, 1));
		gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (program_move_down), NULL );

		button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_GO_UP, 1));
		gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (program_move_up), NULL );


		/*
		 * Menu list
		 */
		vbox = gtk_vbox_new (FALSE, 3);
		gtk_box_pack_end (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

		scrolledwin = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER (vbox), scrolledwin);

		ui_flow_edition.menu_store = gtk_tree_store_new (MENU_N_COLUMN,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING);

		ui_flow_edition.menu_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ui_flow_edition.menu_store));

		g_signal_connect (GTK_OBJECT (ui_flow_edition.menu_view), "row-activated",
				GTK_SIGNAL_FUNC (program_add_to_flow), NULL );

		renderer = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes ("Flow", renderer, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (ui_flow_edition.menu_view), col);
		gtk_tree_view_column_add_attribute (col, renderer, "markup", MENU_TITLE_COLUMN);
		gtk_tree_view_column_set_sort_column_id (col, MENU_TITLE_COLUMN);
		gtk_tree_view_column_set_sort_indicator (col, TRUE);

		renderer = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes ("Description", renderer, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (ui_flow_edition.menu_view), col);
		gtk_tree_view_column_add_attribute (col, renderer, "text", MENU_DESC_COLUMN);

		gtk_container_add (GTK_CONTAINER (scrolledwin), ui_flow_edition.menu_view);
	}

	return ui_flow_edition;
}

/* Function: flow_edition_selected
 * When a flow component (a program in the flow) is selected
 * this funtions get the state of the program and set it on Flow Component Menu
 *
 * PS: this function is called when the signal "cursor-changed" is triggered
 * also by hand.
 */
void
flow_edition_selected(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreePath *		path;
	GeoXmlProgram *		program;
	gchar *			status;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.fseq_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_comp_selected_error, TRUE);
		return;
	}

	path = gtk_tree_model_get_path(model, &iter);
	geoxml_flow_get_program(flow, &program, gtk_tree_path_get_indices(path)[0]);
	status = (gchar *)geoxml_program_get_status(program);

	if (!g_ascii_strcasecmp(status, "configured"))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gebr.configured_menuitem), TRUE);
	else if (!g_ascii_strcasecmp(status, "disabled"))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gebr.disabled_menuitem), TRUE);
	else if (!g_ascii_strcasecmp(status, "unconfigured"))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gebr.unconfigured_menuitem), TRUE);

	gtk_tree_path_free(path);
}

/*
 * Function: flow_edition_change_properties
 * Show the current selected flow components parameters
 */
void
flow_edition_change_properties(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	gchar *			name;
	gchar			message[STRMAX];

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.fseq_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_comp_selected_error, TRUE);
		return;
	}

	gtk_tree_model_get (model, &iter, FSEQ_TITLE_COLUMN, &name, -1);

	sprintf (message, "Configuring flow component '%s'", name);
	log_message(INTERFACE, message, TRUE);

	progpar_config_window();

	g_free(name);
}

/*
 * Function: flow_edition_change_properties
 * Change the flow status when select the status from the "Flow Component" menu.
 */
void
flow_edition_set_status(GtkMenuItem * menuitem)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreePath *		path;
	GtkWidget *		status_menuitem;
	GeoXmlProgram *		program;
	GdkPixbuf *		pixbuf;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.fseq_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_comp_selected_error, TRUE);
		return;
	}

	status_menuitem = GTK_WIDGET(menuitem);
	path = gtk_tree_model_get_path(model, &iter);
	geoxml_flow_get_program(flow, &program, gtk_tree_path_get_indices(path)[0]);
	gtk_tree_path_free(path);

	if (status_menuitem == gebr.configured_menuitem) {
		geoxml_program_set_status(program, "configured");
		pixbuf = gebr.pixmaps.configured_icon;
	} else if (status_menuitem == gebr.disabled_menuitem) {
		geoxml_program_set_status(program, "disabled");
		pixbuf = gebr.pixmaps.disabled_icon;
	} else if (status_menuitem == gebr.unconfigured_menuitem) {
		geoxml_program_set_status(program, "unconfigured");
		pixbuf = gebr.pixmaps.unconfigured_icon;
	} else
		return;

	gtk_list_store_set (gebr.fseq_store, &iter,
			    FSEQ_STATUS_COLUMN, pixbuf, -1);

	flow_save();
}
