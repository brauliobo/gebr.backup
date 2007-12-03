/*   GêBR - An environment for seismic processing.
 *   Copyright (C) 2007 GêBR core team (http://ui_parameters->sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but ui_parameters->THOUT ANY ui_parameters->RRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_parameters.c
 * Program's parameter window stuff
 */

#include <stdlib.h>
#include <string.h>

#include <geoxml.h>
#include <gui/gtkfileentry.h>

#include "ui_parameters.h"
#include "gebr.h"
#include "support.h"
#include "menu.h"
#include "flow.h"
#include "ui_help.h"

#define GTK_RESPONSE_DEFAULT	GTK_RESPONSE_APPLY

/*
 * Prototypes
 */

static void
parameters_actions(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters);

static GtkWidget *
parameters_create_label(GeoXmlProgramParameter * parameter);

static GtkWidget *
parameters_add_input_float(GeoXmlProgramParameter * parameter);

static GtkWidget *
parameters_add_input_int(GeoXmlProgramParameter * parameter);

static GtkWidget *
parameters_add_input_string(GeoXmlProgramParameter * parameter);

static GtkWidget *
parameters_add_input_range(GeoXmlProgramParameter * parameter);

static GtkWidget *
parameters_add_input_flag(GeoXmlProgramParameter * parameter);

static GtkWidget *
parameters_add_input_file(GeoXmlProgramParameter * parameter);

/*
 * Function: parameters_configure_setup_ui
 * Assembly a dialog to configure the current selected program's parameters
 *
 * Return:
 * The structure containing relevant data. It will be automatically freed when the
 * dialog closes.
 */
struct ui_parameters *
parameters_configure_setup_ui(void)
{
	struct ui_parameters *		ui_parameters;
	GtkTreeSelection *		selection;
	GtkTreeModel *			model;
	GtkTreeIter			iter;
	GtkTreePath *			path;

	GtkWidget *			label;
	GtkWidget *			vbox;
	GtkWidget *			scrolledwin;
	GtkWidget *			viewport;

	GeoXmlProgram *			program;
	GeoXmlProgramParameter *	program_parameter;
	gulong				i;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, _("No program selected"));
		return NULL;
	}

	/* alloc struct */
	ui_parameters = g_malloc(sizeof(struct ui_parameters));

	/* get program index */
	path = gtk_tree_model_get_path (model, &iter);
	ui_parameters->program_index = (int)atoi(gtk_tree_path_to_string(path));
	gtk_tree_path_free(path);

	ui_parameters->dialog = gtk_dialog_new_with_buttons(_("Parameters"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	gtk_dialog_add_button(GTK_DIALOG(ui_parameters->dialog), "Default", GTK_RESPONSE_DEFAULT);
	gtk_dialog_add_button(GTK_DIALOG(ui_parameters->dialog), "Help", GTK_RESPONSE_HELP);
	gtk_widget_set_size_request(ui_parameters->dialog, 600, 350);
	gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(ui_parameters->dialog)->vbox), FALSE);

	/* take the apropriate action when a button is pressed */
	g_signal_connect(ui_parameters->dialog, "response",
			G_CALLBACK (parameters_actions), ui_parameters);

	/* flow title */
	label = gtk_label_new(NULL);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ui_parameters->dialog)->vbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment( GTK_MISC(label), 0.5, 0);

	/* scrolled window for parameters */
	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	viewport = gtk_viewport_new(NULL, NULL);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ui_parameters->dialog)->vbox), scrolledwin, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (scrolledwin), viewport);
	gtk_container_add (GTK_CONTAINER (viewport), vbox);

	/* starts reading program and its parameters */
	geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);

	/* program title in bold */
	{
		gchar *	markup;

		markup = g_markup_printf_escaped("<big><b>%s</b></big>", geoxml_program_get_title(program));
		gtk_label_set_markup(GTK_LABEL (label), markup);
		g_free(markup);
	}

	ui_parameters->widgets_number = geoxml_program_get_parameters_number(program);
	ui_parameters->widgets = g_malloc(sizeof(GtkWidget*)*ui_parameters->widgets_number);

	program_parameter = geoxml_program_get_first_parameter(program);
	for (i = 0; i < ui_parameters->widgets_number; ++i) {
		GtkWidget *	widget;

		switch (geoxml_program_parameter_get_type(program_parameter)) {
		case GEOXML_PARAMETERTYPE_FLOAT:
			widget = parameters_add_input_float(program_parameter);
			break;
		case GEOXML_PARAMETERTYPE_INT:
			widget = parameters_add_input_int(program_parameter);
			break;
		case GEOXML_PARAMETERTYPE_STRING:
			widget = parameters_add_input_string(program_parameter);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			widget = parameters_add_input_range(program_parameter);
			break;
		case GEOXML_PARAMETERTYPE_FLAG:
			widget = parameters_add_input_flag(program_parameter);
			break;
		case GEOXML_PARAMETERTYPE_FILE:
			widget = parameters_add_input_file(program_parameter);
			break;
		default:
			widget = NULL;
			break;
		}

		/* set and add */
		ui_parameters->widgets[i] = widget;
		gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);

		geoxml_program_parameter_next(&program_parameter);
	}

	gtk_widget_show_all(ui_parameters->dialog);

	return ui_parameters;
}

