/*   DeBR - GeBR Designer
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

#include <stdlib.h>

#include <gtk/gtk.h>

#include <gui/utils.h>

#include "parametergroup.h"
#include "debr.h"
#include "support.h"
#include "parameter.h"

/*
 * File: parametergroup.c
 * Create dialog for editing a parameter of type group
 */

/*
 * Declarations
 */

static void
parameter_in_group_edit_setup_instances_edit_ui(struct ui_parameter_group_dialog * ui);
static gboolean
on_parameter_group_instances_changed(GtkSpinButton * spin_button, struct ui_parameter_group_dialog * ui);
static void
on_parameter_group_is_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog * ui);
static void
on_parameter_group_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog * ui);

/*
 * Section: Public
 */

/*
 * Function: parameter_group_setup_ui
 * Open a dialog to configure a group
 */
void
parameter_group_dialog_setup_ui(void)
{
	GtkWidget *				dialog;
	GtkWidget *				scrolled_window;
	GtkWidget *				table;
	int					row;
	GtkWidget *				instances_edit_vbox;

	GtkWidget *				label_label;
	GtkWidget *				label_entry;
	GtkWidget *				instanciable_label;
	GtkWidget *				instanciable_check_button;
	GtkWidget *				instances_label;
	GtkWidget *				instances_spin_button;
	GtkWidget *				exclusive_label;
	GtkWidget *				exclusive_check_button;
	GtkWidget *				expanded_label;
	GtkWidget *				expanded_check_button;

	GeoXmlParameterGroup *			parameter_group;
	GeoXmlSequence *			instance;
	GeoXmlSequence *			parameter;
	guint					i;

	struct ui_parameter_group_dialog * ui;

	ui = g_malloc(sizeof(struct ui_parameter_group_dialog));
	ui->parameter_group = parameter_group = GEOXML_PARAMETER_GROUP(debr.parameter);
	ui->dialog = dialog = gtk_dialog_new_with_buttons(_("Edit group"),
		GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 400, 500);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, TRUE, TRUE, 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	table = gtk_table_new(10, 2, FALSE);
	gtk_widget_show(table);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	row = 0;

	/*
	 * Label
	 */
	label_label = gtk_label_new(_("Label:"));
	gtk_widget_show(label_label);
	gtk_table_attach(GTK_TABLE(table), label_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_label), 0, 0.5);

	label_entry = gtk_entry_new();
	gtk_widget_show(label_entry);
	gtk_table_attach(GTK_TABLE(table), label_entry, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;

	/*
	 * Expanded by default
	 */
	expanded_label = gtk_label_new(_("Expanded by default:"));
	gtk_widget_show(expanded_label);
	gtk_table_attach(GTK_TABLE(table), expanded_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(expanded_label), 0, 0.5);

	expanded_check_button = gtk_check_button_new();
	gtk_widget_show(expanded_check_button);
	gtk_table_attach(GTK_TABLE(table), expanded_check_button, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;

	/*
	 * Instanciable
	 */
	instanciable_label = gtk_label_new(_("Instanciable:"));
	gtk_widget_show(instanciable_label);
	gtk_table_attach(GTK_TABLE(table), instanciable_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(instanciable_label), 0, 0.5);

	instanciable_check_button = gtk_check_button_new();
	gtk_widget_show(instanciable_check_button);
	gtk_table_attach(GTK_TABLE(table), instanciable_check_button, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;

	/*
	 * Exclusive
	 */
	exclusive_label = gtk_label_new(_("Is exclusive:"));
	gtk_widget_show(exclusive_label);
	gtk_table_attach(GTK_TABLE(table), exclusive_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(exclusive_label), 0, 0.5);

	exclusive_check_button = gtk_check_button_new();
	gtk_widget_show(exclusive_check_button);
	gtk_table_attach(GTK_TABLE(table), exclusive_check_button, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;

	/*
	 * Instances
	 */
	instances_label = gtk_label_new(_("Instances:"));
	gtk_widget_show(instances_label);
	gtk_table_attach(GTK_TABLE(table), instances_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(instances_label), 0, 0.5);

	instances_spin_button = gtk_spin_button_new_with_range(1, 999999999, 1);
	gtk_widget_show(instances_spin_button);
	gtk_table_attach(GTK_TABLE(table), instances_spin_button, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;
	gtk_widget_set_sensitive(instances_spin_button,
		geoxml_parameter_group_get_instances_number(parameter_group) ? TRUE : FALSE);

	ui->instances_edit_vbox = instances_edit_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(ui->instances_edit_vbox);
	gtk_table_attach(GTK_TABLE(table), instances_edit_vbox, 0, 2, row, row+1,
		(GtkAttachOptions)(GTK_FILL | GTK_EXPAND),
		(GtkAttachOptions)(0), 0, 0), ++row;

	/* group data -> UI */
	gtk_entry_set_text(GTK_ENTRY(label_entry), geoxml_parameter_get_label(debr.parameter));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(expanded_check_button),
		geoxml_parameter_group_get_expand(parameter_group));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(instanciable_check_button),
		geoxml_parameter_group_get_is_instanciable(parameter_group));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(instances_spin_button),
		geoxml_parameter_group_get_instances_number(parameter_group));
	/* scan for an exclusive instance */
	geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exclusive_check_button), TRUE);
	for (i = 0, parameter = NULL; instance != NULL; ++i, geoxml_sequence_next(&instance)) {
		parameter = GEOXML_SEQUENCE(geoxml_parameters_get_exclusive(GEOXML_PARAMETERS(instance)));
		if (parameter == NULL) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exclusive_check_button), FALSE);
			break;
		}
	}
	parameter_in_group_edit_setup_instances_edit_ui(ui);

	/* signals */
	g_signal_connect(instances_spin_button, "output",
		(GCallback)on_parameter_group_instances_changed, ui);
	g_signal_connect(exclusive_check_button, "toggled",
		(GCallback)on_parameter_group_is_exclusive_toggled, ui);

	/* for DeBR it doesn't matter if it's not instanciable */
	geoxml_parameter_group_set_is_instanciable(parameter_group, TRUE);
	/* let the user interact... */
	gtk_dialog_run(GTK_DIALOG(dialog));

	/* things not automatically synced to XML are synced here */
	geoxml_parameter_set_label(debr.parameter, gtk_entry_get_text(GTK_ENTRY(label_entry)));
	geoxml_parameter_group_set_expand(parameter_group,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(expanded_check_button)));
	geoxml_parameter_group_set_is_instanciable(parameter_group,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(instanciable_check_button)));

	parameter_load_selected();
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	/* frees */
	gtk_widget_destroy(dialog);
	g_free(ui);
}

