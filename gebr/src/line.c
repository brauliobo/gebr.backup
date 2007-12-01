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
#include "flow.h"
#include "project.h"

gchar * no_line_selected_error =	_("No line selected");
gchar * no_selection_error =		_("Nothing selected");
gchar * no_project_selected_error =	_("Select a project to which a line will be added to");

/*
 * Function: line_new
 * Create a new line
 *
 */
void
line_new(void)
{
	GtkTreeIter		project_iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreePath *		line_path;
	GtkTreeIter		line_iter;
	GtkTreePath *		line_path;
	gchar *			prj_filename;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEgebr.(gebr.proj_line_view));
	if (gtk_tree_selection_get_selected(selection, &model, &project_iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, no_selection_error);
		return;
	}

	gtk_tree_model_get (model, &project_iter,
				PL_FILENAME, &prj_filename,
				-1);
	path = gtk_tree_model_get_path (model, &project_iter);

	if (gtk_tree_path_get_depth(path) == 2) {
		gebr_message(ERROR, TRUE, FALSE, no_project_selected_error);
		gtk_tree_path_free(path);
		g_free(prj_filename);
		return;
	}
	else {
		time_t t;
		struct tm *lt;
		char lne_filename[20];
		static const char * title = "New Line";

		time (&t);
		lt = localtime (&t);

		strftime (lne_filename, STRMAX, "%Y_%m", lt);
		strcat (lne_filename, "_XXXXXX");
		mktemp (lne_filename);
		strcat (lne_filename, ".lne");

		/* gtk stuff */
		gtk_tree_store_append (gebr.proj_line_store, &line_iter, &project_iter);
		gtk_tree_store_set (gebr.proj_line_store, &line_iter,
				PL_TITLE, title,
				PL_FILENAME, lne_filename,
				-1);

		line_path = gtk_tree_model_get_path (GTK_TREE_MODEL (gebr.proj_line_store),
						&line_iter);
		gtk_tree_view_expand_to_path (GTK_TREE_VIEgebr.(gebr.proj_line_view), line_path);

		/* full path to the project and line */
		GString *lne_path;
		GString *prj_path;

		data_fname(prj_filename, &prj_path);
		data_fname(lne_filename, &lne_path);

		/* libgeoxml stuff */
		GeoXmlProject *prj;
		GeoXmlLine    *lne;

		prj = project_load (prj_path->str);
		if (prj == NULL) {
		   printf("FIXME: %s:%d", __FILE__, __LINE__);
		   g_string_free(lne_path, TRUE);
		   g_string_free(prj_path, TRUE);
		   goto out;
		}
		geoxml_project_add_line (prj, lne_filename);
		geoxml_document_save (GEOXML_DOC(prj), prj_path->str);
		geoxml_document_free (GEOXML_DOC(prj));

		lne = geoxml_line_new ();
		geoxml_document_set_filename (GEOXML_DOC(lne), lne_filename);
		geoxml_document_set_title (GEOXML_DOC(lne), title);
		geoxml_document_set_author (GEOXML_DOC(lne), gebr.pref.username_value->str);
		geoxml_document_set_email (GEOXML_DOC(lne), gebr.pref.email_value->str);

		geoxml_document_save (GEOXML_DOC(lne), lne_path->str);
		geoxml_document_free (GEOXML_DOC(lne));

		g_string_free(lne_path, TRUE);
		g_string_free(prj_path, TRUE);

	}

out:
	gtk_tree_path_free(path);
	g_free(prj_filename);
	g_free(line_path);
}

/*
 * Function: line_delete
 * Delete the selected line
 *
 * TODO:
 * * ask the user about erasing all flows associated to this line.
 */
