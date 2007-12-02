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

#include <gui/gtkfileentry.h>

#include "ui_flow.h"
#include "gebr.h"
#include "support.h"
#include "flow.h"
#include "ui_flow_browse.h"

extern gchar * no_flow_selected_error;

/*
 * Prototypes
 */

void
flow_io_actions(GtkDialog * dialog, gint arg1);

struct ui_flow_io
flow_io_setup_ui(void)
{
	struct ui_flow_io	ui_flow_io;

	GtkWidget *		dialog;
	GtkWidget *		table;
	GtkWidget *		label;

	if (gebr.flow == NULL) {
		gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
		return ui_flow_io;
	}

	dialog = gtk_dialog_new_with_buttons(_("Flow input/output"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	ui_flow_io.dialog = dialog;
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
	ui_flow_io.input = gtk_file_entry_new();
	gtk_widget_set_size_request(ui_flow_io.input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_flow_io.input, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL,  3, 3);
	/* read */
	gtk_file_entry_set_path(GTK_FILE_ENTRY(ui_flow_io.input), geoxml_flow_io_get_input(gebr.flow));

	/* Output */
	label = gtk_label_new(_("Output file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	ui_flow_io.output = gtk_file_entry_new();
	gtk_widget_set_size_request(ui_flow_io.output, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_flow_io.output, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_file_entry_set_path(GTK_FILE_ENTRY(ui_flow_io.output), geoxml_flow_io_get_output(gebr.flow));

	/* Error */
	label = gtk_label_new(_("Error log file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	ui_flow_io.error = gtk_file_entry_new();
	gtk_widget_set_size_request(ui_flow_io.error, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_flow_io.error, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_file_entry_set_path(GTK_FILE_ENTRY(ui_flow_io.error), geoxml_flow_io_get_error(gebr.flow));

	gtk_widget_show_all(dialog);

	return ui_flow_io;
}

void
flow_io_actions(GtkDialog * dialog, gint arg1)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		geoxml_flow_io_set_input(gebr.flow,
			gtk_file_entry_get_path(GTK_FILE_ENTRY(gebr.ui_flow_io.input)));
		geoxml_flow_io_set_output(gebr.flow,
			gtk_file_entry_get_path(GTK_FILE_ENTRY(gebr.ui_flow_io.output)));
		geoxml_flow_io_set_error(gebr.flow,
			gtk_file_entry_get_path(GTK_FILE_ENTRY(gebr.ui_flow_io.error)));

		flow_save();
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(gebr.ui_flow_io.dialog));
}
