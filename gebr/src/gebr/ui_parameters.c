/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_parameters.c
 * Program's parameter window stuff
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <geoxml.h>
#include <gui/utils.h>

#include "ui_parameters.h"
#include "gebr.h"
#include "support.h"
#include "menu.h"
#include "flow.h"
#include "ui_help.h"
#include "ui_flow.h"

#define GTK_RESPONSE_DEFAULT	GTK_RESPONSE_APPLY

/*
 * Prototypes
 */

static void
parameters_load_program(struct ui_parameters * ui_parameters);
static GtkWidget *
parameters_load(struct ui_parameters * ui_parameters, GeoXmlParameters * parameters);
static GtkWidget *
parameters_load_parameter(struct ui_parameters * ui_parameters, GeoXmlParameter * parameter,
	GSList ** radio_group);

static void
parameters_actions(GtkDialog * dialog, gint arg1, struct ui_parameters * ui_parameters);

static void
parameters_change_selected(GtkToggleButton * toggle_button, struct parameter_widget * widget);

static void
parameters_instanciate(GtkButton * button, struct ui_parameters * ui_parameters);

static void
parameters_deinstanciate(GtkButton * button, struct ui_parameters * ui_parameters);

static void
parameters_on_link_button_clicked(GtkButton * button, GeoXmlProgram * program);

static gboolean
parameters_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_parameters * ui_parameters);

/*
 * Section: Public
 * Public functions.
 */

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

	GtkWidget *			dialog;
	GtkWidget *			button;
	GtkWidget *			label;
	GtkWidget *			vbox;
	GtkWidget *			hbox;
	GtkWidget *			scrolledwin;
	GtkWidget *			viewport;

	GeoXmlSequence *		program;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("No program selected"));
		return NULL;
	}

	/* alloc struct */
	ui_parameters = g_malloc(sizeof(struct ui_parameters));
	gtk_tree_model_get(model, &iter, FSEQ_GEOXML_POINTER, &program, -1);

	dialog = gtk_dialog_new_with_buttons(_("Parameters"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Default"), GTK_RESPONSE_DEFAULT);
	g_object_set(G_OBJECT(button),
		"image", gtk_image_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_BUTTON), NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);

	gtk_widget_set_size_request(dialog, 630, 400);
	gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(dialog)->vbox), FALSE);
	/* take the apropriate action when a button is pressed */
	g_signal_connect(dialog, "response",
		G_CALLBACK(parameters_actions), ui_parameters);
	g_signal_connect(dialog, "delete-event",
		G_CALLBACK(parameters_on_delete_event), ui_parameters);

	/* program title in bold */
	label = gtk_label_new(NULL);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0);
	{
		gchar *	markup;

		markup = g_markup_printf_escaped("<big><b>%s</b></big>",
			geoxml_program_get_title(GEOXML_PROGRAM(program)));
		gtk_label_set_markup(GTK_LABEL (label), markup);
		g_free(markup);
	}

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 5);
	/* program description */
	label = gtk_label_new(geoxml_program_get_description(GEOXML_PROGRAM(program)));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	/* program URL */
	if (strlen(geoxml_program_get_url(GEOXML_PROGRAM(program)))) {
		GtkWidget *	alignment;
		GtkWidget *	button;

		alignment = gtk_alignment_new(1, 0, 0, 0);
		gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 5);
		button = gtk_button_new_with_label(_("Link"));
		gtk_container_add(GTK_CONTAINER(alignment), button);
		gtk_widget_show_all(alignment);
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

		g_signal_connect(button, "clicked",
			G_CALLBACK(parameters_on_link_button_clicked), program);
	}

	/* scrolled window for parameters */
	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwin);
	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolledwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrolledwin), viewport);
	gtk_container_add(GTK_CONTAINER(viewport), vbox);

	/* fill struct */
	*ui_parameters = (struct ui_parameters) {
		.dialog = dialog,
		.vbox = vbox,
		.program = GEOXML_PROGRAM(geoxml_sequence_append_clone(program)),
	};
	/* load programs parameters into UI */
	parameters_load_program(ui_parameters);

	gtk_widget_show(dialog);

	return ui_parameters;
}

/*
 * Function: parameters_reset_to_default
 * Change all parameters' values from _parameters_ to their default value
 *
 */
