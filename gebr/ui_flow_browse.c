/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2012 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or *(at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "ui_flow_browse.h"
#include "gebr.h"
#include "document.h"
#include "line.h"
#include "flow.h"
#include "ui_flow_execution.h"
#include "ui_help.h"
#include "callbacks.h"
#include "ui_parameters.h"
#include "menu.h"
#include "ui_flow_program.h"
#include "ui_flows_io.h"
#include "interface.h"

#define JOB_BUTTON_SIZE 120

/*
 * Prototypes
 */

static void selection_changed_signal(void);

static void flow_browse_load(void);

static gboolean flow_browse_static_info_update(void);

static void flow_browse_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
					 GtkTreeViewColumn * column, GebrUiFlowBrowse *fb);

//static void flow_browse_on_flow_move(void);

gboolean gebr_ui_flow_browse_update_speed_slider_sensitiveness(GebrUiFlowBrowse *ufb);

static void flow_browse_add_revisions_graph(GebrGeoXmlFlow *flow,
                                            GebrUiFlowBrowse *fb,
                                            gboolean keep_selection);

static void on_job_info_status_changed(GebrJob *job,
                                       GebrCommJobStatus old_status,
                                       GebrCommJobStatus new_status,
                                       const gchar *parameter,
                                       GtkWidget *container);

static void flow_browse_menu_add(void);

void gebr_flow_browse_cursor_changed(GtkTreeView *tree_view,
                                     GebrUiFlowBrowse *fb);

static void flow_browse_on_query_tooltip(GtkTreeView * treeview,
					 gint x,
					 gint y,
					 gboolean keyboard_tip,
					 GtkTooltip * tooltip,
					 GebrUiFlowBrowse *ui_flow_browse);
					 
static void remove_programs_view(GebrUiFlowBrowse *fb);

static void create_programs_view(GtkTreeIter *parent,
                                 GebrUiFlowBrowse *fb);

void flow_add_program_to_flow(GebrGeoXmlSequence *program,
                              GtkTreeIter *flow_iter,
                              gboolean never_opened);

void
gebr_flow_browse_update_server(GebrUiFlowBrowse *fb,
                               GebrMaestroServer *maestro)
{
	gboolean sensitive = maestro != NULL;

	if (maestro) {
		const gchar *type, *msg;
		gebr_maestro_server_get_error(maestro, &type, &msg);

		if (g_strcmp0(type, "error:none") != 0)
			sensitive = FALSE;
	}

	if (gebr_geoxml_line_get_flows_number(gebr.line) > 0)
		flow_browse_set_run_widgets_sensitiveness(fb, sensitive, TRUE);
	else
		flow_browse_set_run_widgets_sensitiveness(fb, FALSE, FALSE);
}

/*
 * Methods of Menu View
 */
static gboolean
menu_search_func(GtkTreeModel *model,
		 gint column,
		 const gchar *key,
		 GtkTreeIter *iter,
		 gpointer data)
{
	gchar *title, *desc;
	gchar *lt, *ld, *lk; // Lower case strings
	gboolean match;

	if (!key)
		return FALSE;

	gtk_tree_model_get(model, iter,
			   MENU_TITLE_COLUMN, &title,
			   MENU_DESC_COLUMN, &desc,
			   -1);

	lt = title ? g_utf8_strdown(title, -1) : g_strdup("");
	ld = desc ?  g_utf8_strdown(desc, -1)  : g_strdup("");
	lk = g_utf8_strdown(key, -1);

	match = gebr_utf8_strstr(lt, lk) || gebr_utf8_strstr(ld, lk);

	g_free(title);
	g_free(desc);
	g_free(lt);
	g_free(ld);
	g_free(lk);

	return !match;
}

static gboolean flow_browse_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, iter) == FALSE) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No menu selected."));
		return FALSE;
	}
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Select a menu instead of a category."));
		return FALSE;
	}

	return TRUE;
}

static void flow_browse_menu_add(void)
{
	GtkTreeIter iter, parent;
	gchar *name;
	gchar *filename;
	GebrGeoXmlFlow *menu;
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *menu_programs;
	gint menu_programs_index;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	if (!flow_browse_get_selected_menu(&iter, FALSE)) {
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter);
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view), path, FALSE);
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter,
	                   MENU_TITLE_COLUMN, &name, MENU_FILEPATH_COLUMN, &filename, -1);
	menu = menu_load_path(filename);
	if (menu == NULL)
		goto out;

	/* set parameters' values of menus' programs to default
	 * note that menu changes aren't saved to disk
	 */
	GebrGeoXmlProgramControl c1, c2;
	GebrGeoXmlProgram *first_prog = NULL;
	GtkTreeModel *model = GTK_TREE_MODEL (gebr.ui_flow_browse->store);

	gebr_geoxml_flow_get_program(menu, &program, 0);

	gboolean valid = gtk_tree_model_get_iter_first (model, &parent);
	while (valid) {
		valid = gtk_tree_model_iter_children(model, &iter, &parent);
		if (valid)
			break;
		valid = gtk_tree_model_iter_next(model, &parent);
	}
	GebrUiFlowBrowseType type;
	GebrUiFlowProgram *ui_program;
	while (valid) {
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT_TYPE, &type,
		                   -1);

		if (type != STRUCT_TYPE_PROGRAM || first_prog) {
			valid = gtk_tree_model_iter_next(model, &iter);
			continue;
		}

		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_program,
		                   -1);

		first_prog = gebr_ui_flow_program_get_xml(ui_program);
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	c1 = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));
	c2 = gebr_geoxml_program_get_control (first_prog);

	if (c1 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY && c2 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY) {
		document_free(GEBR_GEOXML_DOC(menu));
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, _("This Flow already contains a loop"));
		goto out;
	}

	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		gebr_geoxml_parameters_reset_to_default(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));

	menu_programs_index = gebr_geoxml_flow_get_programs_number(gebr.flow);
	/* add it to the file */
	gebr_geoxml_flow_add_flow(gebr.flow, menu);
	if (c1 == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		GebrGeoXmlParameter *dict_iter = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow)));
		gebr_validator_insert(gebr.validator, dict_iter, NULL, NULL);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	gebr_flow_set_toolbar_sensitive();
	flow_browse_set_run_widgets_sensitiveness(gebr.ui_flow_browse, TRUE, FALSE);

	/* and to the GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &menu_programs, menu_programs_index);
	flow_add_program_sequence_to_view(menu_programs, TRUE, TRUE);

	document_free(GEBR_GEOXML_DOC(menu));
 out:	g_free(name);
	g_free(filename);
}

static void flow_browse_menu_show_help(void)
{
	GtkTreeIter iter;
	gchar *menu_filename;
	GebrGeoXmlFlow *menu;

	if (!flow_browse_get_selected_menu(&iter, TRUE))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);

	menu = menu_load_path(menu_filename);
	if (menu == NULL)
		goto out;
	gebr_help_show(GEBR_GEOXML_OBJECT(menu), TRUE);

out:	g_free(menu_filename);
}

static GtkMenu *flow_browse_menu_popup_menu(GtkWidget * widget, GebrUiFlowBrowse *fb)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_refresh")));

	if (!flow_browse_get_selected_menu(&iter, FALSE)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), fb->menu_view);
		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), fb->menu_view);
		goto out;
	}

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_browse_menu_add), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), fb->menu_view);
	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), fb->menu_view);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	/* help */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_browse_menu_show_help), NULL);
	gchar *menu_filename;
	gtk_tree_model_get(GTK_TREE_MODEL(fb->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);
	GebrGeoXmlDocument *xml = GEBR_GEOXML_DOCUMENT(menu_load_path(menu_filename));
	gchar *tmp_help = gebr_geoxml_document_get_help(xml);
	if (xml != NULL && strlen(tmp_help) <= 1)
		gtk_widget_set_sensitive(menu_item, FALSE);
	if (xml)
		document_free(xml);
	g_free(menu_filename);
	g_free(tmp_help);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/* End of Menu's methods */


static void
on_controller_maestro_state_changed(GebrMaestroController *mc,
				    GebrMaestroServer *maestro,
				    GebrUiFlowBrowse *fb)
{
	if (!gebr.line)
		return;

	gchar *addr1 = gebr_geoxml_line_get_maestro(gebr.line);
	const gchar *addr2 = gebr_maestro_server_get_address(maestro);

	if (g_strcmp0(addr1, addr2) != 0)
		goto out;

	switch (gebr_maestro_server_get_state(maestro)) {
	case SERVER_STATE_DISCONNECTED:
		flow_browse_set_run_widgets_sensitiveness(fb, FALSE, TRUE);
		break;
	case SERVER_STATE_LOGGED:
		gebr_flow_browse_update_server(fb, maestro);
		break;
	default:
		break;
	}
	gebr_flow_set_toolbar_sensitive();

out:
	g_free(addr1);
}

/*
 * Method called when change context of Flow
 */
static void
on_context_button_toggled(GtkToggleButton *button,
                          GebrUiFlowBrowse *fb)
{
	gboolean active = gtk_toggle_button_get_active(button);

	g_signal_handlers_block_by_func(fb->properties_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_block_by_func(fb->snapshots_ctx_button, on_context_button_toggled, fb);

	if (!active) {
		active = TRUE;
		gtk_toggle_button_set_active(button, active);
	}

	if (button == fb->properties_ctx_button) {
		gtk_toggle_button_set_active(fb->snapshots_ctx_button, !active);

		gebr_flow_browse_define_context_to_show(CONTEXT_FLOW, fb);

		gebr_flow_browse_load_parameters_review(gebr.flow, fb, FALSE);
	}
	else if (button == fb->snapshots_ctx_button) {
		gtk_toggle_button_set_active(fb->properties_ctx_button, !active);

		if (!gtk_widget_get_visible(fb->context[CONTEXT_SNAPSHOTS])) {
			if (gebr_geoxml_flow_get_revisions_number(gebr.flow) > 1) {
				GString *cmd = g_string_new("unselect-all\bNone\n");
				gebr_comm_process_write_stdin_string(fb->graph_process, cmd);
				g_string_free(cmd, TRUE);
			}
		}
		gebr_flow_browse_define_context_to_show(CONTEXT_SNAPSHOTS, fb);
	}

	GList *childs = gtk_container_get_children(GTK_CONTAINER(gebr.ui_flow_browse->jobs_status_box));
	for (GList *i = childs; i; i = i->next)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->data), FALSE);


	g_signal_handlers_unblock_by_func(fb->properties_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_unblock_by_func(fb->snapshots_ctx_button, on_context_button_toggled, fb);
}

/*
 * Methods of Jobs Output Info bar
 */
static GebrJob *
get_selected_job_from_info(GebrUiFlowBrowse *fb)
{
	GebrJob *job = NULL;

	GList *childs = gtk_container_get_children(GTK_CONTAINER(fb->jobs_status_box));
	for (GList *i = childs; i; i = i->next) {
		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(i->data)))
			continue;

		GList *child = gtk_container_get_children(GTK_CONTAINER(i->data));
		g_object_get(child->data, "user-data", &job, NULL);
		g_list_free(child);

		break;
	}
	g_list_free(childs);

	return job;
}

static void
on_job_button_clicked(GtkButton *button,
                      GebrUiFlowBrowse *fb)
{
	GebrJob *job;
	job = get_selected_job_from_info(fb);
	if (!job)
		job = gebr_job_control_get_recent_job_from_flow(GEBR_GEOXML_DOCUMENT(gebr.flow), gebr.job_control);

	gebr_job_control_set_automatic_filter(gebr.job_control, TRUE);
	gebr_job_control_reset_filters(gebr.job_control);
	gebr_job_control_apply_flow_filter(gebr.flow, gebr.job_control);
	gebr_job_control_select_job(gebr.job_control, job);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), NOTEBOOK_PAGE_JOB_CONTROL);
}

