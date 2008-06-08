/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
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

/*
 * File: interface.c
 * Assembly the main components of the interface
 *
 * This function assemblies the main window, preferences and menu bar.
 */

#include <string.h>

#include <gdk/gdkkeysyms.h>

#include <gui/pixmaps.h>

#include "interface.h"
#include "gebr.h"
#include "support.h"
#include "callbacks.h"

/*
 * Prototypes functions
 */

static GtkWidget *
assembly_project_menu(void);

static GtkWidget *
assembly_line_menu(void);

static GtkWidget *
assembly_flow_menu(void);

static GtkWidget *
assembly_flow_components_menu(void);

static GtkWidget *
assembly_config_menu(void);

static GtkWidget *
assembly_help_menu(void);

/*
 * Public functions
 */

/*
 * Function: assembly_interface
 * Assembly the whole interface.
 *
 */
void
assembly_interface(void)
{
	GtkWidget *	vboxmain;
	GtkWidget *	menu_bar;
	GtkWidget *	pagetitle;
	GClosure *	closure;

	GtkWidget *	menu;
	GtkWidget *	menu_item;
	GtkWidget *	child_menu_item;

	gebr.about = about_setup_ui("GêBR", _("A plug-and-play environment to\nseismic processing tools"));
	gebr.ui_server_list = server_list_setup_ui();

	/* Create the main window */
	gtk_window_set_default_icon (pixmaps_gebr_icon_16x16());
	gebr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gebr.window), "GêBR");
	gtk_widget_set_size_request(gebr.window, 700, 400);
	gtk_widget_show(gebr.window);

	/* Signals */
	g_signal_connect(GTK_OBJECT(gebr.window), "delete_event",
			GTK_SIGNAL_FUNC(gebr_quit), NULL);

	/* Create the main vbox to hold menu, notebook and status bar */
	vboxmain = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(gebr.window), vboxmain);
	gtk_widget_show(vboxmain);

	/*
	 * Actions
	 */
	/* Flow */
	gebr.actions.flow.new = gtk_action_new("flow_new",
		NULL, NULL, GTK_STOCK_NEW);
	g_signal_connect(gebr.actions.flow.new, "activate",
		(GCallback)on_flow_new_activate, NULL);
	gebr.actions.flow.delete = gtk_action_delete("flow_delete",
		NULL, NULL, GTK_STOCK_DELETE);
	g_signal_connect(gebr.actions.flow.delete, "activate",
		(GCallback)on_flow_delete_activate, NULL);
	gebr.actions.flow.properties = gtk_action_properties("flow_properties",
		NULL, NULL, GTK_STOCK_PROPERTIES);
	g_signal_connect(gebr.actions.flow.properties, "activate",
		(GCallback)on_flow_properties_activate, NULL);
	gebr.actions.flow.io = gtk_action_io("flow_io",
		_("Input and Output"), NULL, GTK_STOCK_JUMP_TO);
	g_signal_connect(gebr.actions.flow.io, "activate",
		(GCallback)on_flow_io_activate, NULL);
	gebr.actions.flow.execute = gtk_action_execute("flow_execute",
		NULL, NULL, GTK_STOCK_EXECUTE);
	g_signal_connect(gebr.actions.flow.execute, "activate",
		(GCallback)on_flow_execute_activate, NULL);
	gebr.actions.flow.import = gtk_action_import("flow_import",
		_("Import"), NULL, NULL);
	g_signal_connect(gebr.actions.flow.import, "activate",
		(GCallback)on_flow_import_activate, NULL);
	gebr.actions.flow.export = gtk_action_export("flow_export",
		_("Export"), NULL, GTK_STOCK_CONVERT);
	g_signal_connect(gebr.actions.flow.export, "activate",
		(GCallback)on_flow_export_activate, NULL);
	gebr.actions.flow.export_as_menu = gtk_action_export_as_menu("flow_export_as_menu",
		NULL, NULL, GTK_STOCK_NEW);
	g_signal_connect(gebr.actions.flow.export_as_menu, "activate",
		(GCallback)on_flow_export_as_menu_activate, NULL);

	/* Flow Edition */
	gebr.actions.flow_edition.configured = gtk_radio_action_new("configured", _("Configured"), NULL, NULL, 1<<0);
	gebr.actions.flow_edition.disabled = gtk_radio_action_new("disabled", _("Disabled"), NULL, NULL, 1<<1);
	gebr.actions.flow_edition.unconfigured = gtk_radio_action_new("unconfigured", _("Unconfigured"), NULL, NULL, 1<<2);
	gtk_radio_action_set_group(gebr.actions.flow_edition.disabled,
		gtk_radio_action_get_group(gebr.actions.flow_edition.configured));
	gtk_radio_action_set_group(gebr.actions.unconfigured,
		gtk_radio_action_get_group(gebr.actions.flow_edition.configured));
	g_signal_connect(gebr.actions.flow_edition.configured, "activate",
		GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);
	g_signal_connect(gebr.actions.flow_edition.disabled, "activate",
		GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);
	g_signal_connect(gebr.actions.flow_edition.unconfigured, "activate",
		GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);

	/*
	 * Create the main menu
	 */
	menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vboxmain), menu_bar, FALSE, FALSE, 0);

	gebr.menu[MENUBAR_PROJECT] = assembly_project_menu();
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), gebr.menu[MENUBAR_PROJECT]);

	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label(_("Flow"));
	gtk_menu_item_set_child_menu_item(GTK_MENU_ITEM(menu_item), menu);

	/* New entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_new_activate), NULL);
	/* Import entry */
	child_menu_item = gtk_image_menu_item_new_with_label(_("Import"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_import_activate), NULL);
	/* Export entry */
	child_menu_item = gtk_image_menu_item_new_with_label(_("Export"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_export_activate), NULL);
	/* Export as Menu entry */
	child_menu_item = gtk_image_menu_item_new_with_label(_("Export as Menu"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(child_menu_item), gtk_image_new_from_stock(GTK_STOCK_CONVERT, 1));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			 GTK_SIGNAL_FUNC(on_flow_export_as_menu_activate), NULL);
	/* Delete entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_delete_activate), NULL);

	/* Separation line */
	child_menu_item = gtk_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);

	/* Properties entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_properties_activate), NULL);
	/* Input/Output entry */
	child_menu_item = gtk_image_menu_item_new_with_label(_("Input/Output"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_io_activate), NULL);
	/* Execute entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_EXECUTE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_execute_activate), NULL);

	gebr.menu[MENUBAR_LINE] = assembly_line_menu();
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), gebr.menu[MENUBAR_LINE]);

	gebr.menu[MENUBAR_FLOW] = assembly_flow_menu();
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), gebr.menu[MENUBAR_FLOW]);

	gebr.menu[MENUBAR_FLOW_COMPONENTS] = assembly_flow_components_menu();
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), gebr.menu[MENUBAR_FLOW_COMPONENTS]);

	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), assembly_config_menu());
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), assembly_help_menu());

	gtk_widget_show_all(menu_bar);

	/*
	 * Create a notebook to hold several pages
	 */
	gebr.notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vboxmain), gebr.notebook, TRUE, TRUE, 0);
	gtk_widget_show(gebr.notebook);

	gebr.ui_project_line = project_line_setup_ui();
	gtk_widget_show_all(gebr.ui_project_line->widget);
	pagetitle = gtk_label_new(_("Projects and Lines"));
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), gebr.ui_project_line->widget, pagetitle);

	gebr.ui_flow_browse = flow_browse_setup_ui();
	gtk_widget_show_all(gebr.ui_flow_browse->widget);
	pagetitle = gtk_label_new(_("Flows"));
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), gebr.ui_flow_browse->widget, pagetitle);

	gebr.ui_flow_edition = flow_edition_setup_ui();
	gtk_widget_show_all(gebr.ui_flow_edition->widget);
	pagetitle = gtk_label_new(_("Flow edition"));
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), gebr.ui_flow_edition->widget, pagetitle);

	gebr.ui_job_control = job_control_setup_ui();
	gtk_widget_show_all(gebr.ui_job_control->widget);
	pagetitle = gtk_label_new(_("Job control"));
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), gebr.ui_job_control->widget, pagetitle);

	/* Create a status bar */
	gebr.ui_log = log_setup_ui();
	gtk_widget_show_all(gebr.ui_log->widget);
	gtk_box_pack_end(GTK_BOX(vboxmain), gebr.ui_log->widget, FALSE, FALSE, 0);

	/*
	 * Create some hot-keys
	 */
	gebr.accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group);
	/* CTRL+R - Run current flow */
	closure = g_cclosure_new((GCallback)on_flow_execute_activate, NULL, NULL);
	gtk_accel_group_connect(gebr.accel_group, GDK_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, closure);
	/* CTR+Q - Quit GeBR */
	closure = g_cclosure_new((GCallback)gebr_quit, NULL, NULL);
	gtk_accel_group_connect(gebr.accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, closure);
}

