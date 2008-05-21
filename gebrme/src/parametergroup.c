/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <gui/utils.h>

#include "groupparameters.h"
#include "support.h"
#include "parameter.h"
#include "menu.h"

/*
 * File: parametergroup.c
 * Create dialog for editing a parameter of type group
 */


/*
 * Section: Public
 */

/*
 * Function: parameter_group_setup_ui
 * Open a dialog to configure a group
 */
void
parameter_group_setup_ui(void)
{
	GtkWidget *	dialog;
	GtkWidget *	table;;
	
	dialog = gtk_dialog_new_with_buttons(_("Edit group"),
		GTK_WINDOW(gebrme.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 400, 300);

	table = gtk_table_new(1, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	if (geoxml_parameter_group_get_can_instanciate(GEOXML_PARAMETER_GROUP(data->parameter)) == TRUE) {
		GtkWidget *	instances_label;
		GtkWidget *	instances_spin;

		instances_label = gtk_label_new(_("Instances:"));
		gtk_widget_show(instances_label);
		gtk_table_attach(GTK_TABLE(table), instances_label, 0, 1, 0, 1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(GTK_FILL), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(instances_label), 0, 0.5);

		instances_spin = gtk_spin_button_new_with_range(1, 999999999, 1);
		gtk_widget_show(instances_spin);
		gtk_table_attach(GTK_TABLE(table), instances_spin, 1, 2, 0, 1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(instances_spin),
			geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(data->parameter)));
		g_signal_connect(instances_spin, "output",
			(GCallback)parameter_group_instances_changed, data);
	}

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