static void
on_output_job_clicked(GtkToggleButton *button,
                      GebrJob *job)
{
	gboolean active = gtk_toggle_button_get_active(button);

	if (gtk_toggle_button_get_inconsistent(button)) {
		gtk_toggle_button_set_inconsistent(button, FALSE);
		return;
	}

	gboolean has_output = FALSE;

	if (active) {
		GList *childs = gtk_container_get_children(GTK_CONTAINER(gebr.ui_flow_browse->jobs_status_box));
		for (GList *i = childs; i; i = i->next) {
			GtkToggleButton *tb = GTK_TOGGLE_BUTTON(i->data);
			if (button != tb && gtk_toggle_button_get_active(tb)) {
				gtk_toggle_button_set_inconsistent(tb, TRUE);
				gtk_toggle_button_set_active(tb, FALSE);
			}
		}

		if(job) {
			has_output = TRUE;
			gebr_flow_browse_define_context_to_show(CONTEXT_JOBS, gebr.ui_flow_browse);
			gebr_job_control_select_job(gebr.job_control, job);
		}
	}

	if (!has_output) {
		if (gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
			gebr_flow_browse_define_context_to_show(CONTEXT_FLOW, gebr.ui_flow_browse);
		else
			gebr_flow_browse_define_context_to_show(CONTEXT_SNAPSHOTS, gebr.ui_flow_browse);
	}
}

static void
on_dismiss_clicked(GtkButton *dismiss,
                   GebrUiFlowBrowse *fb)
{
	GList *childs = gtk_container_get_children(GTK_CONTAINER(fb->jobs_status_box));
	for (GList *i = childs; i; i = i->next) {
		GList *child = gtk_container_get_children(GTK_CONTAINER(i->data));

		GebrJob *job;
		g_object_get(child->data, "user-data", &job, NULL);
		g_signal_handlers_disconnect_by_func(job, on_job_info_status_changed, child->data);
		g_signal_handlers_disconnect_by_func(i->data, on_output_job_clicked, job);
		gtk_container_remove(GTK_CONTAINER(fb->jobs_status_box), GTK_WIDGET(i->data));

		g_list_free(child);
	}
	g_list_free(childs);

	if (dismiss) {
		gebr_flow_browse_reset_jobs_from_flow(gebr.flow, fb);
		if (gtk_widget_get_visible(fb->context[CONTEXT_JOBS])) {
			if (gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
				gebr_flow_browse_define_context_to_show(CONTEXT_FLOW, fb);
			else
				gebr_flow_browse_define_context_to_show(CONTEXT_SNAPSHOTS, fb);
		}
	}

	/* Hide Info bar*/
	gtk_widget_hide(fb->info_jobs);
}

void
gebr_flow_browse_info_job(GebrUiFlowBrowse *fb,
                          const gchar *job_id)
{
	GebrJob *job = gebr_job_control_find(gebr.job_control, job_id);

	if (!job)
		return;

	const gchar *title = gebr_job_get_description(job);

	GtkWidget *job_box = gtk_hbox_new(FALSE, 5);

	GtkWidget *img = gtk_image_new();
	GtkWidget *label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);

	gtk_box_pack_start(GTK_BOX(job_box), img, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(job_box), label, TRUE, TRUE, 5);

	GtkWidget *output_button = gtk_toggle_button_new();
	gtk_widget_set_size_request(output_button, JOB_BUTTON_SIZE, -1);
	gtk_button_set_relief(GTK_BUTTON(output_button), GTK_RELIEF_HALF);
	gtk_button_set_focus_on_click(GTK_BUTTON(output_button), FALSE);

	gtk_container_add(GTK_CONTAINER(output_button), job_box);
	g_signal_connect(output_button, "toggled", G_CALLBACK(on_output_job_clicked), job);

	/* Update status */
	GebrCommJobStatus status = gebr_job_get_status(job);
	on_job_info_status_changed(job, 0, status, NULL, job_box);

	if (status != JOB_STATUS_FINISHED ||
	    status != JOB_STATUS_CANCELED ||
	    status != JOB_STATUS_FAILED ) {
		g_signal_connect(job, "status-change", G_CALLBACK(on_job_info_status_changed), job_box);
		g_object_set(job_box, "user-data", job, NULL);
	}
	gtk_box_pack_start(GTK_BOX(fb->jobs_status_box), output_button, FALSE, FALSE, 0);

	gtk_widget_show_all(fb->jobs_status_box);

	if (!gtk_widget_get_visible(fb->info_jobs))
		gtk_widget_show_all(fb->info_jobs);
}

void
gebr_flow_browse_select_job_output(const gchar *job_id,
                                   GebrUiFlowBrowse *fb)
{
	GList *childs = gtk_container_get_children(GTK_CONTAINER(fb->jobs_status_box));
	for (GList *i = childs; i; i = i->next) {
		GList *child = gtk_container_get_children(GTK_CONTAINER(i->data));
		GebrJob *job;
		g_object_get(child->data, "user-data", &job, NULL);

		g_list_free(child);

		const gchar *id = gebr_job_get_id(job);
		if (!g_strcmp0(id, job_id)) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->data), TRUE);
			break;
		}
	}
	g_list_free(childs);
}

void
gebr_flow_browse_select_job(GebrUiFlowBrowse *fb)
{
	flow_browse_static_info_update();
}
/* End of Jobs Output */

gint
gebr_flow_browse_calculate_n_max(GebrUiFlowBrowse *fb)
{
	GtkAllocation alloc;
	gtk_widget_get_allocation(fb->info_window, &alloc);

	return (alloc.width - 100)/JOB_BUTTON_SIZE;
}

void
on_size_request(GtkWidget      *widget,
                GtkAllocation *allocation,
                GebrUiFlowBrowse *fb)
{
	if (!gebr.flow)
		return;

	if (!fb->jobs_status_box)
		return;

	if (allocation->width == fb->last_info_width)
		return;

	fb->last_info_width = allocation->width;

	gint n_jobs = gebr_flow_browse_calculate_n_max(fb);

	// Reselect last job
	GebrJob *job = get_selected_job_from_info(fb);

	gebr_flow_browse_update_jobs_info(gebr.flow, fb, n_jobs);

	if (job)
		gebr_flow_browse_select_job_output(gebr_job_get_id(job), fb);
}

static void
flow_browse_component_editing_started(GtkCellRenderer *renderer,
                                      GtkCellEditable *editable,
                                      gchar *path,
                                      GebrUiFlowBrowse *fb)
{
	GtkTreeModel *model = GTK_TREE_MODEL(fb->store);
	GtkTreeIter iter;

	GebrUiFlowsIo *io;

	gtk_tree_model_get_iter_from_string(model, &iter, path);
	gtk_tree_model_get(model, &iter,
	                   FB_STRUCT, &io,
	                   -1);

	g_object_set_data(G_OBJECT(editable), "path", g_strdup(path));

	gebr_ui_flows_io_start_editing(io, GTK_ENTRY(editable));
}

static void
flow_browse_component_editing_canceled(GtkCellRenderer *renderer,
                                       GebrUiFlowBrowse *fb)
{
	return;
}

static void
flow_browse_component_edited(GtkCellRendererText *renderer,
                              gchar *strpath,
                              gchar *new_text,
                              GebrUiFlowBrowse *fb)
{
	GtkTreeModel *model = GTK_TREE_MODEL(fb->store);
	GtkTreeIter iter;

	GebrUiFlowsIo *io;

	gtk_tree_model_get_iter_from_string(model, &iter, strpath);
	gtk_tree_model_get(model, &iter,
	                   FB_STRUCT, &io,
	                   -1);

	gebr_ui_flows_io_edited(io, new_text);

	GtkTreePath *path = gtk_tree_path_new_from_string(strpath);
	gtk_tree_model_row_changed(model, path, &iter);
	gtk_tree_path_free(path);

	gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse, TRUE);
	flow_browse_info_update();
}

static GtkMenu *
flow_browse_popup_menu(GtkWidget *widget,
                       GebrUiFlowBrowse *fb)
{
	GtkMenu *menu = NULL;
	GtkTreeModel *model = GTK_TREE_MODEL(fb->store);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, NULL);

	if (!rows)
		return NULL;

	GtkTreeIter iter;
	if (g_list_length(rows) > 1)
		flow_browse_single_selection();

	GebrUiFlowBrowseType type;
	gtk_tree_model_get_iter(model, &iter, rows->data);
	gtk_tree_model_get(model, &iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	if (type == STRUCT_TYPE_FLOW) {
		GebrUiFlow *ui_flow;

		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);
		menu = gebr_ui_flow_popup_menu(ui_flow);
	}
	else if (type == STRUCT_TYPE_IO) {
		GebrUiFlowsIo *ui_io;

		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_io,
		                   -1);

		menu = gebr_ui_flows_io_popup_menu(ui_io, gebr.flow);

		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free(path);
	}
	else if (type == STRUCT_TYPE_PROGRAM) {
		GebrUiFlowProgram *ui_program;

		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_program,
		                   -1);
		menu = gebr_ui_flow_program_popup_menu(ui_program);
	}
	return menu;
}

void flow_browse_program_check_sensitiveness(void)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlProgram *first_program = NULL;
	GebrGeoXmlProgram *last_program = NULL;
	gboolean has_some_error_output = FALSE;
	gboolean has_configured = FALSE;

	gebr_geoxml_flow_get_program(gebr.flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {

		GebrGeoXmlProgramControl control = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));

		if (control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
			continue;

		if (gebr_geoxml_program_get_status (GEBR_GEOXML_PROGRAM(program)) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			if (!has_configured) {
				first_program = GEBR_GEOXML_PROGRAM(program);
				gebr_geoxml_object_ref(first_program);
				has_configured = TRUE;
			}
			if (!has_some_error_output && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))){
				has_some_error_output = TRUE;
			}

			if (last_program)
				gebr_geoxml_object_unref(last_program);

			last_program = GEBR_GEOXML_PROGRAM(program);
			gebr_geoxml_object_ref(last_program);
		}
	}

	gboolean find_io = FALSE;
	GtkTreeIter iter, parent;
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_flow_browse->store);
	gboolean valid = gtk_tree_model_get_iter_first(model, &parent);
	while (valid && !find_io) {
		valid = gtk_tree_model_iter_children(model, &iter, &parent);
		while (valid) {
			GebrUiFlowBrowseType type;
			GebrUiFlowsIo *ui_io;

			gtk_tree_model_get(model, &iter,
			                   FB_STRUCT_TYPE, &type,
			                   FB_STRUCT, &ui_io,
			                   -1);

			if (type != STRUCT_TYPE_IO) {
				valid = gtk_tree_model_iter_next(model, &iter);
				continue;
			}

			find_io = TRUE;

			GebrUiFlowsIoType io_type = gebr_ui_flows_io_get_io_type(ui_io);
			if (io_type == GEBR_IO_TYPE_INPUT)
				gebr_ui_flows_io_set_active(ui_io, gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(first_program)));

			else if (io_type == GEBR_IO_TYPE_OUTPUT)
				gebr_ui_flows_io_set_active(ui_io, gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(last_program)));

			else if (has_some_error_output)
				gebr_ui_flows_io_set_active(ui_io, TRUE);

			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_model_row_changed(model, path, &iter);
			gtk_tree_path_free(path);

			valid = gtk_tree_model_iter_next(model, &iter);
		}
		valid = gtk_tree_model_iter_next(model, &parent);
	}

	if (has_configured)
		gebr_geoxml_object_unref(first_program);
	gebr_geoxml_object_unref(last_program);
}


void
flow_browse_revalidate_programs(GebrUiFlowBrowse *fb)
{
	GtkTreeIter parent, iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	GebrGeoXmlProgram *program;
	GebrUiFlowBrowseType type;
	GebrUiFlowProgram *ui_program;

	model = GTK_TREE_MODEL(fb->store);

	gboolean find_programs = FALSE;
	gboolean valid = gtk_tree_model_get_iter_first(model, &parent);
	while (valid && !find_programs) {
		valid = gtk_tree_model_iter_children(model, &iter, &parent);

		while (valid) {
			gtk_tree_model_get(model, &iter,
					   FB_STRUCT_TYPE, &type,
					   FB_STRUCT, &ui_program,
					   -1);

			if (type != STRUCT_TYPE_PROGRAM)
				goto next;

			find_programs = TRUE;

			program = gebr_ui_flow_program_get_xml(ui_program);

			if (!program)
				goto next;

			if (gebr_geoxml_program_get_status(program) == GEBR_GEOXML_PROGRAM_STATUS_DISABLED)
				goto next;

			if(gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
				if (validate_program_iter(&iter, NULL))
					gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
				else
					gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);

				goto update;
			}

			if (validate_program_iter(&iter, NULL))
				flow_browse_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED, &iter, fb);
			else
				flow_browse_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED, &iter, fb);

		update:
			/* update icon on interface */
			path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_model_row_changed(model, path, &iter);
			gtk_tree_path_free(path);
		next:
			valid = gtk_tree_model_iter_next(model, &iter);
		}

		if (find_programs) {
			path = gtk_tree_model_get_path(model, &parent);
			gtk_tree_model_row_changed(model, path, &parent);
			gtk_tree_path_free(path);
		}
		valid = gtk_tree_model_iter_next(model, &parent);
	}
}


void
flow_browse_change_iter_status(GebrGeoXmlProgramStatus status,
                               GtkTreeIter *iter,
                               GebrUiFlowBrowse *fb)
{
	GtkTreeModel *model;
	model = GTK_TREE_MODEL (fb->store);

	GebrUiFlowBrowseType type;

	gtk_tree_model_get(model, iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	if (type != STRUCT_TYPE_PROGRAM)
		return;


	gboolean has_error;
	GebrGeoXmlProgram *program;
	gboolean never_opened;

	GebrUiFlowProgram *ui_program;
	gtk_tree_model_get(model, iter,
	                   FB_STRUCT, &ui_program,
	                   -1);

	never_opened = gebr_ui_flow_program_get_flag_opened(ui_program);
	program = gebr_ui_flow_program_get_xml(ui_program);

	gboolean is_control = (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR);
	if (gebr_geoxml_program_get_status(program) == status) {
		// If program is control, it may invalidate other programs but not itself
		if (is_control)
			flow_browse_revalidate_programs(fb);
		return;
	}

	if (never_opened) {
		gebr_ui_flow_program_set_flag_opened(ui_program, FALSE);
		validate_program_iter(iter, NULL);
	}
	has_error = gebr_geoxml_program_get_error_id(program, NULL);

	if (has_error && status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
		status = GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED;

	gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), status);

	if (is_control) {
		GebrGeoXmlSequence *parameter;

		if (status == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) {
			parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
			gebr_validator_remove(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
		} else {
			gebr_geoxml_flow_insert_iter_dict(gebr.flow);
			parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
			gebr_validator_insert(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
		}
		flow_browse_revalidate_programs(fb);
	}

	/* Update interface */
	GtkTreePath *path = gtk_tree_model_get_path(model, iter);
	gtk_tree_model_row_changed(model, path, iter);
	gtk_tree_path_free(path);
}

static gboolean
toggle_selected_program_status(GebrUiFlowBrowse *fb)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb->view));
	GList *paths = gtk_tree_selection_get_selected_rows(selection, NULL);

	if (!paths)
		return FALSE;

	g_list_foreach(paths, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(paths);

	flow_browse_toggle_selected_program_status(fb);
	return TRUE;
}

static gboolean
delete_selected_program(GebrUiFlowBrowse *fb)
{
	GtkTreeIter iter;

	if (!flow_browse_get_selected(&iter, FALSE))
		return FALSE;

	GebrUiFlowBrowseType type;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(fb->view));

	gtk_tree_model_get(model, &iter, 0, &type, -1);

	GtkAction *action;

	switch (type) {
	case STRUCT_TYPE_FLOW:
		action = gtk_action_group_get_action(gebr.action_group_flow, "flow_delete");
		break;
	case STRUCT_TYPE_PROGRAM:
		action = gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_delete");
		break;
	case STRUCT_TYPE_IO:
	case STRUCT_TYPE_COLUMN:
		return FALSE;
	}

	gtk_action_activate(action);

	return TRUE;
}

