/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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

#include <stdlib.h>
#include <string.h>

#include <gdome.h>

#include "parameters.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "program_parameter.h"
#include "program_parameter_p.h"

/*
 * internal stuff
 */

struct geoxml_parameters {
	GdomeElement * element;
};

gboolean
__geoxml_parameters_adjust_group_npar(GeoXmlParameters * parameters, glong adjust)
{
	GdomeElement *	parent_element;
	gchar *		value;

	parent_element = (GdomeElement*)gdome_el_parentNode((GdomeElement*)parameters, &exception);
	if (strcmp(gdome_el_nodeName(parent_element, &exception)->str, "group") != 0)
		return TRUE;
	if (strcmp(__geoxml_get_attr_value(parent_element, "instances"), "1") != 0)
		return FALSE;

	value = g_strdup_printf("%lu",
		atol(__geoxml_get_attr_value(parent_element, "npar")) + adjust);
	__geoxml_set_attr_value(parent_element, "npar", value);

	return TRUE;
}

GeoXmlParameter *
__geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type, gboolean adjust_npar)
{
	GdomeElement *	parameter_element;

	/* increases the npar counter if it is a group */
	if (adjust_npar == TRUE && __geoxml_parameters_adjust_group_npar(parameters, +1) == FALSE)
		return NULL;

	/* parameter element */
	parameter_element = __geoxml_new_element((GdomeElement*)parameters, parameter_type_to_str[type]);
	__geoxml_insert_new_element(parameter_element, "label", NULL);
	parameter_element = __geoxml_new_element(parameter_element, parameter_type_to_str[type]);

	if (type != GEOXML_PARAMETERTYPE_GROUP) {
		__geoxml_insert_new_element(parameter_element, "keyword", NULL);
		__geoxml_insert_new_element(parameter_element, "label", NULL);
		__geoxml_insert_new_element(parameter_element, (type != GEOXML_PARAMETERTYPE_FLAG) ? "value" : "state", NULL);

		if (type != GEOXML_PARAMETERTYPE_FLAG)
			geoxml_program_parameter_set_required(
				(GeoXmlProgramParameter*)parameter_element, FALSE);
		switch (type) {
		case GEOXML_PARAMETERTYPE_FILE:
			geoxml_program_parameter_set_file_be_directory(
				(GeoXmlProgramParameter*)parameter_element, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			geoxml_program_parameter_set_range_properties(
				(GeoXmlProgramParameter*)parameter_element, "", "", "", "");
			break;
		default:
			break;
		}
		__geoxml_program_parameter_reset_default((GeoXmlProgramParameter*)parameter_element);
	} else {
		GeoXmlParameters *	parameters;

		parameters = (GeoXmlParameters*)__geoxml_insert_new_element(parameter_element, "parameters", NULL);
		geoxml_parameters_set_exclusive(parameters, NULL);
	}

	return (GeoXmlParameter*)parameter_element;
}

/*
 * library functions.
 */

GeoXmlParameter *
geoxml_parameters_append_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	if (parameters == NULL)
		return NULL;

	GdomeElement *	element;

	element = (GdomeElement*)__geoxml_parameters_new_parameter(parameters, type, TRUE);
	if (element == NULL)
		return NULL;
	gdome_el_insertBefore((GdomeElement*)parameters, (GdomeNode*)element, NULL, &exception);

	return (GeoXmlParameter*)element;
}

void
geoxml_parameters_set_exclusive(GeoXmlParameters * parameters, GeoXmlParameter * parameter)
{
	if (parameters == NULL)
		return;
	if (parameter == NULL) {
		__geoxml_set_attr_value((GdomeElement*)parameters, "exclusive", "0");
		return;
	}

	gchar *	value;

	value = g_strdup_printf("%ld", __geoxml_get_element_index((GdomeElement*)parameter)+1);
	__geoxml_set_attr_value((GdomeElement*)parameters, "exclusive", value);
	g_free(value);

	if (geoxml_parameters_get_selected(parameters) == NULL)
		geoxml_parameters_set_selected(parameters, parameter);
}

void
geoxml_parameters_set_selected(GeoXmlParameters * parameters, GeoXmlParameter * parameter)
{
	if (parameters == NULL)
		return;
	if (geoxml_parameters_get_exclusive(parameters) == NULL)
		return;
	if (parameter == NULL) {
		__geoxml_set_attr_value((GdomeElement*)parameters, "selected", "0");
		return;
	}

	gchar *	value;

	value = g_strdup_printf("%ld", __geoxml_get_element_index((GdomeElement*)parameter)+1);
	__geoxml_set_attr_value((GdomeElement*)parameters, "selected", value);
	g_free(value);
}

GeoXmlSequence *
geoxml_parameters_get_first_parameter(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;
	return (GeoXmlSequence*)__geoxml_get_first_element((GdomeElement*)parameters, "*");
}

int
geoxml_parameters_get_parameter(GeoXmlParameters * parameters, GeoXmlSequence ** _parameter, gulong index)
{
	if (parameters == NULL) {
		*_parameter = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	gulong			i;
	GeoXmlSequence *	parameter;

	parameter = (GeoXmlSequence*)__geoxml_get_first_element((GdomeElement*)parameters, "*");
	for (i = 0; i < index; ++i) {
		parameter = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)parameter);
		if (parameter == NULL) {
			*_parameter = parameter;
			return GEOXML_RETV_INVALID_INDEX;
		}
	}
	*_parameter = parameter;

	return GEOXML_RETV_SUCCESS;
}

glong
geoxml_parameters_get_number(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return -1;

	gint			parameters_number = 0;
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (parameters_number = 0; parameter != NULL; ++parameters_number)
		geoxml_sequence_next(&parameter);

	return parameters_number;
}

GeoXmlParameter *
geoxml_parameters_get_exclusive(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;

	gulong			index;
	GeoXmlSequence *	parameter;

	index = atol(__geoxml_get_attr_value((GdomeElement*)parameters, "exclusive"));
	if (index == 0)
		return NULL;
	index--;
	geoxml_parameters_get_parameter(parameters, &parameter, index);

	/* there isn't a parameter at index, so use the first parameter of the group */
	if (parameter == NULL)
		return (GeoXmlParameter*)geoxml_parameters_get_first_parameter(parameters);

	return (GeoXmlParameter*)parameter;
}

GeoXmlParameter *
geoxml_parameters_get_selected(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;
	if (geoxml_parameters_get_exclusive(parameters) == NULL)
		return NULL;

	gulong			index;
	GeoXmlSequence *	parameter;

	index = atol(__geoxml_get_attr_value((GdomeElement*)parameters, "selected"));
	if (index == 0)
		return NULL;
	index--;
	geoxml_parameters_get_parameter(parameters, &parameter, index);

	/* there isn't a parameter at index, so use the first parameter of the group */
	if (parameter == NULL)
		return (GeoXmlParameter*)geoxml_parameters_get_first_parameter(parameters);

	return (GeoXmlParameter *)parameter;
}

gboolean
geoxml_parameters_get_is_in_group(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;
	return !strcmp(
			gdome_el_tagName((GdomeElement*)gdome_el_parentNode(
				(GdomeElement*)parameters, &exception), &exception)->str,
		       "group") ? TRUE : FALSE;
}

void
geoxml_parameters_reset(GeoXmlParameters * parameters, gboolean recursive)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; geoxml_sequence_next(&parameter))
		geoxml_parameter_reset(GEOXML_PARAMETER(parameter), recursive);
}
