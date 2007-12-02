/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://ui_parameters.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but ui_parameters.THOUT ANY ui_parameters.RRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_prop.c
 * Program's parameter window stuff
 */

#include <stdlib.h>
#include <string.h>

#include <geoxml.h>

#include "ui_parameters.h"
#include "menu.h"

#define GTK_RESPONSE_DEFAULT	GTK_RESPONSE_APPLY

/*
 * Prototypes
 */

static GtkWidget *
parameters_add_input_float    (GtkWidget    *widget,
			    gchar * label_str,
			    gboolean     required  );

static GtkWidget *
parameters_add_input_int      (GtkWidget    *widget,
			    gchar * label_str,
			    gboolean     required  );

static GtkWidget *
parameters_add_input_string   (GtkWidget    *widget,
			    gchar * label_str,
			    gboolean     required  );

static GtkWidget *
parameters_add_input_range    (GtkWidget    *widget,
			    gchar        *label_str,
			    gdouble       min,
			    gdouble       max,
			    gdouble       step,
			    gboolean      required );

static GtkWidget *
parameters_add_input_flag     (GtkWidget    *widget,
			    gchar        *label_str);

static GtkWidget *
parameters_add_input_file     (GtkWidget *		widget,
			    gchar *		label_str,
			    gboolean		is_directory,
			    gboolean		required,
			    const gchar *	path);

static void
parameters_response(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters);

static void
parameters_toogle_file_browse(GtkToggleButton * togglebutton);

/*
 * Function: parameters_configure_setup_ui
 * Assembly a dialog to configure the current selected program's parameters
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

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition.fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, _("No program selected"));
		return NULL;
	}

	/* get program index */
	path = gtk_tree_model_get_path (model, &iter);
	ui_parameters.program_index = (int) atoi(gtk_tree_path_to_string(path));
	gtk_tree_path_free(path);

	ui_parameters.dialog = gtk_dialog_new_with_buttons(_("Parameters"),
						GTK_ui_parameters.NDOui_parameters.ui_parameters.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_ui_parameters.TH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	gtk_dialog_add_button(GTK_DIALOG(ui_parameters.dialog), "Default", GTK_RESPONSE_DEFAULT);
	gtk_dialog_add_button(GTK_DIALOG(ui_parameters.dialog), "Help", GTK_RESPONSE_HELP);
	gtk_widget_set_size_request (ui_parameters.dialog, 490, 350);

	/* Take the apropriate action when a button is pressed */
	g_signal_connect(ui_parameters.dialog, "response",
			G_CALLBACK (parameters_action), ui_parameters);

	gtk_box_set_homogeneous( GTK_BOX(GTK_DIALOG (ui_parameters.dialog)->vbox), FALSE);

	/* Flow Title */
	label = gtk_label_new(NULL);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ui_parameters.dialog)->vbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment( GTK_MISC(label), 0.5, 0);

	/* Scrolled window for parameters */
	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	viewport = gtk_viewport_new(NULL, NULL);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ui_parameters.dialog)->vbox), scrolledwin, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (scrolledwin), viewport);
	gtk_container_add (GTK_CONTAINER (viewport), vbox);

	/* Starts reading program and its parameters
	 */
	geoxml_flow_get_program(ui_parameters.flow, &program, ui_parameters.program_index);

	/* Program title in bold */
	{
		char *markup;

		markup = g_markup_printf_escaped ("<big><b>%s</b></big>", geoxml_program_get_title(program));
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	ui_parameters.parwidgets_number = geoxml_program_get_parameters_number(program);
	ui_parameters.parwidgets = (GtkWidget**) malloc(sizeof(GtkWidget*) * ui_parameters.parwidgets_number);

	program_parameter = geoxml_program_get_first_parameter(program);
	for(i=0; i<ui_parameters.parwidgets_number; i++,  geoxml_program_parameter_next(&program_parameter)) {
		gchar * label = (gchar *)geoxml_program_parameter_get_label(program_parameter);

		switch (geoxml_program_parameter_get_type(program_parameter)) {
		case GEOXML_PARAMETERTYPE_FLOAT:
			ui_parameters.parwidgets[i] =
			parameters_add_input_float (vbox, label,
						geoxml_program_parameter_get_required (program_parameter));

			gtk_entry_set_text(GTK_ENTRY(ui_parameters.parwidgets[i]),
					geoxml_program_parameter_get_value(program_parameter));
		break;
		case GEOXML_PARAMETERTYPE_INT:
			ui_parameters.parwidgets[i] =
			parameters_add_input_int(vbox, label,
						geoxml_program_parameter_get_required (program_parameter));
			gtk_entry_set_text(GTK_ENTRY(ui_parameters.parwidgets[i]),
					geoxml_program_parameter_get_value(program_parameter));
		break;
		case GEOXML_PARAMETERTYPE_STRING:
			ui_parameters.parwidgets[i] =
			parameters_add_input_string(vbox, label,
						geoxml_program_parameter_get_required (program_parameter));

			gtk_entry_set_text(GTK_ENTRY(ui_parameters.parwidgets[i]),
					geoxml_program_parameter_get_value(program_parameter));
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			{
			gchar *min;
			gchar *max;
			gchar *step;

			geoxml_program_parameter_get_range_parameters(program_parameter,
									&min, &max, &step);

			ui_parameters.parwidgets[i] =
				parameters_add_input_range(vbox, label,
							atof(min), atof(max), atof(step),
							geoxml_program_parameter_get_required (program_parameter));

			gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_parameters.parwidgets[i]),
							atof(geoxml_program_parameter_get_value(program_parameter)));
			break;
			}
		case GEOXML_PARAMETERTYPE_FLAG:
			ui_parameters.parwidgets[i] = parameters_add_input_flag (vbox, label);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ui_parameters.parwidgets[i]),
							geoxml_program_parameter_get_flag_status(program_parameter));
		break;
		case GEOXML_PARAMETERTYPE_FILE: {
			ui_parameters.parwidgets[i] = parameters_add_input_file (vbox, label,
				geoxml_program_parameter_get_file_be_directory(program_parameter),
				geoxml_program_parameter_get_required(program_parameter),
				geoxml_program_parameter_get_value(program_parameter));
		} break;
		default:
		break;
		}
	}

	gtk_widget_show_all(ui_parameters.dialog);
	return ui_parameters;
}