static gboolean
flow_browse_component_key_pressed(GtkWidget *view, GdkEventKey *key, GebrUiFlowBrowse *fb)
{
	if (key->keyval == GDK_space)
		return toggle_selected_program_status(fb);

	if (key->keyval == GDK_Delete)
		return delete_selected_program(fb);

	return FALSE;
}
	
void
flow_browse_toggle_selected_program_status(GebrUiFlowBrowse *fb)
{
	GList			* listiter;
	GList			* paths;
	GebrGeoXmlProgram	* program;
	GebrGeoXmlProgramStatus	  status;
	GtkTreeIter		  iter;
	GtkTreeModel		* model;
	GtkTreeSelection	* selection;
	gboolean		  has_disabled = FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fb->view));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	if (!paths)
		return;

	listiter = paths;
	do {
		GebrUiFlowBrowseType type;
		GebrUiFlowProgram *ui_program;
		gboolean has_error;

		gtk_tree_model_get_iter (model, &iter, listiter->data);
		gtk_tree_model_get (model, &iter,
				    FB_STRUCT_TYPE, &type,
				    FB_STRUCT, &ui_program,
				    -1);

		if (type != STRUCT_TYPE_PROGRAM) {
			listiter = listiter->next;
			continue;
		}

		program = gebr_ui_flow_program_get_xml(ui_program);

		status = gebr_geoxml_program_get_status (program);
		has_error = gebr_geoxml_program_get_error_id(program, NULL);

		if ((status == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) ||
		    (status == GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED && !has_error))
		{
			has_disabled = TRUE;
			break;
		}

		listiter = listiter->next;
	} while (listiter);

	if (has_disabled) {
		status = GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED;
	} else {
		status = GEBR_GEOXML_PROGRAM_STATUS_DISABLED;
	}

	listiter = paths;
	while (listiter) {
		GebrUiFlowBrowseType type;
		gtk_tree_model_get_iter(model, &iter, listiter->data);
		gtk_tree_model_get (model, &iter,
		                    FB_STRUCT_TYPE, &type,
		                    -1);

		if (type != STRUCT_TYPE_PROGRAM) {
			listiter = listiter->next;
			continue;
		}

		flow_browse_change_iter_status(status, &iter, fb);
		listiter = listiter->next;
	}

	flow_browse_program_check_sensitiveness();
	flow_browse_revalidate_programs(fb);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	flow_browse_validate_io(fb);
	flow_browse_static_info_update();
	gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse, TRUE);
}

static gboolean
flow_browse_may_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			 GtkTreeViewDropPosition drop_position, GebrUiFlowBrowse *fb)
{
	GtkTreeModel *model;
	GebrGeoXmlProgram *program;
	GebrGeoXmlProgramControl con;

	model = gtk_tree_view_get_model (tree_view);

	gpointer ui_struct;
	GebrUiFlowBrowseType type;

	gtk_tree_model_get(model, iter,
	                   FB_STRUCT_TYPE, &type,
	                   FB_STRUCT, &ui_struct,
	                   -1);

	gpointer ui_struct_dest;
	GebrUiFlowBrowseType type_dest;

	gtk_tree_model_get(model, position,
	                   FB_STRUCT_TYPE, &type_dest,
	                   FB_STRUCT, &ui_struct_dest,
	                   -1);

	if (type == STRUCT_TYPE_FLOW &&
	    type_dest == STRUCT_TYPE_FLOW)
		return TRUE;

	if (type == STRUCT_TYPE_FLOW &&
	    type_dest != STRUCT_TYPE_FLOW)
		return FALSE;

	if (type == STRUCT_TYPE_PROGRAM &&
	    type_dest == STRUCT_TYPE_FLOW)
		return TRUE;

	if (type == STRUCT_TYPE_IO)
		return FALSE;

	if (type_dest == STRUCT_TYPE_IO) {
		if (drop_position != GTK_TREE_VIEW_DROP_AFTER &&
		    gebr_ui_flows_io_get_io_type(GEBR_UI_FLOWS_IO(ui_struct_dest)) == GEBR_IO_TYPE_INPUT)
			return FALSE;

		if (drop_position != GTK_TREE_VIEW_DROP_BEFORE &&
		    gebr_ui_flows_io_get_io_type(GEBR_UI_FLOWS_IO(ui_struct_dest)) == GEBR_IO_TYPE_OUTPUT)
			return FALSE;

		if (gebr_ui_flows_io_get_io_type(GEBR_UI_FLOWS_IO(ui_struct_dest)) == GEBR_IO_TYPE_ERROR)
			return FALSE;
	}

	if (!gtk_tree_store_iter_is_valid(GTK_TREE_STORE(model), iter))
		return FALSE;

	if (type == STRUCT_TYPE_PROGRAM) {
		program = gebr_ui_flow_program_get_xml(GEBR_UI_FLOW_PROGRAM(ui_struct));
		con = gebr_geoxml_program_get_control (program);
		if (con != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
			return FALSE;
	}

	/* Check if the target iter is a control program */
	if (type_dest == STRUCT_TYPE_PROGRAM) {
		program = gebr_ui_flow_program_get_xml(GEBR_UI_FLOW_PROGRAM(ui_struct_dest));
		con = gebr_geoxml_program_get_control (program);
		if (con != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
			return FALSE;
	}

	return TRUE;
}

/**
 * \internal
 */
static gboolean
flow_browse_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
		     GtkTreeViewDropPosition drop_position, GebrUiFlowBrowse *fb)
{
	gpointer ui_struct, ui_struct_dest;
	GebrUiFlowBrowseType type, type_dest;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	gtk_tree_model_get(model, iter,
	                   FB_STRUCT_TYPE, &type,
	                   FB_STRUCT, &ui_struct,
	                   -1);

	gtk_tree_model_get(model, position,
	                   FB_STRUCT_TYPE, &type_dest,
	                   FB_STRUCT, &ui_struct_dest,
	                   -1);

	if (type != STRUCT_TYPE_FLOW && type_dest != STRUCT_TYPE_FLOW) {
		GebrGeoXmlSequence *program;
		GebrGeoXmlSequence *position_program;


		program = GEBR_GEOXML_SEQUENCE(gebr_ui_flow_program_get_xml(GEBR_UI_FLOW_PROGRAM(ui_struct)));
		if (type_dest != STRUCT_TYPE_IO)
			position_program = GEBR_GEOXML_SEQUENCE(gebr_ui_flow_program_get_xml(GEBR_UI_FLOW_PROGRAM(ui_struct_dest)));
		else
			position_program = NULL;

		if (drop_position != GTK_TREE_VIEW_DROP_AFTER) {
			gebr_geoxml_sequence_move_before(program, position_program);
			gtk_tree_store_move_before(fb->store, iter, position);
		} else {
			gebr_geoxml_sequence_move_after(program, position_program);
			gtk_tree_store_move_after(fb->store, iter, position);
		}
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

		flow_browse_revalidate_programs(fb);
		flow_browse_program_check_sensitiveness();
		flow_browse_static_info_update();
		gebr_flow_browse_load_parameters_review(gebr.flow, fb, TRUE);
	}
	else if (type == STRUCT_TYPE_FLOW && type_dest == STRUCT_TYPE_FLOW) {
		GebrGeoXmlSequence *flow;
		GebrGeoXmlSequence *position_flow;

		flow = GEBR_GEOXML_SEQUENCE(gebr_ui_flow_get_line_flow(GEBR_UI_FLOW(ui_struct)));
		position_flow = GEBR_GEOXML_SEQUENCE(gebr_ui_flow_get_line_flow(GEBR_UI_FLOW(ui_struct_dest)));

		remove_programs_view(fb);

		if (drop_position != GTK_TREE_VIEW_DROP_AFTER) {
			gebr_geoxml_sequence_move_before(flow, position_flow);
			gtk_tree_store_move_before(fb->store, iter, position);
		} else {
			gebr_geoxml_sequence_move_after(flow, position_flow);
			gtk_tree_store_move_after(fb->store, iter, position);
		}
		document_save(GEBR_GEOXML_DOCUMENT(gebr.line), TRUE, TRUE);
		flow_browse_reload_selected();
	}
	else if (type == STRUCT_TYPE_PROGRAM && type_dest == STRUCT_TYPE_FLOW) {

		flow_program_copy();
		flow_program_remove();

		remove_programs_view(fb);

		GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view);
		gtk_tree_selection_select_iter(selection, position);
		flow_program_paste();
	}
	return FALSE;
}

static void
on_line_back_clicked(GtkButton *button,
                     GebrUiFlowBrowse *fb)
{
	gebr_interface_change_tab(NOTEBOOK_PAGE_PROJECT_LINE);
}

