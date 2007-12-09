/*   libgebr - GÍBR Library
 *   Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
 *
 *   This parameter_group is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This parameter_group is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this parameter_group.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gdome.h>

#include "parameter_group.h"
#include "xml.h"
#include "parameters.h"

/*
 * internal stuff
 */

struct geoxml_parameter_group {
	GdomeElement * element;
};

/*
 * library functions.
 */

void
geoxml_parameter_group_set_exclusive(GeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "exclusive", (enable == TRUE ? "yes" : "no"));
}

void
geoxml_parameter_group_set_expand(GeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "expand", (enable == TRUE ? "yes" : "no"));
}

gboolean
geoxml_parameter_group_get_exclusive(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_tag_value((GdomeElement*)parameter_group, "exclusive"), "yes"))
		? TRUE : FALSE;
}

gboolean
geoxml_parameter_group_get_expand(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_tag_value((GdomeElement*)parameter_group, "expand"), "yes"))
		? TRUE : FALSE;
}
