/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-tar.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/geoxml/flow.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <gebr-validator.h>

#include "flow.h"
#include "line.h"
#include "gebr.h"
#include "menu.h"
#include "document.h"
#include "callbacks.h"
#include "ui_flow.h"
#include "ui_document.h"
#include "ui_flow_browse.h"
#include "gebr-flow-edition.h"
#include "ui_project_line.h"

static void on_properties_response(gboolean accept)
{
	if (!accept)
		flow_delete(FALSE);
}

void flow_new (void)
{
	GtkTreeIter iter;

	const gchar *line_title;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlLineFlow *line_flow;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	line_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line));

	/* Create a new flow */
	flow = GEBR_GEOXML_FLOW(document_new(GEBR_GEOXML_DOCUMENT_TYPE_FLOW));
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(flow), _("New Flow"));
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(flow), gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(gebr.line)));
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(flow), gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.line)));

	gebr_geoxml_document_set_parent_id(GEBR_GEOXML_DOC(flow), "");

	line_flow = gebr_geoxml_line_append_flow(gebr.line, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(flow)));
	iter = line_append_flow_iter(flow, line_flow);

	document_save (GEBR_GEOXML_DOC (gebr.line), TRUE, FALSE);
	document_save (GEBR_GEOXML_DOC (flow), TRUE, TRUE);

	flow_browse_select_iter(&iter);

	flow_edition_set_io();

	flow_edition_set_run_widgets_sensitiveness(gebr.ui_flow_edition, TRUE, FALSE);

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("New Flow added to Line '%s'."), line_title);
	document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.flow), on_properties_response, TRUE);
	project_line_info_update();
}

void flow_free(void)
{
	gebr.flow = NULL;

	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store);
	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_view_set_model(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), NULL);
	while (valid) {
		GebrGeoXmlProgram *program;
		gtk_tree_model_get(model, &iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		gebr_geoxml_object_unref(program);
		valid = gtk_list_store_remove(gebr.ui_flow_edition->fseq_store, &iter);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), model);

	flow_browse_info_update();
}

void flow_delete(gboolean confirm)
{
	gpointer document;
	GtkTreeIter iter;
	gboolean valid = FALSE;

	gchar *title;
	gchar *filename;
	gboolean there_is_snapshot = FALSE;

	GebrGeoXmlSequence *line_flow;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(rows->next == NULL && current_page == NOTEBOOK_PAGE_FLOW_BROWSE) {
		GebrGeoXmlDocument *flow;
		gchar *flow_id;

		gtk_tree_model_get_iter(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, rows->data);
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_FILENAME, &flow_id,
		                   FB_XMLPOINTER, &flow,
		                   -1);

		if (gebr_geoxml_flow_get_revisions_number(GEBR_GEOXML_FLOW(flow)) > 0) {
			there_is_snapshot = there_is_snapshot || TRUE;
			if (g_list_find_custom(gebr.ui_flow_browse->select_flows, flow_id, (GCompareFunc)g_strcmp0)) {
				gchar *str = g_strdup_printf("delete\b%s\n",flow_id);
				GString *action = g_string_new(str);

				if (gebr_comm_process_write_stdin_string(gebr.ui_flow_browse->graph_process, action) == 0)
					g_debug("Fail to delete snapshots!");

				g_free(str);
				g_string_free(action, TRUE);
				g_free(flow_id);
				g_list_free(rows);
				return;
			}
		}
		g_free(flow_id);
	}
	g_list_free(rows);

	
	gchar *deletion_msg;
	if (there_is_snapshot)
		deletion_msg = g_strdup(_("Are you sure you want to delete the selected Flow(s) and\n"
				 "the associated snapshots?"));
	else
		deletion_msg = g_strdup(_("Are you sure you want to delete the selected Flow(s)?"));

	if (confirm && gebr_gui_confirm_action_dialog(_("Delete Flow(s)?"),
						      deletion_msg) == FALSE) {
		g_free(deletion_msg);
		return;
	}
	g_free(deletion_msg);

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
				   FB_TITLE, &title,
				   FB_FILENAME, &filename,
				   FB_XMLPOINTER, &document,
				   -1);

		/* Some feedback */
		if (confirm) {
			gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Deleting Flow '%s'."), title);
			gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting  Flow '%s' from Line '%s'."),
				     title, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line)));
		}

		/* Seek and destroy */
		gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
		for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
			if (g_strcmp0(filename, gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow))) == 0) {
				gebr_geoxml_sequence_remove(line_flow);
				document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
				break;
			}
		}

		/* Free and delete flow from the disk */
		gebr_remove_help_edit_window(document);
		valid = gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter);
		flow_free();

		document_delete(filename);

		g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");

		g_free(title);
		g_free(filename);
	}
	if (valid)
		flow_browse_select_iter(&iter);

	if (gebr_geoxml_line_get_flows_number(gebr.line) == 0)
		flow_edition_set_run_widgets_sensitiveness(gebr.ui_flow_edition, FALSE, FALSE);

	flow_browse_info_update();

	project_line_info_update();
}

static gboolean flow_import_single (const gchar *path)
{
	gchar *new_title;
	const gchar *title;
	GtkTreeIter iter;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlLineFlow *line_flow;

	if (document_load_path (&flow, path))
		return FALSE;

	title = gebr_geoxml_document_get_title (flow);

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Flow '%s' imported into Line '%s' from the file '%s'."),
		     title, gebr_geoxml_document_get_title (GEBR_GEOXML_DOC (gebr.line)), path);

	document_import (flow, FALSE);
	line_flow = gebr_geoxml_line_append_flow (gebr.line, gebr_geoxml_document_get_filename (flow));
	document_save(GEBR_GEOXML_DOC(gebr.line), FALSE, FALSE);
	iter = line_append_flow_iter(GEBR_GEOXML_FLOW (flow), line_flow);
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
	gebr_geoxml_flow_revalidate(GEBR_GEOXML_FLOW(flow), gebr.validator);
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

	new_title = g_strdup_printf (_("%s (Imported)"), title);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter, FB_TITLE, new_title, -1);
	gebr_geoxml_document_set_title(flow, new_title);
	document_save(flow, FALSE, FALSE);
	g_free(new_title);

	gebr_flow_set_toolbar_sensitive();
	flow_edition_set_run_widgets_sensitiveness(gebr.ui_flow_edition, TRUE, FALSE);

	return TRUE;
}

