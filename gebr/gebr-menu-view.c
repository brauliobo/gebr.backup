/*
 * gebr-menu-view.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR core team (www.gebrproject.com)
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

#include "gebr-menu-view.h"
#include "gebr.h"

#include <libgebr/utils.h>
#include <glib/gi18n.h>
#include <glib-object.h>

G_DEFINE_TYPE(GebrMenuView, gebr_menu_view, G_TYPE_OBJECT);

struct _GebrMenuViewPriv {
	GtkTreeStore *tree_store;
	GtkTreeView *tree_view;
	GtkTreeModel *filter;

	GtkWidget *entry;

	GtkWidget *vbox;
};

enum {
	ADD_MENU = 0,
	LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0, };

/*
 * Private methods
 */
static gboolean
gebr_menu_view_get_selected_menu(GtkTreeIter * iter,
                                 gboolean warn_unselected,
                                 GebrMenuView *view)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view->priv->tree_view));
	if (gtk_tree_selection_get_selected(selection, &model, iter) == FALSE) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No menu selected."));
		return FALSE;
	}
	if (gtk_tree_model_iter_has_child(model, iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Select a menu instead of a category."));
		return FALSE;
	}

	return TRUE;
}

static void
gebr_menu_view_add(GtkTreeView *tree_view,
                   GtkTreePath *path,
                   GtkTreeViewColumn *column,
                   GebrMenuView *view)
{
	GtkTreeIter iter;
	gchar *filename;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	if (!gebr_menu_view_get_selected_menu(&iter, FALSE, view)) {
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(tree_view), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree_view), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(tree_view), path, FALSE);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(view->priv->filter), &iter,
	                   MENU_FILEPATH_COLUMN, &filename,
	                   -1);

	if (filename && *filename)
		g_signal_emit(view, signals[ADD_MENU], 0, filename);

	g_free(filename);
}

static GtkMenu *
gebr_menu_view_popup_menu(GtkWidget * widget,
                          GebrMenuView *view)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_refresh")));

	if (!gebr_menu_view_get_selected_menu(&iter, FALSE, view)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), view->priv->tree_view);
		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), view->priv->tree_view);
		goto out;
	}

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(gebr_menu_view_add), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), view->priv->tree_view);
	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), view->priv->tree_view);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

//static void
//gebr_menu_view_show_help(void)
//{
//	GtkTreeIter iter;
//	gchar *menu_filename;
//	GebrGeoXmlFlow *menu;
//
//	if (!flow_browse_get_selected_menu(&iter, TRUE))
//		return;
//
//	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter,
//			   MENU_FILEPATH_COLUMN, &menu_filename, -1);
//
//	menu = menu_load_path(menu_filename);
//	if (menu == NULL)
//		goto out;
//	gebr_help_show(GEBR_GEOXML_OBJECT(menu), TRUE);
//
//out:	g_free(menu_filename);
//}


static gboolean
gebr_menu_view_visible_func(GtkTreeModel *model,
                            GtkTreeIter *iter,
                            const gchar *key,
                            GebrMenuView *view)
{
	if (!key || !*key)
		return TRUE;

	gchar *title, *desc;
	gchar *lt, *ld, *lk; // Lower case strings
	gboolean match = FALSE;

	gtk_tree_model_get(model, iter,
	                   MENU_TITLE_COLUMN, &title,
	                   MENU_DESCRIPTION_COLUMN, &desc,
	                   -1);

	lt = title ? g_utf8_strdown(title, -1) : g_strdup("");
	ld = desc ? g_utf8_strdown(desc, -1) : g_strdup("");
	lk = g_utf8_strdown(key, -1);

	match = gebr_utf8_strstr(lt, lk) || gebr_utf8_strstr(ld, lk);

	g_free(lt);
	g_free(ld);
	g_free(lk);
	g_free(title);
	g_free(desc);

	return match;
}

static void
on_search_entry_activate(GtkEntry *entry,
                         GebrMenuView *view)
{
	const gchar *key = gtk_entry_get_text(entry);
	if (!key || !*key)
		return;

	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view->priv->tree_view);

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gebr_menu_view_add(view->priv->tree_view, path, NULL, view);
		gtk_tree_path_free(path);
	}
}

static void
on_search_entry_press(GtkEntry *entry,
                      GtkEntryIconPosition pos,
                      GdkEvent *event,
                      GebrMenuView *view)
{
	if (pos == GTK_ENTRY_ICON_SECONDARY)
		gtk_entry_set_text(entry, "");
}

static void
update_menus_visibility(GebrMenuView *view,
                        const gchar *key)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(view->priv->tree_store);
	gboolean valid;

	valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		gboolean has_visible_child = FALSE;
		GtkTreeIter child;
		gboolean was_visible;

		valid = gtk_tree_model_iter_children(model, &child, &iter);
		while (valid) {
			gboolean visible = gebr_menu_view_visible_func(model, &child, key, view);
			if (visible && !has_visible_child)
				has_visible_child = TRUE;

			gtk_tree_model_get(model, &child, MENU_VISIBLE_COLUMN, &was_visible, -1);

			if (was_visible != visible)
				gtk_tree_store_set(view->priv->tree_store, &child,
						   MENU_VISIBLE_COLUMN, visible,
						   -1);
			valid = gtk_tree_model_iter_next(model, &child);
		}

		gtk_tree_model_get(model, &iter, MENU_VISIBLE_COLUMN, &was_visible, -1);
		if (was_visible != has_visible_child)
			gtk_tree_store_set(view->priv->tree_store, &iter,
					   MENU_VISIBLE_COLUMN, has_visible_child,
					   -1);

		if (has_visible_child) {
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_row(view->priv->tree_view, path, TRUE);
			gtk_tree_path_free(path);
		}

		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

