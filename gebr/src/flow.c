/*   G�BR - An environment for seismic processing.
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

/*
 * File: flow.c
 * Flow manipulation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <geoxml.h>
#include <comm.h>

#include "flow.h"
#include "gebr.h"
#include "support.h"
#include "document.h"
#include "server.h"
#include "ui_flow_browse.h"

/* errors strings. */
extern gchar * no_line_selected_error;
gchar * no_flow_selected_error =	_("No flow selected");
gchar * no_program_selected_error =	_("No program selected");

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: flow_save
 * Save the current flow
 */
void
flow_save(void)
{
	/* TODO: report errors on document_save */
	document_save(GEOXML_DOC(gebr.flow));
	flow_browse_info_update();
}

/*
 * Function: flow_export
 * Export current flow to a file
 */
void
flow_export(void)
{
	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;
	gchar *			filename;
	gchar *                 oldfilename;

	if (gebr.flow == NULL) {
		gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose filename to save"), NULL,
						     GTK_FILE_CHOOSER_ACTION_SAVE,
						     GTK_STOCK_SAVE, GTK_RESPONSE_YES,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     NULL);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(filefilter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (chooser_dialog));
	filename = g_path_get_basename(path);

	/* export current flow to disk */
	oldfilename = (gchar*)geoxml_document_get_filename(GEOXML_DOC(gebr.flow));
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), filename);
	geoxml_document_save(GEOXML_DOC(gebr.flow), path);
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), oldfilename);

	gebr_message(INFO, TRUE, TRUE, _("Flow '%s' exported to %s"),
		     (gchar*)geoxml_document_get_title(GEOXML_DOC(gebr.flow)), path);

	g_free(path);
	g_free(filename);

out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: flow_import
 * Import flow from file to the current line
 */
void
flow_import(void)
{
	GtkTreeSelection *	selection;
	GtkTreeIter		iter;

	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;

	gchar *			flow_title;
	GString *		flow_filename;

	GeoXmlFlow *		imported_flow;

	if (gebr.line == NULL) {
		gebr_message(ERROR, TRUE, FALSE, _("Select a line to which a flow will be added to"));
		return;
	}

	/* assembly a file chooser dialog */
	chooser_dialog = gtk_file_chooser_dialog_new( _("Choose filename to save"), NULL,
						      GTK_FILE_CHOOSER_ACTION_OPEN,
						      GTK_STOCK_OPEN, GTK_RESPONSE_YES,
						      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						      NULL);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(filefilter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	/* load flow */
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	imported_flow = GEOXML_FLOW(document_load_path(path));
	if (imported_flow == NULL)
		goto out2;

	/* initialization */
	flow_title = (gchar *)geoxml_document_get_title(GEOXML_DOC(imported_flow));
	flow_filename = document_assembly_filename("flw");

	/* feedback */
	gebr_message(INFO, TRUE, TRUE, _("Flow '%s' imported to line '%s' from file '%s'"),
		     flow_title, geoxml_document_get_title(GEOXML_DOC(gebr.line)), path);

	/* change filename */
	geoxml_document_set_filename(GEOXML_DOC(imported_flow), flow_filename->str);
	document_save(GEOXML_DOC(imported_flow));
	geoxml_document_free(GEOXML_DOC(imported_flow));
	/* and add it to the line */
	geoxml_line_add_flow(gebr.line, flow_filename->str);
	document_save(GEOXML_DOC(gebr.line));
	/* and to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_TITLE, flow_title,
			   FB_FILENAME, flow_filename->str,
			   -1);

	/* select it */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebr.ui_flow_browse->view));
	gtk_tree_selection_select_iter (selection, &iter);
	g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");

	/* frees */
	g_string_free(flow_filename, TRUE);
out2:	g_free(path);
out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: flow_new
 * Create a new flow
 */
void
flow_new(void)
{
	GtkTreeSelection *	selection;
	GtkTreeIter		iter;

	gchar *			flow_title;
	gchar *			line_title;
	gchar *			line_filename;

	GeoXmlFlow *		flow;

	if (gebr.line == NULL) {
		gebr_message(ERROR, TRUE, FALSE, _("Select a line to which a flow will be added to"));
		return;
	}

	flow_title = _("New Flow");
	line_title = (gchar *)geoxml_document_get_title(gebr.doc);
	line_filename = (gchar *)geoxml_document_get_filename(gebr.doc);

	/* Create a new flow */
	flow = GEOXML_FLOW(document_new(GEOXML_DOCUMENT_TYPE_FLOW));
	geoxml_document_set_title(GEOXML_DOC(flow), flow_title);
	geoxml_document_set_author(GEOXML_DOC(flow), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(flow), gebr.config.email->str);
	/* and add to current line */
	geoxml_line_add_flow(gebr.line, geoxml_document_get_filename(GEOXML_DOC(flow)));
	document_save(GEOXML_DOC(gebr.line));
	document_save(GEOXML_DOC(flow));
	geoxml_document_free(GEOXML_DOC(flow));
	/* and add to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_TITLE, flow_title,
			   FB_FILENAME, geoxml_document_get_filename(GEOXML_DOC(flow)),
			   -1);

	/* select it */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_tree_selection_select_iter(selection, &iter);
	g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");

	/* feedback */
	gebr_message(INFO, TRUE, TRUE, _("New flow added to line '%s'"), line_title);
}

/* Function: flow_free
 * Frees the memory allocated to a flow
 *
 * Besides, update the detailed view of a flow in the interface.
 */
void
flow_free(void)
{
	if (gebr.flow != NULL) {
		gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
		geoxml_document_free(GEOXML_DOC(gebr.flow));
		gebr.flow = NULL;

		flow_browse_info_update();
	}
}