/*
 * Function: parameters_actions
 * Take the appropriate action when the parameter dialog emmits
 * a response signal.
 */
static void
parameters_actions(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		GeoXmlProgram          *	program;
		GeoXmlProgramParameter *	parameter;
		int				i;

		geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);
		parameter = geoxml_program_get_first_parameter(program);

		/* Set program state to configured */
		geoxml_program_set_status(program, "configured");

		/* Update interface */
		{
			GtkTreeSelection *	selection;
			GtkTreeModel *		model;
			GtkTreeIter		iter;

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
			gtk_tree_selection_get_selected(selection, &model, &iter);
			gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
					FSEQ_STATUS_COLUMN, gebr.pixmaps.configured_icon,
					-1);

			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gebr.configured_menuitem), TRUE);
		}

		for (i = 0; i < ui_parameters->widgets_number; ++i) {
			GtkWidget *	widget;

			widget = ui_parameters->widgets[i];

			switch (geoxml_program_parameter_get_type(parameter)) {
			case GEOXML_PARAMETERTYPE_FLOAT:
			case GEOXML_PARAMETERTYPE_INT:
			case GEOXML_PARAMETERTYPE_STRING:
				geoxml_program_parameter_set_value(parameter,
					gtk_entry_get_text(GTK_ENTRY(widget)));
				break;
			case GEOXML_PARAMETERTYPE_RANGE: {
				GString *	value;

				value = g_string_new(NULL);

				g_string_printf(value, "%f", gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
				geoxml_program_parameter_set_value(parameter, value->str);

				g_string_free(value, TRUE);
				break;
			} case GEOXML_PARAMETERTYPE_FLAG:
				geoxml_program_parameter_set_flag_state(parameter,
					gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
				break;
			case GEOXML_PARAMETERTYPE_FILE:
				geoxml_program_parameter_set_value(parameter,
					gtk_file_entry_get_path(GTK_FILE_ENTRY(widget)));
				break;
			default:
				break;
			}

			geoxml_program_parameter_next(&parameter);
		}

		flow_save();
		break;
	} case GTK_RESPONSE_CANCEL:
		break;
	case GTK_RESPONSE_DEFAULT: {
		GeoXmlProgram		*	program;
		GeoXmlProgramParameter	*	program_parameter;
		int			 	i;

		geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);
		program_parameter = geoxml_program_get_first_parameter(program);
		for (i = 0; i < ui_parameters->widgets_number; ++i) {
			GtkWidget *	widget;
			gchar *		default_value;

			widget = ui_parameters->widgets[i];
			default_value = (gchar*)geoxml_program_parameter_get_default(program_parameter);

			switch (geoxml_program_parameter_get_type(program_parameter)) {
			case GEOXML_PARAMETERTYPE_FLOAT:
			case GEOXML_PARAMETERTYPE_INT:
			case GEOXML_PARAMETERTYPE_STRING:
				gtk_entry_set_text(GTK_ENTRY(widget), default_value);
				break;
			case GEOXML_PARAMETERTYPE_RANGE: {
				gchar *	endptr;
				double	value;

				value = strtod(default_value, &endptr);
				if (endptr != default_value)
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
				break;
			} case GEOXML_PARAMETERTYPE_FLAG:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
					geoxml_program_parameter_get_flag_default(program_parameter));
				break;
			case GEOXML_PARAMETERTYPE_FILE:
				gtk_file_entry_set_path(GTK_FILE_ENTRY(widget), default_value);
				break;
			default:
				break;
			}

			geoxml_program_parameter_next(&program_parameter);
		}
		return;
	} case GTK_RESPONSE_HELP: {
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;
		GtkTreeIter		iter;
		GtkTreePath *		path;

		GeoXmlFlow *		menu;
		GeoXmlProgram *		program;

		gulong			index;
		gchar *			menu_filename;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
		if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
			gebr_message(ERROR, TRUE, FALSE, _("No flow component selected"));
			return;
		}

		path = gtk_tree_model_get_path (model, &iter);
		index = (gulong)atoi(gtk_tree_path_to_string(path));
		gtk_tree_path_free(path);

		/* get the program and its path on menu */
		geoxml_flow_get_program(gebr.flow, &program, index);
		geoxml_program_get_menu(program, &menu_filename, &index);

		menu = menu_load(menu_filename);
		if (menu == NULL)
			break;

		/* go to menu's program index specified in flow */
		geoxml_flow_get_program(menu, &program, index);
		help_show((gchar*)geoxml_program_get_help(program), _("Program help"), "prog");

		geoxml_document_free(GEOXML_DOC(menu));
		return;
	} default:
		break;
	}

	/* gui free */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(ui_parameters->widgets);
	g_free(ui_parameters);
}