void flow_import(void)
{
	gchar *path;
	GtkWidget *chooser_dialog;
	GtkFileFilter *file_filter;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose a Flow to import"),
						     GTK_WINDOW(gebr.window),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_OPEN, GTK_RESPONSE_YES,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser_dialog), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow file types (*.flw, *.flwz, *.flwx)"));
	gtk_file_filter_add_pattern(file_filter, "*.flw");
	gtk_file_filter_add_pattern(file_filter, "*.flwz");
	gtk_file_filter_add_pattern(file_filter, "*.flwx");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), g_get_home_dir());

	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	if (g_str_has_suffix(path, ".flwz") || g_str_has_suffix(path, ".flwx")) {
		GebrTar *tar;
		tar = gebr_tar_new_from_file (path);
		if (!gebr_tar_extract (tar))
			gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
				      _("Could not import Flow from the file %s"), path);
		else
			gebr_tar_foreach (tar, (GebrTarFunc) flow_import_single, NULL);
		gebr_tar_free (tar);
	} else if (!flow_import_single (path))
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not import Flow from the file %s"), path);

	g_free(path);
out:
	gtk_widget_destroy(chooser_dialog);
}

void flow_export(void)
{
	GList *rows;
	GString *tempdir;
	GString *title;
	GebrGeoXmlDocument *flow;
	GebrTar *tar;
	GtkFileFilter *file_filter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkWidget *chooser_dialog;
	gboolean have_flow = FALSE;
	gboolean error = FALSE;
	gchar *flow_filename;
	gchar *tmp;
	gint len;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_flow_browse->view));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	len = g_list_length (rows);

	if (!rows) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Flow selected"));
		return;
	}

	title = g_string_new(NULL);

	if (len > 1)
		g_string_printf (title, "Save selected Flows as...");
	else {
		GtkTreeIter iter;
		GtkTreePath *path = rows->data;

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, FB_FILENAME, &flow_filename, -1);

		if (document_load (&flow, flow_filename, FALSE))
			goto out;

		g_string_printf (title, _("Save '%s' as..."),
				 gebr_geoxml_document_get_title (flow));

		document_free (flow);
	}

	GtkWidget *box;
	box = gtk_vbox_new(FALSE, 5);

	chooser_dialog = gebr_gui_save_dialog_new(title->str, GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), ".flwx");

	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow file types (*.flwz, *.flwx)"));
	gtk_file_filter_add_pattern(file_filter, "*.flwz");
	gtk_file_filter_add_pattern(file_filter, "*.flwx");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser_dialog), box);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), g_get_home_dir());
	gtk_widget_show_all(box);

	tmp = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!tmp)
		goto out;

	tempdir = gebr_temp_directory_create ();
	tar = gebr_tar_create (tmp);

	for (GList *i = rows; i; i = i->next) {
		gchar *filepath;
		GtkTreeIter iter;
		GtkTreePath *path = i->data;
		GebrGeoXmlDocument *doc;

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter,
				    FB_FILENAME, &flow_filename,
				    FB_XMLPOINTER, &doc,
				    -1);

		flow = gebr_geoxml_document_clone (doc);
		if (!flow) {
			gebr_message (GEBR_LOG_ERROR, FALSE, TRUE, _("Could not clone Flow %s"), flow_filename);
			error = TRUE;
			continue;
		}

		gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
		flow_set_paths_to_relative(GEBR_GEOXML_FLOW(flow), gebr.line, paths, TRUE);
		gebr_pairstrfreev(paths);

		filepath = g_build_path ("/", tempdir->str, flow_filename, NULL);

		if (document_save_at (flow, filepath, FALSE, FALSE, FALSE)) {
			gebr_tar_append (tar, flow_filename);
			have_flow = TRUE;
		} else {
			error = TRUE;
		}

		document_free (flow);
		g_free (filepath);
	}

	if (have_flow && gebr_tar_compact (tar, tempdir->str)) {
		gchar *msg;
		if (error)
			msg = _("Exported selected Flow(s) into %s with error. See log file for more information.");
		else
			msg = _("Exported selected Flow(s) into %s.");
		gebr_message (GEBR_LOG_INFO, TRUE, TRUE, msg, tmp);
	} else {
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not export Flow(s). See log file for more information."));
	}

	gebr_temp_directory_destroy (tempdir);

	g_free(tmp);
	gebr_tar_free (tar);

out:
	if (!len > 1)
		g_free(flow_filename);
	g_string_free(title, TRUE);
}

static void flow_copy_from_dicts_parse_parameters(GebrGeoXmlParameters * parameters);

/** 
 * \internal
 * Parse a parameter.
 */
static void flow_copy_from_dicts_parse_parameter(GebrGeoXmlParameter * parameter)
{
	GebrGeoXmlDocument *documents[] = {
		GEBR_GEOXML_DOCUMENT(gebr.project), GEBR_GEOXML_DOCUMENT(gebr.line),
		GEBR_GEOXML_DOCUMENT(gebr.flow), NULL
	};

	if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GebrGeoXmlSequence *instance;

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
			flow_copy_from_dicts_parse_parameters(GEBR_GEOXML_PARAMETERS(instance));

		return;
	}

	for (int i = 0; documents[i] != NULL; i++) {
		GebrGeoXmlProgramParameter *dict_parameter;

		dict_parameter =
			gebr_geoxml_program_parameter_find_dict_parameter(GEBR_GEOXML_PROGRAM_PARAMETER
									  (parameter), documents[i]);
		if (dict_parameter != NULL)
			gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER
								      (parameter), FALSE,
								      gebr_geoxml_program_parameter_get_first_value
								      (dict_parameter, FALSE));
	}
	gebr_geoxml_program_parameter_set_value_from_dict(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
							  NULL);
}

/** 
 * \internal
 * Parse a set of parameters.
 */