void
line_delete(void)
{
	GtkTreeIter        iter;
	GtkTreeSelection  *selection;
	GtkTreeModel      *model;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEgebr.(gebr.proj_line_view));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
	GtkTreePath *path;
	gchar       *name, *lne_filename;

	gtk_tree_model_get (model, &iter,
				PL_TITLE, &name,
				PL_FILENAME, &lne_filename,
				-1);
	path = gtk_tree_model_get_path (model, &iter);

	if (gtk_tree_path_get_depth(path) == 1)
		gebr_message(ERROR, TRUE, FALSE, no_line_selected_error);
	else {
		GtkTreeIter  piter;
		GString     *message;
		GString     *lne_path;
		GString     *prj_path;
		gchar	*prj_filename;

		gtk_tree_model_iter_parent(model, &piter, &iter);
		gtk_tree_model_get (model, &piter,
					PL_FILENAME, &prj_filename,
					-1);

		/* make the user happy */
		message = g_string_new("Erasing line '");
		g_string_append(message, name);
		g_string_append(message, "'");

		gebr_message(ERROR, TRUE, FALSE, message->str);
		g_string_free(message, TRUE);

		/* Assembly paths */
		data_fname(prj_filename, &prj_path);
		data_fname(lne_filename, &lne_path);

		/* Remove the line from its project */
		GeoXmlProject * prj;

		prj = project_load(prj_path->str);
		if (prj == NULL) {
		printf("FIXME: %s:%d", __FILE__, __LINE__);
		goto out;
		}

		GeoXmlProjectLine * project_line;
		geoxml_project_get_line(prj, &project_line, 0);
		while (project_line != NULL) {
			if (g_ascii_strcasecmp(lne_filename, geoxml_project_get_line_source(project_line)) == 0) {
				geoxml_project_remove_line(prj, project_line);
				geoxml_document_save(GEOXML_DOC(prj), prj_path->str);
				break;
			}

			geoxml_project_next_line(&project_line);
		}
		geoxml_document_free(GEOXML_DOC(prj));

		/* Removes its flows */
		/* TODO: ask for user's confirmation */
		GeoXmlLine * line;

		line = line_load(lne_path->str);
		if (line == NULL) {
		printf("FIXME: %s:%d", __FILE__, __LINE__);
		g_string_free(prj_path, TRUE);
		g_string_free(lne_path, TRUE);
		goto out;
		}

		GeoXmlLineFlow * line_flow;
		geoxml_line_get_flow(line, &line_flow, 0);
		while (line_flow != NULL) {
			GString *flow_path;

			data_fname(geoxml_line_get_flow_source(line_flow), &flow_path);
			unlink (flow_path->str);
			g_string_free(flow_path, TRUE);

			geoxml_line_next_flow(&line_flow);
		}
		geoxml_document_free(GEOXML_DOC(line));

		/* finally, remove it from the disk and from the tree*/
		gtk_tree_store_remove (GTK_TREE_STORE (gebr.proj_line_store), &iter);
		unlink(lne_path->str);

		/* Clear the flow list */
		gtk_list_store_clear (gebr.ui_flow_browse.store);
		gtk_list_store_clear (gebr.ui_flow_edition.fseq_store);
		flow_free();
		g_string_free(prj_path, TRUE);
		g_string_free(lne_path, TRUE);
	}

out:	gtk_tree_path_free(path);
	g_free(name);
	g_free(lne_filename);

	} else
		gebr_message(ERROR, TRUE, FALSE, no_selection_error);
}

/*
 * Function: line_load_flows
 * Load flows associated to the selected line.
 *
 */
void
line_load_flows(void)
{
	GtkTreeIter        iter;
	GtkTreePath       *path;
	GtkTreeSelection  *selection;
	GtkTreeModel      *model;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEgebr.(gebr.proj_line_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		GtkTreePath *	selection_path;
		GString *       lne_path;
		gchar *		name;
		gchar *		lne_filename;

		selection_path = gtk_tree_model_get_path(model, &iter);
		if (gebr.proj_line_selection_path != NULL && !gtk_tree_path_compare(selection_path, gebr.proj_line_selection_path)) {
			/* uhm, the same line/project is selected, don't need to do nothing */
			gtk_tree_path_free(selection_path);
			return;
		}
		gtk_tree_path_free(gebr.proj_line_selection_path);
		gebr.proj_line_selection_path = selection_path;

		/* reset part of GUI */
		gtk_list_store_clear(gebr.ui_flow_browse.store);
		gtk_list_store_clear(gebr.ui_flow_edition.fseq_store);
		flow_free();

		/* check if the item is a project */
		path = gtk_tree_model_get_path (model, &iter);
		if (gtk_tree_path_get_depth(path) == 1) {
			gtk_tree_path_free(path);
			return;
		}

		gtk_tree_model_get (model, &iter,
					PL_TITLE, &name,
					PL_FILENAME, &lne_filename,
					-1);

		/* assembly paths */
		data_fname(lne_filename, &lne_path);

		/* iterate over its flows */
		/* TODO: ask for user's confirmation */
		GeoXmlLine * line;

		line = line_load(lne_path->str);
		g_string_free(lne_path, TRUE);

		if (line == NULL) {
			gebr_message(ERROR, TRUE, FALSE, "Unable to load line");
			goto out;
		}

		GeoXmlLineFlow * line_flow;
		geoxml_line_get_flow(line, &line_flow, 0);
		while (line_flow != NULL) {
			GtkTreeIter	fiter;
			GeoXmlFlow *	flow;
			gchar *		flow_filename;

			flow_filename = (gchar*)geoxml_line_get_flow_source(line_flow);
			flow = GEOXML_FLOgebr.document_load(flow_path->str));
			if (flow == NULL) {
				geoxml_line_next_flow(&line_flow);
				continue;
			}

			/* add to the flow browser. */
			gtk_list_store_append(gebr.ui_flow_browse.store, &fiter);
			gtk_list_store_set(gebr.ui_flow_browse.store, &fiter,
					FB_TITLE, geoxml_document_get_title(GEOXML_DOC(flow)),
					FB_FILENAME, flow_filename,
					-1);

			geoxml_document_free(GEOXML_DOC(flow));
			geoxml_line_next_flow(&line_flow);
		}

		gebr_message(ERROR, TRUE, FALSE, _("Flows loaded"));

out:		gtk_tree_path_free(path);
		g_free(name);
		g_free(lne_filename);
	} else {
		gtk_list_store_clear(gebr.ui_flow_edition.fseq_store);
		geoxml_document_free (GEOXML_DOC (flow));
		flow = NULL;
		gebr_message(ERROR, TRUE, FALSE, no_selection_error);
	}
}
