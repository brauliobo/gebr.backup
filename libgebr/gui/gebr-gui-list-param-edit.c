/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

/* Structures {{{1 */
struct _GebrGuiListParamEdit
{
};

/* Prototypes {{{1*/

GtkWidget *gebr_gui_list_param_edit_create_tree_view(GebrGuiSequenceEdit *sequence_edit);

G_DEFINE_TYPE(GebrGuiListParamEdit,
	      gebr_gui_list_param_edit,
	      GEBR_GUI_TYPE_VALUE_SEQUENCE_EDIT);

/* GObject Initialization */
static void
gebr_gui_list_param_edit_init(GebrGuiListParamEdit *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_GUI_TYPE_LIST_PARAM_EDIT,
						 GebrGuiListParamEditPriv);
}

static void
gebr_gui_list_param_edit_class_init(GebrGuiListParamEditClass *klass)
{
	GebrGuiSequenceEditClass *sequence_edit_class;

	sequence_edit_class = GEBR_GUI_SEQUENCE_EDIT_CLASS(klass);
	sequence_edit_class->create_tree_view = gebr_gui_list_param_edit_create_tree_view;
	g_type_class_add_private(klass, sizeof(GebrGuiListParamEditPriv));
}

/* Overriden methods */
GtkWidget *
gebr_gui_list_param_edit_create_tree_view(GebrGuiSequenceEdit *sequence_edit)
{
	return NULL;
}

/* Public methods {{{1 */
GtkWidget *
gebr_gui_list_param_edit_new(GtkWidget *widget)
{
}