void
parameters_reset_to_default(GeoXmlParameters * parameters)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		if (geoxml_parameter_get_type(GEOXML_PARAMETER(parameter)) == GEOXML_PARAMETERTYPE_GROUP) {
			GeoXmlSequence *	instance;

			geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
			for (; instance != NULL; geoxml_sequence_next(&instance)) {
				parameters_reset_to_default(GEOXML_PARAMETERS(instance));
				geoxml_parameters_set_selected(GEOXML_PARAMETERS(instance),
					geoxml_parameters_get_exclusive(GEOXML_PARAMETERS(instance)));
			}

			continue;
		}

		GeoXmlSequence *	value;
		GeoXmlSequence *	default_value;

		geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter),
			FALSE, &value, 0);
		geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter),
			TRUE, &default_value, 0);
		for (; default_value != NULL; geoxml_sequence_next(&default_value), geoxml_sequence_next(&value)) {
			if (value == NULL)
				value = GEOXML_SEQUENCE(geoxml_program_parameter_append_value(
					GEOXML_PROGRAM_PARAMETER(parameter), FALSE));
			geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(value),
				geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(default_value)));
		}

		/* remove extras values */
		while (value != NULL) {
			GeoXmlSequence *	tmp;

			tmp = value;
			geoxml_sequence_next(&tmp);
			geoxml_sequence_remove(value);
			value = tmp;
		}
	}
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: parameters_load_program
 *
 */
static void
parameters_load_program(struct ui_parameters * ui_parameters)
{
	GtkWidget *	widget;

	gtk_container_foreach(GTK_CONTAINER(ui_parameters->vbox), (GtkCallback)gtk_widget_destroy, NULL);
	widget = parameters_load(ui_parameters, geoxml_program_get_parameters(ui_parameters->program));
	gtk_box_pack_start(GTK_BOX(ui_parameters->vbox), widget, TRUE, TRUE, 0);
}

/*
 * Function: parameters_load
 *
 */
static GtkWidget *
parameters_load(struct ui_parameters * ui_parameters, GeoXmlParameters * parameters)
{
	GtkWidget *		frame;
	GtkWidget *		vbox;
	GeoXmlSequence *	parameter;
	GSList *		radio_group;

	frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	if (geoxml_parameters_get_is_in_group(parameters))
		g_object_set(G_OBJECT(frame), "shadow-type", GTK_SHADOW_NONE, NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	radio_group = NULL;
	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; geoxml_sequence_next(&parameter))
		gtk_box_pack_start(GTK_BOX(vbox),
			parameters_load_parameter(ui_parameters, GEOXML_PARAMETER(parameter), &radio_group),
			FALSE, TRUE, 0);

	return frame;
}

/*
 * Function: parameters_load_parameter
 *
 */
