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

#include <string.h>

#include <gdome.h>

#include "sequence.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "parameter.h"
#include "parameter_p.h"
#include "parameters.h"
#include "parameters_p.h"

/*
 * internal structures and funcionts
 */

struct geoxml_sequence {
	GdomeElement * element;
};

/**
 * \internal
 * Check \p sequence is a parameter
 */
static inline gboolean
__geoxml_sequence_is_parameter(GeoXmlSequence * sequence)
{
	return (gboolean)!strcmp(gdome_el_nodeName((GdomeElement*)sequence, &exception)->str, "parameter");
}

/**
 * \internal
 * Check if \p sequence is really a sequence and can be modified (the case of parameter in a group)
 */
static int
__geoxml_sequence_check(GeoXmlSequence * sequence, gboolean check_master_instance)
{
	if (sequence == NULL)
		return GEOXML_RETV_NULL_PTR;

	GdomeDOMString *	tag;

	tag = gdome_el_tagName((GdomeElement*)sequence, &exception);
	if (check_master_instance && !strcmp(tag->str, "parameter")) {
		GeoXmlParameters *	parameters;

		parameters = (GeoXmlParameters*)gdome_el_parentNode((GdomeElement*)sequence, &exception);
		if (__geoxml_parameters_group_check(parameters) == FALSE)
			return GEOXML_RETV_NOT_MASTER_INSTANCE;

		return GEOXML_RETV_SUCCESS;
	}
	/* on success, return 0 = GEOXML_RETV_SUCCESS */
	return strcmp(tag->str, "value") &&
		strcmp(tag->str, "default") &&
		strcmp(tag->str, "option") &&
		(strcmp(tag->str, "parameters") &&
			geoxml_parameters_get_is_in_group((GeoXmlParameters*)sequence)) &&
		strcmp(tag->str, "program") &&
		strcmp(tag->str, "category") &&
		strcmp(tag->str, "revision") &&
		strcmp(tag->str, "flow") &&
		strcmp(tag->str, "path") &&
		strcmp(tag->str, "line");
}

int
__geoxml_sequence_move_after_before(GeoXmlSequence * sequence, GeoXmlSequence * position,
	int (*move_function)(GeoXmlSequence * sequence, GeoXmlSequence * position))
{
	GeoXmlParameterGroup *	group;
	int			ret;

	if (!__geoxml_sequence_is_parameter(sequence))
		return move_function(sequence, position);

	if ((group = geoxml_parameter_get_group(GEOXML_PARAMETER(sequence))) != NULL) {
		GSList *		sequence_refs;
		GSList *		i;

		if ((ret = move_function(sequence, position)))
			return ret;

		sequence_refs = __geoxml_parameter_get_referencee_list((GdomeElement*)group,
			__geoxml_get_attr_value((GdomeElement*)sequence, "id"));
		if (group == geoxml_parameter_get_group((GeoXmlParameter*)position)) {
			GSList *	position_refs;
			GSList *	j;

			position_refs = __geoxml_parameter_get_referencee_list((GdomeElement*)group,
				__geoxml_get_attr_value((GdomeElement*)position, "id"));
			for (i = sequence_refs, j = position_refs;
			i != NULL && j != NULL; i = g_slist_next(i), j = g_slist_next(j))
				move_function((GeoXmlSequence*)i->data, (GeoXmlSequence*)j->data);

			g_slist_free(position_refs);
		} else {
			for (i = sequence_refs; i != NULL; i = g_slist_next(i))
				gdome_n_removeChild(gdome_el_parentNode((GdomeElement*)i->data, &exception),
					(GdomeNode*)i->data, &exception);

			__geoxml_parameters_do_insert_in_group_stuff(geoxml_parameter_get_parameters(
				GEOXML_PARAMETER(sequence)), GEOXML_PARAMETER(sequence));
		}

		g_slist_free(sequence_refs);
	} else {
		if ((ret = move_function(sequence, position)))
			return ret;

		if (geoxml_parameter_get_group((GeoXmlParameter*)position) != NULL)
			__geoxml_parameters_do_insert_in_group_stuff(geoxml_parameter_get_parameters(
				GEOXML_PARAMETER(sequence)), GEOXML_PARAMETER(sequence));
	}

	return ret;
}

