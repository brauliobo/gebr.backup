/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team(http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_server.c
 * Server related UI.
 *
 * Assembly servers list configuration dialog and
 * server select dialog for flow execution (TODO).
 */

#include <string.h>

#include "ui_server.h"
#include "gebr.h"
#include "support.h"
#include "server.h"

#define GEBR_SERVER_CLOSE	101
#define GEBR_SERVER_REMOVE	102

/*
 * Prototypes
 */

static void
server_list_actions(GtkDialog * dialog, gint arg1, struct ui_server_list * ui_server_list);

static void
server_list_add(GtkEntry * entry, struct ui_server_list * ui_server_list);

/*
 * Function: server_list_setup_ui
 * Assembly the servers configurations dialog.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_server_list *
server_list_setup_ui(void)
{
	struct ui_server_list *		ui_server_list;

	GtkWidget *			dialog;
	GtkWidget *			label;
	GtkWidget *			entry;

	GtkTreeViewColumn *		col;
	GtkCellRenderer *		renderer;

	/* alloc */
	ui_server_list = g_malloc(sizeof(struct ui_server_list));

	ui_server_list->store = gtk_list_store_new(SERVER_N_COLUMN,
					G_TYPE_STRING,
					G_TYPE_POINTER);

	dialog = gtk_dialog_new_with_buttons(_("Servers configuration"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_REMOVE, GEBR_SERVER_REMOVE,
						GTK_STOCK_CLOSE, GEBR_SERVER_CLOSE,
						NULL);
	ui_server_list->dialog = dialog;

	/* Take the apropriate action when a button is pressed */
	g_signal_connect(dialog, "response",
				G_CALLBACK(server_list_actions), ui_server_list);

	gtk_widget_set_size_request(dialog, 380, 300);

	label = gtk_label_new(_("Remote server hostname:"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

	entry = gtk_entry_new();
	g_signal_connect(GTK_OBJECT(entry), "activate",
			GTK_SIGNAL_FUNC(server_list_add), ui_server_list);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);

	ui_server_list->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_server_list->store));

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Servers"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id (col, SERVER_ADDRESS);
	gtk_tree_view_column_set_sort_indicator (col, TRUE);

	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_server_list->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_ADDRESS);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_server_list->view, TRUE, TRUE, 0);

	return ui_server_list;
}

/*
 * Function: server_dialog_actions
 * Take the appropriate action when the server dialog emmits
 * a response signal.
 */
static void
server_list_actions(GtkDialog * dialog, gint arg1, struct ui_server_list * ui_server_list)
{
	switch (arg1) {
	case GEBR_SERVER_CLOSE:
		gtk_widget_hide(ui_server_list->dialog);
		break;
	case GEBR_SERVER_REMOVE: {
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;
		GtkTreeIter		iter;

		struct server *		server;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_server_list->view));
		if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
			return;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
				SERVER_POINTER, &server,
				-1);
		server_free(server);

		gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_server_list->store), &iter);
		break;
	} default:
		break;
	}

	gebr_config_save();
}

/*
 * Function: server_list_add
 * Callback to add a server to the server list
 */
static void
server_list_add(GtkEntry * entry, struct ui_server_list * ui_server_list)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* check if it is already in list */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	while (valid) {
		gchar *	server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
				SERVER_ADDRESS, &server,
				-1);

		if (!g_ascii_strcasecmp(server, gtk_entry_get_text(entry))) {
			GtkTreeSelection *	selection;

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_server_list->view));
			gtk_tree_selection_select_iter(selection, &iter);

			g_free(server);
			return;
		}

		g_free(server);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	}

	/* add to store (creating a new server structure) and ... */
	gtk_list_store_append(gebr.ui_server_list->store, &iter);
	gtk_list_store_set(gebr.ui_server_list->store, &iter,
			SERVER_ADDRESS, gtk_entry_get_text(entry),
			SERVER_POINTER, server_new(gtk_entry_get_text(entry)),
			-1);
	/* reset entry. */
	gtk_entry_set_text(entry, "");
}