static void flow_copy_from_dicts_parse_parameters(GebrGeoXmlParameters * parameters)
{
	GebrGeoXmlSequence *parameter;

	parameter = GEBR_GEOXML_SEQUENCE(gebr_geoxml_parameters_get_first_parameter(parameters));
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter))
		flow_copy_from_dicts_parse_parameter(GEBR_GEOXML_PARAMETER(parameter));
}

void flow_copy_from_dicts(GebrGeoXmlFlow * flow)
{
	GebrGeoXmlSequence *program;

	gebr_geoxml_flow_get_program(flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		flow_copy_from_dicts_parse_parameters(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));
}

void
gebr_flow_modify_paths(GebrGeoXmlFlow *flow,
		       GebrFlowModifyPathsFunc func,
		       gboolean set_programs_unconfigured,
		       gpointer data)
{
	GString *path = g_string_new(NULL);

	gboolean flow_paths_foreach_parameter(GebrGeoXmlParameter * parameter)
	{
		gboolean cleaned = FALSE;
		if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE) {
			GebrGeoXmlSequence *value;

			gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE, &value, 0);
			for (; value != NULL; gebr_geoxml_sequence_next(&value)) {
				GString *path;

				path = g_string_new(gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(value)));
				if (path->len == 0)
					continue;
				else
					cleaned = TRUE;

				func(path, data);
				gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(value), path->str);
				g_string_free(path, TRUE);
			}
			if (set_programs_unconfigured && cleaned) {
				GebrGeoXmlProgram *program = gebr_geoxml_parameter_get_program(parameter);
				gebr_geoxml_program_set_status (program, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
				gebr_geoxml_program_set_error_id(program, FALSE, GEBR_IEXPR_ERROR_PATH);
				gebr_geoxml_object_unref(program);
				return FALSE;
			}
		}
		return TRUE;
	}

	/* flow's IO */
	g_string_assign(path, gebr_geoxml_flow_io_get_input(flow));
	func(path, data);
	gebr_geoxml_flow_io_set_input(flow, path->str);
	g_string_assign(path, gebr_geoxml_flow_io_get_output(flow));
	func(path, data);
	gebr_geoxml_flow_io_set_output(flow, path->str);
	g_string_assign(path, gebr_geoxml_flow_io_get_error(flow));
	func(path, data);
	gebr_geoxml_flow_io_set_error(flow, path->str);

	/* all parameters */
	gebr_geoxml_flow_foreach_parameter(flow, (GebrGeoXmlCallback)flow_paths_foreach_parameter, NULL);

	/* call recursively for each revision */
	GebrGeoXmlSequence *revision;
	gebr_geoxml_flow_get_revision(flow, &revision, 0);
	for (; revision != NULL; gebr_geoxml_sequence_next(&revision)) {
		gchar *xml;
		gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(revision), &xml, NULL, NULL, NULL);
		GebrGeoXmlFlow *rev;
		if (gebr_geoxml_document_load_buffer((GebrGeoXmlDocument **)&rev, xml) == GEBR_GEOXML_RETV_SUCCESS) {
			gebr_flow_modify_paths(rev, func, set_programs_unconfigured, data);
			g_free(xml);
			gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(rev), &xml);
			gebr_geoxml_flow_set_revision_data(GEBR_GEOXML_REVISION(revision), xml, NULL, NULL, NULL);
			document_free(GEBR_GEOXML_DOCUMENT(rev));
		}
		g_free(xml);
	}

	g_string_free(path, TRUE);
}

void flow_set_paths_to_relative(GebrGeoXmlFlow * flow, GebrGeoXmlLine *line, gchar ***paths, gboolean relative)
{
	gchar *mount_point;
	void func(GString *path, gpointer data)
	{
		gchar *tmp;

		if (!paths || path->len == 0) {
			tmp = g_strdup(path->str);
		} else {
			gchar *tmp2 = gebr_relativise_old_home_path(path->str);
			tmp = gebr_relativise_path(tmp2, mount_point, paths);
			g_free(tmp2);
		}

		g_string_assign(path, tmp);
		g_free(tmp);
	}

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, line);
	if (maestro)
		mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	else
		mount_point = NULL;
	gebr_flow_modify_paths(flow, func, FALSE, NULL);
}

void flow_set_paths_to_empty(GebrGeoXmlFlow * flow)
{
	void func(GString *path, gpointer data)
	{
		g_string_assign(path, "");
	}
	gebr_flow_modify_paths(flow, func, TRUE, NULL);
}

static void
on_response_event(GtkDialog *dialog,
                  gint       response_id,
                  gpointer   user_data)
{
	if (response_id == GTK_RESPONSE_HELP) {
		const gchar *section = "flows_browser_save_state_flow";
		gchar *error;

		gebr_gui_help_button_clicked(section, &error);

		if (error) {
			gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, error);
			g_free(error);
		}
	}
}
void
on_comment_changed(GtkEntry *entry,
		   GtkWidget *dialog)
{
	const gchar *text = gtk_entry_get_text(entry);

	if (*text) {
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);
	}
}

void
on_comment_activate(GtkEntry *entry,
		   GtkWidget *dialog)
{
	const gchar *text = gtk_entry_get_text(entry);
	const gchar *err_tooltip;

	if (!*text) {
		err_tooltip = _("Insert a description to the snapshot.");
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
		gtk_entry_set_icon_tooltip_markup(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, err_tooltip);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);
	} else if (strlen(text) > 60) {
		err_tooltip = _("The description accepts up to 60 characters");
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
		gtk_entry_set_icon_tooltip_markup(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, err_tooltip);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);
	} else {
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);
	}
}

void
gebr_flow_set_snapshot_last_modify_date(const gchar *last_date)
{
	const gchar *flow_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
	GtkTreeIter iter;

	gebr_gui_gtk_tree_model_find_by_column(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
	                                       FB_FILENAME, flow_filename);

	gtk_list_store_set(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter,
			   FB_SNP_LAST_MODIF, g_strdup(last_date),
			   -1);
}

