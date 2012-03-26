/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but gebr.THOUT ANY gebr.RRANTY; without even the implied warranty
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "line.h"
#include "gebr.h"
#include "document.h"
#include "project.h"
#include "flow.h"
#include "callbacks.h"
#include "ui_project_line.h"
#include "ui_document.h"

static void
on_assistant_entry_changed(GtkEntry *entry,
		 GtkAssistant *assistant)
{
	GtkWidget *current_page;
	gint page_number;
	const gchar *text;

	page_number = gtk_assistant_get_current_page(assistant);
	current_page = gtk_assistant_get_nth_page(assistant, page_number);
	text = gtk_entry_get_text(entry);

	if (text && *text )
		gtk_assistant_set_page_complete(assistant, current_page, TRUE);
	else
		gtk_assistant_set_page_complete(assistant, current_page, FALSE);
}

static void
on_assistant_base_validate(GtkEntry *entry,
		 GtkAssistant *assistant)
{
	GtkWidget *current_page;
	gint page_number;
	const gchar *text;

	page_number = gtk_assistant_get_current_page(assistant);
	current_page = gtk_assistant_get_nth_page(assistant, page_number);
	text = gtk_entry_get_text(entry);

	if (text && *text)
		gtk_assistant_set_page_complete(assistant, current_page, TRUE);
	else
		gtk_assistant_set_page_complete(assistant, current_page, FALSE);
}

void
on_properties_entry_changed(GtkEntry *entry,
		 GtkWidget *widget)
{
	const gchar *text;

	text = gtk_entry_get_text(entry);

	if (text && *text && gebr_verify_starting_slash(text))
		gtk_widget_set_sensitive(widget, TRUE);
	else
		gtk_widget_set_sensitive(widget, FALSE);
}
static void
on_assistant_cancel(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
	GtkTreeIter iter;
	if (project_line_get_selected(&iter, DontWarnUnselection))
		line_delete(&iter, FALSE);
}

typedef struct {
	gint progress_animation;
	gint timeout;
	GtkWidget *assistant;
	GtkBuilder *builder;
} WizardData;

static gboolean
progress_bar_animate(gpointer user_data)
{
	WizardData *data = user_data;
	GObject *progress = gtk_builder_get_object(data->builder, "progressbar");
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress));
	return TRUE;
}


static void
on_maestro_path_error(GebrMaestroServer *maestro,
		      gint error_id,
		      WizardData *data)
{
	g_source_remove(data->progress_animation);
	g_source_remove(data->timeout);

	GtkWidget *page3 = GTK_WIDGET(gtk_builder_get_object(data->builder, "main_progress"));
	GObject *container_progress = gtk_builder_get_object(data->builder, "container_progressbar");
	GObject *container_message = gtk_builder_get_object(data->builder, "container_message");
	GObject *image = gtk_builder_get_object(data->builder, "image_status");
	GObject *label = gtk_builder_get_object(data->builder, "label_status");

	gchar *summary_txt;
	GObject *label_summary = gtk_builder_get_object(data->builder, "label_summary");

	gtk_widget_hide(GTK_WIDGET(container_progress));
	gtk_widget_show_all(GTK_WIDGET(container_message));

	switch (error_id) {
	case GEBR_COMM_PROTOCOL_STATUS_PATH_OK:
		gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_OK, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(label), _("Success!"));
		gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
					    page3, GTK_ASSISTANT_PAGE_SUMMARY);
		gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
					     page3, _("Done"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page3, TRUE);
		summary_txt = g_markup_printf_escaped("<span size='large'>%s</span>",
						      _("The directories were successfully created!"));
		break;
	case GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR:
		gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(label), _("Press the back buttom to change the BASE directory\n"
						       "or the line title."));
		gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
					    page3, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
					     page3, _("Error!"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page3, FALSE);
		summary_txt = g_markup_printf_escaped("<span size='large'>%s</span>",
						      _("The directory could not be created!"));
		break;
	case GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS:
		gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(label), _("You can change the BASE directory by pressing the\n"
						       "back buttom or use this folder anyway."));
		gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
					    page3, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
					     page3, _("Warning!"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page3, TRUE);
		summary_txt = g_markup_printf_escaped("<span size='large'>%s</span>",
						      _("The directory already exists!"));
		break;
	}

	gtk_label_set_markup(GTK_LABEL(label_summary), summary_txt);
	g_free(summary_txt);

	g_signal_handlers_disconnect_by_func(maestro, on_maestro_path_error, data);

}