static GtkWidget *
parameters_load_parameter(struct ui_parameters * ui_parameters, GeoXmlParameter * parameter,
	GSList ** radio_group)
{
	enum GEOXML_PARAMETERTYPE	type;

	type = geoxml_parameter_get_type(parameter);
	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		GtkWidget *		expander;
		GtkWidget *		label_widget;
		GtkWidget *		label;

		GtkWidget *		depth_hbox;
		GtkWidget *		group_vbox;
		GtkWidget *		instanciate_button;
		GtkWidget *		deinstanciate_button;

		GeoXmlParameterGroup *	parameter_group;
		GeoXmlSequence *	instance;

		parameter_group = GEOXML_PARAMETER_GROUP(parameter);

		expander = gtk_expander_new("");
		gtk_widget_show(expander);
		gtk_expander_set_expanded(GTK_EXPANDER(expander), 
			geoxml_parameter_group_get_expand(parameter_group));

		label_widget = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(label_widget);
		gtk_expander_set_label_widget(GTK_EXPANDER(expander), label_widget);
		gtk_expander_hacked_define(expander, label_widget);
		
		label = gtk_label_new(geoxml_parameter_get_label(parameter));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(label_widget), label, FALSE, TRUE, 0);

		if (geoxml_parameter_group_get_is_instanciable(GEOXML_PARAMETER_GROUP(parameter))) {
			instanciate_button = gtk_button_new();
			gtk_widget_show(instanciate_button);
			gtk_box_pack_start(GTK_BOX(label_widget), instanciate_button, FALSE, TRUE, 2);
			g_signal_connect(instanciate_button, "clicked",
				GTK_SIGNAL_FUNC(parameters_instanciate), ui_parameters);
			g_object_set(G_OBJECT(instanciate_button),
// 				"label", _("Instanciate"),
				"image", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR),
				"relief", GTK_RELIEF_NONE,
				"user-data", parameter_group,
				NULL);

			deinstanciate_button = gtk_button_new();
			gtk_widget_show(deinstanciate_button);
			gtk_box_pack_start(GTK_BOX(label_widget), deinstanciate_button, FALSE, TRUE, 2);
			g_signal_connect(deinstanciate_button, "clicked",
				GTK_SIGNAL_FUNC(parameters_deinstanciate), ui_parameters);
			g_object_set(G_OBJECT(deinstanciate_button),
// 				"label", _("Deinstanciate"),
				"image", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR),
				"relief", GTK_RELIEF_NONE,
				"user-data", parameter_group,
				NULL);

			gtk_widget_set_sensitive(deinstanciate_button,
				geoxml_parameter_group_get_instances_number(GEOXML_PARAMETER_GROUP(parameter)) > 1);
		} else
			deinstanciate_button = NULL;

		depth_hbox = gtk_container_add_depth_hbox(expander);
		gtk_widget_show(depth_hbox);
		group_vbox = gtk_vbox_new(FALSE, 3);
		gtk_widget_show(group_vbox);
		gtk_container_add(GTK_CONTAINER(depth_hbox), group_vbox);

		geoxml_object_set_user_data(GEOXML_OBJECT(parameter_group), group_vbox);
		g_object_set(G_OBJECT(group_vbox), "user-data", deinstanciate_button, NULL);

		geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
		for (; instance != NULL; geoxml_sequence_next(&instance)) {
			GtkWidget *	widget;

			widget = parameters_load(ui_parameters, GEOXML_PARAMETERS(instance));
			geoxml_object_set_user_data(GEOXML_OBJECT(instance), widget);
			gtk_box_pack_start(GTK_BOX(group_vbox), widget, FALSE, TRUE, 0);
		}

		return expander;
	} else {
		GtkWidget *			hbox;
		struct parameter_widget	*	parameter_widget;

		GeoXmlProgramParameter *	program_parameter;
		GeoXmlParameter *		selected;

		program_parameter = GEOXML_PROGRAM_PARAMETER(parameter);

		hbox = gtk_hbox_new(FALSE, 10);
		gtk_widget_show(hbox);

		/* input widget */
		if (type != GEOXML_PARAMETERTYPE_FILE)
			parameter_widget = parameter_widget_new(parameter, FALSE, NULL);
		else
			parameter_widget = parameter_widget_new(parameter, FALSE,
				flow_io_customized_paths_from_line);
		gtk_widget_show(parameter_widget->widget);

		/* exclusive? */
		selected = geoxml_parameters_get_selected(geoxml_parameter_get_parameters(parameter));
		if (selected != NULL) {
			GtkWidget *	radio_button;

			radio_button = gtk_radio_button_new(*radio_group);
			gtk_widget_show(radio_button);
			*radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), selected == parameter);
			g_signal_connect(radio_button, "toggled",
				(GCallback)parameters_change_selected, parameter_widget);

			gtk_box_pack_start(GTK_BOX(hbox), radio_button, FALSE, FALSE, 15);
		}

		/* label */
		if (type != GEOXML_PARAMETERTYPE_FLAG) {
			GtkWidget *	label;
			gchar *		label_str;
			GtkWidget *	align_vbox;

			label_str = (gchar*)geoxml_parameter_get_label(parameter);
			label = gtk_label_new("");
			gtk_widget_show(label);

			if (geoxml_program_parameter_get_required(GEOXML_PROGRAM_PARAMETER(parameter)) == TRUE) {
				gchar *	markup;

				markup = g_markup_printf_escaped("<b>%s</b><sup>*</sup>", label_str);
				gtk_label_set_markup(GTK_LABEL(label), markup);
				g_free(markup);
			} else
				gtk_label_set_text(GTK_LABEL(label), label_str);

			align_vbox = gtk_vbox_new(FALSE, 0);
			gtk_widget_show(align_vbox);
			gtk_box_pack_start(GTK_BOX(align_vbox), label, FALSE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), align_vbox, FALSE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(hbox), parameter_widget->widget, FALSE, TRUE, 0);
		} else {
			g_object_set(G_OBJECT(parameter_widget->value_widget),
				"label", geoxml_parameter_get_label(parameter), NULL);

			gtk_box_pack_start(GTK_BOX(hbox), parameter_widget->widget, FALSE, FALSE, 0);
		}

		return hbox;
	}
}