/*
 * Function: assembly_project_menu
 * Assembly the menu to the project page
 *
 */
static GtkWidget *
assembly_project_menu(void)
{
	GtkWidget *	menu_item;
	GtkWidget *	menu;
	GtkWidget *	child_menu_item;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Project menu"));

	/* New entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_project_new_activate), NULL);
	/* Delete entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_project_delete_activate), NULL);
	/* Refresh entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_project_refresh_activate), NULL);

	/* Separation line */
	child_menu_item = gtk_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);

	/* Properties entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_project_properties_activate), NULL);

	menu_item = gtk_menu_item_new_with_label(_("Project"));
	gtk_menu_item_set_child_menu_item(GTK_MENU_ITEM(menu_item), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_item));

	return menu_item;
}

/*
 * Function: assembly_line_menu
 * Assembly the menu to the line page
 *
 */
static GtkWidget *
assembly_line_menu(void)
{
	GtkWidget *	menu_item;
	GtkWidget *	menu;
	GtkWidget *	child_menu_item;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Line menu"));

	/* New entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_line_new_activate), NULL);
	/*
	* TODO: Add entry
	*
	* In the future, an "add entry" could pop-up a dialog to browse
	* through lines. The user could then import to the current project
	* an existent line. Would that be useful?
	*/
	/* TODO:
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	*/
	/* Delete entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_line_delete_activate), NULL);

	/* Separation line */
	child_menu_item = gtk_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);

	/* Paths */
	child_menu_item = gtk_image_menu_item_new_with_label(_("Paths"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(child_menu_item), gtk_image_new_from_stock(GTK_STOCK_DIRECTORY, 1));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_line_path_activate), NULL);

	/* Properties entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_line_properties_activate), NULL);

	menu_item = gtk_menu_item_new_with_label(_("Line"));
	gtk_menu_item_set_child_menu_item(GTK_MENU_ITEM(menu_item), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_item));

	return menu_item;
}
/*
 * Function: assembly_flow_components_menu
 * Assembly the menu to the flow edit page associated to flow_components
 *
 */