static gboolean
time_out_error(gpointer user_data)
{
	WizardData *data = user_data;

	g_source_remove(data->timeout);

	GObject *image = gtk_builder_get_object(data->builder, "image_status");
	GObject *label = gtk_builder_get_object(data->builder, "label_status");
	GtkWidget *page3 = GTK_WIDGET(gtk_builder_get_object(data->builder, "main_progress"));
	GObject *progress = gtk_builder_get_object(data->builder, "container_progressbar");
	GObject *summary = gtk_builder_get_object(data->builder, "label_summary");
	GObject *container = gtk_builder_get_object(data->builder, "container_message");

	gtk_widget_hide(GTK_WIDGET(progress));
	gtk_widget_show(GTK_WIDGET(container));
	gtk_widget_show(GTK_WIDGET(image));
	gtk_widget_show(GTK_WIDGET(label));

	gchar *tmp = g_markup_printf_escaped("<span size='large'>%s</span>",
	                                     _("Timed Out!"));
	gtk_label_set_markup(GTK_LABEL(summary), tmp);
	g_free(tmp);
	gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
	gtk_label_set_text(GTK_LABEL(label), _("Could not create directory.\nCheck if your maestro is connected."));

	gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
	                            page3, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
	                             page3, _("Error!"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page3, FALSE);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	g_signal_handlers_disconnect_by_func(maestro, on_maestro_path_error, data);

	return FALSE;
}

static void
on_assistant_close(GtkAssistant *assistant,
		   WizardData *data)
{
	gint page = gtk_assistant_get_current_page(assistant) + 1;

	if (page == 3) {
		GtkTreeIter iter;

		gebr_ui_document_set_properties_from_builder(GEBR_GEOXML_DOCUMENT(gebr.line), data->builder);
		project_line_get_selected(&iter, DontWarnUnselection);
		gtk_tree_store_set(gebr.ui_project_line->store, &iter,
				   PL_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line)), -1);
		project_line_info_update();
		gtk_widget_destroy(GTK_WIDGET(assistant));
	}
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar *home = g_build_filename(gebr_maestro_server_get_home_dir(maestro), NULL);
	gebr_geoxml_line_append_path(gebr.line, "HOME", home);
	g_free(home);
}

static void
on_assistant_prepare(GtkAssistant *assistant,
		     GtkWidget *current_page,
		     WizardData *data)
{
	gint page = gtk_assistant_get_current_page(assistant) + 1;
	GObject *entry_base = gtk_builder_get_object(data->builder, "entry_base");

	if (page == 2) {
		GObject *entry_title = gtk_builder_get_object(data->builder, "entry_title");
		gchar *line_key = gebr_geoxml_line_create_key(gtk_entry_get_text(GTK_ENTRY(entry_title)));
		gchar *path = g_build_filename("<HOME>", "GeBR", line_key, NULL);

		gtk_entry_set_text(GTK_ENTRY(entry_base), path);

		g_free(line_key);
		g_free(path);
	}
	else if (page == 3) {
		GObject *container_progress = gtk_builder_get_object(data->builder, "container_progressbar");
		GObject *container_message = gtk_builder_get_object(data->builder, "container_message");
		GObject *label_summary = gtk_builder_get_object(data->builder, "label_summary");
		gchar *tmp = g_markup_printf_escaped("<span size='large'>%s</span>", _("Creating directories..."));
		gtk_label_set_markup(GTK_LABEL(label_summary), tmp);
		g_free(tmp);

		gtk_widget_hide(GTK_WIDGET(container_message));
		gtk_widget_show(GTK_WIDGET(container_progress));

		g_debug("Initiating progress");
		data->progress_animation = g_timeout_add(200, progress_bar_animate, data);
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
		g_signal_connect(maestro, "path-error", G_CALLBACK(on_maestro_path_error), data);
		gebr_ui_document_send_paths_to_maestro(maestro, GEBR_COMM_PROTOCOL_PATH_CREATE,
						       NULL, gtk_entry_get_text(GTK_ENTRY(entry_base)));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), current_page, FALSE);

		data->timeout = g_timeout_add(5000, time_out_error, data);
	}
}

static void
on_base_entry_press(GtkEntry            *entry,
		    GtkEntryIconPosition icon_pos,
		    GdkEvent            *event,
		    gpointer             user_data)
{
	WizardData *data = user_data;

	GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Choose BASE directory", GTK_WINDOW(data->assistant),
	                                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                                      GTK_STOCK_ADD, GTK_RESPONSE_OK,
	                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar *prefix = gebr_maestro_server_get_sftp_prefix(maestro);
	gchar *mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	gchar *home = g_build_filename(prefix, gebr_maestro_server_get_home_dir(maestro), NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), FALSE);
	gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(file_chooser), home);
	gint response = gtk_dialog_run(GTK_DIALOG(file_chooser));

	if (response == GTK_RESPONSE_OK) {
		gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		gtk_entry_set_text(entry, gebr_remove_path_prefix(mount_point, folder));
		g_free(folder);
	}

	g_free(prefix);
	g_free(home);
	gtk_widget_destroy(file_chooser);
}

