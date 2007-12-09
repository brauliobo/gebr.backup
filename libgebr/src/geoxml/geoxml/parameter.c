/*   libgebr - GÍBR Library
 *   Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
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

#include <gdome.h>

#include "parameter.h"
#include "types.h"
#include "parameters_p.h"
#include "program_parameter.h"

/*
 * internal stuff
 */

struct geoxml_parameter {
	GdomeElement * element;
};

const char * parameter_type_to_str[] = {
	"string", "int", "file",
	"flag", "float", "range",
	"enum", "group"
};

const int parameter_type_to_str_len = 8;

/*
 * library functions.
 */

void
geoxml_parameter_set_type(GeoXmlParameter ** parameter, enum GEOXML_PARAMETERTYPE type)
{
	if (*parameter == NULL)
		return;

	GdomeElement *		parent_element;
	GeoXmlParameter *	old_parameter;

	old_parameter = *parameter;
	parent_element = (GdomeElement*)gdome_el_parentNode((GdomeElement*)old_parameter, &exception);

	*parameter = __geoxml_parameters_new_parameter((GeoXmlParameters*)parent_element, (GdomeElement*)old_parameter, type);
	gdome_el_insertBefore(parent_element, (GdomeNode*)*parameter, (GdomeNode*)old_parameter, &exception);

	geoxml_program_parameter_set_keyword(*program_parameter, geoxml_program_parameter_get_keyword(old_program_parameter));
	geoxml_program_parameter_set_label(*program_parameter, geoxml_program_parameter_get_label(old_program_parameter));

	gdome_el_removeChild(parent_element, (GdomeNode*)old_parameter, &exception);
}

enum GEOXML_PARAMETERTYPE
geoxml_parameter_get_type(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return GEOXML_PARAMETERTYPE_STRING;

	GdomeDOMString*		tag_name;
	int			i;

	tag_name = gdome_el_tagName((GdomeElement*)parameter, &exception);

	for (i = 0; i < parameter_type_to_str_len; ++i)
		if (!g_ascii_strcasecmp(parameter_type_to_str[i], tag_name->str))
			return (enum GEOXML_PARAMETERTYPE)i;

	/* here we must have a "parameters" element */
	return GEOXML_PARAMETERTYPE_GROUP;
}

gboolean
geoxml_parameter_get_is_program_parameter(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return GEOXML_PARAMETERTYPE_STRING;
	return (geoxml_parameter_get_type != GEOXML_PARAMETERTYPE_GROUP)
		? TRUE : FALSE;
}
