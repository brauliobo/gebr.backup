/*   libgebr - GÍBR Library
 *   Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
 *
 *   This parameters is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This parameters is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this parameters.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gdome.h>

#include "parameters.h"
#include "xml.h"
#include "types.h"
#include "parameter_group.h"
#include "program_parameter.h"

/*
 * internal stuff
 */

struct geoxml_parameters {
	GdomeElement * element;
};

GeoXmlParameter *
__geoxml_parameters_new_parameter(GeoXmlParameters * parameters, GdomeElement * before, enum GEOXML_PARAMETERTYPE type)
{
	GdomeElement *	parameters_element;
	GdomeElement *	parameter_element;
	gchar *		tag_name;

	parameters_element = (GdomeElement*)parameters;
	parameter_element = __geoxml_new_element(__geoxml_get_first_element((GdomeElement*)parameters, "parameters"),
		before, parameter_type_to_str[type]);
	tag_name = (type != GEOXML_PARAMETERTYPE_FLAG)
		? "value" : "state";

	/* elements/attibutes */
	if (type != GEOXML_PARAMETERTYPE_GROUP) {
		__geoxml_new_element(parameter_element, NULL, "keyword");
		__geoxml_new_element(parameter_element, NULL, "label");
		__geoxml_new_element(parameter_element, NULL, tag_name);
	} else {
		__geoxml_new_element(parameter_element, NULL, "label");
		/* attributes */
		__geoxml_set_attr_value(parameter_element, "instances", "1");
		geoxml_parameter_group_set_exclusive((GeoXmlParameterGroup*)parameter_element, FALSE);
		geoxml_parameter_group_set_expand((GeoXmlParameterGroup*)parameter_element, TRUE);
	}

	if (type != GEOXML_PARAMETERTYPE_FLAG)
		geoxml_program_parameter_set_required((GeoXmlProgramParameter*)parameter_element, FALSE);

	switch (type) {
	case GEOXML_PARAMETERTYPE_FILE:
		geoxml_program_parameter_set_file_be_directory((GeoXmlProgramParameter*)parameter_element, FALSE);
		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		geoxml_program_parameter_set_flag_default((GeoXmlProgramParameter*)parameter_element, FALSE);
		break;
	case GEOXML_PARAMETERTYPE_RANGE:
		geoxml_program_parameter_set_range_properties((GeoXmlProgramParameter*)parameter_element, "", "", "");
		break;
	default:
		break;
	}

	return (GeoXmlParameter*)parameter_element;
}

/*
 * library functions.
 */

GeoXmlParameter *
geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE parameter_type)
{
	if (parameters == NULL)
		return NULL;
	return __geoxml_parameters_new_parameter(parameters, NULL, parameter_type);
}

GeoXmlParameter *
geoxml_parameters_get_first_parameter(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;
	return (GeoXmlParameter*)__geoxml_get_first_element((GdomeElement*)parameters, "*");
}

glong
geoxml_parameters_get_number(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return -1;

	GdomeElement*	parameters_element;
	gint		i;
	gint		parameters_number = 0;

	parameters_element = __geoxml_get_first_element((GdomeElement*)parameters, "parameters");

	for (i = 0; i < parameter_type_to_str_len; ++i)
		parameters_number += __geoxml_get_elements_number((GdomeElement*)parameters, parameter_type_to_str[i]);

	return parameters_number;
}