int
__geoxml_sequence_previous(GeoXmlSequence ** sequence)
{
	*sequence = (GeoXmlSequence*)__geoxml_previous_same_element((GdomeElement*)*sequence);

	return (*sequence != NULL) ? GEOXML_RETV_SUCCESS : GEOXML_RETV_INVALID_INDEX;
}

int
__geoxml_sequence_next(GeoXmlSequence ** sequence)
{
	*sequence = (GeoXmlSequence*)__geoxml_next_same_element((GdomeElement*)*sequence);

	return (*sequence != NULL) ? GEOXML_RETV_SUCCESS : GEOXML_RETV_INVALID_INDEX;
}

void
__geoxml_sequence_remove(GeoXmlSequence * sequence)
{
	gdome_n_removeChild(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, &exception);
}

GeoXmlSequence *
__geoxml_sequence_append_clone(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	clone;
	GeoXmlSequence *	after_last;

	clone = (GeoXmlSequence *)gdome_n_cloneNode((GdomeNode*)sequence, TRUE, &exception);
	__geoxml_element_reassign_ids((GdomeElement*)clone);

	after_last = sequence;
	while (__geoxml_sequence_next(&after_last) == GEOXML_RETV_SUCCESS);
	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)clone, (GdomeNode*)after_last, &exception);

	return clone;
}

int
__geoxml_sequence_move_before(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	if (position == NULL) {
		GeoXmlSequence *	last_element, * tmp;

		last_element = tmp = sequence;
		while (!__geoxml_sequence_next(&tmp))
			last_element = tmp;
		last_element = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)last_element);
		gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
			(GdomeNode*)sequence, (GdomeNode*)last_element, &exception);
	} else
		gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)position, &exception),
			(GdomeNode*)sequence, (GdomeNode*)position, &exception);

	return exception == GDOME_NOEXCEPTION_ERR
		? GEOXML_RETV_SUCCESS : GEOXML_RETV_DIFFERENT_SEQUENCES;
}

int
__geoxml_sequence_move_after(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	if (position == NULL) {
		GeoXmlSequence *	first_element, * tmp;

		first_element = tmp = sequence;
		while (!__geoxml_sequence_previous(&tmp))
			first_element = tmp;
		gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
			(GdomeNode*)sequence, (GdomeNode*)first_element, &exception);
	} else {
		GdomeElement *		next_element;

		next_element = __geoxml_next_element((GdomeElement*)position);
		gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)position, &exception),
			(GdomeNode*)sequence, (GdomeNode*)next_element, &exception);
	}

	return exception == GDOME_NOEXCEPTION_ERR
			? GEOXML_RETV_SUCCESS : GEOXML_RETV_DIFFERENT_SEQUENCES;
}

int
__geoxml_sequence_move_up(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	previous;

	previous = sequence;
	geoxml_sequence_previous(&previous);
	if (previous == NULL)
		return GEOXML_RETV_INVALID_INDEX;

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, (GdomeNode*)previous, &exception);

	return GEOXML_RETV_SUCCESS;
}

int
__geoxml_sequence_move_down(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	next;

	next = sequence;
	geoxml_sequence_next(&next);
	if (next == NULL)
		return GEOXML_RETV_INVALID_INDEX;
	next = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)next);

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
			     (GdomeNode*)sequence, (GdomeNode*)next, &exception);

	return GEOXML_RETV_SUCCESS;
}

/*
 * library functions.
 */

int
geoxml_sequence_previous(GeoXmlSequence ** sequence)
{
	int ret;
	if ((ret = __geoxml_sequence_check(*sequence, FALSE))) {
		*sequence = NULL;
		return ret;
	}

	return __geoxml_sequence_previous(sequence);
}

int
geoxml_sequence_next(GeoXmlSequence ** sequence)
{
	int ret;
	if ((ret = __geoxml_sequence_check(*sequence, FALSE))) {
		*sequence = NULL;
		return ret;
	}

	return __geoxml_sequence_next(sequence);
}

