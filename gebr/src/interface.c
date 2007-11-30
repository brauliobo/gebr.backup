/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

/*
 * File: interface.c
 * Assembly the main components of the interface
 *
 * This function assemblies the main window, preferences and menu bar.
 */

#include <string.h>

#include "interface.h"
#include "gebr.h"
#include "menus.h"
#include "callbacks.h"

/* Pre-defined browser options */
#define NBROWSER 5
const char * browser[] = {
	"epiphany", "firefox", "galeon", "konqueror", "mozilla"
};

/*
 * Function: assembly_interface
 * Assembly the whole interface.
 *
 */
void
assembly_interface(void)
{
	GtkWidget *	vboxmain;

	/* Preferences dialog*/
	gebr.pref.win = NULL;

	/* About dialog */
	gebr.about = assembly_about ();

	/*
	* Parameters dialog
	* (to be created on the fly)
	*/
	gebr.parameters = NULL;

	/* Create the main window */
	gebr.mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (gebr.mainwin), "G√™BR" );
	gtk_widget_set_size_request (GTK_WIDGET (gebr.mainwin), 700, 400);

	/* Signals */
	g_signal_connect (GTK_OBJECT (gebr.mainwin), "delete_event",
			GTK_SIGNAL_FUNC (gebr_quit),
			NULL);
	g_signal_connect (GTK_OBJECT (gebr.mainwin), "show",
			GTK_SIGNAL_FUNC (gebr_init), NULL);

	/* Define the function to be called when the main loops is finished */
	/* (Not implemented yet) */
	/* gtk_quit_add (0, save_and_quit, NULL); */

	/* Create the main vbox to hold menu, notebook and status bar */
	vboxmain = gtk_vbox_new (FALSE, 1);
	gtk_container_add (GTK_CONTAINER (gebr.mainwin), vboxmain );

	/* Create the main menu */
	{
		GtkWidget *mainmenu;

		mainmenu = gtk_menu_bar_new ();
		gtk_box_pack_start (GTK_BOX (vboxmain), mainmenu, FALSE, FALSE, 0);


		gebr.menu[MENUBAR_PROJECT] = assembly_project_menu ();
		gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), gebr.menu[MENUBAR_PROJECT]);

		gebr.menu[MENUBAR_LINE] = assembly_line_menu ();
		gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), gebr.menu[MENUBAR_LINE]);

		gebr.menu[MENUBAR_FLOW] = assembly_flow_menu ();
		gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), gebr.menu[MENUBAR_FLOW]);

		gebr.menu[MENUBAR_FLOW_COMPONENTS] = assembly_flowcomponentsmenu ();
		gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), gebr.menu[MENUBAR_FLOW_COMPONENTS]);

		gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), assembly_config_menu ());
		gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), assembly_help_menu ());
	}

	/* Create a notebook to hold several pages */
	{
		GtkWidget *	pagetitle;

		gebr.notebook = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(vboxmain), gebr.notebook, TRUE, TRUE, 0);

		g_signal_connect(GTK_OBJECT (gebr.notebook), "switch-page",
					GTK_SIGNAL_FUNC (switch_menu), NULL);

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
	gebr.statusbar = gtk_statusbar_new ();
	gtk_box_pack_end (GTK_BOX (vboxmain), gebr.statusbar, FALSE, FALSE, 0);
}

/*
 * Function: assembly_preference_win
 * Assembly preference window.
 *
 */
