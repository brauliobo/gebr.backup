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

#ifndef __LIBGEBR_GEOXML_PARAMETER_GROUP_H
#define __LIBGEBR_GEOXML_PARAMETER_GROUP_H

/**
 * \struct GeoXmlParameterGroup parameter_group.h geoxml/parameter_group.h
 * \brief
 * A list of parameters.
 * \dot
 * digraph parameter_group {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> "GeoXmlFlow";
 * 	"GeoXmlParameter" -> "GeoXmlProgramParameter";
 * 	"GeoXmlParameter" -> "GeoXmlParameterGroup";
 * 	"GeoXmlParameters" -> "GeoXmlParameterGroup";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> "GeoXmlProgram";
 * 	"GeoXmlParameters" -> "GeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GeoXmlProgram" -> "GeoXmlParameters";
 * }
 * \enddot
 * \see parameter_group.h
 */

/**
 * The GeoXmlParameterGroup struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_parameter_group GeoXmlParameterGroup;

#include <glib.h>

/**
 *
 * If \p parameter_group nothing is done.
 */
void
geoxml_parameter_group_set_exclusive(GeoXmlParameterGroup * parameter_group, const gboolean enable);

/**
 *
 * If \p parameter_group nothing is done.
 */
void
geoxml_parameter_group_set_expand(GeoXmlParameterGroup * parameter_group, const gboolean enable);

/**
 *
 * If \p parameter_group returns FALSE.
 */
gboolean
geoxml_parameter_group_get_exclusive(GeoXmlParameterGroup * parameter_group);

/**
 *
 * If \p parameter_group returns FALSE.
 */
gboolean
geoxml_parameter_group_get_expand(GeoXmlParameterGroup * parameter_group);

#endif //__LIBGEBR_GEOXML_PARAMETER_GROUP_H
