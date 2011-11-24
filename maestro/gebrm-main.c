/*
 * gebrm-main.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Team
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "gebrm-app.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <libgebr/comm/gebr-comm.h>

#include "gebrm-app.h"

#include <libgebr/gebr-version.h>

#define GETTEXT_PACKAGE "gebrm"

static gboolean interactive;
static gboolean show_version;

static GOptionEntry entries[] = {
	{"interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive,
		N_("Run server in interactive mode, not as a daemon"), NULL},
	{"version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
		N_("Show GeBR daemon version"), NULL},
	{NULL}
};

void
fork_and_exit_main(void)
{
	pid_t pid = fork();

	if (pid == (pid_t)-1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid != 0)
		exit(EXIT_SUCCESS);

	umask(0);

	if (setsid() == (pid_t)-1) {
		perror("setsid");
		exit(EXIT_FAILURE);
	}

	if (chdir("/") == -1) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
}

static void
gebrm_remove_lock_and_quit(int sig)
{
	g_unlink(gebrm_main_get_lock_file());
	exit(0);
}

int
main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	context = g_option_context_new(_("GêBR Maestro"));
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	g_type_init();
	g_thread_init(NULL);

	if (show_version) {
		fprintf(stdout, "%s (%s)\n", "0.1.0", gebr_version());
		exit(EXIT_SUCCESS);
	}

	const gchar *lock = gebrm_main_get_lock_file();

	if (g_access(lock, R_OK | W_OK) == 0) {
		gchar *contents;
		GError *error = NULL;
		g_file_get_contents(lock, &contents, NULL, &error);

		if (error) {
			g_critical("Error reading lock: %s", error->message);
			exit(1);
		}

		gint port = atoi(contents);

		if (!gebr_comm_listen_socket_is_local_port_available(port)) {
			g_print("%s\n", contents);
			exit(0);
		}
	} else if (g_access(lock, F_OK) == 0) {
		g_critical("Can not read/write into %s", lock);
		exit(1);
	}

	if (!interactive)
		fork_and_exit_main();

	struct sigaction sa;
	sa.sa_handler = gebrm_remove_lock_and_quit;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGTERM, &sa, NULL)
	    || sigaction(SIGINT, &sa, NULL)
	    || sigaction(SIGQUIT, &sa, NULL))
		perror("sigaction");

	gebr_geoxml_init();
	GebrmApp *app = gebrm_app_singleton_get();

	if (!gebrm_app_run(app))
		exit(EXIT_FAILURE);

	g_object_unref(app);
	gebr_geoxml_finalize();

	return EXIT_SUCCESS;
}
