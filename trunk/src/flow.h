/*   This file is part of libgeoxml
 *
 *   Copyright (C) 2007 Bráulio Barros de Oliveira <brauliobo@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __GEOXML_FLOW_H
#define __GEOXML_FLOW_H

#include <glib.h>


/**
 *
 */
struct geoxml_flow {
	GSList*		programs;

	GString*	title;
	GString*	author;
	GString*	description;
	GString*	id;
	GString*	filename;
	GString*	long_description;
};


/**
 *
 */
struct geoxml_flow *
libgeoxml_flow_load_from_file(const gchar * path);

/**
 *
 */
void
libgeoxml_flow_free(struct geoxml_flow * flow);

#endif //__GEOXML_FLOW_H