gboolean flow_revision_save(void)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *align;

	GtkTreeIter iter;
	gboolean ret = FALSE;

	gchar *flow_filename;

	GebrGeoXmlDocument *flow;
	if (!flow_browse_get_selected(&iter, TRUE))
		return FALSE;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gint num_flows = gtk_tree_selection_count_selected_rows(selection);

	const gchar *dialog_title;
	const gchar *dialog_msg;

	if (num_flows == 1) {
		dialog_title = _("Take a snapshot of the current Flow?");
		dialog_msg = _("Enter a description for the snapshot of the current Flow:");
	} else {
		dialog_title = _("Take a snapshot of the selected Flows?");
		dialog_msg = _("Enter a description for the snapshots of the selected Flows:");
	}

	dialog = gtk_dialog_new_with_buttons(dialog_title,
					     GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	align = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 10, 10, 10, 10);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), align, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(align), vbox);

	label = gtk_label_new(dialog_msg);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_entry_set_max_length(GTK_ENTRY(entry), 60);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	g_signal_connect(dialog, "response", G_CALLBACK(on_response_event), NULL);

	const gchar *comm = _("Insert a description of your snapshot.");
	gtk_widget_set_tooltip_text(GTK_WIDGET(entry), comm);

	g_signal_connect(GTK_ENTRY(entry), "activate", G_CALLBACK(on_comment_activate), (gpointer)dialog);
	g_signal_connect(GTK_ENTRY(entry), "changed", G_CALLBACK(on_comment_changed), (gpointer)dialog);
	gtk_widget_show_all(dialog);

	gebr.ui_flow_browse->update_graph = TRUE;
	const gchar *text;
	gint response;
	while (1) {
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		if (response == GTK_RESPONSE_OK) {
			text = gtk_entry_get_text(GTK_ENTRY(entry));
			if (!*text){
				on_comment_activate(GTK_ENTRY(entry), dialog);
				continue;
			}
		}

		if (response != GTK_RESPONSE_HELP)
			break;
	}

	if (response == GTK_RESPONSE_OK) {
		GebrGeoXmlRevision *revision;
		gchar *id;

		gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {

			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_FILENAME, &flow_filename, -1);
			if (document_load((GebrGeoXmlDocument**)(&flow), flow_filename, TRUE))
			{
				g_free(flow_filename);
				return FALSE;
			}

			revision = gebr_geoxml_flow_append_revision(GEBR_GEOXML_FLOW(flow), 
								    gtk_entry_get_text(GTK_ENTRY(entry)));

			gebr_geoxml_flow_get_revision_data(revision, NULL, NULL, NULL, &id);
			gebr_geoxml_document_set_parent_id(GEBR_GEOXML_DOCUMENT(flow), id);

			document_save(flow, TRUE, FALSE);
			flow_browse_reload_selected();
			ret = TRUE;

			gtk_toggle_button_set_active(gebr.ui_flow_browse->snapshots_ctx_button, TRUE);

			//document_free(flow);
			g_free (flow_filename);
		}
		gchar *last_date = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(gebr.flow));
		gebr_flow_set_snapshot_last_modify_date(last_date);
		g_free(last_date);
	}

	gtk_widget_destroy(dialog);

	return ret;
}

gboolean
flow_revision_remove(GebrGeoXmlFlow *flow,
                     gchar *id_remove,
                     gchar *parent_head,
                     GHashTable *hash)
{
	gboolean result;

	gchar *id;
	GebrGeoXmlSequence *seq;
	gchar *parent_id = NULL;

	gboolean removed = FALSE;
	gebr_geoxml_flow_get_revision(flow, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		gchar *xml;
		gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(seq), &xml, NULL, NULL, &id);
		if (!g_strcmp0(id, id_remove)) {
			GebrGeoXmlDocument *doc;
			if (gebr_geoxml_document_load_buffer(&doc, xml) != GEBR_GEOXML_RETV_SUCCESS) {
				g_warn_if_reached();
				return FALSE;
			}

			parent_id = gebr_geoxml_document_get_parent_id(doc);

			removed = TRUE;
			gebr_geoxml_sequence_remove(seq);

			gebr_geoxml_document_free(doc);
			g_free(xml);
			break;
		}
		g_free(xml);
	}

	g_return_val_if_fail(removed == TRUE, FALSE);

	GList *children = g_hash_table_lookup(hash, id_remove);
	if (!children) {
		gboolean has_head = (g_strcmp0(parent_head, id_remove) == 0);
		g_free(id);
		return has_head;
	}

	if (!g_strcmp0(id_remove, parent_head))
		result = TRUE;

	for (GList *i = children; i; i = i->next) {
		//Change children's father in XML
		gchar *flow_xml = NULL;
		GebrGeoXmlDocument *doc_rev;
		GebrGeoXmlRevision *rev = gebr_geoxml_flow_get_revision_by_id(flow, i->data);
		gebr_geoxml_flow_get_revision_data(rev, &flow_xml, NULL, NULL, NULL);

		if (gebr_geoxml_document_load_buffer(&doc_rev, flow_xml) != GEBR_GEOXML_RETV_SUCCESS) {
			g_warn_if_reached();
			return FALSE;
		}
		g_free(flow_xml);

		gebr_geoxml_document_set_parent_id(doc_rev, parent_id);
		gebr_geoxml_document_to_string(doc_rev, &flow_xml);
		gebr_geoxml_flow_set_revision_data(rev, flow_xml, NULL, NULL, NULL);

		gebr_geoxml_document_free(doc_rev);
		g_free(flow_xml);
	}

	GList *parent_children = g_hash_table_lookup(hash, parent_id);
	parent_children = g_list_remove(parent_children, id_remove);

	parent_children = g_list_concat(parent_children, g_list_copy(children));
	g_hash_table_insert(hash, parent_id, parent_children);

	g_hash_table_remove(hash, id_remove);
	g_free(id);
	gebr.ui_flow_browse->update_graph = TRUE;

	return result;
}

