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

#include "ui_flow.h"
#include "gebr.h"
#include "support.h"

extern gchar * no_flow_selected_error;

void
flow_io_setup_ui(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel* 		model;
	GtkWidget *		dialog;
	GtkWidget *		table;
	GtkWidget *		label;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.flow_view));
	if(!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}



	dialog = gtk_dialog_new_with_buttons(_("Flow input/output"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	gebr.flow_io.win = GTK_WIDGET(dialog);
	gtk_widget_set_size_request(dialog, 390, 160);

	g_signal_connect_swapped(dialog, "response",
				G_CALLBACK(flow_io_actions),
				dialog);
	g_signal_connect(GTK_OBJECT(dialog), "delete_event",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), NULL);

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Input */
	label = gtk_label_new(_("Input file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_io.input = save_widget_create();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_io.input.hbox, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL,  3, 3);
	gtk_entry_set_text(GTK_ENTRY(gebr.flow_io.input.entry), geoxml_flow_io_get_input(flow));
	gtk_widget_set_size_request(gebr.flow_io.input.hbox, 140, 30);

	/* Output */
	label = gtk_label_new(_("Output file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_io.output = save_widget_create();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_io.output.hbox, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(gebr.flow_io.output.entry), geoxml_flow_io_get_output(flow));
	gtk_widget_set_size_request(gebr.flow_io.output.hbox, 140, 30);

	/* Error */
	label = gtk_label_new(_("Error log file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.flow_io.error = save_widget_create();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), gebr.flow_io.error.hbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(gebr.flow_io.error.entry), geoxml_flow_io_get_error(flow));
	gtk_widget_set_size_request(gebr.flow_io.error.hbox, 140, 30);

	gtk_widget_show_all(dialog);

	return;
}

void
flow_io_actions(GtkDialog *dialog, gint arg1)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		geoxml_flow_io_set_input(gebr.flow,
			gtk_entry_get_text(GTK_ENTRY(gebr.flow_io.input.entry)));
		geoxml_flow_io_set_output(gebr.flow,
			gtk_entry_get_text(GTK_ENTRY(gebr.flow_io.output.entry)));
		geoxml_flow_io_set_error(gebr.flow,
			gtk_entry_get_text(GTK_ENTRY(gebr.flow_io.error.entry)));

		flow_save();
		flow_info_update();
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(gebr.flow_io.win));
}
