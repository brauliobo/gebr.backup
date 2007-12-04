/*   libgebr - GêBR Library
 *   Copyright (C) 2007 GÃªBR core team (http://gebr.sourceforge.net)
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "utils.h"

/**
 * mkstemp has the exigency that the XXXXXX comes in the end of the template.
 * This function returns an static allocated string which will be used to create
 * temporary filename, possibly with an extension, which is not easily possible with mkstemp
 */
gchar *
make_temp_filename(void)
{
	static gchar	template[7];

	strcpy(template, "XXXXXX");
	mkstemp(template);

	return template;
}

/**
 * Returns the home's permission mode. Useful for
 * preserving permissions when creating files
 */
int
home_mode(void)
{
	struct stat	home_stat;
	gchar *		home;

	home = getenv("HOME");
	g_stat(home, &home_stat);

	return home_stat.st_mode;
}

/**
 * Create all configurations directories for all GêBR programs.
 * Used by GêBR programs before read/write config..
 */
gboolean
gebr_create_config_dirs(void)
{
	GString *	gebr;
	GString *	aux;
	gboolean	ret;

	ret = TRUE;
	gebr = g_string_new(NULL);
	aux = g_string_new(NULL);

	/* Test for gebr conf dir */
	g_string_printf(gebr, "%s/.gebr", getenv("HOME"));
	if (g_file_test(gebr->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
		if (g_mkdir(gebr->str, home_mode())) {
			ret = FALSE;
			goto out;
		}
	}

	/* Test for .gebr/run conf dir */
	g_string_printf(aux, "%s/run", gebr->str);
	if (g_file_test(aux->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
		if (g_mkdir(aux->str, home_mode())) {
			ret = FALSE;
			goto out;
		}
	}

	/* Test for .gebr/log conf dir */
	g_string_printf(aux, "%s/log", gebr->str);
	if (g_file_test(aux->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
		if (g_mkdir(aux->str, home_mode())) {
			ret = FALSE;
			goto out;
		}
	}

out:	g_string_free(gebr, TRUE);
	g_string_free(aux, TRUE);
	return ret;
}