GHashTable *
gebr_flow_revisions_hash_create(GebrGeoXmlFlow *flow)
{
	GHashTable *hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	GebrGeoXmlSequence *seq;
	gebr_geoxml_flow_get_revision(flow, &seq, 0);

	for(; seq; gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlRevision *rev = GEBR_GEOXML_REVISION(seq);
		gchar *flow_xml;
		gchar *id;
		GebrGeoXmlDocument *revdoc;

		gebr_geoxml_flow_get_revision_data(rev, &flow_xml, NULL, NULL, &id);

		//Insert itself in the list as a key
		if (!g_hash_table_lookup(hash, id))
			g_hash_table_insert(hash, g_strdup(id), NULL);

		if (gebr_geoxml_document_load_buffer(&revdoc, flow_xml) != GEBR_GEOXML_RETV_SUCCESS) {
			g_free(flow_xml);
			g_free(id);
			return NULL;
		}
		g_free(flow_xml);

		gchar *parent_id = gebr_geoxml_document_get_parent_id(revdoc);

		if (!parent_id || !*parent_id) {
			GList *childs = g_hash_table_lookup(hash, id);
			g_hash_table_insert(hash, g_strdup(id), childs);
			g_free(id);
			gebr_geoxml_document_free(revdoc);
			continue;
		}

		GList *childs = g_hash_table_lookup(hash, parent_id);
		childs = g_list_prepend(childs, g_strdup(id));
		g_hash_table_insert(hash, parent_id, childs);

		g_free(id);
		gebr_geoxml_document_free(revdoc);
	}

	return hash;
}
void
gebr_flow_revisions_hash_free(GHashTable *revision)
{
	void free_hash(gpointer key, gpointer value)
	{
		GList *list = value;
		g_list_free(list);
	}

	g_hash_table_foreach(revision, (GHFunc) free_hash, NULL);
	g_hash_table_destroy(revision);
}

gboolean
gebr_flow_revisions_create_graph(GebrGeoXmlFlow *flow,
                                 GHashTable *revs,
                                 gchar **dot_file)
{
	gchar *dot_content = gebr_geoxml_flow_create_dot_code(flow, revs);

	if (dot_file)
		*dot_file = g_strdup(dot_content);

	g_free(dot_content);

	return TRUE;
}

void flow_program_remove(void)
{
	GtkTreeIter iter;
	gboolean valid = FALSE;
	GebrGeoXmlProgram *program;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter)){
			flow_edition_set_io();
		} else {
			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
					   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
			gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(program));

			// Remove `iter' variable from dictionary if the Loop is configured
			if (gebr_geoxml_program_get_control (program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR
			    && gebr_geoxml_program_get_status(program) != GEBR_GEOXML_PROGRAM_STATUS_DISABLED) {
				GebrGeoXmlSequence *param;
				GError *err = NULL;
				GList *affected;
				param = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
				gebr_validator_remove(gebr.validator, GEBR_GEOXML_PARAMETER(param), &affected, &err);
				flow_edition_revalidate_programs();
			}
			valid = gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store), &iter);
		}

	}
	flow_program_check_sensitiveness();
	flow_edition_set_io();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	gebr_flow_set_toolbar_sensitive();
	if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0)
		flow_edition_set_run_widgets_sensitiveness(gebr.ui_flow_edition, FALSE, FALSE);

	if (valid)
		flow_edition_select_component_iter(&iter);
}

void flow_program_move_top(void)
{
	GtkTreeIter iter;
	GebrGeoXmlProgramControl control;

	control = gebr_geoxml_program_get_control (gebr.program);
	if (control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return;

	flow_edition_get_selected_component(&iter, FALSE);
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(gebr.program), NULL);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
				  &iter, &gebr.ui_flow_edition->input_iter);
	flow_program_check_sensitiveness();
}

void flow_program_move_bottom(void)
{
	GtkTreeIter iter;
	GebrGeoXmlProgramControl control;

	control = gebr_geoxml_program_get_control (gebr.program);
	if (control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return;

	flow_edition_get_selected_component(&iter, FALSE);
	gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(gebr.program), NULL);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
				   &iter, &gebr.ui_flow_edition->output_iter);
	flow_program_check_sensitiveness();
}

void flow_copy(void)
{
	GtkTreeIter iter;

	if (gebr.flow_clipboard != NULL) {
		g_list_foreach(gebr.flow_clipboard, (GFunc) g_free, NULL);
		g_list_free(gebr.flow_clipboard);
		gebr.flow_clipboard = NULL;
	}

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
		gchar *filename;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_FILENAME, &filename, -1);
		gebr.flow_clipboard = g_list_prepend(gebr.flow_clipboard, filename);
	}
	gebr.flow_clipboard = g_list_reverse(gebr.flow_clipboard);
}

void flow_paste(void)
{
	GList *i;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	for (i = g_list_first(gebr.flow_clipboard); i != NULL; i = g_list_next(i)) {
		GebrGeoXmlDocument *flow;

		if (document_load(&flow, (gchar *)i->data, FALSE)) {
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not paste Flow. It was probably deleted."));
			continue;
		}

		document_import(flow, TRUE);
		line_append_flow_iter(GEBR_GEOXML_FLOW(flow),
				      GEBR_GEOXML_LINE_FLOW(gebr_geoxml_line_append_flow(gebr.line,
											 gebr_geoxml_document_get_filename(flow))));
		document_save(GEBR_GEOXML_DOCUMENT(gebr.line), TRUE, FALSE);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		gebr_geoxml_flow_revalidate(GEBR_GEOXML_FLOW(flow), gebr.validator);
	}
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

	flow_edition_set_run_widgets_sensitiveness(gebr.ui_flow_edition, TRUE, FALSE);
	project_line_info_update();
}

void flow_program_copy(void)
{
	GtkTreeIter iter;

	gebr_geoxml_clipboard_clear();
	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		GebrGeoXmlObject *program;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		gebr_geoxml_clipboard_copy(GEBR_GEOXML_OBJECT(program));
	}
}

void flow_program_paste(void)
{
	GebrGeoXmlSequence *pasted;

	gboolean flow_has_loop = gebr_geoxml_flow_has_control_program(gebr.flow);
	pasted = GEBR_GEOXML_SEQUENCE(gebr_geoxml_clipboard_paste(GEBR_GEOXML_OBJECT(gebr.flow)));
	if (pasted == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not paste program."));
		return;
	}

	if (gebr_geoxml_clipboard_has_forloop() && !flow_has_loop)
	{
		GebrGeoXmlParameter *parameter;
		gebr_geoxml_flow_insert_iter_dict(gebr.flow);
		parameter = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow)));
		gebr_validator_insert(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
	}

	flow_add_program_sequence_to_view(GEBR_GEOXML_SEQUENCE(pasted), TRUE, FALSE);
	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
	flow_edition_revalidate_programs();
	gebr_flow_set_toolbar_sensitive();
	flow_edition_set_run_widgets_sensitiveness(gebr.ui_flow_edition, TRUE, FALSE);
}