GebrUiFlowBrowse *flow_browse_setup_ui()
{
	GebrUiFlowBrowse *ui_flow_browse;

	GtkTreeViewColumn *col;

	GtkWidget *page;
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *infopage;

	/* alloc */
	ui_flow_browse = g_new(GebrUiFlowBrowse, 1);

	ui_flow_browse->graph_process = NULL;
	ui_flow_browse->select_flows = NULL;
	ui_flow_browse->last_info_width = 0;
	ui_flow_browse->program_edit = NULL;

	g_signal_connect_after(gebr.maestro_controller, "maestro-state-changed",
	                       G_CALLBACK(on_controller_maestro_state_changed), ui_flow_browse);

	/*
	 * Create flow browse page
	 */
	page = gtk_vbox_new(FALSE, 0);
	ui_flow_browse->widget = page;
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(page), hpanel);

	/*
	 * Left side: flow list
	 */

	/* View with flows and programs */
	GtkWidget* left_side = gtk_vbox_new(FALSE, 0);
	GtkWidget *frame = gtk_frame_new(NULL);
	GtkWidget *button = gtk_button_new();
	GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
	GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);
	ui_flow_browse->flows_line_label = gtk_label_new("None");
	ui_flow_browse->left_panel = left_side;
	gtk_box_pack_start(GTK_BOX(hbox), ui_flow_browse->flows_line_label, TRUE, TRUE, 5);
	gtk_container_add(GTK_CONTAINER(button), hbox);
	gtk_frame_set_label_widget(GTK_FRAME(frame), button);
	g_signal_connect(button, "clicked", G_CALLBACK(on_line_back_clicked), ui_flow_browse);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(scrolled_window, 250, -1);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	gtk_box_pack_start(GTK_BOX(left_side), frame, TRUE, TRUE, 5);

	gtk_paned_pack1(GTK_PANED(hpanel), left_side, FALSE, FALSE);

	ui_flow_browse->store = gtk_tree_store_new(FB_N_COLUMN,
						   G_TYPE_INT,		/* Type */
						   G_TYPE_POINTER);	/* Struct */

	ui_flow_browse->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_browse->store));

	g_object_set(G_OBJECT(ui_flow_browse->view), "has-tooltip", TRUE, NULL);
	gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(ui_flow_browse->view), 20);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ui_flow_browse->view), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_browse->view);
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(ui_flow_browse->view), FALSE);

	GtkStyle *style = gtk_rc_get_style(gebr.notebook);
	gtk_widget_modify_base(ui_flow_browse->view, GTK_STATE_NORMAL, &(style->bg[GTK_STATE_NORMAL]));

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view)),
				    GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_flow_browse->view), FALSE);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_browse->view),
						  (GebrGuiGtkPopupCallback) flow_browse_popup_menu, ui_flow_browse);

	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(ui_flow_browse->view),
	                                            (GebrGuiGtkTreeViewReorderCallback) flow_browse_reorder,
	                                            (GebrGuiGtkTreeViewReorderCallback) flow_browse_may_reorder,
	                                            ui_flow_browse);

	g_signal_connect(ui_flow_browse->view, "key-press-event",
	                 G_CALLBACK(flow_browse_component_key_pressed), ui_flow_browse);
	g_signal_connect(ui_flow_browse->view, "row-activated", G_CALLBACK(flow_browse_on_row_activated),
			 ui_flow_browse);
	g_signal_connect(ui_flow_browse->view, "query-tooltip", G_CALLBACK(flow_browse_on_query_tooltip),
			 ui_flow_browse);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view));
	g_signal_connect(selection, "changed", G_CALLBACK(selection_changed_signal), NULL);

	/* Icon/Text column */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);

	/* Icon Renderer */
	ui_flow_browse->icon_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, ui_flow_browse->icon_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, ui_flow_browse->icon_renderer, gebr_flow_browse_status_icon, NULL, NULL);

	/* Text Renderer */
	ui_flow_browse->text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, ui_flow_browse->text_renderer, TRUE);
	g_object_set(ui_flow_browse->text_renderer,
	             "ellipsize-set", TRUE,
	             "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_set_cell_data_func(col, ui_flow_browse->text_renderer, gebr_flow_browse_text, NULL, NULL);

	g_signal_connect(ui_flow_browse->text_renderer, "edited", G_CALLBACK(flow_browse_component_edited), ui_flow_browse);
	g_signal_connect(ui_flow_browse->text_renderer, "editing-started", G_CALLBACK(flow_browse_component_editing_started), ui_flow_browse);
	g_signal_connect(ui_flow_browse->text_renderer, "editing-canceled", G_CALLBACK(flow_browse_component_editing_canceled), ui_flow_browse);

	/* Action Icon column */
	ui_flow_browse->action_renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", ui_flow_browse->action_renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_set_expand(col, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, ui_flow_browse->action_renderer, gebr_flow_browse_action_icon, NULL, NULL);

	g_signal_connect(ui_flow_browse->view, "cursor-changed",
	                 G_CALLBACK(gebr_flow_browse_cursor_changed), ui_flow_browse);

	/*
	 * Right side: flow info tab
	 */
	/* Get glade file */
	ui_flow_browse->info.builder_flow = gtk_builder_new();
	gtk_builder_add_from_file(ui_flow_browse->info.builder_flow, GEBR_GLADE_DIR "/flow-properties.glade", NULL);

	infopage = gtk_vbox_new(FALSE, 0);
	GtkWidget *infopage_flow = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "main"));
	ui_flow_browse->info_window = infopage_flow;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED && !gebr.line) {
		ui_flow_browse->warn_window = gtk_label_new(_("No Line is selected\n"));
	} else
		ui_flow_browse->warn_window = gtk_label_new(_("The Maestro of this Line is disconnected,\nthen you cannot edit flows.\n"
							      "Try changing its maestro or connecting it."));

	gtk_widget_set_sensitive(ui_flow_browse->warn_window, FALSE);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->warn_window, TRUE, TRUE, 0);

	gtk_paned_pack2(GTK_PANED(hpanel), infopage, TRUE, FALSE);

	/* Flow Icon Status */
	ui_flow_browse->info.status = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_status"));

	/* Description */
	ui_flow_browse->info.description = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_description"));

	/* Dates */
	ui_flow_browse->info.modified = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_modified"));

	/* Last execution */
	ui_flow_browse->info.lastrun = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_jobs_label"));

	/*
	 * Set button and box context
	 */
	ui_flow_browse->properties_ctx_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "context_button_flow"));
	ui_flow_browse->context[CONTEXT_FLOW] = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "properties_scroll"));
	g_signal_connect(ui_flow_browse->properties_ctx_button, "toggled", G_CALLBACK(on_context_button_toggled), ui_flow_browse);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ui_flow_browse->properties_ctx_button), _("Review of parameters"));

	ui_flow_browse->context[CONTEXT_PARAMETERS] = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "parameters_ctx_box"));

	ui_flow_browse->snapshots_ctx_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "context_button_snaps"));
	ui_flow_browse->context[CONTEXT_SNAPSHOTS] = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "snapshots_box"));
	g_signal_connect(ui_flow_browse->snapshots_ctx_button, "toggled", G_CALLBACK(on_context_button_toggled), ui_flow_browse);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ui_flow_browse->snapshots_ctx_button), "Snapshots");

	ui_flow_browse->context[CONTEXT_JOBS] = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "jobs_output_box"));

	/* Info Bar for Jobs */
	ui_flow_browse->info_jobs = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "info_jobs_box"));
	ui_flow_browse->jobs_status_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "job_status_box"));

	g_signal_connect(ui_flow_browse->info_window, "size-allocate", G_CALLBACK(on_size_request), ui_flow_browse);

	GtkButton *dismiss_button = GTK_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "dismiss_button"));
	g_signal_connect(dismiss_button, "clicked", G_CALLBACK(on_dismiss_clicked), ui_flow_browse);

	GtkButton *job_control_button = GTK_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "job_control_button"));
	g_signal_connect(job_control_button, "clicked", G_CALLBACK(on_job_button_clicked), ui_flow_browse);

	gtk_widget_hide(ui_flow_browse->info_jobs);

	/*
	 * Review of parameters Context
	 */
	ui_flow_browse->html_parameters = gebr_gui_html_viewer_widget_new();

	GtkWidget *properties_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "properties_box"));
	gtk_container_add(GTK_CONTAINER(properties_box), GTK_WIDGET(ui_flow_browse->html_parameters));

	gtk_widget_show_all(ui_flow_browse->html_parameters);

	/*
	 * Snapshots Context
	 */
	GtkWidget *rev_main = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "main_rev"));

	ui_flow_browse->revpage_main = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revisions_main"));

	ui_flow_browse->revpage_warn = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revisions_warn"));

	ui_flow_browse->revpage_warn_label = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revpage_warn_label"));

	gtk_widget_show(ui_flow_browse->revpage_main);
	gtk_widget_hide(ui_flow_browse->revpage_warn);

	gtk_container_add(GTK_CONTAINER(ui_flow_browse->context[CONTEXT_SNAPSHOTS]), rev_main);

	/*
	 * Jobs Context
	 */
	GtkWidget *output_view = gebr_job_control_get_output_view(gebr.job_control);
	gtk_widget_reparent(output_view, ui_flow_browse->context[CONTEXT_JOBS]);

	/*
	 * Menu list context
	 */
	ui_flow_browse->context[CONTEXT_MENU] = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "menu_box"));

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(ui_flow_browse->context[CONTEXT_MENU]), scrolled_window);

	ui_flow_browse->menu_store = gtk_tree_store_new(MENU_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ui_flow_browse->menu_store), MENU_TITLE_COLUMN, GTK_SORT_ASCENDING);

	ui_flow_browse->menu_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_browse->menu_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_browse->menu_view);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_browse->menu_view),
	                                          (GebrGuiGtkPopupCallback) flow_browse_menu_popup_menu,
	                                          ui_flow_browse);
	g_signal_connect(ui_flow_browse->menu_view, "key-press-event",
	                 G_CALLBACK(flow_browse_component_key_pressed), ui_flow_browse);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->menu_view), "row-activated",
	                 G_CALLBACK(flow_browse_menu_add), ui_flow_browse);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(ui_flow_browse->menu_view), MENU_TITLE_COLUMN);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(ui_flow_browse->menu_view),
	                                    menu_search_func, NULL, NULL);

	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_DESC_COLUMN);

	/*
	 * Add Flow Page on GêBR window
	 */
	gtk_box_pack_start(GTK_BOX(infopage), infopage_flow, TRUE, TRUE, 0);

	/* Create Hash Table */
	ui_flow_browse->flow_jobs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return ui_flow_browse;
}

/*
 * Methods of Snapshots and external graph on Python
 */

static void
gebr_flow_browse_snapshot_delete(const gchar *rev_id)
{
	gboolean response;

	gchar **snaps = g_strsplit(rev_id, ",", -1);

	gdk_threads_enter();
	if (!snaps[1])
		response = gebr_gui_confirm_action_dialog(_("Remove this snapshot permanently?"),
		                                          _("If you choose to remove this snapshot "
							    "you will not be able to recover it later."));
	else
		response = gebr_gui_confirm_action_dialog(_("Remove snapshots permanently?"),
		                                          _("If you choose to remove these snapshots "
							    "you will not be able to recover them later."));
	gdk_threads_leave();

	if (response) {
		for (gint i = 0; snaps[i]; i++) {
			GebrGeoXmlRevision *revision;

			revision = gebr_geoxml_flow_get_revision_by_id(gebr.flow, snaps[i]);

			if (!revision)
				continue;

			gchar *id;
			gchar *flow_xml;
			GebrGeoXmlDocument *revdoc;

			gebr_geoxml_flow_get_revision_data(revision, &flow_xml, NULL, NULL, &id);

			if (gebr_geoxml_document_load_buffer(&revdoc, flow_xml) != GEBR_GEOXML_RETV_SUCCESS) {
				g_free(flow_xml);
				g_free(id);
				return;
			}
			g_free(flow_xml);

			gchar *parent_id = gebr_geoxml_document_get_parent_id(revdoc);
			gchar *head_parent = gebr_geoxml_document_get_parent_id(GEBR_GEOXML_DOCUMENT(gebr.flow));

			gebr_geoxml_document_free(revdoc);

			GHashTable *hash_rev = gebr_flow_revisions_hash_create(gebr.flow);

			gboolean change_head_parent = flow_revision_remove(gebr.flow, id, head_parent, hash_rev);

			if (change_head_parent)
				gebr_geoxml_document_set_parent_id(GEBR_GEOXML_DOCUMENT(gebr.flow), parent_id);

			document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);
			gebr_flow_revisions_hash_free(hash_rev);
		}

		flow_browse_load();
		gtk_toggle_button_set_active(gebr.ui_flow_browse->snapshots_ctx_button, TRUE);
	}

}

void
on_snapshot_save_clicked (GtkWidget *widget,
			  GtkWidget *dialog)
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
}

void
on_snapshot_discard_changes_clicked(GtkWidget *widget,
				    GtkWidget *dialog)
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
}

void
on_snapshot_cancel(GtkWidget *widget,
		   GtkWidget *dialog)
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
}

void
on_snapshot_help_button_clicked(GtkWidget *widget,
		   GtkWidget *dialog)
{
	const gchar *section = "flows_browser_save_state_flow";
	gchar *error;

	gebr_gui_help_button_clicked(section, &error);

	if (error) {
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, error);
		g_free(error);
	}
}

/*Show dialog just if snapshot_save_default is not set*/
static gint
gebr_flow_browse_confirm_revert(void)
{
	/* Get glade file */
	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, GEBR_GLADE_DIR "/snapshot-revert.glade", NULL);
	GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(builder, "main"));

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gebr.window));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	GObject *snapshot_button = gtk_builder_get_object(builder, "take_snapshot_button");
	GObject *discard_button = gtk_builder_get_object(builder, "discard_changes_button");
	GObject *cancel_button = gtk_builder_get_object(builder, "cancel_button");
	GObject *help_button = gtk_builder_get_object(builder, "help_button");

	g_signal_connect(snapshot_button, "clicked", G_CALLBACK(on_snapshot_save_clicked), dialog);
	g_signal_connect(discard_button, "clicked", G_CALLBACK(on_snapshot_discard_changes_clicked), dialog);
	g_signal_connect(cancel_button, "clicked", G_CALLBACK(on_snapshot_cancel), dialog);
	g_signal_connect(help_button, "clicked", G_CALLBACK(on_snapshot_help_button_clicked), dialog);

	gtk_widget_show(GTK_WIDGET(dialog));

	gdk_threads_enter();
	gint ret = gtk_dialog_run(GTK_DIALOG(dialog));
	gdk_threads_leave();
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_object_unref(builder);

	return ret;
}

static void
gebr_flow_browse_snapshot_revert(const gchar *rev_id)
{
	gint confirm_revert = GTK_RESPONSE_NONE;
	gboolean confirm_save  = FALSE;
	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
	gchar *flow_last_modified_date = NULL;
	GtkTreeIter iter;

	GebrGeoXmlRevision *revision = gebr_geoxml_flow_get_revision_by_id(gebr.flow, rev_id);

	GebrUiFlow *ui_flow;
	GebrUiFlowBrowseType type;
	const gchar *snapshot_last_modified_date = NULL;

	gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
	while (valid) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
				   FB_STRUCT_TYPE, &type,
				   -1);

		if (type != STRUCT_TYPE_FLOW) {
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
			continue;
		}

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		if (!g_strcmp0(gebr_ui_flow_get_filename(ui_flow), flow_id))
			snapshot_last_modified_date = gebr_ui_flow_get_last_modified(ui_flow);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
	}

	flow_last_modified_date = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(gebr.flow));

	GTimeVal flow_time = gebr_iso_date_to_g_time_val(flow_last_modified_date);
	GTimeVal snapshot_time;
	if (snapshot_last_modified_date)
		snapshot_time = gebr_iso_date_to_g_time_val(snapshot_last_modified_date);

	if (!snapshot_last_modified_date || (snapshot_time.tv_sec < flow_time.tv_sec)) {
		confirm_revert = gebr_flow_browse_confirm_revert();
	}

	if (confirm_revert == GTK_RESPONSE_CANCEL) {
		return;
	} else if (confirm_revert == GTK_RESPONSE_YES) {
		confirm_save = flow_revision_save();
		gdk_threads_leave();
		if(!confirm_save)
			return;
	}

	if (!gebr_geoxml_flow_change_to_revision(gebr.flow, revision)) {
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not revert to snapshot with ID %s"), rev_id);
		return;
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	flow_browse_load();

	gtk_toggle_button_set_active(gebr.ui_flow_browse->snapshots_ctx_button, TRUE);

	gebr_validator_force_update(gebr.validator);
	flow_browse_info_update();
	gebr_ui_flow_browse_update_speed_slider_sensitiveness(gebr.ui_flow_browse);
	gchar *last_date = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(gebr.flow));
	gebr_flow_set_snapshot_last_modify_date(last_date);

	g_free(flow_last_modified_date);
	g_free(last_date);
}

/* Method to parse commands of Graph on Python */
static void
graph_process_read_stderr(GebrCommProcess * process,
                          GebrUiFlowBrowse *fb)
{
	GString *output;
	output = gebr_comm_process_read_stderr_string_all(process);
	gboolean is_detailed = FALSE;

//	g_debug("ERROR OF PYTHON: %s", output->str);

	gchar **action = g_strsplit(output->str, ":", -1);

	if (!g_strcmp0(action[0], "revert")) {
		gebr_flow_browse_snapshot_revert(action[1]);
	}
	else if (!g_strcmp0(action[0], "delete")) {
		gebr_flow_browse_snapshot_delete(action[1]);
	}
	else if (!g_strcmp0(action[0], "snapshot")) {
		gdk_threads_enter();
		flow_revision_save();
		gdk_threads_leave();
	}
	else if (!g_strcmp0(action[0], "select")) {
		const gchar *id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
		GList *find = g_list_find_custom(fb->select_flows, id, (GCompareFunc)g_strcmp0);
		if (!find)
			fb->select_flows = g_list_append(fb->select_flows, g_strdup(id));
	}
	else if (!g_strcmp0(action[0], "unselect")) {
		const gchar *id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
		GList *find = g_list_find_custom(fb->select_flows, id, (GCompareFunc)g_strcmp0);
		if (find)
			fb->select_flows = g_list_remove_link(fb->select_flows, find);
	}
	else if (!g_strcmp0(action[0], "run")) {
		gboolean is_parallel = FALSE;

		if (!g_strcmp0(action[1],"parallel"))
			is_parallel = TRUE;

		if (!g_strcmp0(action[2], "detailed"))
			is_detailed = TRUE;

		gebr_ui_flow_run_snapshots(NULL, gebr.flow, action[3], is_parallel, is_detailed);
	}

	g_string_free(output, TRUE);
	g_strfreev(action);
}

