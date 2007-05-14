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

#ifndef __GEOXML_PROGRAM_H
#define __GEOXML_PROGRAM_H

#include <glib.h>

/**
 *
 */
enum GEOXMLPARAM {
	PARAM_INT, PARAM_FLOAT, PARAM_STRING,
	PARAM_FLAG, PARAM_FILE, PARAM_RANGE
};

/**
 *
 */
struct geoxml_program_paramater {
	GString*		value;
	enum GEOXMLPARAM	type;
	gboolean		required;
	GString*		keyword;
	GString*		description;
};

/**
 *
 */
struct geoxml_program {
	GSList*			paramaters;

	GString*		binary;
	GString*		title;
	GString*		description;
	GString*		long_description;
	GString*		id;
	GString*		stdin;
	GString*		stout;
	GString*		status;
};

#endif //__GEOXML_PROGRAM_H