void
gebr_flow_set_toolbar_sensitive(void)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);

	gboolean sensitive = TRUE;
	gboolean sensitive_exec_slider;
	gboolean maestro_disconnected = FALSE;
	gboolean line_selected = TRUE;

	if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED) {
		sensitive = FALSE;
		maestro_disconnected = TRUE;
	}

	if (!gebr.line) {
		sensitive = FALSE;
		line_selected = FALSE;
	}

	if (gebr_geoxml_line_get_flows_number(gebr.line) == 0 )
		sensitive_exec_slider = FALSE;
	else if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0)
		sensitive_exec_slider = FALSE;
	else
		sensitive_exec_slider = sensitive;


	// Set sensitive for page Flows
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_new"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_delete"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_properties"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_dict_edit"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_import"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_export"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_execute"), sensitive_exec_slider);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_execute_parallel"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_copy"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_paste"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_view"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_edit"), sensitive);
	gtk_widget_set_sensitive(gebr.ui_flow_browse->speed_button, sensitive_exec_slider);

	GtkTreeSelection *flows_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gint flows_nrows = gtk_tree_selection_count_selected_rows(flows_selection);

	if (sensitive && flows_nrows == 1) {
		gtk_widget_show(gebr.ui_flow_browse->info_window);
		gtk_widget_hide(gebr.ui_flow_browse->warn_window);
	} else {
		if (!line_selected) {
			gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->warn_window), _("No Line is selected\n"));
		} else if (maestro_disconnected) {
			gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->warn_window),
					   _("The Maestro of this Line is disconnected,\nthen you cannot edit flows.\n"
					     "Try changing its maestro or connecting it."));
		} else if (flows_nrows  !=  1) {
			gchar *label_msg;
			if (flows_nrows >1 )
				label_msg = g_markup_printf_escaped(_("%d Flows selected.\n\n"
						"GêBR can execute them\n"
						"- <i>sequentially</i> (Ctrl+R) or\n"
						"- <i>parallelly</i> (Ctrl+Shift+R)"),
						flows_nrows);
			else
				label_msg = g_markup_printf_escaped(_("This Line has no Flows.\n\n"
						"Click on <i>New Flow</i> to create a new one\n"
						"or import one through the <i>Import Flow</i> icon."));

			gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->warn_window), label_msg);
			g_free(label_msg);
		}

		gtk_widget_show(gebr.ui_flow_browse->warn_window);
		gtk_widget_hide(gebr.ui_flow_browse->info_window);
	}

	// Set sensitive for page Flow Editor
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_help"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_delete"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_properties"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_refresh"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_copy"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_paste"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_top"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_bottom"), sensitive);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_execute"), sensitive_exec_slider);
	gtk_widget_set_sensitive(gebr.ui_flow_edition->speed_button, sensitive_exec_slider);
}

static void append_parameter_row(GebrGeoXmlParameter * parameter,
                                 GString * dump)
{
	gint i, n_instances;
	GebrGeoXmlSequence * param;
	GebrGeoXmlSequence * instance;
	GebrGeoXmlParameters * parameters;

	if (gebr_geoxml_parameter_get_is_program_parameter(parameter)) {
		GString * str_value;
		GString * default_value;
		GebrGeoXmlProgramParameter * program;
		gint radio_value = GEBR_PARAM_TABLE_NO_TABLE;

		program = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
		str_value = gebr_geoxml_program_parameter_get_string_value(program, FALSE);
                default_value = gebr_geoxml_program_parameter_get_string_value(program, TRUE);

		switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
		case NOTEBOOK_PAGE_PROJECT_LINE:
			radio_value = gebr.config.detailed_line_parameter_table;
			break;
		case NOTEBOOK_PAGE_FLOW_BROWSE:
			radio_value = gebr.config.detailed_flow_parameter_table;
			break;
		default:
			radio_value = GEBR_PARAM_TABLE_ONLY_CHANGED;
			g_warn_if_reached ();
			break;
		}

		if (((radio_value == GEBR_PARAM_TABLE_ONLY_CHANGED) && (g_strcmp0(str_value->str, default_value->str) != 0)) ||
		    ((radio_value == GEBR_PARAM_TABLE_ONLY_FILLED) && (str_value->len > 0)) ||
		    ((radio_value == GEBR_PARAM_TABLE_ALL)))
		{
			/* Translating enum values to labels */
			GebrGeoXmlSequence *enum_option = NULL;

			gebr_geoxml_program_parameter_get_enum_option(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), &enum_option, 0);

			for (; enum_option; gebr_geoxml_sequence_next(&enum_option))
			{
				gchar *enum_value = gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option));
				if (g_strcmp0(str_value->str, enum_value) == 0)
				{
					gchar *label = gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option));
					g_string_printf(str_value, "%s", label);
					g_free(enum_value);
					g_free(label);
					gebr_geoxml_object_unref(enum_option);
					break;
				}
				g_free(enum_value);
			}
			gchar *label = gebr_geoxml_parameter_get_label(parameter);
			str_value->str = g_markup_printf_escaped("%s",str_value->str);
			g_string_append_printf(dump,
			                       "      <tr class=\"param\">\n  "
			                       "        <td class=\"label\">%s</td>\n"
			                       "        <td class=\"value\">%s</td>\n"
			                       "      </tr>\n",
					       label, str_value->str);
			g_free(label);
		}
		g_string_free(str_value, TRUE);
		g_string_free(default_value, TRUE);
	} else {
		GString * previous_table = g_string_new(dump->str);
		gchar *label = gebr_geoxml_parameter_get_label(parameter);
		g_string_append_printf(dump,
		                       "      <tr class=\"group\">\n  "
		                       "        <td class=\"label\" colspan=\"2\">%s</td>\n"
		                       "      </tr>\n",
		                       label);
		g_free(label);
		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		n_instances = gebr_geoxml_parameter_group_get_instances_number(GEBR_GEOXML_PARAMETER_GROUP(parameter));

		i = 1;
		GString * group_table = g_string_new(dump->str);

		while (instance) {
			GString * instance_table = g_string_new(dump->str);
			if (n_instances > 1)
				g_string_append_printf(dump,
				                       "      <tr class=\"group\">\n  "
				                       "        <td class=\"label\" colspan=\"2\">%s %d</td>\n"
				                       "      </tr>\n",
						       _("Instance"), i++);
			parameters = GEBR_GEOXML_PARAMETERS(instance);
			gebr_geoxml_parameters_get_parameter(parameters, &param, 0);

			GString * inner_table = g_string_new(dump->str);

			while (param) {
				append_parameter_row(GEBR_GEOXML_PARAMETER(param), dump);
				gebr_geoxml_sequence_next(&param);
			}
			/*If there are no parameters returned by the dialog choice...*/
			if(g_string_equal(dump, inner_table))
			/*...return the table to it previous content*/
				g_string_assign(dump, instance_table->str);

			g_string_free(inner_table, TRUE);

			gebr_geoxml_sequence_next(&instance);
			g_string_free(instance_table, TRUE);
		}
		/* If there are no instance inside the group...*/
		if(g_string_equal(dump, group_table))
		/*...return the table to it outermost previous content*/
			g_string_assign(dump, previous_table->str);

		g_string_free(previous_table, TRUE);
		g_string_free(group_table, TRUE);
	}
}

