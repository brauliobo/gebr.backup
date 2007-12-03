/*   GêBR - An environment for seismic processing.
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but gebr.THOUT ANY gebr.RRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* File: line.c
 * Lines manipulation functions
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "line.h"
#include "gebr.h"
#include "support.h"
#include "document.h"
#include "project.h"
#include "flow.h"

gchar * no_line_selected_error =		_("No line selected");
gchar * no_selection_error =			_("Nothing selected");
static gchar * no_project_selected_error =	_("Select a project to which a line will be added to");

/*
 * Function: line_new
 * Create a new line
 *
 */
void
line_new(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		project_iter, line_iter;
	GtkTreePath *		path;

	gchar *			title;
	gchar *			project_filename;
	GString *		line_filename;

	GeoXmlProject *		project;
	GeoXmlLine *		line;

	title = _("New Line");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (gtk_tree_selection_get_selected(selection, &model, &project_iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_selection_error);
		return;
	}

	line_filename = document_assembly_filename(".lne");
	gtk_tree_model_get(model, &project_iter,
			PL_FILENAME, &project_filename,
			-1);
	path = gtk_tree_model_get_path(model, &project_iter);

	if (gtk_tree_path_get_depth(path) == 2) {
		gebr_message(ERROR, TRUE, FALSE, no_project_selected_error);
		gtk_tree_path_free(path);
		g_free(project_filename);
		goto out;
	}

	/* gtk stuff */
	gtk_tree_store_append(gebr.ui_project_line->store, &line_iter, &project_iter);
	gtk_tree_store_set(gebr.ui_project_line->store, &line_iter,
			PL_TITLE, title,
			PL_FILENAME, line_filename->str,
			-1);

	gtk_tree_path_free(path);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &line_iter);
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(gebr.ui_project_line->view), path);

	/* libgeoxml stuff */
	project = GEOXML_PROJECT(document_load(project_filename));
	if (project == NULL)
		goto out;
	geoxml_project_add_line(project, line_filename->str);
	document_save(GEOXML_DOC(project));
	geoxml_document_free(GEOXML_DOC(project));

	line = geoxml_line_new();
	geoxml_document_set_filename(GEOXML_DOC(line), line_filename->str);
	geoxml_document_set_title(GEOXML_DOC(line), title);
	geoxml_document_set_author(GEOXML_DOC(line), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(line), gebr.config.email->str);
	document_save(GEOXML_DOC(line));
	geoxml_document_free(GEOXML_DOC(line));

out:	g_string_free(line_filename, TRUE);
	gtk_tree_path_free(path);
	g_free(project_filename);
}

/*
 * Function: line_delete
 * Delete the selected line
 *
 * TODO: ask the user about erasing all flows associated to this line.
 */
void
line_delete(void)
{
	GtkTreeSelection  *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		line_iter, project_iter;
	GtkTreePath *		path;

	gchar *			title;
	gchar *			line_filename;
	gchar *			project_filename;

	GeoXmlProject *		project;
	GeoXmlProjectLine *	project_line;
	GeoXmlLine *		line;
	GeoXmlLineFlow *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (gtk_tree_selection_get_selected(selection, &model, &line_iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_selection_error);
		return;
	}

	gtk_tree_model_get(model, &line_iter,
			PL_TITLE, &title,
			PL_FILENAME, &line_filename,
			-1);
	path = gtk_tree_model_get_path(model, &line_iter);

	if (gtk_tree_path_get_depth(path) == 2) {
		gebr_message(ERROR, TRUE, FALSE, _("Select a line instead of a project"));
		goto out;
	}

	/* get parent project filename */
	gtk_tree_model_iter_parent(model, &project_iter, &line_iter);
	gtk_tree_model_get(model, &project_iter,
			PL_FILENAME, &project_filename,
			-1);

	/* make the user happy */
	gebr_message(INFO, TRUE, TRUE, _("Erasing line '%s'"), title);

	/* Removes its flows */
	/* TODO: ask for user's confirmation */
	line = GEOXML_LINE(document_load(line_filename));
	if (line == NULL)
		goto out;
	geoxml_line_get_flow(line, &line_flow, 0);
	while (line_flow != NULL) {
		GString *	path;

		/* log action */
		gebr_message(INFO, FALSE, TRUE, _("Erasing child flow '%s'"), geoxml_line_get_flow_source(line_flow));

		path = document_get_path(geoxml_line_get_flow_source(line_flow));
		unlink(path->str);
		g_string_free(path, TRUE);

		geoxml_line_next_flow(&line_flow);
	}

	/* finally, remove it from the disk and from the tree*/
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), &line_iter);
	document_delete(line_filename);
	geoxml_document_free(GEOXML_DOC(line));

	/* Remove the line from its project */
	project = GEOXML_PROJECT(document_load(project_filename));
	if (project == NULL)
		goto out;
	geoxml_project_get_line(project, &project_line, 0);
	while (project_line != NULL) {
		if (g_ascii_strcasecmp(line_filename, geoxml_project_get_line_source(project_line)) == 0) {
			geoxml_project_remove_line(project, project_line);
			document_save(GEOXML_DOC(project));
			break;
		}

		geoxml_project_next_line(&project_line);
	}
	geoxml_document_free(GEOXML_DOC(project));

	/* Clear the flow list */
	gtk_list_store_clear (gebr.ui_flow_browse->store);
	gtk_list_store_clear (gebr.ui_flow_edition->fseq_store);
	flow_free();

