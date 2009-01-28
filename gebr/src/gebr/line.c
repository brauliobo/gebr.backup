/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

/* File: line.c
 * Lines manipulation functions
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <gui/utils.h>

#include "line.h"
#include "gebr.h"
#include "support.h"
#include "document.h"
#include "project.h"
#include "flow.h"
#include "callbacks.h"

gchar * no_line_selected_error =		_("No line selected");
gchar * no_selection_error =			_("Nothing selected");

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: line_new
 * Create a new line
 *
 */
gboolean
line_new(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		project_iter, line_iter;
	GtkTreePath *		path;

	gchar *			line_title;
	gchar *			project_filename;
	gchar *                 project_title;

	GeoXmlLine *		line;

	if (gebr.project_line == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_selection_error);
		return FALSE;
	}

	/* get project iter */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (geoxml_document_get_type(gebr.project_line) == GEOXML_DOCUMENT_TYPE_PROJECT)
		gtk_tree_selection_get_selected(selection, &model, &project_iter);
	else {
		gtk_tree_selection_get_selected(selection, &model, &line_iter);
		gtk_tree_model_iter_parent(model, &project_iter, &line_iter);
	}

	/* create it */
	line = GEOXML_LINE(document_new(GEOXML_DOCUMENT_TYPE_LINE));
	line_title = _("New Line");

	/* gtk stuff */
	gtk_tree_model_get(model, &project_iter,
			   PL_TITLE, &project_title,
			   PL_FILENAME, &project_filename,
			   -1);
	gtk_tree_store_append(gebr.ui_project_line->store, &line_iter, &project_iter);
	gtk_tree_store_set(gebr.ui_project_line->store, &line_iter,
			   PL_TITLE, line_title,
			   PL_FILENAME, geoxml_document_get_filename(GEOXML_DOC(line)),
			   -1);

	/* add to project */
	geoxml_project_append_line(gebr.project, geoxml_document_get_filename(GEOXML_DOC(line)));
	document_save(GEOXML_DOC(gebr.project));

	/* set line stuff, save and free */
	geoxml_document_set_title(GEOXML_DOC(line), line_title);
	geoxml_document_set_author(GEOXML_DOC(line), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(line), gebr.config.email->str);
	document_save(GEOXML_DOC(line));
	geoxml_document_free(GEOXML_DOC(line));

	/* feedback */
	gebr_message(LOG_INFO, FALSE, TRUE, _("New line created in project '%s'"), project_title);

	/* UI: select it expand parent */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &line_iter);
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(gebr.ui_project_line->view), path);
	gtk_tree_selection_select_iter(selection, &line_iter);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gebr.ui_project_line->view), path,
				     NULL, FALSE, 0, 0);
	g_signal_emit_by_name(gebr.ui_project_line->view, "cursor-changed");
        on_project_line_properties_activate();

	gtk_tree_path_free(path);
	g_free(project_title);
	g_free(project_filename);

	return TRUE;
}

/*
 * Function: line_delete
 * Delete the selected line
 *
 * TODO: ask the user about erasing all flows associated to this line.
 */
gboolean
line_delete(void)
{
	GtkTreeSelection  *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		line_iter;

	GeoXmlSequence *	project_line;
	GeoXmlSequence *	line_flow;
	const gchar *		line_filename;

	if (gebr.line == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_selection_error);
		return FALSE;
	}
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	gtk_tree_selection_get_selected(selection, &model, &line_iter);

	if (confirm_action_dialog(_("Delete line"), _("Are you sure you want to delete line '%s' and all its flows?"),
		geoxml_document_get_title(GEOXML_DOC(gebr.line))) == FALSE)
		return FALSE;

	/* Removes its flows */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	while (line_flow != NULL) {
		GString *	path;
		const gchar *	flow_source;

		flow_source = geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow));
		path = document_get_path(flow_source);
		g_unlink(path->str);
		g_string_free(path, TRUE);

		/* log action */
		gebr_message(LOG_INFO, FALSE, TRUE, _("Erasing child flow '%s'"), flow_source);

		geoxml_sequence_next(&line_flow);
	}

	/* Remove the line from its project */
	line_filename = geoxml_document_get_filename(GEOXML_DOC(gebr.line));
	geoxml_project_get_line(gebr.project, &project_line, 0);
	while (project_line != NULL) {
		if (strcmp(line_filename, geoxml_project_get_line_source(GEOXML_PROJECT_LINE(project_line))) == 0) {
			geoxml_sequence_remove(project_line);
			document_save(GEOXML_DOC(gebr.project));
			break;
		}

		geoxml_sequence_next(&project_line);
	}

	/* inform the user */
	gebr_message(LOG_INFO, TRUE, FALSE, _("Erasing line '%s'"), geoxml_document_get_title(GEOXML_DOC(gebr.line)));
	gebr_message(LOG_INFO, FALSE, TRUE, _("Erasing line '%s' from project '%s'"),
		     geoxml_document_get_title(GEOXML_DOC(gebr.line)),
		     geoxml_document_get_title(GEOXML_DOC(gebr.project)));

	/* finally, remove it from the disk */
	document_delete(line_filename);
	/* and from the GUI */
	gtk_tree_view_select_sibling(GTK_TREE_VIEW(gebr.ui_project_line->view));
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), &line_iter);
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	flow_free();
	project_line_free();
	project_line_info_update();

	return TRUE;
}

/* Function: line_save
 * Save current line
 */
void
line_save(void)
{
	document_save(GEOXML_DOC(gebr.line));
}

/*
 * Function: line_load_flows
 * Load flows associated to the selected line.
 * Called only by project_line_load
 */
void
line_load_flows(void)
{
	GtkTreeIter		flow_iter;
	GeoXmlSequence *	line_flow;

	/* reset flow parts of GUI */
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	flow_free();

	/* iterate over its flows */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	while (line_flow != NULL) {
		GeoXmlFlow *	flow;
		gchar *		flow_filename;

		flow_filename = (gchar*)geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow));
		flow = GEOXML_FLOW(document_load(flow_filename));
		if (flow == NULL) {
			gebr_message(LOG_ERROR, TRUE, TRUE, _("Flow file %s corrupted. Ignoring"), flow_filename);
			geoxml_sequence_next(&line_flow);
			continue;
		}

		/* add to the flow browser. */
		gtk_list_store_append(gebr.ui_flow_browse->store, &flow_iter);
		gtk_list_store_set(gebr.ui_flow_browse->store, &flow_iter,
			FB_TITLE, geoxml_document_get_title(GEOXML_DOC(flow)),
			FB_FILENAME, flow_filename,
			FB_LINE_FLOW_POINTER, line_flow,
			-1);

		geoxml_document_free(GEOXML_DOC(flow));
		geoxml_sequence_next(&line_flow);
	}

	gebr_message(LOG_INFO, TRUE, FALSE, _("Flows loaded"));

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &flow_iter) == TRUE)
		flow_browse_select_iter(&flow_iter);
}

/*
 * Function: line_move_flow_top
 * Move flow top
 */
void
line_move_flow_top(void)
{
	GtkTreeIter		iter;
	GtkTreeModel *		model;
	GtkTreeSelection *	selection;

	GeoXmlSequence *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_after(line_flow, NULL);
	line_save();
	/* GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}

/*
 * Function: line_move_flow_bottom
 * Move flow bottom
 */
void
line_move_flow_bottom(void)
{
	GtkTreeIter		iter;
	GtkTreeModel *		model;
	GtkTreeSelection *	selection;

	GeoXmlSequence *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_before(line_flow, NULL);
	line_save();
	/* GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}