static void
on_import_entry_press(GtkEntry            *entry,
                      GtkEntryIconPosition icon_pos,
                      GdkEvent            *event,
                      gpointer             user_data)
{
	WizardData *data = user_data;

	GtkWidget *file_chooser = gtk_file_chooser_dialog_new("Choose IMPORT directory", GTK_WINDOW(data->assistant),
	                                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                                      GTK_STOCK_ADD, GTK_RESPONSE_OK,
	                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar *prefix = gebr_maestro_server_get_sftp_prefix(maestro);
	gchar *mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	gchar *home = g_build_filename(prefix, gebr_maestro_server_get_home_dir(maestro), NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), FALSE);
	gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(file_chooser), home);
	gint response = gtk_dialog_run(GTK_DIALOG(file_chooser));

	if (response == GTK_RESPONSE_OK) {
		gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		gtk_entry_set_text(entry, gebr_remove_path_prefix(mount_point, folder));
		g_free(folder);
	}
	g_free(prefix);
	g_free(home);
	gtk_widget_destroy(file_chooser);
}

static void
line_setup_wizard(GebrGeoXmlLine *line)
{
	GtkBuilder *builder = gtk_builder_new();

	if (!gtk_builder_add_from_file(builder, GEBR_GLADE_DIR "/document-properties.glade", NULL))
		return;

	GtkWidget *page1 = GTK_WIDGET(gtk_builder_get_object(builder, "main_props"));
	GtkWidget *page2 = GTK_WIDGET(gtk_builder_get_object(builder, "widget_paths"));
	GtkWidget *page3 = GTK_WIDGET(gtk_builder_get_object(builder, "main_progress"));
	GtkWidget *assistant = gtk_assistant_new();
	gtk_window_set_title(GTK_WINDOW(assistant), _("Creating a new Line"));

	WizardData *data = g_new(WizardData, 1);
	data->assistant = assistant;
	data->builder = builder;
	g_signal_connect(assistant, "cancel", G_CALLBACK(on_assistant_cancel), NULL);
	g_signal_connect(assistant, "close", G_CALLBACK(on_assistant_close), data);
	g_signal_connect(assistant, "prepare", G_CALLBACK(on_assistant_prepare), data);

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page1);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page1, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page1, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page1, _("Basic line information"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page2);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page2, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page2, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page2, _("Line paths"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page3);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page3, GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page3, _("Creating directories..."));

	GObject *entry_title = gtk_builder_get_object(builder, "entry_title");
	GObject *entry_base = gtk_builder_get_object(builder, "entry_base");
	GObject *entry_import = gtk_builder_get_object(builder, "entry_import");
	GObject *entry_author = gtk_builder_get_object(builder, "entry_author");
	GObject *entry_email = gtk_builder_get_object(builder, "entry_email");

	g_signal_connect(entry_base, "icon-press", G_CALLBACK(on_base_entry_press), data);
	g_signal_connect(entry_import, "icon-press", G_CALLBACK(on_import_entry_press), data);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	gchar *path = g_build_filename(gebr_maestro_server_get_home_dir(maestro),"GeBR",
	                               gebr_geoxml_line_create_key(gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(line))),
	                               NULL);
	gtk_entry_set_text (GTK_ENTRY(entry_base), path);
	g_free(path);

	gchar *title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(line));
	gtk_entry_set_text(GTK_ENTRY(entry_title), title);
	g_free(title);

	gtk_entry_set_text(GTK_ENTRY(entry_author), gebr.config.username->str);
	gtk_entry_set_text(GTK_ENTRY(entry_email), gebr.config.email->str);

	g_signal_connect(entry_title, "changed", G_CALLBACK(on_assistant_entry_changed), assistant);
	g_signal_connect(entry_base, "changed", G_CALLBACK(on_assistant_base_validate), assistant);

	gtk_widget_show(assistant);
}

