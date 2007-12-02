/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team(http://gebr.sourceforge.net)
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

#include "ui_preferences.h"

/* Pre-defined browser options */
#define NBROWSER 5
static const gchar * browser[] = {
	"epiphany", "firefox", "galeon", "konqueror", "mozilla"
};

/*
 * Function: assembly_preference_win
 * Assembly preference window.
 *
 */
struct ui_preferences
preferences_setup_ui(void)
{
	struct ui_preferences	ui_preferences;
	GtkWidget *		table;
	GtkWidget *		label;
	GtkTooltips *		tips;
	GtkWidget *		eventbox;

	ui_preferences.win = gtk_dialog_new_with_buttons(_("Preferences"),
					GTK_WINDOW(ui_preferences.win),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);
	/* Tooltips */
	tips = gtk_tooltips_new();

	/* Take the apropriate action when a button is pressed */
	g_signal_connect_swapped(ui_preferences.win, "response",
				  G_CALLBACK(preferences_ac),
				  ui_preferences.win);

	gtk_widget_set_size_request(ui_preferences.win, 380, 300);

	table = gtk_table_new(6, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ui_preferences.win)->vbox), table, TRUE, TRUE, 0);

	/* User name */
	label = gtk_label_new(_("User name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_preferences.username = gtk_entry_new();
	gtk_tooltips_set_tip(tips, ui_preferences.username, _("You should know your name"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_preferences.username, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	gtk_entry_set_text(GTK_ENTRY(ui_preferences.username), ui_preferences.username_value->str);

	/* User ui_preferences.email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_preferences.email = gtk_entry_new();
	gtk_tooltips_set_tip(tips, ui_preferences.email, _("Your email address"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_preferences.email, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	gtk_entry_set_text(GTK_ENTRY(ui_preferences.email), ui_preferences.email_value->str);

	/* GÍBR dir */
	label = gtk_label_new(_("User's menus directory"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	/* Browse button for user's menus dir */
	ui_preferences.usermenus = gtk_file_chooser_button_new("G√™BR dir",
						GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences.usermenus);
	gtk_tooltips_set_tip(tips, eventbox, _("Path to look for private user's menus"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	if (ui_preferences.usermenus_value->len > 0)
	   gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ui_preferences.usermenus),
						ui_preferences.usermenus_value->str);

	/* Data dir */
	label = gtk_label_new(_("Data directory"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	/* Browse button for ui_preferences.data */
	ui_preferences.data = gtk_file_chooser_button_new(_("Browser data dir"),
						GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences.data);
	gtk_tooltips_set_tip(tips, eventbox, _("Path to store projects, lines and flows"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	if (ui_preferences.data_value->len > 0)
	   gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ui_preferences.data),
						ui_preferences.data_value->str);

	/* Editor */
	label = gtk_label_new(_("HTML editor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_preferences.editor = gtk_entry_new();
	gtk_tooltips_set_tip(tips, ui_preferences.editor, _("An HTML capable editor to edit helps and reports"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_preferences.editor, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	gtk_entry_set_text(GTK_ENTRY(ui_preferences.editor), ui_preferences.editor_value->str);

	/* Browser */
	label = gtk_label_new(_("Browser"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_preferences.browser = gtk_combo_box_entry_new_text();
	{
		int		i;
		gboolean	new_browser;

		new_browser = TRUE;
		for (i=0; i < NBROWSER; i++){
			gtk_combo_box_append_text(GTK_COMBO_BOX(ui_preferences.browser), browser[i]);
			if((ui_preferences.browser_value->len > 0) && new_browser) {
				if(strcmp(browser[i], ui_preferences.browser_value->str)==0){
					new_browser = FALSE;
					gtk_combo_box_set_active(GTK_COMBO_BOX(ui_preferences.browser), i );
				}
			}
		}
		if ((ui_preferences.browser_value->len > 0) && new_browser) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(ui_preferences.browser), ui_preferences.browser_value->str);
			gtk_combo_box_set_active(GTK_COMBO_BOX(ui_preferences.browser), NBROWSER );
		}
	}

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences.browser);
	gtk_tooltips_set_tip(tips, eventbox, _("An HTML browser to display helps and reports"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 3, 3);

	return ui_preferences;
}

/*
 * Function: preferences_actions
 * Take the appropriate action when the parameter dialog emmits
 * a response signal.
 */
void
preferences_actions(GtkDialog * dialog, gint arg1)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		/* Save preferences to file and the relod */
		g_string_assign(ui_preferences.username,
				gtk_entry_get_text(GTK_ENTRY(ui_preferences.username)));
		g_string_assign(ui_preferences.email,
				gtk_entry_get_text(GTK_ENTRY(ui_preferences.email)));
		g_string_assign(ui_preferences.usermenus,
				gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(ui_preferences.usermenus)));
		g_string_assign(ui_preferences.data,
				gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(ui_preferences.data)));
		g_string_assign(ui_preferences.editor,
				gtk_entry_get_text(GTK_ENTRY(ui_preferences.editor)));
		g_string_assign(ui_preferences.browser,
				gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui_preferences.browser)));

		gebr_config_save();
		gebr_config_reload();
		break;
	case GTK_RESPONSE_CANCEL: /* does nothing */
	default:
		break;
	}
}
