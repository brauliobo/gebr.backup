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

/*
 * File: menus.c
 * Populate menus into GUI (see <menus_populate>)
 */

#include <geoxml.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>

#include "menus.h"
#include "gebr.h"
#include "support.h"
#include "document.h"

/*
 * Function: menu_load
 * Look for a given menu filename and load it if found
 */
GeoXmlFlow *
menu_load(const gchar * filename)
{
	GeoXmlFlow *	menu;
	GString *	path;

	path = menu_get_path(filename);
	if (path == NULL)
		return NULL;

	menu = menu_load_path(path->str);
	g_string_free(path, TRUE);

	return menu;
}

/*
 * Function: menu_load_path
 * Load menu at the given _path_
 */
GeoXmlFlow *
menu_load_path(const gchar * path)
{
	GeoXmlFlow *	menu;

	menu = GEOXML_FLOW(document_load_path(path));

	return menu;
}

/*
 * Function: menu_get_path
 * Look for a given menu and fill in its path
 */
GString *
menu_get_path(const gchar * filename)
{
	GString *	path;

	/* system directory */
	path = g_string_new(NULL);
	g_string_printf(path, "%s/%s", GEBRMENUSYS, filename);
	if (access ((path)->str, F_OK) == 0)
		goto out;

	/* user's menus directory */
	g_string_printf(path, "%s/%s", gebr.config.usermenus->str, filename);
	if (access ((path)->str, F_OK) == 0)
		goto out;

err:	g_string_free(path, TRUE);
	return NULL;

out:	return path;
}

/*
 * Function: menus_populate
 * Read index and add menus from it to the view
 */
int
menus_populate (void)
{
	FILE *		menuindex_fp;
	gchar		fname[STRMAX];
	gchar		line[STRMAX];
	GtkTreeIter	category_iter;
	GtkTreeIter *	parent_iter;

	strcpy (fname, getenv ("HOME"));
	strcat (fname, "/.gebr/menus.idx");

	if ( (menuindex_fp = fopen(fname, "r")) == NULL ) {
		if (! menus_create_index ())
			return EXIT_FAILURE;
		else
			menuindex_fp = fopen(fname, "r");
	}

	/* Remove any previous menus from the list */
	gtk_tree_store_clear (gebr.menu_store);
	parent_iter = NULL;

	while (read_line(line, STRMAX, menuindex_fp)){
		gchar *	parts[5];
		GString * dummy;
		GString *	titlebf;

		desmembra(line, 4, parts);
		titlebf = g_string_new(NULL);
		g_string_printf(titlebf, "<b>%s</b>", parts[0]);

		if (menus_fname(parts[3], &dummy) == EXIT_SUCCESS) {
			GtkTreeIter iter;

			if (parts[0] == NULL || !strlen(parts[0]))
				parent_iter = NULL;
			else {
				gchar * category;

				if (parent_iter != NULL) {
					gtk_tree_model_get ( GTK_TREE_MODEL(gebr.menu_store), parent_iter,
								MENU_TITLE_COLUMN, &category,
								-1);


					/* different category? */
					if (g_ascii_strcasecmp(category, titlebf->str)) {
						gtk_tree_store_append (gebr.menu_store, &category_iter, NULL);


						gtk_tree_store_set (gebr.menu_store, &category_iter,
								MENU_TITLE_COLUMN, titlebf->str,
								-1);

						parent_iter = &category_iter;
					}

					g_free(category);
				} else {
					gtk_tree_store_append (gebr.menu_store, &category_iter, NULL);

					gtk_tree_store_set (gebr.menu_store, &category_iter,
								MENU_TITLE_COLUMN, titlebf->str,
								-1);
					parent_iter = &category_iter;
				}
			}

			gtk_tree_store_append (gebr.menu_store, &iter, parent_iter);
			gtk_tree_store_set (gebr.menu_store, &iter,
					MENU_TITLE_COLUMN, parts[1],
					MENU_DESC_COLUMN, parts[2],
					MENU_FILE_NAME_COLUMN, parts[3],
					-1);
		}
		g_string_free(titlebf, TRUE);
	}

	fclose (menuindex_fp);
	return EXIT_SUCCESS;
}

/*
 * Function: scan_dir
 * Scans user's and system menus' directories for menus
 */
void
scan_dir (const char *path, FILE *fp)
{
	DIR *		dir;
	struct dirent *	file;

	if ((dir = opendir(path)) == NULL)
		return;

	while ((file = readdir (dir)) != NULL){
		if (fnmatch ("*.mnu", file->d_name, 1))
			continue;

		GeoXmlDocument *	doc;
		GeoXmlCategory *	category;
		gchar *			category_str;
		int			ret;
		gchar			filename[STRMAX];

		sprintf(filename, "%s/%s", path, file->d_name);

		if ((ret = geoxml_document_load (&doc, filename))) {
			/* TODO: */
			switch (ret) {
			case GEOXML_RETV_DTD_SPECIFIED:

			break;
			case GEOXML_RETV_INVALID_DOCUMENT:

			break;
			case GEOXML_RETV_CANT_ACCESS_FILE:

			break;
			case GEOXML_RETV_CANT_ACCESS_DTD:

			break;
			default:

			break;
			}
			return;
		}

		geoxml_flow_get_category(GEOXML_FLOW(doc), &category, 0);
		while ( category != NULL ){
		        category_str = (gchar *)geoxml_category_get_name(category);

			fprintf(fp, "%s|%s|%s|%s\n",
				category_str,
				geoxml_document_get_title(doc),
				geoxml_document_get_description(doc),
				geoxml_document_get_filename(doc) );

			geoxml_category_next(&category);
		}
		geoxml_document_free (doc);
	}
	closedir (dir);
}

/*
 * Function: menus_create_index
 * Create menus from found using scan_dir
 *
 * Returns TRUE if successful
 */
gboolean
menus_create_index(void)
{
	GString *	path;
	FILE *		menuindex;

	strcpy (fname, getenv ("HOME"));
	strcat (fname, "/.gebr/menus.idx");

	if ((menuindex = fopen(fname, "w")) == NULL ) {
		gebr_message(ERROR, TRUE, FALSE, "Unable to access user's menus directory");
		return FALSE;
	}

	scan_dir(GEBRMENUSYS,   menuindex);
	scan_dir(gebr.config.usermenus->str, menuindex);

	fclose(menuindex);

	/* Sort index */
	{
		GString	*	cmd_line;

		cmd_line = g_string_new(NULL);
		g_string_printf(cmd_line, "sort %s >/tmp/gebrmenus.tmp; mv /tmp/gebrmenus.tmp %s",
			fname, fname);

		system(cmd_line->str);
		g_string_free(cmd_line, TRUE);
	}

	return TRUE;
}
