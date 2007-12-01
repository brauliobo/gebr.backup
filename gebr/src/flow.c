/*   G�BR - An environment for seismic processing.
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

/*
 * File: cb_flow.c
 * Callbacks for the flows manipulation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <comm.h>

#include "flow.h"
#include "gebr.h"
#include "support.h"
#include "document.h"
#include "ui_flow_browse.h"

/* errors strings. */
extern gchar * no_line_selected_error;
gchar * no_flow_selected_error =	_("No flow selected");
gchar * no_program_selected_error =	_("No program selected");

/*
 * Function: flow_load
 * Load a selected flow from file
 */
void
flow_load(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	gchar *			filename;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.flow_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		if (flow != NULL) {
			gtk_list_store_clear(gebr.ui_flow_edition.fseq_store);
			geoxml_document_free (GEOXML_DOC (flow));
			flow = NULL;
		}
		return;
	}

	/* free previous flow */
	if (flow != NULL)
		geoxml_document_free(GEOXML_DOC(gebr.flow));
	gtk_list_store_clear(gebr.ui_flow_edition.fseq_store);

	gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_flow_browse.store), &iter,
				FB_FILENAME, &filename,
				-1);

	path = document_get_path(filename);
	flow = document_load(path->str);
	g_string_free(path, TRUE);

	if (flow != NULL) {
		flow_add_programs_to_view (flow);
	}

out:	g_free(filename);
}

/*
 * Function: flow_save
 * Save the current flow
 */
int
flow_save(void)
{
	document_save(GEOXML_DOC(gebr.flow));
	flow_browse_info_update();
}

/*
 * Fucntion: flow_export
 * Export current flow to a file
 */
void
flow_export(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel     *	model;
	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;
	gchar *			filename;
	gchar *                 oldfilename;

	if (flow == NULL) {
		gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(	_("Choose filename to save"), NULL,
							GTK_FILE_CHOOSER_ACTION_SAVE,
							GTK_STOCK_SAVE, GTK_RESPONSE_YES,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							NULL);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Flow files (*.flow)"));
	gtk_file_filter_add_pattern(filefilter, "*.flow");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;
	path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser_dialog));
	filename = g_path_get_basename(path);

	/* export current flow to disk */
	oldfilename = geoxml_document_get_filename(GEOXML_DOC(gebr.flow));
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), filename);
	geoxml_document_save(GEOXML_DOC(gebr.flow), path);
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), oldfilename);

	g_free(path);
	g_free(filename);

out:
	gtk_widget_destroy(chooser_dialog);
}

void
flow_free(void)
{
	if (gebr.flow != NULL) {
		geoxml_document_free(GEOXML_DOC(gebr.flow));
		gebr.flow = NULL;
	}
	flow_browse_info_update();
}

void
flow_add_programs_to_view   (GeoXmlFlow * f)
{
	GeoXmlProgram *program;

	for (geoxml_flow_get_program(f, &program, 0); program != NULL; geoxml_program_next(&program)) {
		gchar  *menu;
		gulong  prog_index;

		geoxml_program_get_menu(program, &menu, &prog_index);

		/* Add to the GUI */
		GtkTreeIter piter;
		gtk_list_store_append (gebr.ui_flow_edition.fseq_store, &piter);
		gtk_list_store_set (gebr.ui_flow_edition.fseq_store, &piter,
				FSEQ_TITLE_COLUMN, geoxml_program_get_title(program),
				FSEQ_MENU_FILE_NAME_COLUMN, menu,
				FSEQ_MENU_INDEX, prog_index,
				-1);

		{
		   GdkPixbuf    *pixbuf;

		   if (strcmp(geoxml_program_get_status(program), "unconfigured") == 0)
		      pixbuf = gebr.pixmaps.unconfigured_icon;
		   else if (strcmp(geoxml_program_get_status(program), "configured") == 0)
		      pixbuf = gebr.pixmaps.configured_icon;
		   else if (strcmp(geoxml_program_get_status(program), "disabled") == 0)
		      pixbuf = gebr.pixmaps.disabled_icon;
		   else
		      continue;

		   gtk_list_store_set (gebr.ui_flow_edition.fseq_store, &piter,
				       FSEQ_STATUS_COLUMN, pixbuf, -1);
		}
	}
}

/*
 * Function: flow_new
 * Create a new flow
 */
void
flow_new(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	gchar *			name;
	gchar *			line_filename;
	GString *		flow_filename;
	gchar *			title;
	GeoXmlLine *		line;
	GeoXmlFlow *		flow;
	GString *		message;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.proj_line_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, _("Select a line to which a flow will be added to"));
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.proj_line_store), &iter,
		PL_TITLE, &name,
		PL_FILENAME, &line_filename,
		-1);
	title = _("New Flow");
	flow_filename = document_assembly_filename(".flw");

	if (gtk_tree_store_iter_depth(gebr.proj_line_store, &iter) < 1) {
		gebr_message(ERROR, TRUE, FALSE, no_line_selected_error);
		goto out;
	}

	/* feedback */
	message = g_string_new(NULL);
	g_string_printf(message, _("Adding flow to line %s"), name);
	gebr_message(ERROR, TRUE, FALSE, message->str);
	g_string_free(message, TRUE);

	/* Add flow to the line */
	line = document_load(line_filename);
	if (line == NULL) {
		gebr_message(ERROR, TRUE, TRUE, _("Could not load associated line"));
		goto out;
	}
	geoxml_line_add_flow(line, flow_filename);
	document_save(GEOXML_DOC(line));
	geoxml_document_free(GEOXML_DOC(line));

	/* Create a new flow */
	flow = geoxml_flow_new();
	geoxml_document_set_filename(GEOXML_DOC(flow), flow_filename);
	geoxml_document_set_title(GEOXML_DOC(flow), title);
	geoxml_document_set_author(GEOXML_DOC(flow), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(flow), gebr.config.email->str);
	document_save(GEOXML_DOC(flow));
	geoxml_document_free(GEOXML_DOC(flow));

	/* Add to the GUI */
	gtk_list_store_append (gebr.ui_flow_browse.store, &iter);
	gtk_list_store_set (gebr.ui_flow_browse.store, &iter,
			FB_TITLE, title,
			FB_FILENAME, flow_filename,
			-1);