static void
graph_process_finished(GebrCommProcess *process)
{
	gebr_comm_process_free(process);
}

static void
flow_browse_add_revisions_graph(GebrGeoXmlFlow *flow,
                                GebrUiFlowBrowse *fb,
                                gboolean keep_selection)
{
	GHashTable *revs = gebr_flow_revisions_hash_create(flow);

	if (fb->update_graph) {
		gchar *dotfile;
		if (gebr_flow_revisions_create_graph(flow, revs, &dotfile)) {
			fb->update_graph = FALSE;
			GString *file = g_string_new(dotfile);
			gchar *flow_filename = g_strdup(gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow)));

			gchar *command = g_strdup_printf("draw\b%s\b%s\b", flow_filename, keep_selection? "yes" : "no");
			g_string_prepend(file, command);

			if (gebr_comm_process_write_stdin_string(fb->graph_process, file) == 0)
				g_debug("Can't create dotfile.");

			g_string_free(file, TRUE);
			g_free(flow_filename);
			g_free(command);
		}
		g_free(dotfile);
	}
}

static void
gebr_flow_browse_create_graph(GebrUiFlowBrowse *fb)
{
	if (fb->graph_process)
		return;

	fb->update_graph = TRUE;
	fb->graph_process = gebr_comm_process_new();

	GtkWidget *box = GTK_WIDGET(gtk_builder_get_object(fb->info.builder_flow, "graph_box"));

	GtkWidget *socket = gtk_socket_new();
	gtk_box_pack_start(GTK_BOX(box), socket, TRUE, TRUE, 0);
	GdkNativeWindow socket_id = gtk_socket_get_id(GTK_SOCKET(socket));

	g_debug("SOCKET ID %d", socket_id);

	gchar *cmd_line = g_strdup_printf("python %s/gebr-xdot-graph.py %d %s", GEBR_PYTHON_DIR, socket_id, PACKAGE_LOCALE_DIR);
	GString *cmd = g_string_new(cmd_line);

	g_signal_connect(fb->graph_process, "ready-read-stderr", G_CALLBACK(graph_process_read_stderr), fb);
	g_signal_connect(fb->graph_process, "finished", G_CALLBACK(graph_process_finished), NULL);

	if (!gebr_comm_process_start(fb->graph_process, cmd))
		g_debug("FAIL");

	gtk_widget_show_all(socket);

	g_free(cmd_line);
	g_string_free(cmd, TRUE);
}
/* End of Snapshots methods */

static void
on_job_info_status_changed(GebrJob *job,
                           GebrCommJobStatus old_status,
                           GebrCommJobStatus new_status,
                           const gchar *parameter,
                           GtkWidget *container)
{
	gchar *icon, *job_state;
	gchar *title;
	const gchar *snap_id = gebr_job_get_snapshot_id(job);

	gchar *aux_title = g_markup_printf_escaped("<span>#%s</span>",
	                                           gebr_job_get_job_counter(job));

	if(snap_id && *snap_id)
		title = g_strdup_printf("%s (%s)", aux_title,
				gebr_job_get_snapshot_title(job));
	else
		title = g_strdup(aux_title);

	gchar *tooltip_beginning = g_markup_printf_escaped(_("Output of <span font_style='italic'>"));
	gchar *tooltip_end = g_markup_printf_escaped("</span>");
	gchar *tooltip = g_strconcat(tooltip_beginning, aux_title, tooltip_end, NULL);

	switch(new_status) {
	case JOB_STATUS_FINISHED:
		icon = GTK_STOCK_APPLY;
		job_state = g_strdup(_("finished"));
		break;
	case JOB_STATUS_RUNNING:
		icon = GTK_STOCK_EXECUTE;
		job_state = g_strdup(_("started"));
		break;
	case JOB_STATUS_CANCELED:
		icon = GTK_STOCK_CANCEL;
		job_state = g_strdup(_("canceled"));
		break;
	case JOB_STATUS_FAILED:
		icon = GTK_STOCK_CANCEL;
		job_state = g_strdup(_("failed"));
		break;
	case JOB_STATUS_QUEUED:
	case JOB_STATUS_INITIAL:
	default:
		icon = "chronometer";
		job_state = g_strdup(_("submitted"));
		g_free(tooltip);
		tooltip = g_strdup(_("This job doesn't have output yet"));
		break;
	}
	gtk_widget_set_tooltip_markup(container, tooltip);

	GList *children = gtk_container_get_children(GTK_CONTAINER(container));
	GtkWidget *image = GTK_WIDGET(g_list_nth_data(children, 0));
	gtk_image_set_from_stock(GTK_IMAGE(image), icon, GTK_ICON_SIZE_BUTTON);
	GtkWidget *label = GTK_WIDGET(g_list_nth_data(children, 1));

	gtk_label_set_markup(GTK_LABEL(label), title);

	g_free(aux_title);
	g_free(title);
	g_free(tooltip);
	g_free(tooltip_beginning);
	g_free(tooltip_end);
	g_free(job_state);
}

static gboolean
flow_browse_static_info_update(void)
{
	if (gebr.flow == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), "");

		/* Update snapshots context */
		gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->revpage_warn_label), _("No Flow selected."));
		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);
		return FALSE;
	}

	/* Status Icon */
	gchar *flow_icon_path = g_build_filename(LIBGEBR_ICONS_DIR, "gebr-theme", "48x48", "stock", "flow-icon.png", NULL);
	GIcon *flow_icon = g_icon_new_for_string(flow_icon_path, NULL);

	gchar *status_icon_path;

	GError *error = NULL;
	gebr_geoxml_flow_validate(gebr.flow, gebr.validator, &error);
	if (error) {
		status_icon_path = g_build_filename(LIBGEBR_ICONS_DIR, "gebr-theme", "22x22", "stock", "dialog-warning.png", NULL);
		gtk_widget_set_tooltip_text(gebr.ui_flow_browse->info.status, error->message);
		g_clear_error(&error);
	} else {
		status_icon_path = g_build_filename(LIBGEBR_ICONS_DIR, "gebr-theme", "22x22", "stock", "gtk-apply.png", NULL);
		gtk_widget_set_tooltip_text(gebr.ui_flow_browse->info.status, _("Ready to execute"));
	}
	GIcon *status_icon = g_icon_new_for_string(status_icon_path, NULL);
	GEmblem *status_emblem = g_emblem_new(status_icon);

	GIcon *icon = g_emblemed_icon_new(flow_icon, status_emblem);

	gtk_image_set_from_gicon(GTK_IMAGE(gebr.ui_flow_browse->info.status), icon, GTK_ICON_SIZE_DIALOG);

	g_free(flow_icon_path);
	g_free(status_icon_path);

	gchar *markup;

	/* Description */
	gchar *description = gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(gebr.flow));
	if (!description || !*description){
		markup = g_markup_printf_escaped(_("<span size='x-large'>No description available</span>"));
		gtk_widget_set_sensitive(gebr.ui_flow_browse->info.description, FALSE);
	}
	else {
		markup = g_markup_printf_escaped("<span size='x-large'>%s</span>",description);
		gtk_widget_set_sensitive(gebr.ui_flow_browse->info.description, TRUE);
	}
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.description), markup);
	gtk_label_set_ellipsize(GTK_LABEL(gebr.ui_flow_browse->info.description), PANGO_ELLIPSIZE_END);
	g_free(markup);
	g_free(description);

	/* Modified date */
	gchar *modified = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOC(gebr.flow));
	gchar *mod_date = g_markup_printf_escaped(_("Modified on %s"),
	                                          gebr_localized_date(modified));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.modified), mod_date);
	g_free(mod_date);
	g_free(modified);

	gchar *last_text = NULL;
	gchar *last_run_date = gebr_geoxml_flow_get_date_last_run(gebr.flow);
	if (!last_run_date || !*last_run_date) {
		last_text = g_strdup(_("This flow was never executed"));
	} else {
		const gchar *last_run = gebr_localized_date(last_run_date);
		last_text = g_markup_printf_escaped(_("Submitted on %s"), last_run);
	}
	g_free(last_run_date);

	if (last_text) {
		gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), last_text);
		g_free(last_text);
	}
	return TRUE;
}

void flow_browse_info_update(void)
{
	if (!flow_browse_static_info_update())
		return;

        /* Update flow list */
        GtkTreeIter iter;
        if (flow_browse_get_selected(&iter, FALSE)) {
        	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
        	gtk_tree_model_row_changed(GTK_TREE_MODEL(gebr.ui_flow_browse->store), path, &iter);
        }

        /* Clean job list view */
        on_dismiss_clicked(NULL, gebr.ui_flow_browse);

        gint nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view)));
        /* Get the list and rebuild Info Bar */
        GList *jobs = gebr_flow_browse_get_jobs_from_flow(gebr.flow, gebr.ui_flow_browse);
        if (jobs) {
        	if (nrows == 1)
        		gebr_flow_browse_update_jobs_info(gebr.flow, gebr.ui_flow_browse,
        		                                  gebr_flow_browse_calculate_n_max(gebr.ui_flow_browse));
        }
}

gboolean flow_browse_get_selected(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Flow selected"));
		return FALSE;
	}
	return TRUE;
}

void flow_browse_reload_selected(void)
{
	flow_browse_load();
}

void flow_browse_select_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter);
}

void flow_browse_single_selection(void)
{
	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
}

void flow_browse_status_changed(guint status)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
		flow_browse_change_iter_status(status, &iter, gebr.ui_flow_browse);
	}

	flow_browse_program_check_sensitiveness();
	flow_browse_revalidate_programs(gebr.ui_flow_browse);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
}

gboolean
gebr_flow_browse_get_io_iter(GtkTreeModel *model,
                             GtkTreeIter *io_iter,
                             GebrUiFlowsIoType io_type)
{
	GtkTreeIter iter, parent;
	gboolean valid;
	GebrUiFlowBrowseType type;
	GebrUiFlowsIo *ui_io;

	valid = gtk_tree_model_get_iter_first(model, &parent);
	while (valid) {
		valid = gtk_tree_model_iter_children(model, &iter, &parent);
		while (valid) {
			gtk_tree_model_get(model, &iter,
					   FB_STRUCT_TYPE, &type,
					   FB_STRUCT, &ui_io,
					   -1);

			if (type == STRUCT_TYPE_IO) {
				if (gebr_ui_flows_io_get_io_type(ui_io) == io_type) {
					*io_iter = iter;
					return TRUE;
				}
			}
			valid = gtk_tree_model_iter_next(model, &iter);
		}
		valid = gtk_tree_model_iter_next(model, &parent);
	}
	return FALSE;
}

static void
remove_programs_view(GebrUiFlowBrowse *fb)
{
	GtkTreeIter parent, iter;
	GtkTreeModel *model = GTK_TREE_MODEL(fb->store);

	gboolean removed = FALSE;
	gboolean valid = gtk_tree_model_get_iter_first(model, &parent);
	while (valid && !removed) {
		valid = gtk_tree_model_iter_children(model, &iter, &parent);
		while (valid) {
			removed = TRUE;
			valid = gtk_tree_store_remove(fb->store, &iter);
		}
		valid = gtk_tree_model_iter_next(model, &parent);
	}
}

static void
create_programs_view(GtkTreeIter *parent,
                     GebrUiFlowBrowse *fb)
{
	GtkTreeModel *model = GTK_TREE_MODEL(fb->store);

	GebrUiFlow *ui_flow;

	GtkTreePath *path = gtk_tree_model_get_path(model, parent);

	gtk_tree_model_get(model, parent,
	                   FB_STRUCT, &ui_flow,
	                   -1);

	GebrGeoXmlFlow *flow = gebr_ui_flow_get_flow(ui_flow);
	gebr_ui_flow_set_is_selected(ui_flow, TRUE);

	/* add Input file */
	GtkTreeIter input_iter;
	GebrUiFlowsIo *input_io = gebr_ui_flows_io_new(GEBR_IO_TYPE_INPUT);
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gebr_ui_flows_io_load_from_xml(input_io, gebr.line, gebr.flow, maestro, gebr.validator);
	gtk_tree_store_insert(fb->store, &input_iter, parent, 0);
	gtk_tree_store_set(fb->store, &input_iter,
	                   FB_STRUCT_TYPE, STRUCT_TYPE_IO,
	                   FB_STRUCT, input_io,
	                   -1);

	/* populate with programs */
	GtkTreeIter iter;
	GebrGeoXmlProgramControl control;
	GebrGeoXmlSequence *program;
	gebr_geoxml_flow_get_program(flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
		control = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));

		if (control != GEBR_GEOXML_PROGRAM_CONTROL_FOR)
			gtk_tree_store_append(fb->store, &iter, parent);
		else
			gtk_tree_store_insert_before(fb->store, &iter, NULL, &input_iter);

		gebr_geoxml_object_ref(program);
		GebrUiFlowProgram *prog = gebr_ui_flow_program_new(GEBR_GEOXML_PROGRAM(program));
		gtk_tree_store_set(fb->store, &iter,
		                   FB_STRUCT_TYPE, STRUCT_TYPE_PROGRAM,
		                   FB_STRUCT, prog,
		                   -1);
	}

	/* Add Output file */
	GebrUiFlowsIo *output_io = gebr_ui_flows_io_new(GEBR_IO_TYPE_OUTPUT);
	gebr_ui_flows_io_load_from_xml(output_io, gebr.line, gebr.flow, maestro, gebr.validator);
	gtk_tree_store_append(fb->store, &iter, parent);
	gtk_tree_store_set(fb->store, &iter,
	                   FB_STRUCT_TYPE, STRUCT_TYPE_IO,
	                   FB_STRUCT, output_io,
	                   -1);

	/* Add Error file */
	GebrUiFlowsIo *error_io = gebr_ui_flows_io_new(GEBR_IO_TYPE_ERROR);
	gebr_ui_flows_io_load_from_xml(error_io, gebr.line, gebr.flow, maestro, gebr.validator);
	gtk_tree_store_append(fb->store, &iter, parent);
	gtk_tree_store_set(fb->store, &iter,
	                   FB_STRUCT_TYPE, STRUCT_TYPE_IO,
	                   FB_STRUCT, error_io,
	                   -1);

	gtk_tree_view_expand_row(GTK_TREE_VIEW(fb->view), path, TRUE);
}