/*
 * Function: parameters_add_input_float
 * Add an input entry to a float parameter
 */
static GtkWidget *
parameters_add_input_float    (GtkWidget    *widget,
			    gchar        *label_str,
			    gboolean      required)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	entry;

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX (widget), hbox, FALSE, TRUE, 0);

	label = gtk_label_new(label_str);
	gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

	if (required) {
		gchar *	markup;

		markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	entry = gtk_entry_new();
	gtk_widget_set_size_request (entry, 90, 30);
	gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

	return entry;
}

/*
 * Function: parameters_add_input_int
 * Add an input entry to an integer parameter
 */
static GtkWidget *
parameters_add_input_int      (GtkWidget    *widget,
			    gchar        *label_str,
			    gboolean      required)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	entry;

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX (widget), hbox, FALSE, TRUE, 0);

	label = gtk_label_new(label_str);
	gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

	if (required) {
		gchar *	markup;

		markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	entry = gtk_entry_new();
	gtk_widget_set_size_request (entry, 90, 30);
	gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

	return entry;
}

/*
 * Function: parameters_add_input_string
 * Add an input entry to a string parameter
 */
static GtkWidget *
parameters_add_input_string   (GtkWidget    *widget,
			    gchar        *label_str,
			    gboolean      required )
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	entry;

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX (widget), hbox, FALSE, TRUE, 0);

	label = gtk_label_new(label_str);
	gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

	if (required) {
		gchar *	markup;

		markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	entry = gtk_entry_new();
	gtk_widget_set_size_request (entry, 140, 30);
	gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

	return entry;
}

/*
 * Function: parameters_add_input_range
 * Add an input entry to a range parameter
 */
static GtkWidget *
parameters_add_input_range    (GtkWidget    *widget,
			    gchar        *label_str,
			    gdouble       min,
			    gdouble       max,
			    gdouble       step,
			    gboolean      required )
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	spin;

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX (widget), hbox, FALSE, TRUE, 0);

	label = gtk_label_new(label_str);
	gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

	if (required) {
		gchar *	markup;

		markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	spin = gtk_spin_button_new_with_range(min, max, step);
	gtk_widget_set_size_request (spin, 90, 30);
	gtk_box_pack_end(GTK_BOX (hbox), spin, FALSE, FALSE, 0);

	return spin;
}

/*
 * Function: parameters_add_input_flag
 * Add an input entry to a flag parameter
 */
static GtkWidget *
parameters_add_input_flag     (GtkWidget    *widget,
			    gchar        *label_str)
{
	GtkWidget *	hbox;
	GtkWidget *	checkbutton;

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX (widget), hbox, FALSE, TRUE, 0);

	checkbutton = gtk_check_button_new_with_label( label_str );
	gtk_box_pack_start(GTK_BOX (hbox), checkbutton, FALSE, TRUE, 0);

	return checkbutton;
}

/*
 * Function: parameters_add_input_file
 * Add an input entry and button to browse for a file or directory
 */