void
assembly_preferences(void)
{
	if (gebr.pref.win != NULL)
		return;

	GtkWidget *table;
	GtkWidget *label;
	GtkTooltips *tips;
	GtkWidget *eventbox;

	gebr.pref.win = gtk_dialog_new_with_buttons ("Preferences",
					   GTK_WINDOW(gebr.pref.win),
					   0,
					   GTK_STOCK_OK,     GTK_RESPONSE_OK,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   NULL);
	/* Tooltips */
	tips = gtk_tooltips_new();

	/* Take the apropriate action when a button is pressed */
	g_signal_connect_swapped (gebr.pref.win, "response",
				  G_CALLBACK (pref_actions),
				  gebr.pref.win);

	gtk_widget_set_size_request (gebr.pref.win, 380, 300);

	table = gtk_table_new (6, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gebr.pref.win)->vbox), table, TRUE, TRUE, 0);

	/* User name */
	label = gtk_label_new ("User name");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.pref.username = gtk_entry_new ();
	gtk_tooltips_set_tip(tips, gebr.pref.username, "You should know your name", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), gebr.pref.username, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	if (gebr.pref.username_value->len > 0)
	   gtk_entry_set_text (GTK_ENTRY (gebr.pref.username), gebr.pref.username_value->str);

	/* User gebr.pref.email */
	label = gtk_label_new ("Email");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.pref.email = gtk_entry_new ();
	gtk_tooltips_set_tip(tips, gebr.pref.email, "Your email address", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), gebr.pref.email, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	if (gebr.pref.email_value->len > 0)
	   gtk_entry_set_text (GTK_ENTRY (gebr.pref.email), gebr.pref.email_value->str);

	/* GÍBR dir */
	label = gtk_label_new ("User's menus directory");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	/* Browse button for user's menus dir */
	gebr.pref.usermenus = gtk_file_chooser_button_new ("G√™BR dir",
						GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), gebr.pref.usermenus);
	gtk_tooltips_set_tip(tips, eventbox, "Path to look for private user's menus", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), eventbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
 	if (gebr.pref.usermenus_value->len > 0)
	   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gebr.pref.usermenus),
						gebr.pref.usermenus_value->str);

	/* Data dir */
	label = gtk_label_new ("Data directory");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	/* Browse button for gebr.pref.data */
	gebr.pref.data = gtk_file_chooser_button_new ("Browser data dir",
						GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), gebr.pref.data);
	gtk_tooltips_set_tip(tips, eventbox, "Path to store projects, lines and flows", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), eventbox, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
 	if (gebr.pref.data_value->len > 0)
	   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gebr.pref.data),
						gebr.pref.data_value->str);

	/* Editor */
	label = gtk_label_new ("HTML editor");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.pref.editor = gtk_entry_new ();
	gtk_tooltips_set_tip(tips, gebr.pref.editor, "An HTML capable editor to edit helps and reports", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), gebr.pref.editor, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
 	if (gebr.pref.editor_value->len > 0)
	   gtk_entry_set_text (GTK_ENTRY (gebr.pref.editor), gebr.pref.editor_value->str);

	/* Browser */
	label = gtk_label_new ("Browser");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	gebr.pref.browser = gtk_combo_box_entry_new_text();
	{
	   int ii;
	   int newbrowser = 1;
	   for (ii=0; ii < NBROWSER; ii++){
	      gtk_combo_box_append_text(GTK_COMBO_BOX(gebr.pref.browser), browser[ii]);
	      if ( (gebr.pref.browser_value->len > 0) && newbrowser){
		 if (strcmp(browser[ii], gebr.pref.browser_value->str)==0){
		    newbrowser = 0;
		    gtk_combo_box_set_active (GTK_COMBO_BOX(gebr.pref.browser), ii );
		 }
	      }
	   }
	   if ((gebr.pref.browser_value->len > 0) && newbrowser){
	      gtk_combo_box_append_text(GTK_COMBO_BOX(gebr.pref.browser), gebr.pref.browser_value->str);
	      gtk_combo_box_set_active (GTK_COMBO_BOX(gebr.pref.browser), NBROWSER );
	   }
	}

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), gebr.pref.browser);
	gtk_tooltips_set_tip(tips, eventbox, "An HTML browser to display helps and reports", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), eventbox, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 3, 3);

	gtk_widget_show_all(gebr.pref.win);
}

/*
 * Function: assembly_config_menu
 * Assembly the config menu
 *
 */
GtkWidget *
assembly_config_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (menu), "Config menu");

	/* Pref entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );

	g_signal_connect (GTK_OBJECT (submenu), "activate",
			G_CALLBACK (assembly_preference_win), NULL);

	/* Server entry */
	submenu =  gtk_image_menu_item_new_with_mnemonic("_Servers");

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );

	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (assembly_server_win),
			NULL);

	menuitem = gtk_menu_item_new_with_label ("Configure");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

	return menuitem;
}

/*
 * Function: assembly_help_menu
 * Assembly the help menu
 *
 */
