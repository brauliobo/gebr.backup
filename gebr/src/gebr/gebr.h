/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#ifndef __GEBR_H
#define __GEBR_H

#include <gtk/gtk.h>

#include <geoxml.h>
#include <gui/about.h>
#include <misc/log.h>

#include "cmdline.h"
#include "interface.h"
#include "ui_project_line.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_job_control.h"
#include "ui_log.h"
#include "ui_preferences.h"
#include "ui_server.h"

/* global variable of common needed stuff */
extern struct gebr gebr;

struct gebr {
	GtkWidget *			window;
	GtkWidget *			menu[MENUBAR_N];
	GtkWidget *			notebook;
	struct about			about;
	GtkAccelGroup *			accel_group;
	/* for strange things ;) */
	GtkWidget *			invisible;

	/* the project or the line selected */
	GeoXmlDocument *		project_line;
	GeoXmlProject *			project;
	GeoXmlLine *			line;
	GeoXmlFlow *			flow;

	/* log file */
	struct log *			log;

	/* List of temporary file to be deleted */
	GSList *			tmpfiles;

	/* Persistant GUI */
	GtkWidget *			navigation_box_label;
	struct ui_project_line *	ui_project_line;
	struct ui_flow_browse *		ui_flow_browse;
	struct ui_flow_edition *	ui_flow_edition;
	struct ui_job_control *		ui_job_control;
	struct ui_log *			ui_log;
	struct ui_server_list *		ui_server_list;

	struct gebr_config {
		GKeyFile *		key_file;
		GString *		path;

		GString *		username;
		GString *		email;
		GString *		usermenus;
		GString *		data;
		GString *		editor;
		GString *		browser;
		int			width;
		int			height;
		gboolean		log_expander_state;
	} config;

	/* Pixmaps */
	struct gebr_pixmaps {
		GdkPixbuf *		stock_apply;
		GdkPixbuf *		stock_warning;
		GdkPixbuf *		stock_cancel;
		GdkPixbuf *		stock_execute;
		GdkPixbuf *		stock_connect;
		GdkPixbuf *		stock_disconnect;
		GdkPixbuf *		stock_go_back;
		GdkPixbuf *		stock_go_forward;
		GdkPixbuf *		stock_info;
	} pixmaps;

	struct gebr_actions {
		struct {
			GtkAction *             quit;
		} main;
		struct {
			GtkAction *		new_project;
			GtkAction *		new_line;
			GtkAction *		delete;
			GtkAction *		properties;
			GtkAction *		line_paths;
		} project_line;
		struct {
			GtkAction *		new;
			GtkAction *		delete;
			GtkAction *		properties;
			GtkAction *		io;
			GtkAction *		execute;
			GtkAction *		import;
			GtkAction *		export;
			GtkAction *		export_as_menu;
		} flow;
		struct {
			GtkAction *		help;
			GtkAction *		delete;
			GtkAction *		properties;
			GtkAction *		refresh;
			GtkRadioAction *	configured;
			GtkRadioAction *	disabled;
			GtkRadioAction *	unconfigured;
		} flow_edition;
		struct {
			GtkAction *		save;
			GtkAction *		cancel;
			GtkAction *		close;
			GtkAction *		clear;
			GtkAction *		stop;
		} job_control;
	} actions;
};

void
gebr_init(void);

gboolean
gebr_quit(void);

void
gebr_log_load(void);

void
gebr_config_load(void);

void
gebr_config_apply(void);

void
gebr_config_save(gboolean verbose);

void
gebr_message(enum log_message_type type, gboolean in_statusbar, gboolean in_log_file, const gchar * message, ...);

#endif //__GEBR_H
