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

#ifndef __GEBR_H
#define __GEBR_H

#include <glib.h>

#include <geoxml.h>
#include <gui/about.h>

#include "ui_flow_component.h"

/* global variable of common needed stuff */
extern struct gebr gebr;

enum msg_type {
	START, END, ACTION, SERVER, ERROR, WARNING, INTERFACE
};

#define NABAS 4

/* Projects/Lines store fields */
enum {
   PL_NAME = 0,
   PL_FILENAME,
   PL_N_COLUMN
};

/* Flow browser store fields */
enum {
   FB_NAME = 0,
   FB_FILENAME,
   FB_N_COLUMN
};

/* Menubar */
enum {
   MENUBAR_PROJECT,
   MENUBAR_LINE,
   MENUBAR_FLOW,
   MENUBAR_FLOW_COMPONENTS,
   MENUBAR_N
};

/* Menu store fields */
enum {
   MENU_TITLE_COLUMN,
   MENU_DESC_COLUMN,
   MENU_FILE_NAME_COLUMN,
   MENU_N_COLUMN
};


/* Job control store fields */
enum {
   JC_ICON,
   JC_TITLE,
   JC_STRUCT,
   JC_N_COLUMN
};


/* Server store field */
enum {
   SERVER_ADDRESS,
   SERVER_POINTER,
   SERVER_N_COLUMN
};

typedef struct {
	GtkWidget *hbox;

	GtkWidget *entry;
	GtkWidget *browse_button;
} gebr_save_widget_t;

typedef struct {

   GtkWidget *win;

   GtkWidget *username;
   GtkWidget *email;
   GtkWidget *usermenus;
   GtkWidget *data;
   GtkWidget *editor;
   GtkWidget *browser;

} gebrw_pref_t;

typedef struct {
   GtkWidget *win;

   GtkWidget *title;
   GtkWidget *description;
   GtkWidget *help;
   GtkWidget *author;
   GtkWidget *email;

} gebr_flow_prop_t;

typedef struct {
	GtkWidget *		win;

	gebr_save_widget_t	input;
	gebr_save_widget_t	output;
	gebr_save_widget_t	error;
} gebr_flow_io_t;

typedef struct {

   GtkWidget *title;
   GtkWidget *description;

   GtkWidget *input_label;
   GtkWidget *input;
   GtkWidget *output_label;
   GtkWidget *output;
   GtkWidget *error_label;
   GtkWidget *error;

   GtkWidget *help;

   GtkWidget *author;

} gebr_flow_info_t;

typedef struct {
   GdkPixbuf *	configured_icon;
   GdkPixbuf *	unconfigured_icon;
   GdkPixbuf *	disabled_icon;
   GdkPixbuf *	running_icon;
} gebr_pixmaps;

typedef struct {
	GtkWidget *	job_label;
	GtkWidget *	text_view;
	GtkTextBuffer *	text_buffer;
} gebr_job_control_t;

struct gebr {
	GtkWidget *			mainwin;
	GtkWidget *			menu[MENUBAR_N];
	GtkWidget *			notebook;
	GtkWidget *			statusbar;
	struct about			about;

	struct config {
		/* config options from gengetopt
		 * loaded in gebr_config_load at callbacks.c
		 */
		struct ggopt		config;

		GString *		username;
		GString *		email;
		GString *		usermenus;
		GString *		data;
		GString *		editor;
		GString *		browser;
	} config;

	struct ui_flow_component	ui_flow_component;

	/* Trees and Lists */
	GtkTreePath  *proj_line_selection_path;
	GtkTreeStore *proj_line_store;
	GtkListStore *flow_store;

        GtkListStore *job_store;
        GtkListStore *server_store;

	/* Views */
	GtkWidget *proj_line_view; /* projects and lines             */
	GtkWidget *flow_view;      /* flows of a line                */

        GtkWidget *job_view;       /* Running jobs                   */
        GtkWidget *server_view;    /* Server view                    */

	/* preferences window */
	gebrw_pref_t		pref;
	/* log file */
	FILE *			log_fp;

	/* flow info window */
	gebr_flow_info_t	flow_info;

	/* flow properties window */
	gebr_flow_prop_t	flow_prop;

	/* job control */
	gebr_job_control_t	job_ctrl;

	/* status menu items */
	GtkWidget *		configured_menuitem;
	GtkWidget *		disabled_menuitem;
	GtkWidget *		unconfigured_menuitem;

	/* flow io window. */
	gebr_flow_io_t		flow_io;

	GtkWidget *	parameters;
	GtkWidget **	parwidgets;
	int		parwidgets_number;
	int		program_index;

	/* Pixmaps */
	gebr_pixmaps pixmaps;

	/* List of temporary file to be deleted */
	GSList * tmpfiles;
}

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
log_message(enum msg_type type, const gchar * message, gboolean in_statusbar);

#endif //__GEBR_H
