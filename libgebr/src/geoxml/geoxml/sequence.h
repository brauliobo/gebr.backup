/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#ifndef __LIBGEBR_GEOXML_SEQUENCE_H
#define __LIBGEBR_GEOXML_SEQUENCE_H

/**
 * \struct GeoXmlSequence sequence.h geoxml/sequence.h
 * \brief
 * Abstract class for elements of a sequence in libgeoxml
 * \dot
 * digraph sequence {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 	]
 *
 * 	"GeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlProjectLine" [ URL = "\ref GeoXmlProjectLine" ];
 * 	"GeoXmlLineFlow" [ URL = "\ref GeoXmlLineFlow" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlPropertyValue" [ URL = "\ref GeoXmlPropertyValue" ];
 * 	"GeoXmlEnumOption" [ URL = "\ref enum_option.h" ];
 * 	"GeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlObject" -> "GeoXmlSequence"
 * 	"GeoXmlSequence" -> "GeoXmlProjectLine";
 * 	"GeoXmlSequence" -> "GeoXmlLineFlow";
 * 	"GeoXmlSequence" -> "GeoXmlProgram";
 * 	"GeoXmlSequence" -> "GeoXmlParameters";
 * 	"GeoXmlSequence" -> "GeoXmlParameter";
 * 	"GeoXmlSequence" -> "GeoXmlPropertyValue";
 * 	"GeoXmlSequence" -> "GeoXmlEnumOption";
 * 	"GeoXmlSequence" -> "GeoXmlValueSequence";
 * 	"GeoXmlValueSequence" -> "GeoXmlCategory";
 * 	"GeoXmlValueSequence" -> "GeoXmlLinePath";
 * 	"GeoXmlValueSequence" -> "GeoXmlPropertyValue";
 * }
 * \enddot
 * \see sequence.h
 */

/**
 * \file sequence.h
 * Abstract class for elements of a sequence in libgeoxml
 *
 * GeoXmlProjectLine, GeoXmlLineFlow, GeoXmlProgram, GeoXmlProgramParameter and GeoXmlCategory
 * can be treated as a super class of GeoXmlSequence.
 */

/**
 * Cast to super types of GeoXmlSequence to it.
 */
#define GEOXML_SEQUENCE(seq) ((GeoXmlSequence*)(seq))

/**
 * The GeoXmlSequence struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_sequence GeoXmlSequence;

#include <glib.h>

/**
 * Use as an auxiliary function to geoxml_sequence_next.
 * Assign \p sequence to the previous sequence sequenced
 * or NULL if there isn't.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 *
 * \see geoxml_sequence_next
 */
int
geoxml_sequence_previous(GeoXmlSequence ** sequence);

/**
 * Use to iterate over sequences.
 * Assign \p sequence to the next sequence sequenced
 * or NULL if there isn't.
 * Example:
 * \code
 * GeoXmlSequence * i;
 * geoxml_[parent]_get_[sequence]([parent], &i, 0);
 * while (i != NULL) {
 * 	...
 * 	geoxml_sequence_next(&i);
 * }
 * \endcode
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 *
 * \see geoxml_sequence_previous
 */
int
geoxml_sequence_next(GeoXmlSequence ** sequence);

/**
 * Clone \p sequence element and add it to the end of the sequence.
 * Returns the cloned sequence element.
 *
 * If \p sequence is NULL or is not a sequence, NULL is returned.
 *
 */
GeoXmlSequence *
geoxml_sequence_append_clone(GeoXmlSequence * sequence);

/**
 * Get the index of \p sequence.
 *
 * If \p sequence is NULL returns -1.
 */
gint
geoxml_sequence_get_index(GeoXmlSequence * sequence);

/**
 * Returns the sequence at index in \p sequence.
 *
 * If \p sequence is NULL returns NULL.
 */
GeoXmlSequence *
geoxml_sequence_get_at(GeoXmlSequence * sequence, gulong index);

/**
 * Removes \p sequence from its sequence. It is not deleted and can be reinserted
 * into sequence using geoxml_sequence_prepend or geoxml_sequence_append.
 *
 * A special case are the parameter. It cannot be removed if it belongs to an
 * instanciated group. Also, if it is removed, all referenced parameters are
 * automatically removed
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR,
 * GEOXML_RETV_NOT_A_SEQUENCE, GEOXML_RETV_MORE_THAN_ONE_INSTANCES
 */
int
geoxml_sequence_remove(GeoXmlSequence * sequence);

/**
 * Moves \p sequence to the position before \p position. If \p position is NULL then
 * moves to the end of the sequence
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR,
 * GEOXML_RETV_NOT_A_SEQUENCE, GEOXML_RETV_DIFFERENT_SEQUENCES
 */
int
geoxml_sequence_move_before(GeoXmlSequence * sequence, GeoXmlSequence * position);

/**
 * Moves \p sequence to the position after \p position. If \p position is NULL then
 * moves to the beggining of the sequence
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR,
 * GEOXML_RETV_NOT_A_SEQUENCE, GEOXML_RETV_DIFFERENT_SEQUENCES
 */
int
geoxml_sequence_move_after(GeoXmlSequence * sequence, GeoXmlSequence * position);

/**
 * Exchange positions of the sequence above \p sequence with \p sequence in \p sequence.
 * \p sequence must belong to \p sequence.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 */
int
geoxml_sequence_move_up(GeoXmlSequence * sequence);

/**
 * Exchange positions of the sequence below \p sequence with \p sequence in \p sequence.
 * \p sequence must belong to \p sequence.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 */
int
geoxml_sequence_move_down(GeoXmlSequence * sequence);

#endif //__LIBGEBR_GEOXML_SEQUENCE_H