/*
 * Function: parameters_change_selected
 *
 */
static void
parameters_change_selected(GtkToggleButton * toggle_button, struct parameter_widget * widget)
{
	gtk_widget_set_sensitive(widget->widget, gtk_toggle_button_get_active(toggle_button));
}

/*
 * Function: parameters_instanciate
 *
 */
static void
parameters_instanciate(GtkButton * button, struct ui_parameters * ui_parameters)
{
	GeoXmlParameterGroup *	parameter_group;
	GeoXmlParameters *	instance;
	GtkWidget *		group_vbox;
	GtkWidget *		deinstanciate_button;
	GtkWidget *		widget;

	g_object_get(button, "user-data", &parameter_group, NULL);
	group_vbox = geoxml_object_get_user_data(GEOXML_OBJECT(parameter_group));
	g_object_get(group_vbox, "user-data", &deinstanciate_button, NULL);

	instance = geoxml_parameter_group_instanciate(parameter_group);
	widget = parameters_load(ui_parameters, GEOXML_PARAMETERS(instance));
	geoxml_object_set_user_data(GEOXML_OBJECT(instance), widget);
	gtk_box_pack_start(GTK_BOX(group_vbox), widget, FALSE, TRUE, 0);

	gtk_widget_set_sensitive(deinstanciate_button, TRUE);
}

/*
 * Function: parameters_deinstanciate
 *
 */
static void
parameters_deinstanciate(GtkButton * button, struct ui_parameters * ui_parameters)
{
	GeoXmlParameterGroup *	parameter_group;
	GeoXmlSequence *	last_instance;
	GtkWidget *		widget;

	g_object_get(button, "user-data", &parameter_group, NULL);
	geoxml_parameter_group_get_instance(parameter_group, &last_instance,
		geoxml_parameter_group_get_instances_number(parameter_group)-1);

	widget = geoxml_object_get_user_data(GEOXML_OBJECT(last_instance));
	gtk_widget_destroy(widget);
	geoxml_parameter_group_deinstanciate(parameter_group);

	gtk_widget_set_sensitive(GTK_WIDGET(button),
		geoxml_parameter_group_get_instances_number(parameter_group) > 1);
}

/*
 * Function: parameters_actions
 * Take the appropriate action when the parameter dialog emmits
 * a response signal.
 */
static void
parameters_actions(GtkDialog * dialog, gint arg1, struct ui_parameters * ui_parameters)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;
		GtkTreeIter		iter;

		GeoXmlSequence *	program;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
		gtk_tree_selection_get_selected(selection, &model, &iter);
		gtk_tree_model_get(model, &iter, FSEQ_GEOXML_POINTER, &program, -1);

		geoxml_program_set_status(GEOXML_PROGRAM(ui_parameters->program), "configured");
		geoxml_sequence_move_before(GEOXML_SEQUENCE(ui_parameters->program), program);
		geoxml_sequence_remove(program);

		/* Update interface */
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
			FSEQ_GEOXML_POINTER, ui_parameters->program,
			FSEQ_STATUS_COLUMN, gebr.pixmaps.stock_apply,
			-1);
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(gebr.action_group, "flow_edition_configured")), TRUE);

		flow_save();
		break;
	} case GTK_RESPONSE_DEFAULT: {
		parameters_reset_to_default(geoxml_program_get_parameters(ui_parameters->program));
		parameters_load_program(ui_parameters);
		return;
	} case GTK_RESPONSE_HELP: {
		program_help_show();
		return;
	} case GTK_RESPONSE_CANCEL:
	default:
		geoxml_sequence_remove(GEOXML_SEQUENCE(ui_parameters->program));
		break;
	}

	/* gui free */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(ui_parameters);
}

static void
parameters_on_link_button_clicked(GtkButton * button, GeoXmlProgram * program)
{
	GString * cmd_line;

	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s %s &", gebr.config.browser->str, geoxml_program_get_url(program));
	system(cmd_line->str);

	g_string_free(cmd_line, TRUE);
}

static gboolean
parameters_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_parameters * ui_parameters)
{
	parameters_actions(dialog, GTK_RESPONSE_CANCEL, ui_parameters);
	return FALSE;
}
