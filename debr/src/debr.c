/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <libgebr.h>
#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/gui/utils.h>

#include "debr.h"
#include "menu.h"
#include "preferences.h"

/* global instance for data exchange */
struct debr debr;

void
debr_init(void)
{
	libgebr_init();

	debr_config_load();

	debr.categories_model = gtk_list_store_new(CATEGORY_N_COLUMN, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(debr.categories_model),
		CATEGORY_NAME, GTK_SORT_ASCENDING);
	debr.menu = NULL;
	debr.program = NULL;
	debr.parameter = NULL;
	debr.unsaved_count = 0;

	/* list of temporaries files */
	debr.tmpfiles = g_slist_alloc();

	/* load status icons */
	debr.invisible = gtk_invisible_new();
	debr.pixmaps.stock_apply = gtk_widget_render_icon(debr.invisible,
		GTK_STOCK_APPLY, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	debr.pixmaps.stock_cancel = gtk_widget_render_icon(debr.invisible,
		GTK_STOCK_CANCEL, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	debr.pixmaps.stock_no = gtk_widget_render_icon(debr.invisible, GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

	if (!strcmp(debr.config.menu_dir->str, ""))
		preferences_dialog_setup_ui();
	else
		menu_load_user_directory();
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(debr.ui_menu.list_store), NULL) == 0)
		menu_new(FALSE);
}

gboolean
debr_quit(void)
{
	if (!menu_cleanup())
		return TRUE;

	gtk_widget_destroy(debr.about.dialog);
	g_object_unref(debr.accel_group);

	/* free config stuff */
	g_key_file_free(debr.config.key_file);
	g_string_free(debr.config.path, TRUE);
	g_string_free(debr.config.name, TRUE);
	g_string_free(debr.config.email, TRUE);
	g_string_free(debr.config.menu_dir, TRUE);
	g_string_free(debr.config.browser, TRUE);
	g_string_free(debr.config.htmleditor, TRUE);

	/* remove temporaries files */
	g_slist_foreach(debr.tmpfiles, (GFunc)g_unlink, NULL);
	g_slist_foreach(debr.tmpfiles, (GFunc)g_free, NULL);
	g_slist_free(debr.tmpfiles);

	g_object_unref(debr.pixmaps.stock_apply);
	g_object_unref(debr.pixmaps.stock_cancel);
	g_object_unref(debr.pixmaps.stock_no);
	gtk_widget_destroy(debr.invisible);

	gtk_main_quit();

	return FALSE;
}

void
debr_config_load(void)
{
	GError *	error;

	gebr_create_config_dirs();
	debr.config.path = g_string_new(getenv("HOME"));
	g_string_append(debr.config.path, "/.gebr/debr/debr.conf");

	error = NULL;
	debr.config.key_file = g_key_file_new();
	g_key_file_load_from_file(debr.config.key_file, debr.config.path->str, G_KEY_FILE_NONE, &error);

	debr.config.name = g_key_file_load_string_key(debr.config.key_file, "general", "name", g_get_real_name());
	debr.config.email = g_key_file_load_string_key(debr.config.key_file, "general", "email", g_get_user_name());
	debr.config.menu_dir = g_key_file_load_string_key(debr.config.key_file, "general", "menu_dir", "");
	debr.config.browser = g_key_file_load_string_key(debr.config.key_file, "general", "browser", "firefox");
	debr.config.htmleditor = g_key_file_load_string_key(debr.config.key_file, "general", "htmleditor", "gedit");
}

void
debr_config_save(void)
{
	gsize		length;
	gchar *		string;
	GError *	error;
	FILE *		configfp;

	g_key_file_set_string(debr.config.key_file, "general", "name", debr.config.name->str);
	g_key_file_set_string(debr.config.key_file, "general", "email", debr.config.email->str);
	g_key_file_set_string(debr.config.key_file, "general", "menu_dir", debr.config.menu_dir->str);
	g_key_file_set_string(debr.config.key_file, "general", "browser", debr.config.browser->str);
	g_key_file_set_string(debr.config.key_file, "general", "htmleditor", debr.config.htmleditor->str);

	error = NULL;
	string = g_key_file_to_data(debr.config.key_file, &length, &error);
	configfp = fopen(debr.config.path->str, "w");
	if (configfp == NULL) {
		/* TODO: warn user */
		return;
	}
	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);
}

/* Function: debr_message
 * Log a message. If in_statusbar is TRUE it is writen to status bar.
 */
void
debr_message(enum log_message_type type, const gchar * message, ...)
{
	gchar *		string;
	va_list		argp;

#ifndef DEBR_DEBUG
	if (type == LOG_DEBUG)
		return;
#endif

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	if (type == LOG_DEBUG)
		g_print("%s\n", string);
	gtk_statusbar_push(GTK_STATUSBAR(debr.statusbar), 0, string);

	g_free(string);
}

/* Function: debr_has_category
 * Return TRUE if _category_ is present in the list of categories
 * If _category_ doesn't belong to the list and _add_ is TRUE, then it is added
 */
gboolean
debr_has_category(const gchar * category, gboolean add)
{
	GtkTreeIter	iter;

	libgebr_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.categories_model)) {
		gchar *	i;
		gint	ref_count;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.categories_model), &iter,
			CATEGORY_NAME, &i,
			CATEGORY_REF_COUNT, &ref_count,
			-1);
		if (strcmp(i, category) == 0) {
			if (add)
				gtk_list_store_set(debr.categories_model, &iter,
					CATEGORY_REF_COUNT, ref_count+1,
					-1);
			g_free(i);
			return TRUE;
		}

		g_free(i);
	}
	if (add) {
		gtk_list_store_append(debr.categories_model, &iter);
		gtk_list_store_set(debr.categories_model, &iter,
			CATEGORY_NAME, category,
			CATEGORY_REF_COUNT, 1,
			-1);
		return TRUE;
	}

	return FALSE;
}