void line_new(void)
{
	GtkTreeIter iter;
	GtkTreeIter parent;
	GtkTreeModel *model;
	GebrGeoXmlLine *line;
	GebrGeoXmlDocument *doc;

	if (!project_line_get_selected (&parent, ProjectLineSelection))
		return;

	model = GTK_TREE_MODEL (gebr.ui_project_line->store);
	gtk_tree_model_get (model, &parent, PL_XMLPOINTER, &doc, -1);

	if (gebr_geoxml_document_get_type (doc) == GEBR_GEOXML_DOCUMENT_TYPE_LINE) {
		iter = parent;
		gtk_tree_model_iter_parent (model, &parent, &iter);
	}

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	line = GEBR_GEOXML_LINE(document_new(GEBR_GEOXML_DOCUMENT_TYPE_LINE));
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(line), _("New Line"));
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(line), gebr.config.username->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(line), gebr.config.email->str);

	if (maestro)
		gebr_geoxml_line_set_maestro(line, gebr_maestro_server_get_address(maestro));

	iter = project_append_line_iter(&parent, line);
	gebr_geoxml_project_append_line(gebr.project, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(line)));
	document_save(GEBR_GEOXML_DOC(gebr.project), TRUE, FALSE);
	document_save(GEBR_GEOXML_DOC(line), TRUE, FALSE);

	project_line_select_iter(&iter);

	line_setup_wizard(gebr.line);

	//document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.line), on_properties_response, TRUE);
}

gboolean line_delete(GtkTreeIter * iter, gboolean warn_user)
{
	GebrGeoXmlLine * line;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
			   PL_XMLPOINTER, &line, -1);
	GtkTreeIter parent;
	gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent, iter);
	GebrGeoXmlProject * project;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent,
			   PL_XMLPOINTER, &project, -1);

	/* removes its flows */
	GebrGeoXmlSequence *line_flow;
	for (gebr_geoxml_line_get_flow(line, &line_flow, 0); line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		const gchar *flow_source;

		flow_source = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		GString *path = document_get_path(flow_source);
		g_unlink(path->str);
		g_string_free(path, TRUE);

		/* log action */
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting child Flow '%s'."), flow_source);
	}
	/* remove the line from its project */
	const gchar *line_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(line));
	if (gebr_geoxml_project_remove_line(project, line_filename)) {
		document_save(GEBR_GEOXML_DOC(project), TRUE, FALSE);
		document_delete(line_filename);
	}
	/* GUI */
	gebr_remove_help_edit_window(GEBR_GEOXML_DOCUMENT(line));
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), iter);

	/* inform the user */
	if (warn_user) {
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Deleting Line '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(line)));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting Line '%s' from project '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(line)),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(project)));
	}

	return TRUE;
}

void line_set_paths_to_relative(GebrGeoXmlLine *line, gboolean relative)
{
	GString *path = g_string_new(NULL);
	GebrGeoXmlSequence *line_path;
	gebr_geoxml_line_get_path(line, &line_path, 0);
	for (; line_path != NULL; gebr_geoxml_sequence_next(&line_path)) {
		g_string_assign(path, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path)));
		gebr_path_set_to(path, relative);
		gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(line_path), path->str);
	}
	g_string_free(path, TRUE);
}

void line_set_paths_to_empty (GebrGeoXmlLine *line)
{
	GebrGeoXmlSequence *seq;
	gebr_geoxml_line_get_path (line, &seq, 0);
	while (seq) {
		gebr_geoxml_sequence_remove (seq);
		gebr_geoxml_line_get_path (line, &seq, 0);
	}

	GebrGeoXmlSequence *line_flow;
	gebr_geoxml_line_get_flow(line, &line_flow, 0);
	for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		GebrGeoXmlFlow *flow;
	       	document_load((GebrGeoXmlDocument **)&flow, filename, TRUE);
		flow_set_paths_to_empty(flow);
	}
}

GtkTreeIter line_append_flow_iter(GebrGeoXmlFlow * flow, GebrGeoXmlLineFlow * line_flow)
{
	GtkTreeIter iter;

	/* add to the flow browser. */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gebr_geoxml_object_ref(flow);
	gebr_geoxml_object_ref(line_flow);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow)),
			   FB_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(flow)),
			   FB_LINE_FLOW_POINTER, line_flow,
			   FB_XMLPOINTER, flow,
			   -1);

	return iter;
}

void line_load_flows(void)
{
	GebrGeoXmlSequence *line_flow;
	GtkTreeIter iter;
	gboolean error = FALSE;

	flow_free();
	project_line_get_selected(&iter, DontWarnUnselection);

	/* iterate over its flows */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
	for (; line_flow; gebr_geoxml_sequence_next(&line_flow)) {
		GebrGeoXmlFlow *flow;

//		GebrGeoXmlSequence * next = line_flow;
//		gebr_geoxml_sequence_next(&next);

		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		int ret = document_load_with_parent((GebrGeoXmlDocument**)(&flow), filename, &iter, TRUE);
		if (ret) {
//			line_flow = next;
			error = TRUE;
			continue;
		}

		line_append_flow_iter(flow, GEBR_GEOXML_LINE_FLOW(line_flow));

//		line_flow = next;
	}

	if (!error)
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Flows loaded."));

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter) == TRUE)
		flow_browse_select_iter(&iter);
}

void line_move_flow_top(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_after(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}

void line_move_flow_bottom(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_before(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}