/*
 * Function: flow_delete
 * Delete the selected flow in flow browser
 *
 */
void
flow_delete(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		flow_iter;

	gchar *			title;
	gchar *			filename;

	GeoXmlLineFlow *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (gtk_tree_selection_get_selected(selection, &model, &flow_iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &flow_iter,
			   FB_TITLE, &title,
			   FB_FILENAME, &filename,
			   -1);

	if (confirm_action_dialog(_("Are you want to delete flow '%s'?"), title) == FALSE)
		goto out;

	/* Some feedback */
	gebr_message(INFO, TRUE, FALSE, _("Erasing flow '%s'"), title);
	gebr_message(INFO, FALSE, TRUE, _("Erasing flow '%s' from line '%s'"),
		     title, geoxml_document_get_title(gebr.doc));

	/* Seek and destroy */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	while (line_flow != NULL) {
		if (g_ascii_strcasecmp(filename, geoxml_line_get_flow_source(line_flow)) == 0) {
			geoxml_line_remove_flow(gebr.line, line_flow);
			document_save(GEOXML_DOC(gebr.line));
			break;
		}
		geoxml_line_next_flow(&line_flow);
	}

	/* Free and delete flow from the disk */
	flow_free();
	document_delete(filename);

	/* Finally, from the GUI */
	gtk_list_store_remove(GTK_LIST_STORE (gebr.ui_flow_browse->store), &flow_iter);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);

out:	g_free(title);
	g_free(filename);
}

/*
 * Function: flow_run
 * Runs a flow
 */
void
flow_run(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel     *	model;
	GtkTreeIter		first_iter;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	GtkWidget *		dialog;
	GtkWidget *		view;

	gboolean		has_first;
	struct server *		server;

	/* check for a flow selected */
	if (gebr.flow == NULL) {
		gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	has_first = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->store), &first_iter);
	if (!has_first) {
		gebr_message(ERROR, TRUE, FALSE,
			_("There is no servers available. Please add at least one at Configure/Server"));
		return;
	}

	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr.ui_server_list->store), NULL) == 1){
		gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_server_list->store), &first_iter,
			       SERVER_POINTER, &server,
			       -1);
		server_run_flow(server);
		return;
	}

	dialog = gtk_dialog_new_with_buttons(_("Choose server to run"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	gtk_widget_set_size_request(dialog, 380, 300);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL (gebr.ui_server_list->store));

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Servers"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id  (col, SERVER_ADDRESS);
	gtk_tree_view_column_set_sort_indicator  (col, TRUE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
	gtk_tree_view_column_add_attribute (col, renderer, "text", SERVER_ADDRESS);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), view, TRUE, TRUE, 0);

	/* select the first server */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_select_iter(selection, &first_iter);

	gtk_widget_show_all(dialog);
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_OK: {
		GtkTreeIter       	iter;

		gtk_tree_selection_get_selected(selection, &model, &iter);
		gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
				SERVER_POINTER, &server,
				-1);

		server_run_flow(server);

		break;
	}
	case GTK_RESPONSE_CANCEL:
		break;
	}
	gtk_widget_destroy(dialog);
}

/*
 * Function: flow_program_remove
 * Remove selected program from flow process
 */
void
flow_program_remove(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	GeoXmlProgram *		program;
	gulong 			nprogram;
	gchar *			node;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}

	node = gtk_tree_model_get_string_from_iter(model, &iter);
	nprogram = (gulong) atoi(node);
	g_free(node);

	geoxml_flow_get_program(gebr.flow, &program, nprogram);
	geoxml_flow_remove_program(gebr.flow, program);

	flow_save();
	gtk_list_store_remove (GTK_LIST_STORE (gebr.ui_flow_edition->fseq_store), &iter);
}

/*
 * Function: flow_program_move_up
 * Move selected program up in the processing flow
 */
void
flow_program_move_up(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreePath *		previous_path;
	GtkTreeIter 		previous;
	GeoXmlProgram *		program;
	gulong 			nprogram;
	gchar *			node;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter);
	if (gtk_tree_path_prev(previous_path) == FALSE)
		goto out;

	/* Get the index. */
	node = gtk_tree_model_get_string_from_iter(model, &iter);
	nprogram = (gulong) atoi(node);
	g_free(node);

	/* XML change */
	geoxml_flow_get_program(gebr.flow, &program, nprogram);
	geoxml_flow_move_program_up(gebr.flow, program);
	flow_save();

	/* View change */
	gtk_tree_model_get_iter (GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store),
				&previous, previous_path);
	gtk_list_store_move_before (gebr.ui_flow_edition->fseq_store, &iter, &previous);

out:	gtk_tree_path_free(previous_path);
}

/*
 * Function: flow_program_move_down
 * Move selected program down in the processing flow
 */
void
flow_program_move_down(void)
{
	GtkTreeIter		iter, next;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GeoXmlProgram *		program;
	gulong			nprogram;
	gchar *			node;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	next = iter;
	if (gtk_tree_model_iter_next ( GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store), &next) == FALSE)
		return;

	/* Get index */
	node = gtk_tree_model_get_string_from_iter(model, &iter);
	nprogram = (gulong) atoi(node);
	g_free(node);

	/* Update flow */
	geoxml_flow_get_program(gebr.flow, &program, nprogram);
	geoxml_flow_move_program_down(gebr.flow, program);
	flow_save();

	/* Update GUI */
	gtk_list_store_move_after (gebr.ui_flow_edition->fseq_store, &iter, &next);
}
