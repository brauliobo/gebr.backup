/*   libgebr - GeBR Library
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

#ifndef __GEBR_GUI_LIST_PARAM_EDIT_H__
#define __GEBR_GUI_LIST_PARAM_EDIT_H__

#include <gtk/gtk.h>
#include <geoxml.h>

#include "gebr-gui-value-sequence-edit.h"

G_BEGIN_DECLS

#define GEBR_GUI_TYPE_LIST_PARAM_EDIT			(gebr_gui_list_param_edit_get_type())
#define GEBR_GUI_LIST_PARAM_EDIT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_LIST_PARAM_EDIT, GebrGuiListParamEdit))
#define GEBR_GUI_LIST_PARAM_EDIT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_LIST_PARAM_EDIT, GebrGuiListParamEditClass))
#define GEBR_GUI_IS_LIST_PARAM_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_LIST_PARAM_EDIT))
#define GEBR_GUI_IS_LIST_PARAM_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_LIST_PARAM_EDIT))
#define GEBR_GUI_LIST_PARAM_EDIT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_LIST_PARAM_EDIT, GebrGuiListParamEditClass))

typedef struct _GebrGuiListParamEdit GebrGuiListParamEdit;
typedef struct _GebrGuiListParamEditPriv GebrGuiListParamEditPriv;
typedef struct _GebrGuiListParamEditClass GebrGuiListParamEditClass;

struct _GebrGuiListParamEdit {
	GebrGuiValueSequenceEdit parent;

	/*< private >*/
	GebrGuiListParamEdit *priv;
};

struct _GebrGuiListParamEditClass {
	GebrGuiValueSequenceEditClass parent_class;
};

GType gebr_gui_list_param_edit_get_type(void) G_GNUC_CONST;

GtkWidget *gebr_gui_list_param_edit_new(GtkWidget *widget);

G_END_DECLS

#endif /* __GEBR_GUI_LIST_PARAM_EDIT_H__ */