static void
save_parameters(GebrGuiProgramEdit *program_edit)
{
	GtkTreeIter iter, parent;

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	/* Update interface */
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_flow_browse->store);

	gboolean valid = gtk_tree_model_get_iter_first(model, &parent);
	GebrUiFlowBrowseType type;
	gpointer ui_struct;
	gboolean find_program = FALSE;

	while (valid && !find_program) {
		valid = gtk_tree_model_iter_children(model, &iter, &parent);
		while (valid) {
			gtk_tree_model_get(model, &iter,
					   FB_STRUCT_TYPE, &type,
					   FB_STRUCT, &ui_struct,
					   -1);
			if (type == STRUCT_TYPE_PROGRAM) {
				GebrGeoXmlProgram *prog = gebr_ui_flow_program_get_xml(GEBR_UI_FLOW_PROGRAM(ui_struct));
				if (prog == gebr.program) {
					find_program = TRUE;
					break;
				}
			} 
			valid = gtk_tree_model_iter_next(model, &iter);
		}
		valid = gtk_tree_model_iter_next(model, &parent);
	}

	if (!ui_struct || (iter.stamp == 0))
		return;

	GebrUiFlowProgram *ui_program = GEBR_UI_FLOW_PROGRAM(ui_struct);

	gebr_ui_flow_program_set_xml(ui_program, program_edit->program);

	if (gebr_ui_flow_program_get_flag_opened(ui_program))
		gebr_ui_flow_program_set_flag_opened(ui_program, FALSE);

	GebrGeoXmlProgram *program = program_edit->program;

	if (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		if (gebr_geoxml_program_get_status(program) == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) {
			gebr_geoxml_flow_insert_iter_dict(gebr.flow);
			GebrGeoXmlSequence *parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
			gebr_validator_insert(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
		} else {
			gebr_geoxml_flow_update_iter_dict_value(gebr.flow);
			GebrGeoXmlProgramParameter *dict_iter = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow)));
			const gchar *value = gebr_geoxml_program_parameter_get_first_value(dict_iter, FALSE);
			gebr_validator_change_value(gebr.validator, GEBR_GEOXML_PARAMETER(dict_iter), value, NULL, NULL);
		}
	}

	if (validate_selected_program(NULL))
		flow_browse_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED, &iter, gebr.ui_flow_browse);
	else
		flow_browse_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED, &iter, gebr.ui_flow_browse);

	flow_browse_revalidate_programs(gebr.ui_flow_browse);
	flow_browse_validate_io(gebr.ui_flow_browse);

	/* Update parameters review on Flow Browse */
	gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse, FALSE);

	flow_browse_info_update();

	gebr.ui_flow_browse->program_edit = NULL;
}

static gboolean
flow_browse_on_multiple_selection(GtkTreeModel *model,
                                  GtkTreeIter *last_iter,
                                  GebrUiFlowBrowseType last_type,
                                  GebrUiFlowBrowse *fb)
{
	gebr_flow_browse_block_changed_signal(fb);

	gboolean mixed_selection = FALSE;
	gboolean change_type = FALSE;
	gint n_rows = 1;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, NULL);
	if (rows) {
		n_rows = g_list_length(rows);
		if (n_rows > 1) {
			GebrUiFlowBrowseType each_type;
			for (GList *i = rows; i; i = i->next) {
				GtkTreeIter it;
				if (!gtk_tree_model_get_iter(model, &it, i->data))
					continue;

				gtk_tree_model_get(model, &it,
				                   FB_STRUCT_TYPE, &each_type,
				                   -1);

				if (last_type == STRUCT_TYPE_IO) {
					gtk_tree_selection_unselect_iter(selection, last_iter);
					change_type = TRUE;
					last_type = STRUCT_TYPE_PROGRAM;
				}
				if (last_type != each_type) {
					mixed_selection = TRUE;
					gtk_tree_selection_unselect_iter(selection, &it);
				}
			}
		}
	}

	if (mixed_selection && !change_type)
		gtk_tree_selection_select_iter(selection, last_iter);

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	rows = gtk_tree_selection_get_selected_rows(selection, NULL);

	gebr_flow_browse_unblock_changed_signal(fb);

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);

	return n_rows > 1;
}

static void selection_changed_signal(void)
{
	flow_browse_load();
}

/**
 * Load a selected flow from file when selected in "Flow Browser".
 */
static void flow_browse_load(void)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GebrUiFlowBrowseType type;

	model = GTK_TREE_MODEL(gebr.ui_flow_browse->store);

	/* Update line title on back button */
	if (gebr.line) {
		gchar *line_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line));
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->flows_line_label), line_title);
		g_free(line_title);
	}

	gebr_flow_set_toolbar_sensitive();

	GtkTreePath *curr_path = NULL;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(gebr.ui_flow_browse->view), &curr_path, NULL);
	if (curr_path) {
		if (!gtk_tree_model_get_iter(model, &iter, curr_path))
			return;
	}
	else {
		if (!flow_browse_get_selected(&iter, FALSE))
			return;
	}
	gtk_tree_path_free(curr_path);

	gtk_tree_model_get(model, &iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	if (gebr.ui_flow_browse->program_edit)
		save_parameters(gebr.ui_flow_browse->program_edit);

	GebrGeoXmlFlow *old_flow = gebr.flow;
	gebr_geoxml_document_ref(GEBR_GEOXML_DOCUMENT(old_flow));

	gboolean multiple_selection = flow_browse_on_multiple_selection(model, &iter, type, gebr.ui_flow_browse);

	gint nrows;

	if (type == STRUCT_TYPE_FLOW) {
		gebr_flow_browse_create_graph(gebr.ui_flow_browse);

		flow_free();

		flow_browse_set_run_widgets_sensitiveness(gebr.ui_flow_browse, TRUE, FALSE);

		nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view)));
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"), nrows > 1? FALSE : TRUE);

		GebrUiFlow *ui_flow;

		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		gebr.flow = gebr_ui_flow_get_flow(ui_flow);

		if (gebr.validator)
			gebr_validator_update(gebr.validator);

		gebr_ui_flow_set_is_selected(ui_flow, FALSE);

		if (!multiple_selection) {
			create_programs_view(&iter, gebr.ui_flow_browse);
			flow_browse_program_check_sensitiveness();
		} else {
			remove_programs_view(gebr.ui_flow_browse);
		}

	} else if (type == STRUCT_TYPE_PROGRAM) {
		gboolean has_error;
		GtkAction * action;

		GebrUiFlowProgram *ui_program;
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_program,
		                   -1);

		gebr.program = gebr_ui_flow_program_get_xml(ui_program);

		/* If the program has an error, disable the 'Configured' status */
		action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_toggle_status");
		has_error = gebr_geoxml_program_get_error_id(gebr.program, NULL);
		gtk_action_set_sensitive(action, !has_error);

		action = gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_help");
		gchar *tmp_help_p = gebr_geoxml_program_get_help(gebr.program);
		gtk_action_set_sensitive(action, strlen(tmp_help_p) != 0);
		g_free(tmp_help_p);

		gint anchor = 0;
		gint index = 1;
		GebrGeoXmlSequence *progs;
		gebr_geoxml_flow_get_program(gebr.flow, &progs, 0);
		for (; progs && !anchor; gebr_geoxml_sequence_next(&progs)) {
			if (GEBR_GEOXML_PROGRAM(progs) == gebr.program)
				anchor = index;
			index++;
		}

		gebr_gui_html_viewer_widget_load_anchor(GEBR_GUI_HTML_VIEWER_WIDGET(gebr.ui_flow_browse->html_parameters), anchor);
	}

	if (type == STRUCT_TYPE_PROGRAM || type == STRUCT_TYPE_IO)
		nrows = 1;

	/* check if has revisions */
	gboolean has_revision = gebr_geoxml_flow_get_revisions_number(gebr.flow) > 0;

	if (has_revision && nrows == 1) {
		gtk_widget_show(gebr.ui_flow_browse->revpage_main);
		gtk_widget_hide(gebr.ui_flow_browse->revpage_warn);

		gebr.ui_flow_browse->update_graph = TRUE;
		flow_browse_add_revisions_graph(gebr.flow,
		                                gebr.ui_flow_browse,
		                                FALSE);
	} else if (nrows > 1) {
		gchar *multiple_selection_msg = g_strdup_printf(_("%d Flows selected.\n\n"
				"GêBR can take a snapshot\n"
				"of the current state for\n"
				"each of the selected Flows.\n"
				"To do it, just click on the\n"
				"camera icon."), nrows);
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->revpage_warn_label),
		                   multiple_selection_msg);
		g_free(multiple_selection_msg);

		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);
	} else {
		const gchar *no_snapshots_msg = _("There are no snapshots.\n\n"
				"A snapshot stores the settings of "
				"your flow so you can restore it at any "
				"moment. To take a snapshot, just click "
				"on the camera icon and give a non-empty "
				"description.");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->revpage_warn_label),
		                   no_snapshots_msg);

		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);
	}

	flow_browse_info_update();

	/* Set correct view */
	if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0) {
		gebr_flow_browse_define_context_to_show(CONTEXT_MENU, gebr.ui_flow_browse);
	} else {
		if (type == STRUCT_TYPE_FLOW) {
			GebrGeoXmlFlow *flow;
			GebrUiFlow *ui_flow;

			gtk_tree_model_get(model, &iter,
			                   FB_STRUCT, &ui_flow,
			                   -1);

			flow = gebr_ui_flow_get_flow(ui_flow);
			if (gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
				gebr_flow_browse_load_parameters_review(flow, gebr.ui_flow_browse, FALSE);
			else
				gebr_flow_browse_define_context_to_show(CONTEXT_SNAPSHOTS, gebr.ui_flow_browse);
		} else {
			if (!gtk_widget_get_visible(gebr.ui_flow_browse->context[CONTEXT_JOBS]) &&
			    !gtk_widget_get_visible(gebr.ui_flow_browse->context[CONTEXT_MENU])) {
				if (gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
					gebr_flow_browse_define_context_to_show(CONTEXT_FLOW, gebr.ui_flow_browse);
				else
					gebr_flow_browse_define_context_to_show(CONTEXT_SNAPSHOTS, gebr.ui_flow_browse);
			}
		}
	}

	gebr_geoxml_document_unref(GEBR_GEOXML_DOCUMENT(old_flow));
}

gboolean
gebr_ui_flow_browse_update_speed_slider_sensitiveness(GebrUiFlowBrowse *ufb)
{
	gboolean sensitive = FALSE;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ufb->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreeIter iter;
		GebrUiFlow *ui_flow;
		GebrUiFlowBrowseType type;
		GebrGeoXmlFlow *flow;

		gtk_tree_model_get_iter(model, &iter, i->data);
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT_TYPE, &type,
		                   -1);

		if (type != STRUCT_TYPE_FLOW) {
			GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_first_mpi_program(gebr.flow);
			if ((gebr_geoxml_flow_is_parallelizable(gebr.flow, gebr.validator))
			    || (gebr_geoxml_program_get_status(prog) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED))
				sensitive = TRUE;
			gebr_geoxml_object_unref(prog);
			break;
		}

		gtk_tree_model_get_iter(model, &iter, i->data);
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		flow = gebr_ui_flow_get_flow(ui_flow);

		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		gboolean parallel = gebr_geoxml_flow_is_parallelizable(flow, gebr.validator);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_first_mpi_program(flow);
		gboolean has_mpi = (gebr_geoxml_program_get_status(prog) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
		gebr_geoxml_object_unref(prog);

		if (parallel || has_mpi) {
			sensitive = TRUE;
			break;
		}

	}
	if (gebr.line)
		gebr_flow_set_toolbar_sensitive();

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);

	return sensitive;
}

/**
 */
void flow_browse_show_help(void)
{
	if (!gebr.flow)
		return;
	gebr_help_show(GEBR_GEOXML_OBJECT(gebr.flow), FALSE);
}

/**
 */
void flow_browse_edit_help(void)
{
	if (!gebr.flow)
		return;
	gebr_help_edit_document(GEBR_GEOXML_DOC(gebr.flow));
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
}

/**
 * \internal
 * Go to flow components tab
 */
static void
flow_browse_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
			     GtkTreeViewColumn * column, GebrUiFlowBrowse *fb)
{
	GtkTreeModel *model = GTK_TREE_MODEL(fb->store);
	GtkTreeIter iter;

	GebrUiFlowBrowseType type;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	if (type == STRUCT_TYPE_FLOW) {
		gebr_flow_browse_define_context_to_show(CONTEXT_MENU, fb);
	}
	else if (type == STRUCT_TYPE_PROGRAM) {
		GebrGuiProgramEdit *program_edit = parameters_configure_setup_ui();

		if (program_edit) {
			GebrUiFlowProgram *ui_program;

			gtk_tree_model_get(model, &iter,
			                   FB_STRUCT, &ui_program,
			                   -1);

			gebr_ui_flow_program_set_flag_opened(ui_program, FALSE);

			GtkTreeIter parent;
			gtk_tree_model_iter_parent(model, &parent, &iter);
			GtkTreePath *path = gtk_tree_model_get_path(model, &parent);
			gtk_tree_model_row_changed(model, path, &parent);
			gtk_tree_path_free(path);

			if (fb->program_edit)
				gebr_gui_program_edit_destroy(fb->program_edit);

			fb->program_edit = program_edit;

			GList *children = gtk_container_get_children(GTK_CONTAINER(fb->context[CONTEXT_PARAMETERS]));
			if (children)
				gtk_widget_destroy(children->data);

			gtk_container_add(GTK_CONTAINER(fb->context[CONTEXT_PARAMETERS]), program_edit->widget);
			gebr_flow_browse_define_context_to_show(CONTEXT_PARAMETERS, fb);
		}
	}
}

