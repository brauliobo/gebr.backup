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
 * File: gebr.c
 */
#include "gebr.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "interface.h"
#include "cmdline.h"
#include "callbacks.h"
#include "interface.h"
#include "project.h"
#include "server.h"
#include "menus.h"

/*
 * Function: gebr_init
 * Take initial measures
 */
void
gebr_init(void)
{
	GString *	log_filename;
	gchar *		home;

	/* initialization */
	gebr.server_store = NULL;

	/* assembly user's gebr directory */
	log_filename = g_string_new(NULL);
	g_string_printf(log_filename, "%s/.gebr/gebr.log", getenv("HOME"));

	/* log file */
	gebr.log = log_open(log_filename->str);

	/* allocating list of temporary files */
	gebr.tmpfiles = g_slist_alloc();

	/* list of servers */
	protocol_init();

	/* icons */
	{
		GError *	error;

		error = NULL;
		gebr.pixmaps.unconfigured_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_unconfigured.png", &error);
		gebr.pixmaps.configured_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_configured.png", &error);
		gebr.pixmaps.disabled_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_disabled.png", &error);
		gebr.pixmaps.running_icon = gdk_pixbuf_new_from_file(GEBR_PIXMAPS_DIR "gebr_running.png", &error);
	}

	log_message(START, "GêBR Initiating...", TRUE);

	g_string_free(log_filename, TRUE);
}

/*
 * Function: gebr_init
 * Free memory, remove temporaries files and quit
 */
gboolean
gebr_quit(void)
{
	g_slist_foreach(gebr.tmpfiles, (GFunc) unlink, NULL);
	g_slist_foreach(gebr.tmpfiles, (GFunc) free, NULL);

	g_slist_free(gebr.tmpfiles);

	g_object_unref(gebr.pixmaps.unconfigured_icon);
	g_object_unref(gebr.pixmaps.configured_icon);
	g_object_unref(gebr.pixmaps.disabled_icon);

	log_close(gebr.log);

	gtk_main_quit();

	return FALSE;
}

/*
 * Function: gebr_config_load
 * Initialize configuration for G�BR
 */
int
gebr_config_load(int argc, char ** argv)
{
	gchar	fname[STRMAX];
	gchar *	home = getenv("HOME");

	/* assembly config. directory */
	sprintf(fname, "%s/.gebr", home);
	/* create it if necessary */
	if (access(fname, F_OK)) {
		struct stat	home_stat;

		stat(home, &home_stat);
		if (mkdir(fname, home_stat.st_mode))
			exit(EXIT_FAILURE);
	}

	/* Init server list store */
	gebr.server_store = gtk_list_store_new(SERVER_N_COLUMN,
					G_TYPE_STRING,
					G_TYPE_POINTER);
	strcat(fname,"/gebr.conf");

	gebr.config.username = g_string_new("");
	gebr.config.email = g_string_new("");
	gebr.config.editor = g_string_new("");
	gebr.config.usermenus = g_string_new("");
	gebr.config.data = g_string_new("");
	gebr.config.browser = g_string_new("");

	if (access (fname, F_OK) == 0) {
		int i;
		/* Initialize GeBR with option in gebr.conf */
		if(cmdline_parser_configfile (fname, &gebr.config, 1, 1, 0) != 0) {
			fprintf(stderr,"%s: try '--help' option\n", argv[0]);
			exit(EXIT_FAILURE);
		}

		/* Filling our structure in */
		g_string_append(gebr.config.username, gebr.config.name_arg);
		g_string_append(gebr.config.email, gebr.config.email_arg);
		g_string_append(gebr.config.editor, gebr.config.editor_arg);
		g_string_append(gebr.config.usermenus, gebr.config.usermenus_arg);
		g_string_append(gebr.config.data, gebr.config.data_arg);
		g_string_append(gebr.config.browser, gebr.config.browser_arg);

		if (!(gebr.config.usermenus_given && gebr.config.data_given &&
			gebr.config.editor_given && gebr.config.browser_given))
			gtk_widget_show(gebr.ui_preferences.dialog);
		else {
			menus_populate ();
			projects_refresh();
		}

		if (!gebr.config.server_given) {
			GtkTreeIter	iter;
			gchar hostname[100];

			gethostname(hostname, 100);
			gtk_list_store_append (gebr.server_store, &iter);
			gtk_list_store_set (gebr.server_store, &iter,
					SERVER_ADDRESS, hostname,
					SERVER_POINTER, server_new(hostname),
					-1);
		} else {
			for (i=0; i <gebr.config.server_given; i++) {
				GtkTreeIter	iter;

				/* TODO: free servers structs on exit */
				gtk_list_store_append (gebr.server_store, &iter);
				gtk_list_store_set (gebr.server_store, &iter,
							SERVER_ADDRESS, gebr.config.server_arg[i],
							SERVER_POINTER, server_new(gebr.config.server_arg[i]),
							-1);
			}
		}
	}
	else
		gtk_widget_show(gebr.ui_preferences.dialog);

	return EXIT_SUCCESS;
}

/*
 * Function: gebr_config_reload
 * Reload configuration for G�BR
 */
int
gebr_config_reload(void)
{
	menus_create_index();
	menus_populate ();
	projects_refresh();

	return EXIT_SUCCESS;
}

/*
 * Function: gebr_config_save
 * Save G�BR config to file.
 *
 * Write .gebr.conf file.
 */
int
gebr_config_save(void)
{
	FILE *		fp;
	char 		fname[STRMAX];

	strcpy (fname, getenv ("HOME"));
	strcat (fname, "/.gebr/gebr.conf");

	fp = fopen (fname, "w");
	if (fp == NULL) {
		log_message(ERROR, "Unable to write configuration", TRUE);
		return EXIT_FAILURE;
	}

	if (gebr.config.username->str != NULL)
		fprintf(fp, "name = \"%s\"\n", gebr.config.username->str);
	if (gebr.config.email->str != NULL)
		fprintf(fp, "email = \"%s\"\n", gebr.config.email->str);
	if (gebr.config.usermenus->str != NULL)
		fprintf(fp, "usermenus = \"%s\"\n", gebr.config.usermenus->str);
	if (gebr.config.data->str != NULL)
		fprintf(fp, "data = \"%s\"\n",  gebr.config.data->str);
	if (gebr.config.editor->str != NULL)
		fprintf(fp, "editor = \"%s\"\n", gebr.config.editor->str);
	if (gebr.config.browser->str != NULL)
		fprintf(fp, "browser = \"%s\"\n", gebr.config.browser->str);

	{
		GtkTreeIter	iter;
		gboolean	valid;

		/* check if it is already open */
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.server_store), &iter);
		while (valid) {
			gchar *	server;

			gtk_tree_model_get (GTK_TREE_MODEL(gebr.server_store), &iter,
					SERVER_ADDRESS, &server,	-1);

			fprintf(fp, "server = \"%s\"\n", server);

			g_free(server);
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.server_store), &iter);
		}
	}

	fclose(fp);
	log_message (ACTION, "Configuration saved", TRUE);

	return EXIT_SUCCESS;
}

/*
 * Function: gebr_message
 * Log a message. If in_statusbar is TRUE it is writen to status bar.
 *
 */
void
gebr_message(enum log_message_type type, gboolean in_statusbar, gboolean in_log_file, const gchar * message)
{
	if (in_log_file)
		log_add_message(gebr.log, type, message);
	if (in_statusbar)
		gtk_statusbar_push(GTK_STATUSBAR (gebr.statusbar), 0, message);
}
