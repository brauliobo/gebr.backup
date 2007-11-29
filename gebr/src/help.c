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

/* File: help.c
 * Callbacks for the help manipulation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>

#include "ui_help.h"
#include "gebr.h"
#include "support.h"
#include "callbacks.h"

/* Function ui_help_edit
 *
 */
void
ui_help_edit(GtkButton * button, GeoXmlDocument * document);
{
	FILE *		tmp_fp;
	GString *	tmp_fn;
	gchar *		buffer;
	GString *	help;
	GString *	cmdline;

	/* Call an editor */
	if (!W.config.editor_given) {
		log_message(ERROR, "No editor defined", TRUE);
		return;
	}

	/* Temporary file */
	tmp_fn = g_string_new("/tmp/gebr_");
	g_string_printf("")
	sprintf (random, "XXXXXX");
	mktemp (random);

	g_string_append(tmp_fn, random);
	g_string_append(tmp_fn, ".html");

	/* Write current help to temporary file */
	tmp_fp = fopen(tmp_fn->str, "w");

	buffer = geoxml_document_get_help(document);
	if ((buffer != NULL) && (strlen(buffer)>1) )
	fputs(buffer, tmp_fp);
	fclose(tmp_fp);

	cmdline = g_string_new(NULL);
	g_string_printf(cmdline, "%s %s", W.pref.editor_value->str, tmp_fn->str);
	system(cmdline->str);
	g_string_free(cmdline, TRUE);

	/* Read back the help from file */
	tmp_fp = fopen(tmp_fn->str, "r");
	buffer = malloc (sizeof(char) * STRMAX);
	help = g_string_new(NULL);

	if (fgets(buffer, STRMAX, tmp_fp)!=NULL)
	g_string_append(help, buffer);

	while (!feof(tmp_fp)){
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