GtkWidget *
assembly_help_menu(void)
{
	GtkWidget *menuitem;
	GtkWidget *menu;
	GtkWidget *submenu;

	menu = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (menu), "Help menu");

	/* About entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (gtk_widget_show),
			GTK_WIDGET (W.about));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

	menuitem = gtk_menu_item_new_with_label ("Help");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

	gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

	return menuitem;
}

/*
 * Function: assembly_project_menu
 * Assembly the menu to the project page
 *
 */
GtkWidget *
assembly_project_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (menu), "Project menu");

	/* New entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (project_new), NULL );

	/* Delete entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (project_delete), NULL );

	/* Refresh entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_REFRESH, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (projects_refresh), NULL );

	menuitem = gtk_menu_item_new_with_label ("Project");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

	gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

	return menuitem;
}

/*
 * Function: assembly_line_menu
 * Assembly the menu to the line page
 *
 */
GtkWidget *
assembly_line_menu(void)
{
	GtkWidget *menuitem;
	GtkWidget *menu;
	GtkWidget *submenu;

	menu = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (menu), "Line menu");

	/* New entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (line_new), NULL );

	/*
	* TODO: Add entry
	*
	* In the future, an "add entry" could pop-up a dialog to browse
	* through lines. The user could then import to the current project
	* an existent line. Would that be useful?
	*/
	/* TODO:
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	*/

	/* Delete entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (line_delete), NULL );

	/*
	* Properties entry
	*
	* Infos about a line, like title, description, etc.
	*/
	/* TODO:
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	*/

	menuitem = gtk_menu_item_new_with_label ("Line");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

	gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

	return menuitem;

}

/*
 * Function: assembly_flow_menu
 * Assembly the menu to the flow page
 *
 */
GtkWidget *
assembly_flow_menu (void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;

	menu = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (menu), "Flow menu");

	/* New entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_new), NULL );

	/* Export entry */
	submenu = gtk_image_menu_item_new_with_label ("Export");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_export), NULL );


	/* Delete entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_delete), NULL );

	/* Separation line */
	submenu = gtk_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);


	/* Properties entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_properties), NULL );

	/* Input/Output entry */
	submenu = gtk_image_menu_item_new_with_label ("Input/Output");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_io), NULL );

	/* Execute entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_EXECUTE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_run), NULL );

	menuitem = gtk_menu_item_new_with_label ("Flow");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

	gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

	return menuitem;
}


/*
 * Function: assembly_flow_components_menu
 * Assembly the menu to the flow edit page associated to flow_components
 *
 */
GtkWidget *
assembly_flow_components_menu(void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;
	GSList *	radio_slist;

	menu = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (menu), "Flow component menu");

	/* Properties entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_component_properties_set), NULL );

	/* Refresh entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_REFRESH, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (menus_create_index), NULL);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (menus_populate), NULL );

	/* separator */
	submenu = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	/* component status items */
	/* configured */
	radio_slist = NULL;
	W.configured_menuitem = gtk_radio_menu_item_new_with_label (radio_slist, "Configured");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), W.configured_menuitem);
	g_signal_connect (	GTK_OBJECT (W.configured_menuitem), "activate",
				GTK_SIGNAL_FUNC (flow_component_set_status), NULL );
	/* disabled */
	radio_slist = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (W.configured_menuitem));
	W.disabled_menuitem = gtk_radio_menu_item_new_with_label (radio_slist, "Disabled");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), W.disabled_menuitem);
	g_signal_connect (	GTK_OBJECT (W.disabled_menuitem), "activate",
				GTK_SIGNAL_FUNC (flow_component_set_status), NULL );
	/* unconfigured */
	radio_slist = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (W.disabled_menuitem));
	W.unconfigured_menuitem = gtk_radio_menu_item_new_with_label (radio_slist, "Unconfigured");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), W.unconfigured_menuitem);
	g_signal_connect (	GTK_OBJECT (W.unconfigured_menuitem), "activate",
				GTK_SIGNAL_FUNC (flow_component_set_status), NULL );

	menuitem = gtk_menu_item_new_with_label ("Flow component");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
	gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

	return menuitem;
}
