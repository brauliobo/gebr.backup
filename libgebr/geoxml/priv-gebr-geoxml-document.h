/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __PRIV_GEBR_GEOXML_DOCUMENT_H__
#define __PRIV_GEBR_GEOXML_DOCUMENT_H__

#include <gdome.h>

G_BEGIN_DECLS

typedef struct {
	GString *filename;
	/** For #gebr_geoxml_object_set_user_data */
	gpointer user_data;
} GebrGeoXmlDocumentData;

/**
 * gebr_geoxml_document_new:
 * @name:
 * @version:
 *
 * Private constructor. Used by super classes to create a new document
 * @param name refer to the root element (flow, line or project) @param version
 * to its corresponding last version (support by this version of libgeoxml)
 */
GebrGeoXmlDocument *gebr_geoxml_document_new(const gchar * name, const gchar * version);

GdomeElement *gebr_geoxml_document_root_element(GebrGeoXmlDocument *self);

#define _gebr_geoxml_document_get_data(document) \
	((GebrGeoXmlDocumentData*)((GdomeDocument*)document)->user_data)

G_END_DECLS

#endif /* __PRIV_GEBR_GEOXML_DOCUMENT_H__ */