static GtkWidget *
assembly_flow_components_menu(void)
{
	GtkWidget *	menu_item;
	GtkWidget *	menu;
	GtkWidget *	child_menu_item;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Flow component menu"));

	/* Properties entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_component_properties_activate), NULL);
	/* Refresh entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_flow_component_refresh_activate), NULL);

	/* separator */
	child_menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	/* component status items */
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(GTK_ACTION(gebr.actions.configured)));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(GTK_ACTION(gebr.actions.disabled)));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(GTK_ACTION(gebr.actions.unconfigured)));

	menu_item = gtk_menu_item_new_with_label(_("Flow component"));
	gtk_menu_item_set_child_menu_item(GTK_MENU_ITEM(menu_item), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_item));

	return menu_item;
}

/*
 * Function: assembly_config_menu
 * Assembly the config menu
 *
 */
static GtkWidget *
assembly_config_menu(void)
{
	GtkWidget *	menu_item;
	GtkWidget *	menu;
	GtkWidget *	child_menu_item;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Config menu"));

	/* Preferences entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			G_CALLBACK(on_configure_preferences_activate), NULL);

	/* Server entry */
	child_menu_item =  gtk_image_menu_item_new_with_mnemonic(_("_Servers"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(child_menu_item), gtk_image_new_from_stock(GTK_STOCK_NETWORK, 1));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_configure_servers_activate), NULL);

	menu_item = gtk_menu_item_new_with_label(_("Configure"));
	gtk_menu_item_set_child_menu_item(GTK_MENU_ITEM(menu_item), menu);

	return menu_item;
}

/*
 * Function: assembly_help_menu
 * Assembly the help menu
 *
 */
static GtkWidget *
assembly_help_menu(void)
{
	GtkWidget *	menu_item;
	GtkWidget *	menu;
	GtkWidget *	child_menu_item;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Help menu"));

	/* About entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_help_about_activate), NULL);

	menu_item = gtk_menu_item_new_with_label(_("Help"));
	gtk_menu_item_set_child_menu_item(GTK_MENU_ITEM(menu_item), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_item));

	return menu_item;
}
