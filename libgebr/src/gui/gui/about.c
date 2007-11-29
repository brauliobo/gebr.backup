/*   libgebr - G�BR Library
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include "about.h"

struct about
about_setup_ui(void)
{
	struct about	about;
	const gchar *	authors[] = {
		"GêBR Core Team:",
		"Developers",
		"  Bráulio Oliveira <brauliobo@gmail.com>",
		"  Eduardo Filpo <efilpo@gmail.com>",
		"  Fernando Roxo <roxo@roxo.org>",
		"  Ricardo Biloti <biloti@gmail.com>",
		"  Rodrigo Portugal <rosoport@gmail.com>",
		"SU Port Team",
		"  Jesse Costa",
		"  Ellen Costa",
		"  Various students",
		NULL
	};

	about.dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about.dialog), "GêBR");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about.dialog), "1.0"); /* FIXME: use version define */
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about.dialog),
					"GêBR Core Team");

	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about.dialog),
		"Copyright (C) 2007 GêBR Core Team (http://groups.google.com/group/gebr)\n"
		"\n"
		"This program is free software: you can redistribute it and/or modify"
		"it under the terms of the GNU General Public License as published by"
		"the Free Software Foundation, either version 3 of the License, or"
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License"
		"along with this program.  If not, see <http://www.gnu.org/licenses/>.");
	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about.dialog), TRUE);

	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about.dialog), "http://groups.google.com/group/gebr");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about.dialog), "GêBR Home page");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about.dialog), authors);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about.dialog),
					"A plug-and-play environment to\nseismic processing tools");

	g_signal_connect(GTK_OBJECT(about.dialog), "delete_event",
			GTK_SIGNAL_FUNC(gtk_widget_hide), NULL );

	return about;
}