static void
gebr_program_generate_parameter_value_table (GebrGeoXmlProgram *program,
                                             GString *tables_content)
{
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *sequence;

	gchar *translated = g_strdup (_("Parameters"));
	
	g_string_append_printf(tables_content,
	                       "  <table class=\"parameters\">\n"
	                       "    <caption>%s</caption>\n"
	                       "    <thead>\n",
	                       translated);
	g_free (translated);

	parameters = gebr_geoxml_program_get_parameters (program);
	gebr_geoxml_parameters_get_parameter (parameters, &sequence, 0);
	gebr_geoxml_object_unref(parameters);

	if (sequence == NULL) {
		g_string_append(tables_content,
		                "      <tr>\n"
		                "        <td>this program has no parameters.</td>\n"
		                "      </tr>\n"
		                "    </thead>"
		                "    <tbody>"
		                "      <tr>"
		                "        <td></td>"
		                "      </tr>");
	} else {
		g_string_append_printf(tables_content,
		                       "      <tr>\n"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "      </tr>\n"
		                       "    </thead>"
		                       "    <tbody>",
		                       _("Parameter"), _("Value"));

		GString * initial_table = g_string_new(tables_content->str);

		while (sequence) {
			append_parameter_row(GEBR_GEOXML_PARAMETER(sequence), tables_content);
			gebr_geoxml_sequence_next (&sequence);
		}

		if (g_string_equal(initial_table, tables_content)) {
			switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
			case NOTEBOOK_PAGE_PROJECT_LINE:
				if (gebr.config.detailed_line_parameter_table == GEBR_PARAM_TABLE_ONLY_CHANGED)
					g_string_append_printf(tables_content,
					                       "      <tr>\n"
					                       "        <td colspan=\"2\">%s</td>\n"
					                       "      </tr>\n",
					                       _("This program has only default parameters"));

				else if (gebr.config.detailed_line_parameter_table == GEBR_PARAM_TABLE_ONLY_FILLED)
					g_string_append_printf(tables_content,
					                       "      <tr>\n"
					                       "        <td colspan=\"2\">%s</td>\n"
					                       "      </tr>\n",
					                       _("This program has only empty parameters"));

				break;
			case NOTEBOOK_PAGE_FLOW_BROWSE:
				if (gebr.config.detailed_flow_parameter_table == GEBR_PARAM_TABLE_ONLY_CHANGED)
					g_string_append_printf(tables_content,
					                       "      <tr>\n"
					                       "        <td colspan=\"2\">%s</td>\n"
					                       "      </tr>\n",
					                       _("This program has only default parameters"));

				else if (gebr.config.detailed_flow_parameter_table == GEBR_PARAM_TABLE_ONLY_FILLED)
					g_string_append_printf(tables_content,
					                       "      <tr>\n"
					                       "        <td colspan=\"2\">%s</td>\n"
					                       "      </tr>\n",
					                       _("This program has only empty parameters"));
				break;
			default:
				g_warn_if_reached ();
				break;
			}

			g_string_free(initial_table, TRUE);
		} 
	}

	g_string_append_printf (tables_content,
	                        "    </tbody>\n"
	                        "  </table>\n");
}

static void
gebr_generate_variables_header_table(GString *tables_content,
                                     gboolean insert_header,
                                     const gchar *scope)
{
	if (insert_header)
		g_string_append_printf(tables_content,
		                       "  <div class=\"variables\">\n"
		                       "    <table>\n"
		                       "      <caption>%s</caption>\n"
		                       "      <thead>\n"
		                       "        <tr>\n"
		                       "          <td>%s</td>\n"
		                       "          <td>%s</td>\n"
		                       "          <td>%s</td>\n"
		                       "          <td>%s</td>\n"
		                       "        </tr>\n"
		                       "      </thead>\n"
		                       "      <tbody>\n",
		                       _("Variables"), _("Keyword"), _("Value"),_("Result"), _("Comment"));


	g_string_append_printf(tables_content,
	                       "      <tr class=\"scope\">\n"
	                       "        <td colspan=\"4\">%s</td>\n"
	                       "      </tr>\n",
	                       scope);
}

