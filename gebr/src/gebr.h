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

#ifndef __GEBR_H
#define __GEBR_H

#include <glib.h>

#include <geoxml.h>
#include <gui/about.h>
#include <misc/utils.h>

#include "cmdline.h"
#include "ui_project_line.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_job_control.h"
#include "ui_preferences.h"
#include "ui_servers.h"

/* global variable of common needed stuff */
extern struct gebr gebr;

/* Menubar entries */
enum {
	MENUBAR_PROJECT,
	MENUBAR_LINE,
	MENUBAR_FLOW,
	MENUBAR_FLOW_COMPONENTS,
	MENUBAR_N
};

typedef struct {
	GtkWidget *		win;

	gebr_save_widget_t	input;
	gebr_save_widget_t	output;
	gebr_save_widget_t	error;
} gebr_flow_io_t;

struct gebr {
	GtkWidget *			window;
	GtkWidget *			menu[MENUBAR_N];
	GtkWidget *			notebook;
	GtkWidget *			statusbar;
	struct about			about;

	GeoXmlFlow *			flow;

	/* status menu items */
	GtkWidget *			configured_menuitem;
	GtkWidget *			disabled_menuitem;
	GtkWidget *			unconfigured_menuitem;

	/* Persistant GUI */
	struct ui_project_line		ui_project_line;
	struct ui_flow_browse		ui_flow_browse;
	struct ui_flow_edition		ui_flow_edition;
	struct ui_job_control		ui_job_control;
	struct ui_preferences		ui_preferences;
	struct ui_servers		ui_servers;

		/* flow info window */
	gebr_flow_info_t	flow_info;

	/* flow io window. */
	gebr_flow_io_t		flow_io;

	struct config {
		/* config options from gengetopt
		 * loaded in gebr_config_load at gebr.c
		 */
		struct ggopt		ggopt;

		GString *		username;
		GString *		email;
		GString *		usermenus;
		GString *		data;
		GString *		editor;
		GString *		browser;
	} config;

	/* log file */
	struct log *			log;

	/* List of temporary file to be deleted */
	GSList *			tmpfiles;

	/* Pixmaps */
	struct pixmaps {
		GdkPixbuf *		configured_icon;
		GdkPixbuf *		unconfigured_icon;
		GdkPixbuf *		disabled_icon;
		GdkPixbuf *		running_icon;
	} pixmaps;
};

void
gebr_init(void);

gboolean
gebr_quit(void);

int
gebr_config_load(int argc, char ** argv);

int
gebr_config_reload(void);

int
gebr_config_save(void);

void
gebr_message(enum log_message_type type, gboolean in_statusbar, gboolean in_log_file, const gchar * message);

#endif //__GEBR_H
