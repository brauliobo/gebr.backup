/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

/* File: project.c
 * Functions for projects manipulation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>

#include "cb_proj.h"
#include "gebr.h"
#include "flow.h"
#include "line.h"

/*
 * Function: project_populate_list
 * Reload the projets from the data directory
 */
void
project_populate_list(void)
{
	struct dirent *	file;
	DIR *		dir;

	if (access(gebr.pref.data_value->str, F_OK | R_OK | W_OK)) {
		gebr_message(ERROR, TRUE, FALSE, "Unable to access data directory");
		return;
	}

	if (gebr.proj_line_selection_path != NULL) {
		gtk_tree_path_free(gebr.proj_line_selection_path);
		gebr.proj_line_selection_path = NULL;
	}
	if ((dir = opendir (gebr.pref.data_value->str)) == NULL)
		return;

	/* Remove any previous menus from the list */
	gtk_tree_store_clear (gebr.proj_line_store);

	while ((file = readdir (dir)) != NULL) {
		if (fnmatch ("*.prj", file->d_name, 1))
			continue;

		GeoXmlProject * prj;
		GString *projectfn;

		data_fname(file->d_name, &projectfn);
		prj = project_load(projectfn->str);
		g_string_free(projectfn, TRUE);

		if (prj == NULL)
			goto out;

		/* Gtk stuff */
		GtkTreeIter iproj, iline;

		gtk_tree_store_append (gebr.proj_line_store, &iproj, NULL);
		gtk_tree_store_set (gebr.proj_line_store, &iproj,
				PL_TITLE, geoxml_document_get_title(GEOXML_DOC(prj)),
				PL_FILENAME, geoxml_document_get_filename(GEOXML_DOC(prj)),
				-1);

		GeoXmlProjectLine * project_line;
		geoxml_project_get_line(prj, &project_line, 0);
		while (project_line != NULL) {
			GeoXmlLine * line;
			/* full path to the project and line */
			GString *lne_path;
			data_fname(geoxml_project_get_line_source(project_line), &lne_path);
			line = line_load(lne_path->str);
			g_string_free(lne_path, TRUE);
			if (line == NULL) {
				geoxml_project_remove_line(prj, project_line);
				geoxml_document_save (GEOXML_DOC(prj), geoxml_document_get_filename(GEOXML_DOC(prj)));
				continue;
			}

			gtk_tree_store_append (gebr.proj_line_store, &iline, &iproj);
			gtk_tree_store_set (gebr.proj_line_store, &iline,
					PL_TITLE, geoxml_document_get_title(GEOXML_DOC(line)),
					PL_FILENAME, geoxml_project_get_line_source(project_line),
					-1);

			geoxml_document_free(GEOXML_DOC(line));
			geoxml_project_next_line(&project_line);
		}

		/* reset part of flow GUI */
		gtk_list_store_clear(gebr.flow_store);
		gtk_list_store_clear(gebr.fseq_store);
		flow_free();

		geoxml_document_free (GEOXML_DOC(prj));
	}

out:	closedir (dir);
}

/*
 * Function: project_new
 * Create a new project
 *
 */
void
project_new     (GtkMenuItem *menuitem,
		 gpointer     user_data)
{
   GeoXmlProject *prj;
   GtkTreeIter iter;
   time_t t;
   struct tm *lt;
   char filename[20];
   static const char * title = "New project";

   time (&t);
   lt = localtime (&t);

   strftime (filename, STRMAX, "%Y_%m", lt);
   strcat (filename, "_XXXXXX");
   mktemp (filename);
   strcat (filename, ".prj");

   prj = geoxml_project_new();
   geoxml_document_set_filename(GEOXML_DOC(prj), filename);
   geoxml_document_set_title(GEOXML_DOC(prj), title);
   geoxml_document_set_author (GEOXML_DOC(prj), gebr.pref.username_value->str);
   geoxml_document_set_email (GEOXML_DOC(prj), gebr.pref.email_value->str);

   gtk_tree_store_append (gebr.proj_line_store, &iter, NULL);
   gtk_tree_store_set (gebr.proj_line_store, &iter,
		       PL_TITLE, title,
		       PL_FILENAME, filename,
		       -1);

   /* assembly the file path */
   GString *str;

   data_fname(filename, &str);
   geoxml_document_save(GEOXML_DOC(prj), str->str);
   geoxml_document_free(GEOXML_DOC(prj));
   g_string_free(str, TRUE);
}

/*
 * Function: project_delete
 * Delete the selected project
 *
 * TODO:
 * * If a project is not empty, the user user should be
 *   warned. Besides, it should be asked about erasing
 *   all project's lines.
 * * Project's line files should be deleted as well.
 */
void
project_delete     (GtkMenuItem *menuitem,
		    gpointer     user_data)
{
   GtkTreeIter        iter;
   GtkTreeSelection  *selection;
   GtkTreeModel      *model;
   static const char *no_project_selected_error = "No project selected";

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.proj_line_view));

   if (gtk_tree_selection_get_selected (selection, &model, &iter)){
      GtkTreePath *path;
      gchar       *name, *filename;

      gtk_tree_model_get (model, &iter,
			  PL_TITLE, &name,
     			  PL_FILENAME, &filename,
			  -1);
      path = gtk_tree_model_get_path (model, &iter);

      if (gtk_tree_path_get_depth(path) > 1)
	 gebr_message(ERROR, TRUE, FALSE, no_project_selected_error);
      else {
	 char message[STRMAX];
	 int nlines;

	/* TODO: If this project is not empty,
	    prompt the user to take about erasing its lines */

	 /* Delete each line of the project */
	 if ((nlines = gtk_tree_model_iter_n_children (model, &iter)) > 0){
	    sprintf (message, "Project '%s' still has %i lines", name, nlines);
	    gebr_message(ERROR, TRUE, FALSE, message);
	 }
	 else {
	    sprintf (message, "Erasing project '%s'", name);
	    gebr_message(ERROR, TRUE, FALSE, message);

	    /* Remove the project from the store (and its children) */
	    gtk_tree_store_remove (GTK_TREE_STORE (gebr.proj_line_store), &iter);

	    /* finally, remove it from the disk */
	    GString *str;

	    data_fname(filename, &str);
	    unlink(str->str);
	    g_string_free(str, TRUE);

	 }
      }

      gtk_tree_path_free(path);
   }
   else
      gebr_message(ERROR, TRUE, FALSE, no_project_selected_error);
}
