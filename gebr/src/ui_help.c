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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

#include <misc/utils.h>

#include "ui_help.h"
#include "gebr.h"
#include "callbacks.h"

void
help_show(gchar * help, gchar * title, gchar * fname)
{
	GtkWidget *	dialog;
	GtkWidget *	scrolled_window;
	GtkWidget *	html;
	FILE *		html_fp;
	GString *	html_filename;
	GString *	url;
	GString *	ghelp;

	if (help == NULL || !strlen(help))
		return;

	dialog = gtk_dialog_new_with_buttons(title,
					GTK_WINDOW(gebr.window),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					NULL);
	g_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL );

	/* Temporary file */
	html_filename = g_string_new(NULL);
	g_string_printf(tmp_fn, "/tmp/gebr_%s.html", make_temp_filename());

	/* Gambiarra */
	{
		gchar *	gebrcsspos;
		int	pos;

		ghelp = g_string_new(help);

		if ((gebrcsspos = strstr(ghelp->str, "gebr.css")) != NULL) {
			pos = (gebrcsspos - ghelp->str)/sizeof(char);
			g_string_erase(ghelp, pos, 8);
			g_string_insert(ghelp, pos, "file://" GEBRDATADIR "/gebr.css");
		}
	}

	htmlfp = fopen(html_filename->str,"w");
	if (htmlfp == NULL) {
		/* TODO */
		return;
	}
	fwrite(ghelp->str, sizeof(char), strlen(ghelp->str), htmlfp);
	fclose(htmlfp);
	g_string_free(ghelp, TRUE);

	/* Add file to list of files to be removed */
	gebr.tmpfiles = g_slist_append(gebr.tmpfiles, html_filename->str);

	url = g_string_new("file://");
	g_string_append(url, html_filename->str);

	/* Launch an external browser */
	{
	GString *cmdline;

	cmdline = g_string_new (gebr.config.browser_arg);
	g_string_append(cmdline, " ");
	g_string_append(cmdline, html_filename->str);
	g_string_append(cmdline, " &");
	system(cmdline->str);
	}

	g_string_free(url, FALSE);
	g_string_free(html_filename, FALSE);
}

/* Function ui_help_edit
 *
 */
void
help_edit(GtkButton * button, GeoXmlDocument * document);
{
	FILE *		tmp_fp;
	GString *	tmp_fn;
	gchar *		buffer;
	GString *	help;
	GString *	cmdline;

	/* Call an editor */
	if (!gebr.config.editor_given) {
		log_message(ERROR, "No editor defined", TRUE);
		return;
	}

	/* Temporary file */
	tmp_fn = g_string_new(NULL);
	g_string_printf(tmp_fn, "/tmp/gebr_%s.html", make_temp_filename());

	/* Write current help to temporary file */
	tmp_fp = fopen(tmp_fn->str, "w");

	buffer = geoxml_document_get_help(document);
	if ((buffer != NULL) && (strlen(buffer)>1))
		fputs(buffer, tmp_fp);
	fclose(tmp_fp);

	cmdline = g_string_new(NULL);
	g_string_printf(cmdline, "%s %s", gebr.config.editor->str, tmp_fn->str);
	system(cmdline->str);
	g_string_free(cmdline, TRUE);

	/* Read back the help from file */
	tmp_fp = fopen(tmp_fn->str, "r");
	buffer = malloc (sizeof(char) * STRMAX);
	help = g_string_new(NULL);

	if (fgets(buffer, STRMAX, tmp_fp) != NULL)
		g_string_append(help, buffer);

	while (!feof(tmp_fp)) {
	if (fgets(buffer, STRMAX, tmp_fp) != NULL)
		g_string_append(help, buffer);
	}

	free(buffer);
	fclose(tmp_fp);

	geoxml_document_set_help(document, help->str);

	unlink(tmp_fn->str);
	g_string_free(tmp_fn, TRUE);
	g_string_free(help, TRUE);

}