static GtkWidget *
parameters_add_input_file(GtkWidget * widget,
			gchar *			label_str,
			gboolean		is_directory,
			gboolean		required,
			const gchar *		path)
{
	GtkWidget *	hbox;
	GtkWidget *	label;

	/* hbox */
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX (widget), hbox, FALSE, TRUE, 0);

	/* label */
	label = gtk_label_new (label_str);
	gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);
	if (required) {
		char *markup;

		markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	/* file chooser */
	if (is_directory) {
		GtkWidget *file_chooser;

		if (!required) {
			GtkWidget *checkbutton;
			GtkWidget *hbox2;

			hbox2 = gtk_hbox_new(FALSE, 10);

			checkbutton = gtk_check_button_new();
			gtk_box_pack_start(GTK_BOX (hbox2), checkbutton, FALSE, TRUE, 0);
			file_chooser = gtk_file_chooser_button_new ("Browse", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
			gtk_box_pack_start(GTK_BOX (hbox2), file_chooser, TRUE, TRUE, 0);

 			g_signal_connect(checkbutton, "clicked",
					 G_CALLBACK (parameters_toogle_file_browse),
					 file_chooser );

			if (strlen(path)) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), TRUE);
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), path);
			}
			else{
			   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), FALSE);
			   g_object_set(file_chooser, "sensitive", FALSE, NULL);
			}

			gtk_widget_set_size_request (hbox2, 180, 30);
			gtk_box_pack_end(GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

			return hbox2;
		} else {
			file_chooser = gtk_file_chooser_button_new ("Browse", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

			if (!strlen(path))
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), getenv("HOME"));
			else
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), path);

			gtk_widget_set_size_request (file_chooser, 180, 30);
			gtk_box_pack_end(GTK_BOX (hbox), file_chooser, FALSE, FALSE, 0);

			return file_chooser;
		}
	} else {
		gebr_save_widget_t save_widget;

		save_widget = save_widget_create();
		gtk_entry_set_text (GTK_ENTRY (save_widget.entry), path);

		gtk_widget_set_size_request (save_widget.hbox, 180, 30);
		gtk_box_pack_end(GTK_BOX (hbox), save_widget.hbox, FALSE, FALSE, 0);

		return save_widget.entry;
	}
}

/*
 * Function: parameters_response
 * Take the appropriate action when the parameter dialog emmits
 * a response signal.
 */
