/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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
 * File: gebr.c
 * General purpose functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <glib/gstdio.h>

#include <misc/utils.h>

#include "gebr.h"
#include "support.h"
#include "cmdline.h"
#include "project.h"
#include "server.h"
#include "job.h"
#include "menu.h"
#include "callbacks.h"
#include "document.h"
#include "flow.h"

struct gebr gebr;

/*
 * Function: gebr_init
 * Take initial measures.
 *
 * This function is the callback of when the gebr.window is shown.
 */
void
gebr_init(int argc, char ** argv)
{
	GString *	log_filename;
	GError *	error;


	/* initialization */
	gebr.doc = NULL;
	gebr.project = NULL;
	gebr.line = NULL;
	gebr.flow = NULL;
	log_filename = g_string_new(NULL);
	error = NULL;

	/* assembly user's gebr directory */
	g_string_printf(log_filename, "%s/.gebr/gebr.log", getenv("HOME"));

	/* log file */
	gebr.log = log_open(log_filename->str);

	/* allocating list of temporary files */
	gebr.tmpfiles = g_slist_alloc();

	/* list of servers */
	protocol_init();

	/* icons */
	gebr.pixmaps.unconfigured_icon = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr_unconfigured.png", &error);
	gebr.pixmaps.configured_icon = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr_configured.png", &error);
	gebr.pixmaps.disabled_icon = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr_disabled.png", &error);
	gebr.pixmaps.running_icon = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr_running.png", &error);

	/* message */
	gebr_message(START, TRUE, TRUE, _("G√™BR Initiating..."));

	/* finally the config. file */
	gebr_config_load(argc, argv);

	/* frees */
	g_string_free(log_filename, TRUE);

}

/*
 * Function: gebr_quit
 * Free memory, remove temporaries files and quit.
 */
gboolean
gebr_quit(void)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/*
	 * Data frees and cleanups
	 */

	flow_free();
	document_free();

	g_slist_foreach(gebr.tmpfiles, (GFunc) unlink, NULL);
	g_slist_foreach(gebr.tmpfiles, (GFunc) free, NULL);

	g_slist_free(gebr.tmpfiles);

	g_string_free(gebr.config.username, TRUE);
	g_string_free(gebr.config.email, TRUE);
	g_string_free(gebr.config.editor, TRUE);
	g_string_free(gebr.config.usermenus, TRUE);
	g_string_free(gebr.config.data, TRUE);
	g_string_free(gebr.config.browser, TRUE);

	gebr_message(END, TRUE, TRUE, _("G√™BR Finalizing..."));
	log_close(gebr.log);

	/* Free servers structs */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	while (valid) {
		struct server *	server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
				SERVER_POINTER, &server,
				-1);
		server_free(server);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	}
	/* Free jobs structs */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	while (valid) {
		struct job *	job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				JC_STRUCT, &job,
				-1);
		job_free(job);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	}

	/*
	 * Interface frees
	 */

	g_free(gebr.ui_project_line);
 	g_free(gebr.ui_flow_browse);
	g_free(gebr.ui_flow_edition);
	g_free(gebr.ui_job_control);
	g_free(gebr.ui_server_list);

	g_object_unref(gebr.pixmaps.unconfigured_icon);
	g_object_unref(gebr.pixmaps.configured_icon);
	g_object_unref(gebr.pixmaps.disabled_icon);

	gtk_main_quit();

	return FALSE;
}

/*
 * Function: gebr_config_load
 * Initialize configuration for GÍBR
 */