static GtkWidget *
parameters_create_label(GeoXmlProgramParameter * parameter)
{
	GtkWidget *	label;
	gchar *		label_str;

	label_str = (gchar*)geoxml_program_parameter_get_label(parameter);
	label = gtk_label_new("");

	if (geoxml_program_parameter_get_required(parameter) == TRUE) {
		gchar *	markup;

		markup = g_markup_printf_escaped("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free(markup);
	} else
		gtk_label_set_text(GTK_LABEL(label), label_str);

	return label;
}

/*
 * Function: parameters_add_input_float
 * Add an input entry to a float parameter
 */
static GtkWidget *
parameters_add_input_float(GeoXmlProgramParameter * parameter)
{
	return parameters_add_input_string(parameter);
}

/*
 * Function: parameters_add_input_int
 * Add an input entry to an integer parameter
 */
static GtkWidget *
parameters_add_input_int(GeoXmlProgramParameter * parameter)
{
	return parameters_add_input_string(parameter);
}

/*
 * Function: parameters_add_input_string
 * Add an input entry to a string parameter
 */
static GtkWidget *
parameters_add_input_string(GeoXmlProgramParameter * parameter)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	entry;

	hbox = gtk_hbox_new(FALSE, 10);
	label = parameters_create_label(parameter);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	entry = gtk_entry_new();
	gtk_widget_set_size_request (entry, 140, 30);
	gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

	gtk_entry_set_text(GTK_ENTRY(entry),
		geoxml_program_parameter_get_value(parameter));

	return entry;
}

/*
 * Function: parameters_add_input_range
 * Add an input entry to a range parameter
 */
static GtkWidget *
parameters_add_input_range(GeoXmlProgramParameter * parameter)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	spin;

	gchar *		min;
	gchar *		max;
	gchar *		step;

	hbox = gtk_hbox_new(FALSE, 10);
	label = parameters_create_label(parameter);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	geoxml_program_parameter_get_range_properties(
		parameter, &min, &max, &step);

	spin = gtk_spin_button_new_with_range(atof(min), atof(max), atof(step));
	gtk_widget_set_size_request(spin, 90, 30);
	gtk_box_pack_end(GTK_BOX(hbox), spin, FALSE, FALSE, 0);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin),
		atof(geoxml_program_parameter_get_value(parameter)));

	return spin;
}

/*
 * Function: parameters_add_input_flag
 * Add an input entry to a flag parameter
 */
static GtkWidget *
parameters_add_input_flag(GeoXmlProgramParameter * parameter)
{
	GtkWidget *	hbox;
	GtkWidget *	check_button;

	hbox = gtk_hbox_new(FALSE, 10);
	check_button = gtk_check_button_new_with_label(geoxml_program_parameter_get_label(parameter));
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		geoxml_program_parameter_get_flag_status(parameter));

	return check_button;
}

/*
 * Function: parameters_add_input_file
 * Add an input entry and button to browse for a file or directory
 */
static GtkWidget *
parameters_add_input_file(GeoXmlProgramParameter * parameter)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	file_entry;

	/* hbox */
	hbox = gtk_hbox_new(FALSE, 10);
	label = parameters_create_label(parameter);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	/* file entry */
	file_entry = gtk_file_entry_new();
	gtk_widget_set_size_request(file_entry, 220, 30);
	gtk_box_pack_end(GTK_BOX(hbox), file_entry, FALSE, FALSE, 0);

	gtk_file_entry_set_path(GTK_FILE_ENTRY(file_entry),
		geoxml_program_parameter_get_value(parameter));
	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry),
		geoxml_program_parameter_get_file_be_directory(parameter));

	return file_entry;
}