void
gebr_generate_variables_value_table(GebrGeoXmlDocument *doc,
                                    gboolean insert_header,
                                    gboolean close,
                                    GString *tables_content,
                                    const gchar *scope)
{
	/* Set validator to @doc */
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &doc, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

	gebr_generate_variables_header_table(tables_content, insert_header, scope);

	GebrGeoXmlParameters *params;
	GebrGeoXmlSequence *sequence;
	GebrGeoXmlParameterType type;

	gchar *name;
	gchar *value;
	gchar *comment;
	gchar *eval = NULL;
	gchar *result = NULL;
	gchar *var_type;
	gboolean have_vars = FALSE;
	GError *error = NULL;

	params = gebr_geoxml_document_get_dict_parameters(doc);
	gebr_geoxml_parameters_get_parameter(params, &sequence, 0);
	gebr_geoxml_object_unref(params);
	while (sequence) {
		have_vars = TRUE;
		name = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(sequence));
		value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(sequence), FALSE);
		comment = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(sequence));

		gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
		gchar *mount_point;
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
		if (maestro)
			mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
		else
			mount_point = NULL;
		value = gebr_relativise_path(value,mount_point,paths);
		value = g_markup_printf_escaped("%s",value);
		gebr_validator_evaluate_param(gebr.validator, GEBR_GEOXML_PARAMETER(sequence), &eval, &error);

		if (!error) {
			result = g_strdup(eval);
			type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(sequence));
			if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING)
				var_type = g_strdup("string");
			else
				var_type = g_strdup("numeric");
		} else {
			result = g_strdup(error->message);
			var_type = g_strdup("error");
		}

		g_string_append_printf(tables_content,
		                       "      <tr class=\"scope-content %s\">"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "      </tr>\n",
		                       var_type, name, value, result, comment);

		g_free(name);
		g_free(value);
		g_free(comment);
		g_free(result);

		if (error)
			g_clear_error(&error);
		else
			g_free(eval);

		gebr_geoxml_sequence_next(&sequence);
	}
	if (error)
		g_error_free(error);

	if(!have_vars)
		g_string_append_printf(tables_content,
		                       "      <tr class=\"scope-content\">\n"
		                       "        <td colspan='4'>%s</td>\n"
		                       "      </tr>\n",
		                       _("There are no variables in this scope"));

	if (close)
		g_string_append (tables_content,
		                 "      </tbody>\n"
		                 "    </table>\n"
		                 "  </div>\n");

	/* Set default validator */
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
}

void
gebr_flow_generate_io_table(GebrGeoXmlFlow *flow,
                            GString *tables_content)
{
	gchar * input;
	gchar * output;
	gchar * error;

	input = gebr_geoxml_flow_io_get_input (flow);
	output = gebr_geoxml_flow_io_get_output (flow);
	error = gebr_geoxml_flow_io_get_error (flow);

	input = g_markup_printf_escaped("%s",input);
	output = g_markup_printf_escaped("%s",output);
	error = g_markup_printf_escaped("%s",error);

	g_string_append_printf(tables_content,
	                       "  <div class=\"io\">\n"
	                       "    <table>\n"
	                       "      <caption>%s</caption>\n"
	                       "      <tbody>\n"
	                       "        <tr>\n"
	                       "          <td class=\"type\">%s</td>\n"
	                       "          <td class=\"value\">%s</td>\n"
	                       "        </tr>\n"
	                       "        <tr>\n"
	                       "          <td class=\"type\">%s</td>\n"
	                       "          <td class=\"value\">%s</td>\n"
	                       "        </tr>\n"
	                       "        <tr>\n"
	                       "          <td class=\"type\">%s</td>\n"
	                       "          <td class=\"value\">%s</td>\n"
	                       "        </tr>\n"
	                       "      </tbody>\n"
	                       "    </table>\n"
	                       "  </div>\n",
	                       _("I/O table"),
	                       _("Input"), strlen (input) > 0? input : _("(none)"),
	                       _("Output"), strlen (output) > 0? output : _("(none)"),
	                       _("Error"), strlen (error) > 0? error : _("(none)"));

	g_free(input);
	g_free(output);
	g_free(error);
}

void
gebr_flow_generate_parameter_value_table(GebrGeoXmlFlow *flow,
                                         GString *prog_content,
                                         const gchar *index)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));
	GebrGeoXmlSequence * sequence;

	gebr_geoxml_flow_get_program(flow, &sequence, 0);
	while (sequence) {
		GebrGeoXmlProgram * prog;
		GebrGeoXmlProgramStatus status;

		prog = GEBR_GEOXML_PROGRAM (sequence);
		status = gebr_geoxml_program_get_status (prog);

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			gchar *title = gebr_geoxml_program_get_title(prog);
			gchar *description = gebr_geoxml_program_get_description(prog);

			gchar *link = g_strdup_printf("%s%s%d",
			                              has_index? index : "",
	                        		      has_index? "." : "",
                       				      i);

			g_string_append_printf(prog_content,
			                       "<div class=\"programs\">\n"
			                       "  <div class=\"program\">\n"
			                       "    <a name=\"%s\"></a>\n"
			                       "    <div class=\"title\">%s</div>\n"
			                       "    <div class=\"description\">%s</div>\n",
			                       link, title, description);

			gebr_program_generate_parameter_value_table(prog, prog_content);

			g_string_append(prog_content,
			                "  </div>\n"
			                "</div>\n");

			i++;

			g_free(title);
			g_free(description);
			g_free(link);
		}

		gebr_geoxml_sequence_next(&sequence);
	}
	g_free(flow_title);
}

void
gebr_flow_generate_flow_revisions_index(GebrGeoXmlFlow *flow,
                                        GString *content,
                                        const gchar *index)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gboolean has_snapshots = FALSE;
	GString *snapshots = g_string_new(NULL);
	gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));

	GebrGeoXmlSequence *seq;
	gebr_geoxml_flow_get_revision(flow, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		has_snapshots = TRUE;
		gchar *comment;
		gchar *date;

		gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(seq), NULL, &date, &comment, NULL);

		gchar *link = g_strdup_printf("%s%s%s%d",
		                              "snap",
		                              has_index? index : "",
		                              has_index? "." : "",
		                              i);

		g_string_append_printf(snapshots,
		                       "      <li>\n"
		                       "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
		                       "        <span class=\"description\">%s</span>\n"
		                       "      </li>\n",
		                       link, comment, date);

		i++;


		g_free(comment);
		g_free(date);
		g_free(link);
	}

	if (has_snapshots)
		g_string_append_printf(content,
		                       "<div class=\"index\">\n"
		                       "  %s\n"
		                       "  <ul>\n"
		                       "    %s\n"
		                       "  </ul>\n"
		                       "</div>\n",
		                       _("Snapshots of this Flow:"),
		                       snapshots->str);

	g_string_free(snapshots, TRUE);
	g_free(flow_title);
}