void
flow_add_program_to_flow(GebrGeoXmlSequence *program,
                         GtkTreeIter *flow_iter,
                         gboolean never_opened)
{
       gebr_geoxml_object_ref(program);
       GebrUiFlowProgram *ui_program = gebr_ui_flow_program_new(GEBR_GEOXML_PROGRAM(program));
       gebr_ui_flow_program_set_flag_opened(ui_program, never_opened);
       gtk_tree_store_set(gebr.ui_flow_browse->store, flow_iter,
                          FB_STRUCT_TYPE, STRUCT_TYPE_PROGRAM,
                          FB_STRUCT, ui_program,
                          -1);
}

void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program,
                                      gboolean select_last,
                                      gboolean never_opened)
{
       GtkTreeIter iter, parent;
       GebrGeoXmlProgramControl control;
       gboolean has_control;
       GebrGeoXmlProgram *first_prog = NULL;

       GtkTreeModel *model = GTK_TREE_MODEL (gebr.ui_flow_browse->store);

       gboolean valid = gtk_tree_model_get_iter_first (model, &parent);
       while (valid) {
               valid = gtk_tree_model_iter_children(model, &iter, &parent);
               if (valid)
                       break;
               valid = gtk_tree_model_iter_next(model, &parent);
       }


       GtkTreeIter output_iter;
       GebrUiFlowBrowseType type;
       GebrUiFlowProgram *ui_program;
       GebrUiFlowsIo *ui_io;
       while (valid) {
               gtk_tree_model_get(model, &iter,
                                  FB_STRUCT_TYPE, &type,
                                  -1);

               if (type == STRUCT_TYPE_IO) {
                       gtk_tree_model_get(model, &iter,
                                          FB_STRUCT, &ui_io,
                                          -1);

                       if (gebr_ui_flows_io_get_io_type(ui_io) == GEBR_IO_TYPE_OUTPUT) {
                               output_iter = iter;
                               break;
                       }
               }

               if (type != STRUCT_TYPE_PROGRAM || first_prog) {
                       valid = gtk_tree_model_iter_next(model, &iter);
                       continue;
               }

               gtk_tree_model_get(model, &iter,
                                  FB_STRUCT, &ui_program,
                                  -1);

               if (!first_prog)
                       first_prog = gebr_ui_flow_program_get_xml(ui_program);

               valid = gtk_tree_model_iter_next(model, &iter);
       }

       control = gebr_geoxml_program_get_control (first_prog);
       has_control = control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;

       // Reference this program so _sequence_next don't destroy it
       gebr_geoxml_object_ref(program);
       for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
               control = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));

               if (has_control && control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
                       continue;

               if (!has_control && control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY) {
                       gtk_tree_store_prepend(gebr.ui_flow_browse->store, &iter, &parent);
                       has_control = TRUE;
               } else
                       gtk_tree_store_insert_before(gebr.ui_flow_browse->store, &iter, &parent, &output_iter);

               flow_add_program_to_flow(program, &iter, never_opened);

               GebrIExprError undef;
               gebr_geoxml_program_get_error_id(GEBR_GEOXML_PROGRAM(program), &undef);
               if (never_opened || undef == GEBR_IEXPR_ERROR_PATH)
                       continue;

               gebr_geoxml_program_is_valid(GEBR_GEOXML_PROGRAM(program), gebr.validator, NULL);
       }

       if (select_last)
               flow_browse_select_iter(&iter);
}

void
flow_browse_validate_io(GebrUiFlowBrowse *fb)
{
       GtkTreeIter iter, parent;
       GtkTreeModel *model = GTK_TREE_MODEL(fb->store);

       gboolean valid = gtk_tree_model_get_iter_first (model, &parent);
       while (valid) {
               valid = gtk_tree_model_iter_children(model, &iter, &parent);
               if (valid)
                       break;
               valid = gtk_tree_model_iter_next(model, &parent);
       }

       GebrUiFlowBrowseType type;
       GebrUiFlowsIo *ui_io;
       while (valid) {
               gtk_tree_model_get(model, &iter,
                                  FB_STRUCT_TYPE, &type,
                                  -1);

               if (type == STRUCT_TYPE_IO) {
                       gtk_tree_model_get(model, &iter,
                                          FB_STRUCT, &ui_io,
                                          -1);

                       GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
                       gebr_ui_flows_io_load_from_xml(ui_io, gebr.line, gebr.flow, maestro, gebr.validator);

                       GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                       gtk_tree_model_row_changed(model, path, &iter);
                       gtk_tree_path_free(path);
               }
               valid = gtk_tree_model_iter_next(model, &iter);
       }
       flow_browse_program_check_sensitiveness();
}

static void
flow_browse_on_query_tooltip(GtkTreeView * treeview,
			     gint x,
			     gint y,
			     gboolean keyboard_tip,
			     GtkTooltip * tooltip,
			     GebrUiFlowBrowse *ui_flow_browse)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *message = NULL;
	GebrUiFlowBrowseType type;

	if (!gtk_tree_view_get_tooltip_context(treeview, &x, &y, keyboard_tip, &model, NULL, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_browse->store),
			   &iter, FB_STRUCT_TYPE, &type, -1);

	if (type == STRUCT_TYPE_FLOW) {
		GebrUiFlow *ui_flow;
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		GebrGeoXmlFlow *flow = gebr_ui_flow_get_flow(ui_flow);
		gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));
		message = g_markup_printf_escaped(_("Flow <i>%s</i>"), flow_title);
		g_free(flow_title);
	} else if (type == STRUCT_TYPE_PROGRAM) {
		GebrUiFlowProgram *ui_program;
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_program,
		                   -1);
		message = g_strdup(gebr_ui_flow_program_get_tooltip(ui_program));
	} else if (type == STRUCT_TYPE_IO) {
		GebrUiFlowsIo *ui_io;
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_io,
		                   -1);
		message = g_strdup(gebr_ui_flows_io_get_tooltip(ui_io));
	}

	gtk_tooltip_set_markup(tooltip, message);
	g_free(message);
}
/**
 * \internal
 * Saves the current selected line.
 */
//static void flow_browse_on_flow_move(void)
//{
//	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, TRUE);
//}

void
gebr_flow_browse_hide(GebrUiFlowBrowse *self)
{
	return;
}

void
gebr_flow_browse_show(GebrUiFlowBrowse *self)
{
	if (gebr.line)
		gebr_flow_set_toolbar_sensitive();

	flow_browse_info_update();

	GtkWidget *output_view = gebr_job_control_get_output_view(gebr.job_control);
	gtk_widget_reparent(output_view, self->context[CONTEXT_JOBS]);

	/* Set default on properties flow */
	if (!gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
		gtk_toggle_button_set_active(gebr.ui_flow_browse->properties_ctx_button, TRUE);
	else
		gebr_flow_browse_load_parameters_review(gebr.flow, self, TRUE);
}

void
gebr_flow_browse_status_icon(GtkTreeViewColumn *tree_column,
                             GtkCellRenderer *cell,
                             GtkTreeModel *model,
                             GtkTreeIter *iter,
                             gpointer data)
{
	GebrUiFlowBrowseType type;
	gtk_tree_model_get(model, iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	GdkColor color;
	GtkStyle *style = gtk_rc_get_style(gebr.notebook);

	if (type == STRUCT_TYPE_FLOW) {
		GebrGeoXmlFlow * flow;
		GebrUiFlow *ui_flow;
		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		flow = gebr_ui_flow_get_flow(ui_flow);

		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

		if (gebr_geoxml_flow_validate(flow, gebr.validator, NULL))
			g_object_set(cell, "stock-id", "flow-icon", NULL);
		else
			g_object_set(cell, "stock-id", GTK_STOCK_DIALOG_WARNING, NULL);

		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

		g_object_set(cell, "cell-background-gdk", &(style->bg[GTK_STATE_NORMAL]), NULL);
	}
	else if (type == STRUCT_TYPE_PROGRAM) {
		GebrUiFlowProgram *ui_program;
		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &ui_program,
		                   -1);

		GebrGeoXmlProgram *program = gebr_ui_flow_program_get_xml(ui_program);
		const gchar *icon = gebr_gui_get_program_icon(program);

		g_object_set(cell, "stock-id", icon, NULL);
		gtk_style_lookup_color(style, "base_color", &color);
		g_object_set(cell, "cell-background-gdk", &color, NULL);
	}
	else if (type == STRUCT_TYPE_IO) {
		GebrUiFlowsIo *ui_io;
		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &ui_io,
		                   -1);

		const gchar *icon = gebr_ui_flows_io_get_stock_id(ui_io);

		g_object_set(cell, "stock-id", icon, NULL);
		gtk_style_lookup_color(style, "base_color", &color);
		g_object_set(cell, "cell-background-gdk", &color, NULL);
	}
}

void
gebr_flow_browse_text(GtkTreeViewColumn *tree_column,
                      GtkCellRenderer *cell,
                      GtkTreeModel *model,
                      GtkTreeIter *iter,
                      gpointer data)
{
	GebrUiFlowBrowseType type;
	gtk_tree_model_get(model, iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	gchar *title = NULL;
	GdkColor color;
	GtkStyle *style = gtk_rc_get_style(gebr.notebook);

	if (type == STRUCT_TYPE_FLOW) {
		GebrGeoXmlFlow * flow;
		GebrUiFlow *ui_flow;

		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		flow = gebr_ui_flow_get_flow(ui_flow);
		gchar *_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));
		gboolean is_selected = gebr_ui_flow_get_is_selected(ui_flow);

		if (is_selected)
			title = g_markup_printf_escaped("<b>%s</b>", _title);
		else
			title = g_strdup(_title);

		g_free(_title);

		g_object_set(cell, "sensitive", TRUE, NULL);
		g_object_set(cell, "editable", FALSE, NULL);
		g_object_set(cell, "cell-background-gdk", &(style->bg[GTK_STATE_NORMAL]), NULL);
	}
	else if (type == STRUCT_TYPE_IO) {

		GebrUiFlowsIo *io;
		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &io,
		                   -1);

		title = g_strdup(gebr_ui_flows_io_get_value(io));

		if (!title || !*title) {
			g_free(title);
			title = g_strdup(gebr_ui_flows_io_get_label_markup(io));
		}

		if (gebr_ui_flows_io_get_active(io)) {
			g_object_set(cell, "sensitive", TRUE, NULL);
			g_object_set(cell, "editable", TRUE, NULL);
		} else {
			g_object_set(cell, "sensitive", FALSE, NULL);
			g_object_set(cell, "editable", FALSE, NULL);
			g_free(title);
			title = g_strdup(gebr_ui_flows_io_get_label_markup(io));
		}
		gtk_style_lookup_color(style, "base_color", &color);
		g_object_set(cell, "cell-background-gdk", &color, NULL);
	}
	else if (type == STRUCT_TYPE_PROGRAM) {
		GebrUiFlowProgram *program;
		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &program,
		                   -1);

		GebrGeoXmlProgram *prog = gebr_ui_flow_program_get_xml(program);

		title = gebr_geoxml_program_get_title(prog);

		g_object_set(cell, "sensitive", TRUE, NULL);
		g_object_set(cell, "editable", FALSE, NULL);
		gtk_style_lookup_color(style, "base_color", &color);
		g_object_set(cell, "cell-background-gdk", &color, NULL);
	}

	g_object_set(cell, "markup", title, NULL);

	g_free(title);
}