/*
 * Section: Private
 */

static void
parameter_in_group_edit_setup_instances_edit_ui(struct ui_parameter_group_dialog * ui)
{
	GeoXmlSequence *	instance;
	gint			i, j;
	GString *		string;

	gtk_container_foreach(GTK_CONTAINER(ui->instances_edit_vbox), (GtkCallback)gtk_widget_destroy, NULL);
	
	string = g_string_new(NULL);
	geoxml_parameter_group_get_instance(ui->parameter_group, &instance, 0);
	for (i = 1; instance != NULL; i++, geoxml_sequence_next(&instance)) {
		GtkWidget *		frame;
		GtkWidget *		table;

		GeoXmlSequence *	parameter;
		GeoXmlSequence *	exclusive;

		table = gtk_table_new(geoxml_parameters_get_number(GEOXML_PARAMETERS(instance)), 2, FALSE);
		gtk_widget_show(table);
		gtk_table_set_row_spacings(GTK_TABLE(table), 6);
		gtk_table_set_col_spacings(GTK_TABLE(table), 6);

		exclusive = GEOXML_SEQUENCE(geoxml_parameters_get_exclusive(GEOXML_PARAMETERS(instance)));
		geoxml_parameters_get_parameter(GEOXML_PARAMETERS(instance), &parameter, 0);
		for (j = 0; parameter != NULL; ++j, geoxml_sequence_next(&parameter)) {
			struct parameter_widget *	widget;
			GtkWidget *			label_widget;

			if (exclusive == NULL)
				label_widget = gtk_label_new(geoxml_parameter_get_label(GEOXML_PARAMETER(parameter)));
			else {
				label_widget = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label_widget),
					geoxml_parameter_get_label(GEOXML_PARAMETER(parameter)));
				if (exclusive == parameter)
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label_widget), TRUE);
				g_signal_connect(label_widget, "toggled",
					(GCallback)on_parameter_group_exclusive_toggled, ui);
				g_object_set(label_widget, "user-data", parameter, NULL);
			}
			gtk_widget_show(label_widget);
			gtk_table_attach(GTK_TABLE(table), label_widget, 0, 1, j, j+1,
				(GtkAttachOptions)(GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			gtk_misc_set_alignment(GTK_MISC(label_widget), 0, 0.5);

			widget = parameter_widget_new(GEOXML_PARAMETER(parameter), FALSE, NULL);
			gtk_widget_show(widget->widget);
			gtk_table_attach(GTK_TABLE(table), widget->widget, 1, 2, j, j+1,
				(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
		}

		g_string_printf(string, _("Instance #%d"), i);

		frame = gtk_frame_new(string->str);
		gtk_widget_show(frame);
		gtk_container_add(GTK_CONTAINER(frame), table);
		gtk_box_pack_start(GTK_BOX(ui->instances_edit_vbox), frame, TRUE, TRUE, 5);
	}
	g_string_free(string, TRUE);
}

static gboolean
on_parameter_group_instances_changed(GtkSpinButton * spin_button, struct ui_parameter_group_dialog * ui)
{
	gint			i, instanciate;

	instanciate = gtk_spin_button_get_value(spin_button) -
		geoxml_parameter_group_get_instances_number(ui->parameter_group);
	if (instanciate == 0)
		return FALSE;

	if (instanciate > 0)
		for (i = 0; i < instanciate; ++i)
			geoxml_parameter_group_instanciate(ui->parameter_group);
	else
		for (i = instanciate; i < 0; ++i)
			geoxml_parameter_group_deinstanciate(ui->parameter_group);

	parameter_in_group_edit_setup_instances_edit_ui(ui);

	return FALSE;
}

static void
on_parameter_group_is_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog * ui)
{
	GeoXmlSequence *	instance;

	geoxml_parameter_group_get_instance(ui->parameter_group, &instance, 0);
	for (; instance != NULL; geoxml_sequence_next(&instance)) {
		if (gtk_toggle_button_get_active(toggle_button) == FALSE)
			geoxml_parameters_set_exclusive(GEOXML_PARAMETERS(instance), NULL);
		else {
			GeoXmlSequence *	first_parameter;

			geoxml_parameters_get_parameter(GEOXML_PARAMETERS(instance), &first_parameter, 0);
			geoxml_parameters_set_exclusive(GEOXML_PARAMETERS(instance), GEOXML_PARAMETER(first_parameter));
		}
	}

	parameter_in_group_edit_setup_instances_edit_ui(ui);
}

static void
on_parameter_group_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog * ui)
{
	if (gtk_toggle_button_get_active(toggle_button) == FALSE)
		return;

	GeoXmlParameter *	parameter;

	g_object_get(toggle_button, "user-data", &parameter, NULL);
	geoxml_parameters_set_exclusive(geoxml_parameter_get_parameters(parameter), parameter);
}