out:	g_free(line_name);
	g_free(line_filename);
	g_string_free(flow_filename, TRUE);
}

/*
 * Function: flow_delete
 * Delete the selected flow in flow browser
 *
 */
void
flow_delete(void)
{
   GtkTreeSelection *selection;
   GtkTreeIter       iter, liter;
   GtkTreeModel     *model;

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.flow_view));
   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

      gchar *name, *flow_filename, *line_filename;
      GString *line_path;
      GString *flow_path;
      GString *message;

      gtk_tree_model_get ( GTK_TREE_MODEL(gebr.ui_flow_browse.store), &iter,
			   FB_TITLE, &name,
			   FB_FILENAME, &flow_filename,
			   -1);

      /* Get the line filename */
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.proj_line_view));
      gtk_tree_selection_get_selected (selection, &model, &liter);
      gtk_tree_model_get ( model, &liter,
			   PL_FILENAME, &line_filename,
			   -1);

      /* Some feedback */
      message = g_string_new(NULL);
      g_string_printf (message, "Erasing flow %s", name);
      gebr_message(ERROR, TRUE, FALSE, message->str);
      g_string_free(message, TRUE);

      data_fname(line_filename, &line_path);
      data_fname(flow_filename, &flow_path);

      /* Remove flow from its line */
      GeoXmlLine     *line;
      GeoXmlLineFlow *line_flow;
      line = line_load(line_path->str);
      if (line == NULL) {
         printf("FIXME: %s:%d", __FILE__, __LINE__);
	 g_string_free(line_path, TRUE);
	 g_string_free(flow_path, TRUE);
         goto out;
      }

      /* Seek and destroy */
      geoxml_line_get_flow(line, &line_flow, 0);
      while (line_flow != NULL) {
         if (g_ascii_strcasecmp(flow_filename, geoxml_line_get_flow_source(line_flow)) == 0) {
            geoxml_line_remove_flow(line, line_flow);
            geoxml_document_save(GEOXML_DOC(line), line_path->str);
            break;
         }

	 geoxml_line_next_flow(&line_flow);
      }
      geoxml_document_free(GEOXML_DOC(line));

      /* Frees and delete flow from the disk */
      flow_free();
      unlink(flow_path->str);
      g_string_free(line_path, TRUE);
      g_string_free(flow_path, TRUE);

      /* Finally, from the GUI */
      gtk_list_store_remove (GTK_LIST_STORE (gebr.ui_flow_browse.store), &iter);
      gtk_list_store_clear(gebr.ui_flow_edition.fseq_store);

out:
      g_free(name);
      g_free(flow_filename);
      g_free(line_filename);
   } else
      gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
}

/*
 * Function: flow_rename
 * Rename a flow upon double click.
 */
void
flow_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text)
{
	GtkTreeIter	iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (gebr.ui_flow_browse.store),
					&iter,
					path_string);
	gtk_list_store_set (gebr.ui_flow_browse.store, &iter,
			FB_TITLE, new_text,
			-1);

	/* Update XML */
	geoxml_document_set_title (GEOXML_DOC(gebr.flow), new_text);
	flow_save();
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

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.fseq_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}

	node = gtk_tree_model_get_string_from_iter(model, &iter);
	nprogram = (gulong) atoi(node);
	g_free(node);

	geoxml_flow_get_program(gebr.flow, &program, nprogram);
	geoxml_flow_remove_program(gebr.flow, program);

	flow_save();
	gtk_list_store_remove (GTK_LIST_STORE (gebr.ui_flow_edition.fseq_store), &iter);
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

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.fseq_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	next = iter;
	if (gtk_tree_model_iter_next ( GTK_TREE_MODEL (gebr.ui_flow_edition.fseq_store), &next) == FALSE)
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
	gtk_list_store_move_after (gebr.ui_flow_edition.fseq_store, &iter, &next);
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

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.fseq_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gebr_message(ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_edition.fseq_store), &iter);
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
	gtk_tree_model_get_iter (GTK_TREE_MODEL (gebr.ui_flow_edition.fseq_store),
				&previous, previous_path);
	gtk_list_store_move_before (gebr.ui_flow_edition.fseq_store, &iter, &previous);

out:	gtk_tree_path_free(previous_path);
}

void
flow_run(void)
{
	GtkWidget *		dialog;
	GtkWidget *		view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;
	GtkTreeSelection *	selection;
	GtkTreeModel     *	model;
	GtkTreeIter		iter;
	GtkTreeIter		first_iter;
	gboolean		has_first;
	struct server *		server;

	/* check for a flow selected */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.flow_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	has_first = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list.store), &first_iter);
	if (!has_first) {
		gebr_message(ERROR, TRUE, FALSE,
			_("There is no servers available. Please add at least one at Configure/Server"));
		return;
	}

	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr.ui_server_list.store), NULL) == 1){
	   gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_server_list.store), &first_iter,
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

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL (gebr.ui_server_list.store));

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
		gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_server_list.store), &iter,
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