out:	g_free(title);
	g_free(line_filename);
	gtk_tree_path_free(path);
	g_free(project_filename);
}

/*
 * Function: line_load_flows
 * Load flows associated to the selected line.
 *
 */
void
line_load_flows(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;
	GtkTreePath *		path;
	GtkTreePath *		selection_path;

	gchar *			title;
	gchar *			line_filename;

	GeoXmlLine *		line;
	GeoXmlLineFlow *	line_flow;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_selection_error);
		flow_free();
	}

	selection_path = gtk_tree_model_get_path(model, &iter);
	if (gebr.ui_project_line->selection_path != NULL && !gtk_tree_path_compare(selection_path, gebr.ui_project_line->selection_path)) {
		/* uhm, the same line/project is selected, don't need to do nothing */
		gtk_tree_path_free(selection_path);
		return;
	}
	gtk_tree_path_free(gebr.ui_project_line->selection_path);
	gebr.ui_project_line->selection_path = selection_path;

	/* reset part of GUI */
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	flow_free();

	gtk_tree_model_get(model, &iter,
			PL_TITLE, &title,
			PL_FILENAME, &line_filename,
			-1);

	/* check if the item is a project */
	path = gtk_tree_model_get_path (model, &iter);
	if (gtk_tree_path_get_depth(path) == 1) {
		gtk_tree_path_free(path);
		return;
	}

	/* iterate over its flows */
	/* TODO: ask for user's confirmation */
	line = GEOXML_LINE(document_load(line_filename));
	if (line == NULL) {
		gebr_message(ERROR, TRUE, TRUE, _("Unable to load line"));
		goto out;
	}
	geoxml_line_get_flow(line, &line_flow, 0);
	while (line_flow != NULL) {
		GtkTreeIter	flow_iter;
		GeoXmlFlow *	flow;
		gchar *		flow_filename;

		flow_filename = (gchar*)geoxml_line_get_flow_source(line_flow);
		flow = GEOXML_FLOW(document_load(flow_filename));
		if (flow == NULL) {
			geoxml_line_next_flow(&line_flow);
			continue;
		}

		/* add to the flow browser. */
		gtk_list_store_append(gebr.ui_flow_browse->store, &flow_iter);
		gtk_list_store_set(gebr.ui_flow_browse->store, &flow_iter,
				FB_TITLE, geoxml_document_get_title(GEOXML_DOC(flow)),
				FB_FILENAME, flow_filename,
				-1);

		geoxml_document_free(GEOXML_DOC(flow));
		geoxml_line_next_flow(&line_flow);
	}

	gebr_message(ERROR, TRUE, FALSE, _("Flows loaded"));

out:	gtk_tree_path_free(path);
	g_free(title);
	g_free(line_filename);
}