GeoXmlSequence *
geoxml_sequence_append_clone(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	clone;

	if (__geoxml_sequence_check(sequence, TRUE))
		return NULL;
	clone = __geoxml_sequence_append_clone(sequence);
	/* append reference to clone for others group instances */
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	parameter_element;

		__geoxml_foreach_element(parameter_element, __geoxml_parameter_get_referencee_list(
		(GdomeElement*)geoxml_parameter_get_group((GeoXmlParameter*)sequence), __geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_append_clone((GeoXmlSequence*)parameter_element);
	}

	return clone;
}

gint
geoxml_sequence_get_index(GeoXmlSequence * sequence)
{
	gint index;

	for (index = -1; sequence != NULL; __geoxml_sequence_previous(&sequence))
		index++;

	return index;
}

//TODO: improve performance
GeoXmlSequence *
geoxml_sequence_get_at(GeoXmlSequence * sequence, gulong index)
{
	if (sequence == NULL)
		return NULL;

	gulong	i;

	i = geoxml_sequence_get_index(sequence);
	if (i == index)
		return sequence;
	if (i < index)
		for (; i < index && sequence != NULL; ++i, geoxml_sequence_next(&sequence));
	else
		for (; i > index && sequence != NULL; --i, geoxml_sequence_previous(&sequence));

	return sequence;
}

int
geoxml_sequence_remove(GeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	parameter_element;

		__geoxml_foreach_element(parameter_element, __geoxml_parameter_get_referencee_list(
		(GdomeElement*)geoxml_parameter_get_group((GeoXmlParameter*)sequence),
		__geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_remove((GeoXmlSequence*)parameter_element);
	}
	/* must be done after cause we need the id */
	__geoxml_sequence_remove(sequence);

	return GEOXML_RETV_SUCCESS;
}

gboolean
geoxml_sequence_is_same_sequence(GeoXmlSequence * sequence, GeoXmlSequence * other)
{
	if (sequence == NULL || other == NULL)
		return FALSE;
	return (gboolean)gdome_str_equal(
		gdome_el_nodeName((GdomeElement*)sequence, &exception),
		gdome_el_nodeName((GdomeElement*)other, &exception));
}

int
geoxml_sequence_move_into_group(GeoXmlSequence * sequence, GeoXmlParameterGroup * parameter_group)
{
	int 			ret;
	GeoXmlSequence *	first_instance;

	if (parameter_group == NULL)
		return GEOXML_RETV_NULL_PTR;
	if ((ret = __geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (!__geoxml_sequence_is_parameter(sequence))
		return GEOXML_RETV_DIFFERENT_SEQUENCES;

	if (geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	parameter_element;

		__geoxml_foreach_element(parameter_element, __geoxml_parameter_get_referencee_list(
		(GdomeElement*)geoxml_parameter_get_group((GeoXmlParameter*)sequence),
		__geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_remove((GeoXmlSequence*)parameter_element);
	}

	geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
	gdome_el_insertBefore((GdomeElement*)first_instance,
		(GdomeNode*)sequence, NULL, &exception);
	__geoxml_parameters_do_insert_in_group_stuff(GEOXML_PARAMETERS(first_instance), GEOXML_PARAMETER(sequence));

	return ret;
}

int
geoxml_sequence_move_before(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (position != NULL && !geoxml_sequence_is_same_sequence(sequence, position))
		return GEOXML_RETV_DIFFERENT_SEQUENCES;
	return __geoxml_sequence_move_after_before(sequence, position, __geoxml_sequence_move_before);
}

int
geoxml_sequence_move_after(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (position != NULL && !geoxml_sequence_is_same_sequence(sequence, position))
		return GEOXML_RETV_DIFFERENT_SEQUENCES;
	return __geoxml_sequence_move_after_before(sequence, position, __geoxml_sequence_move_after);
}

int
geoxml_sequence_move_up(GeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if ((ret = __geoxml_sequence_move_up(sequence)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	parameter_element;

		__geoxml_foreach_element(parameter_element, __geoxml_parameter_get_referencee_list(
		(GdomeElement*)geoxml_parameter_get_group((GeoXmlParameter*)sequence),
		__geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_move_up((GeoXmlSequence*)parameter_element);
	}
	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_move_down(GeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if ((ret = __geoxml_sequence_move_down(sequence)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	parameter_element;

		__geoxml_foreach_element(parameter_element, __geoxml_parameter_get_referencee_list(
		(GdomeElement*)geoxml_parameter_get_group((GeoXmlParameter*)sequence),
		__geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_move_down((GeoXmlSequence*)parameter_element);
	}
	return GEOXML_RETV_SUCCESS;
}
