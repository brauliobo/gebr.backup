/*   G�BR - An environment for seismic processing.
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#include <time.h>
#include <unistd.h>

#include <misc/utils.h>

#include "document.h"
#include "gebr.h"
#include "support.h"

/*
 * Function: document_load
 * Load a document (flow, line or project) from its filename, handling errors
 */
GeoXmlDocument *
document_load(const gchar * filename)
{
	GeoXmlDocument *	document;
	GString *		path;

	path = document_get_path(filename);
	document = document_load_path(path->str);
	g_string_free(path, TRUE);

	return document;
}

/*
 * Function: document_load_path
 * Load a document from its path, handling errors
 */
GeoXmlDocument *
document_load_path(const gchar * path)
{
	GeoXmlDocument *	document;
	int			ret;

	if ((ret = geoxml_document_load(&document, path)) < 0) {
		GString *	error;

		error = g_string_new(NULL);
		g_string_printf(error, _("Can't load document at %s: "), path);
		switch (ret) {
		case GEOXML_RETV_DTD_SPECIFIED:
			g_string_append(error, _("DTD specified"));
			break;
		case GEOXML_RETV_INVALID_DOCUMENT:
			g_string_append(error, _("invalid document"));
			break;
		case GEOXML_RETV_CANT_ACCESS_FILE:
			g_string_append(error, _("can't access file"));
			break;
		case GEOXML_RETV_CANT_ACCESS_DTD:
			g_string_append(error, _("can't access DTD"));
			break;
		default:
			g_string_append(error, _("unspecified error"));
			break;
		}

		gebr_message(ERROR, TRUE, TRUE, error->str);
		g_string_free(error, TRUE);
	}

	return document;
}

/*
 * Function: document_save
 * Save _document_ using its filename field at data directory.
 */
void
document_save(GeoXmlDocument * document)
{
	GString *	path;

	/* TODO: check save */
	path = document_get_path(geoxml_document_get_filename(document));
	geoxml_document_save(document, path->str);

	g_string_free(path, TRUE);
}

/*
 * Function: document_get_path
 * Prefix filename with data diretory path
 */
GString *
document_get_path(const gchar * filename)
{
	GString *	path;

	path = g_string_new(NULL);
	g_string_printf(path, "%s/%s", gebr.config.data->str, filename);

	return path;
}

/*
 * Function: document_assembly_filename
 * Creates a filename for a document
 * Creates a filename for a document using the current date and a random
 * generated string and _extension_.
 */
GString *
document_assembly_filename(const gchar * extension)
{
	time_t 		t;
	struct tm *	lt;
	gchar		date[21];
	GString *	filename;

	time(&t);
	lt = localtime(&t);
	strftime(date, 20, "%Y_%m_%d", lt);

	filename = g_string_new(NULL);
	g_string_printf(filename, "%s_%s.%s", date, make_temp_filename(), extension);

	return filename;
}
