/*   G�BR - An environment for seismic processing.
 *   Copyright(C) 2007 G�BR core team(http://gebr.sourceforge.net)
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

/*
 * File: interface.c
 * Assembly the main components of the interface
 *
 * This function assemblies the main window, preferences and menu bar.
 */

#include <string.h>

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

	gebr.about = about_setup_ui();
	gebr.ui_preferences = preferences_setup_ui();

	/* Create the main window */
	gebr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gebr.window), "GêBR");
	gtk_widget_set_size_request(GTK_WIDGET(gebr.window), 700, 400);

	/* Signals */
	g_signal_connect(GTK_OBJECT(gebr.window), "delete_event",
			GTK_SIGNAL_FUNC(gebr_quit),
			NULL);
	g_signal_connect(GTK_OBJECT(gebr.window), "show",
			GTK_SIGNAL_FUNC(gebr_init), NULL);

	/* Define the function to be called when the main loops is finished */
	/*(Not implemented yet) */
	/* gtk_quit_add(0, save_and_quit, NULL); */

	/* Create the main vbox to hold menu, notebook and status bar */
	vboxmain = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(gebr.window), vboxmain);

	/* Create the main menu */
	{
		GtkWidget *mainmenu;

		mainmenu = gtk_menu_bar_new();
		gtk_box_pack_start(GTK_BOX(vboxmain), mainmenu, FALSE, FALSE, 0);


		gebr.menu[MENUBAR_PROJECT] = assembly_project_menu();
		gtk_menu_bar_append(GTK_MENU_BAR(mainmenu), gebr.menu[MENUBAR_PROJECT]);

		gebr.menu[MENUBAR_LINE] = assembly_line_menu();
		gtk_menu_bar_append(GTK_MENU_BAR(mainmenu), gebr.menu[MENUBAR_LINE]);

		gebr.menu[MENUBAR_FLOW] = assembly_flow_menu();
		gtk_menu_bar_append(GTK_MENU_BAR(mainmenu), gebr.menu[MENUBAR_FLOW]);

		gebr.menu[MENUBAR_FLOW_COMPONENTS] = assembly_flowcomponentsmenu();
		gtk_menu_bar_append(GTK_MENU_BAR(mainmenu), gebr.menu[MENUBAR_FLOW_COMPONENTS]);

		gtk_menu_bar_append(GTK_MENU_BAR(mainmenu), assembly_config_menu());
		gtk_menu_bar_append(GTK_MENU_BAR(mainmenu), assembly_help_menu());
	}

	/* Create a notebook to hold several pages */
	{
		GtkWidget *	pagetitle;

		gebr.notebook = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(vboxmain), gebr.notebook, TRUE, TRUE, 0);

		g_signal_connect(GTK_OBJECT(gebr.notebook), "switch-page",
					GTK_SIGNAL_FUNC(switch_page), NULL);

		gebr.ui_project_line = project_line_setup_ui();
		pagetitle = gtk_label_new(_("Projects and Lines"));
		gtk_notebook_append_page(GTK_NOTE_BOOK(gebr.notebook), gebr.ui_project_line.widget, pagetitle);

		gebr.ui_flow_browse = flow_browse_setup_ui();
		pagetitle = gtk_label_new(_("Flows"));
		gtk_notebook_append_page(GTK_NOTE_BOOK(gebr.notebook), gebr.ui_flow_browse.widget, pagetitle);

		gebr.ui_flow_edition = flow_edition_setup_ui();
		pagetitle = gtk_label_new(_("Flow edition"));
		gtk_notebook_append_page(GTK_NOTE_BOOK(gebr.notebook), gebr.ui_flow_edition.widget, pagetitle);

		gebr.ui_job_control = job_control_setup_ui();
		pagetitle = gtk_label_new(_("Job control"));
		gtk_notebook_append_page(GTK_NOTE_BOOK(gebr.notebook), gebr.ui_job_control.widget, pagetitle);
	}

	/* Create a status bar */
	gebr.statusbar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(vboxmain), gebr.statusbar, FALSE, FALSE, 0);
}

/*
 * Function: assembly_project_menu
 * Assembly the menu to the project page
 *
 */