void
gebr_config_load(int argc, char ** argv)
{
	GString	*	config;

	/* initialization */
	config = g_string_new(NULL);
	gebr.config.username = g_string_new("");
	gebr.config.email = g_string_new("");
	gebr.config.editor = g_string_new("");
	gebr.config.usermenus = g_string_new("");
	gebr.config.data = g_string_new("");
	gebr.config.browser = g_string_new("");

	/* TODO: check return */
	gebr_create_config_dirs();

	g_string_printf(config, "%s/.gebr/gebr.conf", getenv("HOME"));
	if (g_access(config->str, F_OK)) {
		on_configure_preferences_activate();
		goto out;
	}

	/* Initialize GÍBR with options in gebr.conf */
	if (cmdline_parser_configfile(config->str, &gebr.config.ggopt, 1, 1, 0) != 0) {
		fprintf(stderr, "%s: try '--help' option\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Filling our structure in */
	g_string_assign(gebr.config.username, gebr.config.ggopt.name_arg);
	g_string_assign(gebr.config.email, gebr.config.ggopt.email_arg);
	g_string_assign(gebr.config.editor, gebr.config.ggopt.editor_arg);
	g_string_assign(gebr.config.usermenus, gebr.config.ggopt.usermenus_arg);
	g_string_assign(gebr.config.data, gebr.config.ggopt.data_arg);
	g_string_assign(gebr.config.browser, gebr.config.ggopt.browser_arg);

	if (!(gebr.config.ggopt.usermenus_given && gebr.config.ggopt.data_given &&
		gebr.config.ggopt.editor_given && gebr.config.ggopt.browser_given))
		on_configure_preferences_activate();
	else {
		menu_list_populate();
		project_list_populate();
	}

	if (!gebr.config.ggopt.server_given) {
		GtkTreeIter	iter;
		gchar		hostname[100];

		gethostname(hostname, 100);
		gtk_list_store_append(gebr.ui_server_list->store, &iter);
		gtk_list_store_set(gebr.ui_server_list->store, &iter,
				SERVER_ADDRESS, hostname,
				SERVER_POINTER, server_new(hostname),
				-1);
	} else {
		int		i;

		for (i = 0; i < gebr.config.ggopt.server_given; ++i) {
			GtkTreeIter	iter;

			gtk_list_store_append(gebr.ui_server_list->store, &iter);
			gtk_list_store_set(gebr.ui_server_list->store, &iter,
						SERVER_ADDRESS, gebr.config.ggopt.server_arg[i],
						SERVER_POINTER, server_new(gebr.config.ggopt.server_arg[i]),
						-1);
		}
	}

	/* frees */
out:	g_string_free(config, TRUE);
}

/*
 * Function: gebr_config_apply
 * Apply configurations to GÍBR
 */
void
gebr_config_apply(void)
{
	menu_list_create_index();
	menu_list_populate();
	project_list_populate();
}

/*
 * Function: gebr_config_save
 * Save GÍBR config to file.
 *
 * Write ~/.gebr/.gebr.conf file.
 */
gboolean
gebr_config_save(void)
{
	FILE *		fp;
	GString *	config;

	GtkTreeIter	iter;
	gboolean	valid;

	/* initialization */
	config = g_string_new(NULL);
	g_string_printf(config, "%s/.gebr/gebr.conf", getenv("HOME"));

	fp = fopen(config->str, "w");
	if (fp == NULL) {
		gebr_message(ERROR, TRUE, TRUE, _("Unable to write configuration"));
		return FALSE;
	}

	fprintf(fp, "name = \"%s\"\n", gebr.config.username->str);
	fprintf(fp, "email = \"%s\"\n", gebr.config.email->str);
	fprintf(fp, "usermenus = \"%s\"\n", gebr.config.usermenus->str);
	fprintf(fp, "data = \"%s\"\n",  gebr.config.data->str);
	fprintf(fp, "editor = \"%s\"\n", gebr.config.editor->str);
	fprintf(fp, "browser = \"%s\"\n", gebr.config.browser->str);

	/* Save list of servers */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	while (valid) {
		gchar *	server;

		gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
				SERVER_ADDRESS, &server,
				-1);

		fprintf(fp, "server = \"%s\"\n", server);

		g_free(server);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	}

	fclose(fp);
	gebr_message(INFO, TRUE, TRUE, _("Configuration saved"));

	return TRUE;
}

/*
 * Function: gebr_message
 * Log a message. If in_statusbar is TRUE it is writen to status bar.
 *
 */
void
gebr_message(enum log_message_type type, gboolean in_statusbar, gboolean in_log_file, const gchar * message, ...)
{
	gchar *		string;
	va_list		argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	if (in_log_file)
		log_add_message(gebr.log, type, string);
	if (in_statusbar)
		gtk_statusbar_push(GTK_STATUSBAR (gebr.statusbar), 0, string);

	g_free(string);
}
