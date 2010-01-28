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

#ifndef __PARAMETER_H
#define __PARAMETER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>
#include <libgebr/geoxml.h>

#include <libgebr/intl.h>
#include <libgebr/gui/parameter.h>

/* same order as combo_box_map */
const GtkRadioActionEntry parameter_type_radio_actions_entries[] = {
	{"parameter_type_real", NULL, N_("real"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_FLOAT},
	{"parameter_type_integer", NULL, N_("integer"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_INT},
	{"parameter_type_range", NULL, N_("range"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_RANGE},
	{"parameter_type_flag", NULL, N_("flag"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_FLAG},
	{"parameter_type_text", NULL, N_("text"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_STRING},
	{"parameter_type_enum", NULL, N_("enum"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_ENUM},
	{"parameter_type_file", NULL, N_("file"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_FILE},
	{"parameter_type_group", NULL, N_("group"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_GROUP},
};
const gsize combo_type_map_size = 8;

enum {
	PARAMETER_TYPE,
	PARAMETER_KEYWORD,
	PARAMETER_LABEL,
	PARAMETER_XMLPOINTER,
	PARAMETER_N_COLUMN
};

struct ui_parameter {
	GtkWidget *widget;

	GtkTreeStore *tree_store;
	GtkWidget *tree_view;
};

struct ui_parameter_dialog {
	GtkWidget *dialog;

	GebrGeoXmlParameter *parameter;

	GtkWidget *default_widget_hbox;
	struct gebr_gui_parameter_widget *gebr_gui_parameter_widget;
	GtkWidget *separator_entry;
};

/**
 * Set interface and its callbacks.
 */
void parameter_setup_ui(void);

/**
 * Load current program parameters' to the UI.
 */
void parameter_load_program(void);

void parameter_load_selected(void);

void parameter_new(void);

void parameter_remove(gboolean confirm);

void parameter_top(void);

void parameter_bottom(void);

gboolean parameter_change_type_setup_ui(void);

void parameter_copy(void);

void parameter_paste(void);

void parameter_change_type(enum GEBR_GEOXML_PARAMETER_TYPE type);

void parameter_properties(void);

#ifdef __cplusplus
}
#endif

#endif				//__PARAMETER_H