static GtkWidget *
assembly_project_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Project menu"));

	/* New entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_project_new_activate), NULL);
	/* Delete entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_project_delete_activate), NULL);
	/* Refresh entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_project_refresh_activate, NULL);

	/* Separation line */
	submenu = gtk_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

	/* Properties entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_project_properties_activate), NULL);

	menuitem = gtk_menu_item_new_with_label(_("Project"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);

	gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));

	return menuitem;
}

/*
 * Function: assembly_line_menu
 * Assembly the menu to the line page
 *
 */
static GtkWidget *
assembly_line_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Line menu"));

	/* New entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_line_new_activate), NULL);
	/*
	* TODO: Add entry
	*
	* In the future, an "add entry" could pop-up a dialog to browse
	* through lines. The user could then import to the current project
	* an existent line. Would that be useful?
	*/
	/* TODO:
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	*/
	/* Delete entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_line_delete_activate), NULL);

	/* Separation line */
	submenu = gtk_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

	/* Properties entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_line_properties_activate), NULL);

	menuitem = gtk_menu_item_new_with_label(_("Line"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);

	gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));

	return menuitem;

}

/*
 * Function: assembly_flow_menu
 * Assembly the menu to the flow page
 *
 */
static GtkWidget *
assembly_flow_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Flow menu"));

	/* New entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_new_activate), NULL);
	/* Export entry */
	submenu = gtk_image_menu_item_new_with_label(_("Export"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_export_activate), NULL);
	/* Delete entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_delete_activate), NULL);

	/* Separation line */
	submenu = gtk_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

	/* Properties entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_properties_activate), NULL);
	/* Input/Output entry */
	submenu = gtk_image_menu_item_new_with_label(_("Input/Output"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_io_activate), NULL);
	/* Execute entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_EXECUTE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_execute_activate), NULL);

	menuitem = gtk_menu_item_new_with_label(_("Flow"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));

	return menuitem;
}


/*
 * Function: assembly_flow_components_menu
 * Assembly the menu to the flow edit page associated to flow_components
 *
 */
static GtkWidget *
assembly_flow_components_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;
	GSList *	radio_slist;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Flow component menu"));

	/* Properties entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_component_properties_activate), NULL);
	/* Refresh entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_flow_component_refresh_activate), NULL);

	/* separator */
	submenu = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	/* component status items */
	/* configured */
	radio_slist = NULL;
	W.configured_menuitem = gtk_radio_menu_item_new_with_label(radio_slist, _("Configured"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), W.configured_menuitem);
	g_signal_connect(	GTK_OBJECT(W.configured_menuitem), "activate",
				GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);
	/* disabled */
	radio_slist = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(W.configured_menuitem));
	W.disabled_menuitem = gtk_radio_menu_item_new_with_label(radio_slist, _("Disabled"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), W.disabled_menuitem);
	g_signal_connect(	GTK_OBJECT(W.disabled_menuitem), "activate",
				GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);
	/* unconfigured */
	radio_slist = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(W.disabled_menuitem));
	W.unconfigured_menuitem = gtk_radio_menu_item_new_with_label(radio_slist, _("Unconfigured"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), W.unconfigured_menuitem);
	g_signal_connect(	GTK_OBJECT(W.unconfigured_menuitem), "activate",
				GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);

	menuitem = gtk_menu_item_new_with_label(_("Flow component"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));

	return menuitem;
}

/*
 * Function: assembly_config_menu
 * Assembly the config menu
 *
 */
static GtkWidget *
assembly_config_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Config menu"));

	/* Preferences entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			G_CALLBACK(on_configure_preferences_activate), NULL);

	/* Server entry */
	submenu =  gtk_image_menu_item_new_with_mnemonic(_("_Servers"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_configure_servers_activate),
			NULL);

	menuitem = gtk_menu_item_new_with_label(_("Configure"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);

	return menuitem;
}

/*
 * Function: assembly_help_menu
 * Assembly the help menu
 *
 */
static GtkWidget *
assembly_help_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Help menu"));

	/* About entry */
	submenu = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	g_signal_connect(GTK_OBJECT(submenu), "activate",
			GTK_SIGNAL_FUNC(on_help_about_activate), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

	menuitem = gtk_menu_item_new_with_label(_("Help"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);

	gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));

	return menuitem;
}
