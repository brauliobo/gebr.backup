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
struct _GebrGuiListParamEditPriv
{
};

/* Prototypes {{{1*/
GtkWidget *gebr_gui_list_param_edit_create_tree_view(GebrGuiSequenceEdit *sequence_edit);

G_DEFINE_TYPE(GebrGuiListParamEdit,
	      gebr_gui_list_param_edit,
	      GEBR_GUI_TYPE_VALUE_SEQUENCE_EDIT);

/* GObject Stuff {{{1 */
static void
gebr_gui_list_param_edit_get_property(GObject      *object,
				      guint         property_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gebr_gui_list_param_edit_set_property(GObject    *object,
				      guint       property_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

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
	GObjectClass *gobject_class;
	GebrGuiSequenceEditClass *sequence_edit_class;

	gobject_class = G_OBJECT_CLASS(klass);
	sequence_edit_class = GEBR_GUI_SEQUENCE_EDIT_CLASS(klass);

	gobject_class->get_property = gebr_gui_list_param_edit_get_property;
	gobject_class->set_property = gebr_gui_list_param_edit_set_property;
	sequence_edit_class->create_tree_view = gebr_gui_list_param_edit_create_tree_view;
	sequence_edit_class->remove = gebr_gui_list_param_edit_remove;
	sequence_edit_class->move = gebr_gui_list_param_edit_move;
	sequence_edit_class->move_top = gebr_gui_list_param_edit_move_top;
	sequence_edit_class->move_bottom = gebr_gui_list_param_edit_move_bottom;

	g_type_class_add_private(klass, sizeof(GebrGuiListParamEditPriv));
}

/* Overriden methods {{{1 */
static GtkWidget *
gebr_gui_list_param_edit_create_tree_view(GebrGuiSequenceEdit *sequence_edit)
{
	return NULL;
}

static void
gebr_gui_list_param_edit_remove(GebrGuiSequenceEdit *sequence_edit,
			        GtkTreeIter *iter)
{
}

static void
gebr_gui_list_param_edit_move(GebrGuiSequenceEdit *sequence_edit,
			      GtkTreeIter *iter,
			      GtkTreeIter *position,
			      GtkTreeViewDropPosition drop_position)
{
}

static void
gebr_gui_list_param_edit_move_top(GebrGuiSequenceEdit *sequence_edit,
				  GtkTreeIter *iter)
{
}

static void
gebr_gui_list_param_edit_move_bottom(GebrGuiSequenceEdit *sequence_edit,
				     GtkTreeIter *iter)
{
}

/* Public methods {{{1 */
GtkWidget *
gebr_gui_list_param_edit_new(GtkWidget *widget)
{
	GtkListStore *store;

	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

	return g_object_new(GEBR_GUI_TYPE_LIST_PARAM_EDIT,
			    "value-widget", widget,
			    "list-store", store,
			    NULL);
}
