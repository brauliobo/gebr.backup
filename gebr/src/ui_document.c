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

#include "ui_document.h"

/*
 * Function: document_properties_setup_ui
 * Show the _document_ properties in a dialog
 * Create the user interface for editing _document_ (flow, line or project) properties,
 * like author, email, report, etc.
 */
void
document_properties_setup_ui(GeoXmlDocument * document);
{
	GtkTreeIter       iter;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.flow_view));
	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_selected_error, TRUE);
		return;
	}

	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *label;

	dialog = gtk_dialog_new_with_buttons("Flow properties",
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);

	gebr.flow_prop.win = GTK_WIDGET(dialog);

	g_signal_connect_swapped(dialog, "response",
				G_CALLBACK(flow_properties_actions),
				dialog);

	gtk_widget_set_size_request(dialog, 390, 260);

	g_signal_connect(GTK_OBJECT(dialog), "delete_event",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), NULL );


	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Title */
	label = gtk_label_new("Title");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_prop.title = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_prop.title, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(gebr.flow_prop.title), geoxml_document_get_title( GEOXML_DOC(flow)));

	/* Description */
	label = gtk_label_new("Description");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_prop.description = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_prop.description, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(gebr.flow_prop.description), geoxml_document_get_description( GEOXML_DOC(flow)));

	/* Help */
	label = gtk_label_new("Help");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_prop.help = gtk_button_new_from_stock( GTK_STOCK_EDIT );
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_prop.help, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	g_signal_connect(GTK_OBJECT(gebr.flow_prop.help), "clicked",
			GTK_SIGNAL_FUNC(help_edit), flow );

	/* Author */
	label = gtk_label_new("Author");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_prop.author = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_prop.author, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(gebr.flow_prop.author), geoxml_document_get_author( GEOXML_DOC(flow)));

	/* User email */
	label = gtk_label_new("Email");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_prop.email = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_prop.email, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(gebr.flow_prop.email), geoxml_document_get_email( GEOXML_DOC(flow)));

	gtk_widget_show_all(dialog);

	return;
}

/*
 * Function: document_properties_actions
 * Take the appropriate action when the document properties dialog emmits
 * a response signal.
 */
void
document_properties_actions(GtkDialog * dialog, gint arg1)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		GtkTreeIter		iter;
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;

		geoxml_document_set_title(GEOXML_DOC(flow), gtk_entry_get_text( GTK_ENTRY(gebr.flow_prop.title)));
		geoxml_document_set_description(GEOXML_DOC(flow), gtk_entry_get_text( GTK_ENTRY(gebr.flow_prop.description)));
		geoxml_document_set_author(GEOXML_DOC(flow), gtk_entry_get_text( GTK_ENTRY(gebr.flow_prop.author)));
		geoxml_document_set_email(GEOXML_DOC(flow), gtk_entry_get_text( GTK_ENTRY(gebr.flow_prop.email)));

		/* Update flow title in flow_store */
		{


		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.flow_view));
		if(gtk_tree_selection_get_selected(selection, &model, &iter)){

			gtk_list_store_set(gebr.flow_store, &iter,
					FB_NAME, geoxml_document_get_title(GEOXML_DOC(flow)),
					-1);
		}
		}

		flow_save();
		flow_info_update();
		break;
	} default:                  /* does nothing */
		break;
	}
	gtk_widget_destroy(GTK_WIDGET(gebr.flow_prop.win));
}