static void
parameters_response(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		GeoXmlProgram          *	program;
		GeoXmlProgramParameter *	parameter;
		int				i;

		geoxml_flow_get_program(ui_parameters.flow, &program, ui_parameters.program_index);
		parameter = geoxml_program_get_first_parameter(program);

		/* Set program state to configured */
		geoxml_program_set_status(program, "configured");

		/* Update interface */
		{
			GtkTreeIter		iter;
			GtkTreeSelection *	selection;
			GtkTreeModel *		model;

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEui_parameters.ui_parameters.fseq_view));
			gtk_tree_selection_get_selected(selection, &model, &iter);

			gtk_list_store_set(ui_parameters.ui_flow_edition.fseq_store, &iter,
					FSEQ_STATUS_COLUMN, ui_parameters.pixmaps.configured_icon, -1);
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(ui_parameters.configured_menuitem), TRUE);
		}

		for (i = 0; i < ui_parameters.parwidgets_number; i++, geoxml_program_parameter_next(&parameter)) {
			switch (geoxml_program_parameter_get_type(parameter)) {
			case GEOXML_PARAMETERTYPE_FLOAT:
			case GEOXML_PARAMETERTYPE_INT:
			case GEOXML_PARAMETERTYPE_STRING:
				geoxml_program_parameter_set_value(parameter,
								gtk_entry_get_text(GTK_ENTRY(ui_parameters.parwidgets[i])));

				break;
			case GEOXML_PARAMETERTYPE_RANGE: {
				char valuestr[30];

				sprintf(valuestr, "%f", gtk_spin_button_get_value(GTK_SPIN_BUTTON(ui_parameters.parwidgets[i])));
				geoxml_program_parameter_set_value(parameter, valuestr);
				break;
			}
			case GEOXML_PARAMETERTYPE_FLAG:
				geoxml_program_parameter_set_flag_state(parameter,
									gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(ui_parameters.parwidgets[i])));
				break;
			case GEOXML_PARAMETERTYPE_FILE: {
				if (geoxml_program_parameter_get_file_be_directory(parameter)) {
					if (geoxml_program_parameter_get_required(parameter)) {
						gchar * path;

						path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (ui_parameters.parwidgets[i]));
						geoxml_program_parameter_set_value(parameter, path);

						g_free(path);
					} else {
						GList *		list;
						GtkWidget *	file_chooser;
						GtkWidget *	checkbox;

						list = gtk_container_get_children(GTK_CONTAINER(ui_parameters.parwidgets[i]));
						checkbox = (GtkWidget*)g_list_nth_data(list, 0);
						file_chooser = (GtkWidget*)g_list_nth_data(list, 1);

						if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox))) {
							gchar * path;

							path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (file_chooser));
							geoxml_program_parameter_set_value(parameter, path);

							g_free(path);
						} else
							geoxml_program_parameter_set_value(parameter, "");

						g_list_free(list);
					}
				} else
					geoxml_program_parameter_set_value(parameter,
						gtk_entry_get_text(GTK_ENTRY(ui_parameters.parwidgets[i])));
				break;
			}
			default:
				break;
			}
		}

		flow_save();
		parameters_parameters_set_to_flow();

		break;
	}
	case GTK_RESPONSE_CANCEL:
		break;
	case GTK_RESPONSE_DEFAULT: {
		GeoXmlProgram		*	program;
		GeoXmlProgramParameter	*	program_parameter;
		int			 	i;

		geoxml_flow_get_program(ui_parameters.flow, &program, ui_parameters.program_index);
		program_parameter = geoxml_program_get_first_parameter(program);
		for (i = 0; i < ui_parameters.parwidgets_number; i++, geoxml_program_parameter_next(&program_parameter)) {
			gchar *	default_str;

			default_str = (gchar*)geoxml_program_parameter_get_default(program_parameter);

			switch (geoxml_program_parameter_get_type(program_parameter)) {
			case GEOXML_PARAMETERTYPE_FLOAT:
			case GEOXML_PARAMETERTYPE_INT:
			case GEOXML_PARAMETERTYPE_STRING:
				gtk_entry_set_text(GTK_ENTRY(ui_parameters.parwidgets[i]), default_str);
				break;
			case GEOXML_PARAMETERTYPE_RANGE: {
				char *	endptr;
				double	value;

				value = strtod(default_str, &endptr);
				if (endptr != default_str)
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_parameters.parwidgets[i]), value);
				break;
			}
			case GEOXML_PARAMETERTYPE_FLAG:
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ui_parameters.parwidgets[i]),
					geoxml_program_parameter_get_flag_default(program_parameter));
				break;
			case GEOXML_PARAMETERTYPE_FILE:
				if (geoxml_program_parameter_get_file_be_directory(program_parameter)) {
					if (geoxml_program_parameter_get_required(program_parameter))
						gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (ui_parameters.parwidgets[i]), geoxml_program_parameter_get_value(program_parameter));
					else {
						GList *		list;
						GtkWidget *	file_chooser;
						GtkWidget *	checkbox;

						list = gtk_container_get_children(GTK_CONTAINER(ui_parameters.parwidgets[i]));
						checkbox = (GtkWidget*)g_list_nth_data(list, 0);
						file_chooser = (GtkWidget*)g_list_nth_data(list, 1);

						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), FALSE);
						/* TODO: deactivate file_chooser */

						g_list_free(list);
					}
				} else
					gtk_entry_set_text(GTK_ENTRY(ui_parameters.parwidgets[i]), default_str);
				break;
			default:
				break;
			}
		}
		return;
	}
	case GTK_RESPONSE_HELP: {
		GtkWidget *		dialog;
		GtkWidget *		scrolled_window;
		GtkWidget *		html;

		GtkTreeIter		iter;
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;
		GtkTreePath *		path;

		GeoXmlFlow *		menu;
		GeoXmlProgram *		program;
		gulong			index;
		gchar *			menu_filename;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEui_parameters.(ui_parameters.fseq_view));
		if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
			gebr_message(ERROR, TRUE, FALSE, _("No flow component selected"));
			return;
		}

		path = gtk_tree_model_get_path (model, &iter);
		index = (gulong)atoi(gtk_tree_path_to_string(path));
		gtk_tree_path_free(path);

		/* get the program and its path on menu */
		geoxml_flow_get_program(ui_parameters.flow, &program, index);
		geoxml_program_get_menu(program, &menu_filename, &index);

		menu = menu_load(menu_filename);
		if (menu == NULL)
			break;

		/* go to menu's program index specified in flow */
		geoxml_flow_get_program(menu, &program, index);
		show_help((gchar*)geoxml_program_get_help(program),"Program help","prog");

		geoxml_document_free(GEOXML_DOC(menu));
		return;
	}
	default:
		break;
	}

	/* gui free */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(ui_parameters->parwidgets);
	g_free(ui_parameters);
}

static void
parameters_toogle_file_browse(GtkToggleButton * togglebutton)
{
	gboolean	state;

	state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	g_object_set((GtkWidget *) user_data, "sensitive", state, NULL);
}
