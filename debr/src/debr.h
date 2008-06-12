/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#ifndef __GEBRME_H
#define __GEBRME_H

#include <gtk/gtk.h>
#include <geoxml.h>

#include <gui/about.h>
#include <misc/log.h>

#include "menu.h"
#include "program.h"
#include "parameter.h"

extern struct debr debr;

struct debr {
	/* current stuff being edited */
	GeoXmlFlow *		menu;
	GeoXmlProgram *		program;
	GeoXmlParameter *	parameter;

	/* diverse widgets */
	GtkWidget *		window;
	struct about		about;
	GtkWidget *		statusbar;
	GtkWidget *		invisible;
	GtkAccelGroup *		accel_group;
	struct ui_menu		ui_menu;
	struct ui_program	ui_program;
	struct ui_parameter	ui_parameter;

	/* actions */
	struct debr_actions {
		struct {
			GtkAction *             quit;
		} main;
		struct {
			GtkAction *		new;
			GtkAction *		open;
			GtkAction *		properties;
			GtkAction *		save;
			GtkAction *		save_as;
			GtkAction *		revert;
			GtkAction *		delete;
			GtkAction *		close;
		} menu;
		struct {
			GtkAction *		new;
			GtkAction *		delete;
			GtkAction *		properties;
			GtkAction *		up;
			GtkAction *		down;
		} program;
		struct {
			GtkAction *		new;
			GtkAction *		delete;
			GtkAction *		duplicate;
			GtkAction *		properties;
			GtkAction *		up;
			GtkAction *		down;
			GtkAction *		change_type;
		} parameter;
	} actions;

        /* icons */
	struct debr_pixmaps {
        	GdkPixbuf *	stock_no;
	} pixmaps;

	/* config file */
	struct debr_config {
		GKeyFile *	keyfile;
		GString *	path;

		GString *	name;
		GString *	email;
		GString *	menu_dir;
		GString *	htmleditor;
		GString *	browser;
	} config;

	/* temporary files removed when GeBRME quits */
	GSList *		tmpfiles;
};

void
debr_init(void);

void
debr_quit(void);

void
debr_config_load(void);

void
debr_config_save(void);

void
debr_message(enum log_message_type type, const gchar * message, ...);

#endif //__GEBRME_H