static void
on_search_entry(GtkWidget *widget,
                GebrMenuView *view)
{
	const gchar *key = gtk_entry_get_text(GTK_ENTRY(widget));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view->priv->tree_view);

	if (key && *key) {
		GtkTreeIter iter, child;
		if (gtk_tree_model_get_iter_first(view->priv->filter, &iter)) {
			if (gtk_tree_model_iter_children(view->priv->filter, &child, &iter))
				gtk_tree_selection_select_iter(selection, &child);
			else
				gtk_tree_selection_select_iter(selection, &iter);
		}
		gtk_entry_set_icon_sensitive(GTK_ENTRY(view->priv->entry), GTK_ENTRY_ICON_SECONDARY, TRUE);
	} else {
		gtk_entry_set_icon_sensitive(GTK_ENTRY(view->priv->entry), GTK_ENTRY_ICON_SECONDARY, FALSE);
	}

	update_menus_visibility(view, key);
}

static void
on_menu_view_data_func(GtkTreeViewColumn *tree_column,
                       GtkCellRenderer *cell,
                       GtkTreeModel *model,
                       GtkTreeIter *iter)
{
	gchar *title;
	gchar *desc;
	gchar *escape_text;

	gtk_tree_model_get(model, iter,
	                   MENU_TITLE_COLUMN, &title,
	                   MENU_DESCRIPTION_COLUMN, &desc,
	                   -1);

	// Category
	if (!desc) {
		escape_text = g_markup_printf_escaped("<b>%s</b>", title);
		g_object_set(cell, "markup", escape_text, NULL);
	} else {
		escape_text = g_markup_printf_escaped("%s\n<small>%s</small>", title, desc);
		g_object_set(cell, "markup", escape_text, NULL);
	}

	g_free(desc);
	g_free(title);
	g_free(escape_text);
}

static void
gebr_menu_view_init(GebrMenuView *view)
{
	view->priv = G_TYPE_INSTANCE_GET_PRIVATE(view,
			GEBR_TYPE_MENU_VIEW,
			GebrMenuViewPriv);

	view->priv->tree_view = GTK_TREE_VIEW(gtk_tree_view_new());
	view->priv->tree_store = gtk_tree_store_new(MENU_N_COLUMN,
	                                            G_TYPE_STRING,
	                                            G_TYPE_STRING,
	                                            G_TYPE_STRING,
	                                            G_TYPE_BOOLEAN);
	view->priv->vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_set_size_request(GTK_WIDGET(view->priv->vbox), 500, 450);

	view->priv->filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(view->priv->tree_store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(view->priv->filter), MENU_VISIBLE_COLUMN);

	gtk_tree_view_set_model(view->priv->tree_view,
	                        GTK_TREE_MODEL(view->priv->filter));
	gtk_tree_view_set_headers_visible(view->priv->tree_view, FALSE);

	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(view->priv->tree_view));

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view->priv->tree_view),
	                                          (GebrGuiGtkPopupCallback) gebr_menu_view_popup_menu,
	                                          view);

	g_signal_connect(GTK_OBJECT(view->priv->tree_view), "row-activated",
	                 G_CALLBACK(gebr_menu_view_add), view);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;

	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(view->priv->tree_view), col);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer,
	                                        (GtkTreeCellDataFunc)on_menu_view_data_func,
	                                        NULL, NULL);
	g_object_set(renderer, "ypad", 2, NULL);
	g_object_set(renderer, "wrap-mode", PANGO_WRAP_WORD_CHAR, "wrap-width", 490, NULL);


	/*
	 * Create search entry
	 */
	view->priv->entry = gtk_entry_new();

	gtk_entry_set_icon_from_stock(GTK_ENTRY(view->priv->entry),
	                              GTK_ENTRY_ICON_PRIMARY,
	                              GTK_STOCK_FIND);

	gtk_entry_set_icon_from_stock(GTK_ENTRY(view->priv->entry),
	                              GTK_ENTRY_ICON_SECONDARY,
	                              GTK_STOCK_CLOSE);
	gtk_entry_set_icon_sensitive(GTK_ENTRY(view->priv->entry), GTK_ENTRY_ICON_SECONDARY, FALSE);

	g_signal_connect(view->priv->entry, "icon-press", G_CALLBACK(on_search_entry_press), view);
	g_signal_connect(view->priv->entry, "activate", G_CALLBACK(on_search_entry_activate), view);
	g_signal_connect(view->priv->entry, "changed", G_CALLBACK(on_search_entry), view);

	// Add Search entry
	gtk_box_pack_start(GTK_BOX(view->priv->vbox), view->priv->entry, FALSE, FALSE, 5);

	// Add menu list
	gtk_box_pack_start(GTK_BOX(view->priv->vbox), scrolled_window, TRUE, TRUE, 0);
}

static void
gebr_menu_view_class_init(GebrMenuViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	signals[ADD_MENU] =
		g_signal_new("add-menu",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrMenuViewClass, add_menu),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1,
			     G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(GebrMenuViewPriv));
}

/*
 * Public Methods
 */

GebrMenuView *
gebr_menu_view_new(void)
{
	return g_object_new(GEBR_TYPE_MENU_VIEW, NULL);
}

GtkTreeStore *
gebr_menu_view_get_model(GebrMenuView *view)
{
	return view->priv->tree_store;
}

GtkWidget *
gebr_menu_view_get_widget(GebrMenuView *view)
{
	return view->priv->vbox;
}