void
gebr_flow_browse_action_icon (GtkTreeViewColumn *tree_column,
                              GtkCellRenderer *cell,
                              GtkTreeModel *model,
                              GtkTreeIter *iter,
                              gpointer data)
{
	GebrGeoXmlFlow * flow;
	GebrUiFlowBrowseType type;
	gtk_tree_model_get(model, iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	GdkColor color;
	GtkStyle *style = gtk_rc_get_style(gebr.notebook);

	if (type == STRUCT_TYPE_FLOW) {
		GebrUiFlow *ui_flow;
		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		flow = gebr_ui_flow_get_flow(ui_flow);

		if (gebr_geoxml_flow_get_revisions_number(flow) > 0)
			g_object_set(cell, "stock-id", "photos", NULL);
		else
			g_object_set(cell, "stock-id", NULL, NULL);
		g_object_set(cell, "sensitive", TRUE, NULL);
		g_object_set(cell, "cell-background-gdk", &(style->bg[GTK_STATE_NORMAL]), NULL);
	}
	else if (type == STRUCT_TYPE_IO) {
		g_object_set(cell, "stock-id", GTK_STOCK_DIRECTORY, NULL);
		gtk_style_lookup_color(style, "base_color", &color);
		g_object_set(cell, "cell-background-gdk", &color, NULL);

		GebrUiFlowsIo *ui_io;
		gtk_tree_model_get(model, iter,
		                   FB_STRUCT, &ui_io,
		                   -1);
		if (gebr_ui_flows_io_get_active(ui_io))
			g_object_set(cell, "sensitive", TRUE, NULL);
		else
			g_object_set(cell, "sensitive", FALSE, NULL);
	}
	else if (type == STRUCT_TYPE_PROGRAM) {
		g_object_set(cell, "stock-id", NULL, NULL);
		gtk_style_lookup_color(style, "base_color", &color);
		g_object_set(cell, "cell-background-gdk", &color, NULL);
	}
}

static void
flow_browse_open_activated(GebrUiFlowBrowse *fb,
                           GebrUiFlowsIo *ui_io,
                           const gchar *path)
{
	gchar *entry_text;

	GtkFileChooserAction action;
	gchar *stock;
	gchar *title = NULL;

	GebrUiFlowsIoType type = gebr_ui_flows_io_get_io_type(ui_io);

	if (type == GEBR_IO_TYPE_INPUT) {
		entry_text = gebr_geoxml_flow_io_get_input(gebr.flow);

		action = GTK_FILE_CHOOSER_ACTION_OPEN;
		stock = GTK_STOCK_OPEN;
		title = g_strdup(_("Choose an input file"));
	} else {
		action = GTK_FILE_CHOOSER_ACTION_SAVE;
		stock = GTK_STOCK_SAVE;
		if (type == GEBR_IO_TYPE_OUTPUT) {
			entry_text = gebr_geoxml_flow_io_get_output(gebr.flow);

			title = g_strdup(_("Choose an output file"));
		}
		else {
			entry_text = gebr_geoxml_flow_io_get_error(gebr.flow);
			title = g_strdup(_("Choose a log file"));
		}
	}
	if (!title)
		title = g_strdup(_("Choose file"));

	GtkWidget *dialog = gtk_file_chooser_dialog_new(title, GTK_WINDOW(gebr.window), action,
							stock, GTK_RESPONSE_YES,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
	gchar *prefix = gebr_maestro_server_get_sftp_prefix(maestro);

	gchar *new_text;
	gint response = gebr_file_chooser_set_remote_navigation(dialog, entry_text,
	                                                        prefix, paths, TRUE,
	                                                        &new_text);
	if (response == GTK_RESPONSE_YES) {
		g_object_set(fb->text_renderer, "text", new_text, NULL);

		gebr_ui_flows_io_edited(ui_io, new_text);
	}

	g_free(new_text);
	g_free(title);
	g_free(entry_text);
	gebr_pairstrfreev(paths);
}

void
gebr_flow_browse_select_file_column(GtkTreeView *tree_view,
                                    GtkTreeIter *iter,
                                    GebrUiFlowBrowse *fb)
{
	GebrUiFlowsIo *ui_io;

	gtk_tree_model_get(GTK_TREE_MODEL(fb->store), iter,
	                   FB_STRUCT, &ui_io,
	                   -1);

	if (!gebr_ui_flows_io_get_active(ui_io))
		return;

	GtkTreePath *path;
	GtkTreeViewColumn *column;
	gtk_tree_view_get_cursor(tree_view, &path, &column);

	if (!path || !column)
		return;

	gint pos, wid;
	if(!gtk_tree_view_column_cell_get_position(column, fb->action_renderer, &pos, &wid))
		return;

	gchar *path_str = gtk_tree_path_to_string(path);

	gtk_tree_view_unset_rows_drag_source(tree_view);

	flow_browse_open_activated(fb, ui_io, path_str);

	gtk_tree_model_row_changed(GTK_TREE_MODEL(fb->store), path, iter);

	g_free(path_str);
	gtk_tree_path_free(path);
}

void
gebr_flow_browse_select_snapshot_column(GtkTreeView *tree_view,
                                        GtkTreeIter *iter,
                                        GebrUiFlowBrowse *fb)
{
	GebrGeoXmlFlow *flow;
	GebrUiFlow *ui_flow;

	gtk_tree_model_get(GTK_TREE_MODEL(fb->store), iter,
	                   FB_STRUCT, &ui_flow,
	                   -1);

	flow = gebr_ui_flow_get_flow(ui_flow);

	if (!gebr_geoxml_flow_get_revisions_number(flow))
		return;

	GtkTreePath *path;
	GtkTreeViewColumn *column;
	gtk_tree_view_get_cursor(tree_view, &path, &column);

	if (!column || !path)
		return;

	gint pos, wid;
	if(!gtk_tree_view_column_cell_get_position(column, fb->action_renderer, &pos, &wid))
		return;

	gchar *path_str = gtk_tree_path_to_string(path);

	gtk_tree_view_unset_rows_drag_source(tree_view);

	gtk_toggle_button_set_active(fb->snapshots_ctx_button, TRUE);

	g_free(path_str);
	gtk_tree_path_free(path);
}

void
gebr_flow_browse_cursor_changed(GtkTreeView *tree_view,
                                GebrUiFlowBrowse *fb)
{
	gint nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view)));
	if (nrows > 1)
		return;

	GtkTreeIter iter;
	if (!flow_browse_get_selected(&iter, TRUE))
		return;

	gebr_gui_gtk_tree_view_set_drag_source_dest(tree_view);

	GebrUiFlowBrowseType type;
	gtk_tree_model_get(GTK_TREE_MODEL(fb->store), &iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	if (type == STRUCT_TYPE_FLOW)
		gebr_flow_browse_select_snapshot_column(tree_view, &iter, fb);

	else if (type == STRUCT_TYPE_IO)
		gebr_flow_browse_select_file_column(tree_view, &iter, fb);
}

void
gebr_flow_browse_append_job_on_flow(GebrGeoXmlFlow *flow,
                                    const gchar *job_id,
                                    GebrUiFlowBrowse *fb)
{
	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));

	GList *jobs = NULL;

	jobs = g_hash_table_lookup(fb->flow_jobs, flow_id);

	if (g_list_length(jobs) == 20) {
		GList *last_job = g_list_last(jobs);
		jobs = g_list_remove_link(jobs, last_job);
	}
	jobs = g_list_prepend(jobs, (gchar*)job_id);

	g_hash_table_insert(fb->flow_jobs, g_strdup(flow_id), g_list_copy(jobs));

	g_list_free(jobs);
}

GList *
gebr_flow_browse_get_jobs_from_flow(GebrGeoXmlFlow *flow,
                                    GebrUiFlowBrowse *fb)
{
	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));
	GList *jobs;

	jobs = g_hash_table_lookup(fb->flow_jobs, flow_id);

	return jobs;
}

void
gebr_flow_browse_update_jobs_info(GebrGeoXmlFlow *flow,
                                  GebrUiFlowBrowse *fb,
                                  gint n_max)
{
	on_dismiss_clicked(NULL, fb);

	gint i = 0;
	GList *jobs = gebr_flow_browse_get_jobs_from_flow(flow, fb);
	for (GList *job = jobs; job && i < n_max; job = job->next, i++)
		gebr_flow_browse_info_job(gebr.ui_flow_browse, job->data);
}

void
gebr_flow_browse_reset_jobs_from_flow(GebrGeoXmlFlow *flow,
                                      GebrUiFlowBrowse *fb)
{
	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));

	g_hash_table_insert(fb->flow_jobs, g_strdup(flow_id), NULL);

	on_dismiss_clicked(NULL, fb);
}

void
gebr_flow_browse_load_parameters_review(GebrGeoXmlFlow *flow,
                                        GebrUiFlowBrowse *fb,
                                        gboolean same_flow)
{
	if (!flow)
		return;

	if (same_flow) {
		if (!gtk_widget_get_visible(gebr.ui_flow_browse->context[CONTEXT_JOBS]) &&
		    !gtk_widget_get_visible(gebr.ui_flow_browse->context[CONTEXT_MENU])) {
			if (gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
				gebr_flow_browse_define_context_to_show(CONTEXT_FLOW, gebr.ui_flow_browse);
			else
				gebr_flow_browse_define_context_to_show(CONTEXT_SNAPSHOTS, gebr.ui_flow_browse);
		}
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gebr.ui_flow_browse->properties_ctx_button), TRUE);
		gebr_flow_browse_define_context_to_show(CONTEXT_FLOW, gebr.ui_flow_browse);
	}

	GString *prog_content = g_string_new("");
	g_string_append_printf(prog_content, "<html>\n"
					     "  <head>\n"
					     "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
	g_string_append_printf(prog_content, "    <link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/gebr-report.css\" />"
					     "    <link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/gebr-flow-review.css\" />",
						  LIBGEBR_STYLES_DIR, LIBGEBR_DATA_DIR);

	g_string_append_printf(prog_content, "  </head>\n"
					     "  <body oncontextmenu=\"return false;\"/>\n");

	gebr_flow_generate_io_table(flow, prog_content);
	gebr_flow_generate_parameter_value_table(flow, prog_content, NULL, TRUE);

	g_string_append_printf(prog_content, "  </body>\n"
					     "</html>");
	gebr_gui_html_viewer_widget_show_html(GEBR_GUI_HTML_VIEWER_WIDGET(fb->html_parameters), prog_content->str);

	g_string_free(prog_content, TRUE);
}

void
flow_browse_set_run_widgets_sensitiveness(GebrUiFlowBrowse *fb,
                                          gboolean sensitive,
                                          gboolean maestro_err)
{
	if (gebr_geoxml_line_get_flows_number(gebr.line) <= 0){
		gtk_widget_set_can_focus (fb->view,FALSE);
		gtk_widget_grab_focus(gtk_widget_get_parent(GTK_WIDGET(fb->view)));
		if (sensitive)
			sensitive = FALSE;
	}
	else
		gtk_widget_set_can_focus (fb->view,TRUE);

	const gchar *tooltip_disconn;
	const gchar *tooltip_execute;

	if (!gebr.line) {
		if (!gebr.project)
			tooltip_disconn = _("Select a line to execute a flow");
		else
			tooltip_disconn = _("Select a line of this project to execute a flow");
	} else {
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
		if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED)
			tooltip_disconn = _("The Maestro of this line is disconnected.\nConnecting it to execute a flow.");
		else if (gebr_geoxml_line_get_flows_number(gebr.line) == 0)
			tooltip_disconn = _("This line does not contain flows\nCreate a flow to execute this line");
		else if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0)
			tooltip_disconn = _("This flow does not contain programs\nAdd at least one to execute this flow");
		else
			tooltip_disconn = _("Execute");
	}
	tooltip_execute = _("Execute");

	GtkAction *action = gtk_action_group_get_action(gebr.action_group_flow, "flow_execute");
	GtkAction *detailed_execution = gtk_action_group_get_action(gebr.action_group_flow, "flow_execute_details");
	const gchar *tooltip = sensitive ? tooltip_execute : tooltip_disconn;

	gtk_action_set_stock_id(action, "gtk-execute");
	gtk_action_set_sensitive(action, sensitive);
	gtk_action_set_tooltip(action, tooltip);
	gtk_action_set_sensitive(detailed_execution, sensitive);
	gtk_action_set_tooltip(detailed_execution, tooltip);
}

void
gebr_flow_browse_define_context_to_show(GebrUiFlowBrowseContext current_context,
                                        GebrUiFlowBrowse *fb)
{
	gtk_widget_show(fb->context[current_context]);

	for (gint i = CONTEXT_FLOW; i < CONTEXT_N_TYPES; i++) {
		if (i == current_context)
			continue;

		gtk_widget_hide(fb->context[i]);
	}
}

void
gebr_flow_browse_escape_context(GebrUiFlowBrowse *fb)
{
	if (gtk_widget_get_visible(fb->context[CONTEXT_FLOW]))
		return;

	if (gtk_widget_get_visible(fb->context[CONTEXT_PARAMETERS])) {
		if (gebr.ui_flow_browse->program_edit)
		save_parameters(gebr.ui_flow_browse->program_edit);
	}
	else if (gtk_widget_get_visible(fb->context[CONTEXT_JOBS])) {
		flow_browse_info_update();
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fb->properties_ctx_button)))
		gebr_flow_browse_define_context_to_show(CONTEXT_FLOW, fb);
	else
		gebr_flow_browse_define_context_to_show(CONTEXT_SNAPSHOTS, fb);
}

void
gebr_flow_browse_block_changed_signal(GebrUiFlowBrowse *fb)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb->view));
	g_signal_handlers_block_by_func(selection, flow_browse_load, NULL);
}

void
gebr_flow_browse_unblock_changed_signal(GebrUiFlowBrowse *fb)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb->view));
	g_signal_handlers_unblock_by_func(selection, flow_browse_load, NULL);
}

void flow_edition_component_activated(void)
{
	GtkTreeIter iter;
	gchar *title;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (!flow_browse_get_selected(&iter, TRUE))
		return;

	GebrUiFlowBrowseType type;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	if (type != STRUCT_TYPE_PROGRAM)
		return;

	GebrUiFlowProgram *ui_program;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
	                   FB_STRUCT, &ui_program,
	                   -1);

	GebrGeoXmlProgram *program = gebr_ui_flow_program_get_xml(ui_program);
	title = gebr_geoxml_program_get_title(program);

	gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Configuring program '%s'."), title);

	GebrGuiProgramEdit *program_edit = parameters_configure_setup_ui();

	if (program_edit) {
		gebr_ui_flow_program_set_flag_opened(ui_program, FALSE);

		GtkTreeIter parent;
		gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &parent, &iter);
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &parent);
		gtk_tree_model_row_changed(GTK_TREE_MODEL(gebr.ui_flow_browse->store), path, &parent);
		gtk_tree_path_free(path);

		if (gebr.ui_flow_browse->program_edit)
			gebr_gui_program_edit_destroy(gebr.ui_flow_browse->program_edit);

		gebr.ui_flow_browse->program_edit = program_edit;

		GList *children = gtk_container_get_children(GTK_CONTAINER(gebr.ui_flow_browse->context[CONTEXT_PARAMETERS]));
		if (children)
			gtk_widget_destroy(children->data);

		gtk_container_add(GTK_CONTAINER(gebr.ui_flow_browse->context[CONTEXT_PARAMETERS]), program_edit->widget);
		gebr_flow_browse_define_context_to_show(CONTEXT_PARAMETERS, gebr.ui_flow_browse);
	}
	g_free(title);
}
