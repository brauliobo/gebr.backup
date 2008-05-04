/*   GeBR ME - GeBR Menu Editor
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

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <gui/utils.h>

#include "callbacks.h"
#include "gebrme.h"
#include "support.h"
#include "menu.h"
#include "program.h"
#include "preferences.h"

/*
 * File: callbacks.c
 * General interface callbacks. See <interface.c>
 */

/*
 * Function: on_menu_new_activate
 * Call <menu_new>
 */
void
on_menu_new_activate(void)
{
	menu_new();
}

/*
 * Function: on_menu_open_activate
 * Create a open dialog and manage it to open a menu
 */
void
on_menu_open_activate(void)
{
	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;

	/* create file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Open flow"), NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_OPEN, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("System flow files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* run file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	/* open it */
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	menu_open(path, TRUE);

	g_free(path);
out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: on_menu_save_activate
 * Save a menu. If it doesn't have a path assigned, call <on_menu_save_as_activate>
 */
void
on_menu_save_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			path;

	/* get path of selection */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebrme.ui_menu.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_menu.list_store), &iter,
		MENU_PATH, &path,
		-1);

	/* is this a new menu? */
	if (strlen(path)) {
		menu_save(path);
		g_free(path);
		return;
	}

	g_free(path);
	on_menu_save_as_activate();
}

/*
 * Function: on_menu_save_as_activate
 * Open a save dialog; get the path and save the menu at it
 */
void
on_menu_save_as_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;

	gchar *			path;
	gchar *			filename;

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose file"), GTK_WINDOW(gebrme.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_SAVE, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("System flow files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	if (gebrme.config.menu_dir != NULL && strlen(gebrme.config.menu_dir->str) > 0){
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), gebrme.config.menu_dir->str);
	}

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;
	path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser_dialog));
	filename = g_path_get_basename(path);

	/* get selection, change the view and save to disk */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebrme.ui_menu.tree_view));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	gtk_list_store_set (gebrme.ui_menu.list_store, &iter,
			MENU_FILENAME, filename,
			MENU_PATH, path,
			-1);
	geoxml_document_set_filename(GEOXML_DOC(gebrme.menu), filename);
	menu_save(path);
	g_free(path);
	g_free(filename);

out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: on_menu_new_activate
 * Confirm action and if confirmed reload menu from file
 */
void
on_menu_revert_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			path;
	GeoXmlFlow *		menu;

	if (confirm_action_dialog(_("Revert changes"), _("All unsaved changes will be lost. Are you sure you want to revert flow '%s'?"),
	geoxml_document_get_filename(GEOXML_DOC(gebrme.menu))) == FALSE)
		return;

	/* get path of selection */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebrme.ui_menu.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_menu.list_store), &iter,
		MENU_PATH, &path,
		-1);

	/* is this a new menu? */
	if (!strlen(path)) {
		gebrme_message(LOG_ERROR, _("Menu was not saved yet."));
		g_free(path);
		return;
	}

	menu = menu_load(path);
	if (menu == NULL)
		return;
	/* revert to the one in disk */
	geoxml_document_free(GEOXML_DOC(gebrme.menu));
	gtk_list_store_set(gebrme.ui_menu.list_store, &iter,
		MENU_XMLPOINTER, menu,
		-1);
	menu_selected();
	menu_saved_status_set(MENU_STATUS_SAVED);

	/* frees */
	g_free(path);
}

/*
 * Function: on_menu_new_activate
 * Confirm action and if confirm delete it from the disk and call <on_menu_close_activate>
 */
void
on_menu_delete_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			path;

	if (confirm_action_dialog(_("Delete flow"), _("Are you sure you delete flow '%s'?"),
	geoxml_document_get_filename(GEOXML_DOC(gebrme.menu))) == FALSE)
		return;

	/* get path of selection */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_menu.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_menu.list_store), &iter,
		MENU_PATH, &path,
		-1);

	if (g_unlink(path)) {
		GtkWidget *	dialog;

		dialog = gtk_message_dialog_new(GTK_WINDOW(gebrme.window),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			_("Could not delete flow"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	} else
		on_menu_close_activate();

	g_free(path);
}

/*
 * Function: on_menu_new_activate
 * Delete menu from the view.
 */
void
on_menu_close_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GdkPixbuf *		pixbuf;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_menu.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_menu.list_store), &iter,
				MENU_STATUS, &pixbuf,
				-1);

	if (pixbuf == gebrme.pixmaps.stock_no) {
		GtkWidget *	dialog;
		gboolean	cancel;

		/* TODO: add cancel button */
		cancel = FALSE;
		dialog = gtk_message_dialog_new(GTK_WINDOW(gebrme.window),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("This flow has unsaved changes. Do you want to save it?"));
		switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
		case GTK_RESPONSE_YES: {
			gchar *	path;

			gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_menu.list_store), &iter,
				MENU_PATH, &path,
				-1);

			geoxml_document_save(GEOXML_DOC(gebrme.menu), path);
			break;
		} case GTK_RESPONSE_NO:
			break;
		case GTK_RESPONSE_CANCEL:
			cancel = TRUE;
			return;
		}

		gtk_widget_destroy(dialog);
		if (cancel == TRUE)
			return;
	}

	geoxml_document_free(GEOXML_DOC(gebrme.menu));
	gtk_list_store_remove(gebrme.ui_menu.list_store, &iter);

	if (gtk_tree_model_iter_n_children(model, NULL) == 0)
		menu_new();
	else {
		GtkTreeIter	first_iter;

		gtk_tree_model_iter_nth_child(model, &first_iter, NULL, 0);
		gtk_tree_selection_select_iter(selection, &first_iter);
		menu_selected();
	}
}

/*
 * Function: on_program_new_activate
 * Call <program_new>
 */
void
on_program_new_activate(void)
{
	program_new();
}

/*
 * Function: on_program_new_activate
 * Call <program_remove>
 */
void
on_program_delete_activate(void)
{
	program_remove();
}

/*
 * Function: on_parameter_new_activate
 * Call <parameter_new>
 */
void
on_parameter_new_activate(void)
{
// 	parameter_new();
}

/*
 * Function: on_parameter_new_activate
 * Call <parameter_remove>
 */
void
on_parameter_delete_activate(void)
{
// 	parameter_remove();
}

/*
 * Function: on_configure_preferences_activate
 * Call <create_preferences_window>
 */
void
on_configure_preferences_activate(void)
{
	create_preferences_window();
}

/*
 * Function: on_help_about_activate
 * Show gebrme.about.dialog
 */
void
on_help_about_activate(void)
{
	gtk_widget_show(gebrme.about.dialog);
}
