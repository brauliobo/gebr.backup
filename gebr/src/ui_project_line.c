/*   GêBR - An environment for seismic processing.
 *   Copyright(C) 2007 GêBR core team(http://ui_project_line->sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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
 * File: ui_project_line.c
 * Builds the "Project and Lines" UI and distribute callbacks
 */

#include "ui_project_line.h"
#include "gebr.h"
#include "support.h"
#include "line.h"
#include "document.h"

/*
 * Prototypes
 */

static void
project_line_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text, struct ui_project_line * ui_project_line);

/*
 * Function: project_line_setup_ui
 * Assembly the project/lines widget.
 *
 * Return:
 * The structure containing relevant data.
 *
 * TODO:
 * Add an info summary about the project/line.
 */
struct ui_project_line *
project_line_setup_ui(void)
{
	struct ui_project_line *	ui_project_line;

	GtkTreeSelection *		selection;
	GtkTreeViewColumn *		col;
	GtkCellRenderer *		renderer;

	/* alloc */
	ui_project_line = g_malloc(sizeof(struct ui_project_line));

	/* Create projects/lines ui_project_line->widget */
	ui_project_line->widget = gtk_scrolled_window_new(NULL, NULL);

	ui_project_line->store = gtk_tree_store_new(PL_N_COLUMN,
						G_TYPE_STRING,  /* Name (title for libgeoxml) */
						G_TYPE_STRING); /* Filename */
	ui_project_line->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_project_line->store));
	gtk_container_add(GTK_CONTAINER(ui_project_line->widget), ui_project_line->view);

	/* Projects/lines column */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	col = gtk_tree_view_column_new_with_attributes(_("Projects/lines index"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, PL_TITLE);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_project_line->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PL_TITLE);
	g_signal_connect(GTK_OBJECT(renderer), "edited",
			GTK_SIGNAL_FUNC(project_line_rename), ui_project_line);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_project_line->view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	g_signal_connect(GTK_OBJECT(ui_project_line->view), "cursor-changed",
			GTK_SIGNAL_FUNC(line_load_flows), ui_project_line);

	ui_project_line->selection_path = NULL;

	return ui_project_line;
}

/*
 * Function: project_line_rename
 * Rename a projet or a line upon double click
 */
static void
project_line_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text, struct ui_project_line * ui_project_line)
{
	GtkTreeIter		iter;
	gchar *			filename;
	GeoXmlDocument *	document;

	if (gtk_tree_model_get_iter_from_string(
		GTK_TREE_MODEL(ui_project_line->store), &iter, path_string) == FALSE)
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_project_line->store), &iter,
			PL_FILENAME, &filename,
			-1);

	/* TODO: remove line or project if it doesn't exist? */
	document = document_load(filename);
	if (document == NULL)
		goto out;

	/* change it on the xml. */
	geoxml_document_set_title(document, new_text);
	document_save(document);
	geoxml_document_free(document);

	/* store's change */
	gtk_tree_store_set(ui_project_line->store, &iter,
			PL_TITLE, new_text,
			-1);

out:	g_free(filename);
}
