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

#ifndef __GEBR_GEOXML_H
#define __GEBR_GEOXML_H

/**
 * \mainpage
 * \copydoc geoxml.h
 */

/**
 * \file geoxml.h
 * libgeoxml intends to be an abstraction way to specify the
 * the way programs understand seismic processing programs
 * as SU (Seismic Unix) and Madagascar
 * \dot
 * digraph geoxml {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 	color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 *		fontsize = 9
 * 	]
 *
 * 	"GebrGeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GebrGeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GebrGeoXmlProject" [ URL = "\ref project.h" ];
 * 	"GebrGeoXmlProjectLine" [ URL = "\ref GebrGeoXmlProjectLine" ];
 * 	"GebrGeoXmlLine" [ URL = "\ref line.h" ];
 * 	"GebrGeoXmlLineFlow" [ URL = "\ref GebrGeoXmlLineFlow" ];
 * 	"GebrGeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GebrGeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GebrGeoXmlParameter" [ URL = "\ref gebr-gui-parameter.h" ];
 * 	"GebrGeoXmlProgramParameter" [ URL = "\ref program-parameter.h" ];
 * 	"GebrGeoXmlPropertyValue" [ URL = "\ref GebrGeoXmlPropertyValue" ];
 * 	"GebrGeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlObject" -> "GebrGeoXmlDocument"
 * 	"GebrGeoXmlDocument" -> { "GebrGeoXmlFlow" "GebrGeoXmlLine" "GebrGeoXmlProject" };
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlProgramParameter";
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlParameterGroup";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GebrGeoXmlFlow" -> { "GebrGeoXmlCategory" "GebrGeoXmlProgram" };
 * 	"GebrGeoXmlLine" -> "GebrGeoXmlLineFlow";
 * 	"GebrGeoXmlProject" -> "GebrGeoXmlProjectLine";
 * 	"GebrGeoXmlParameters" -> "GebrGeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GebrGeoXmlProgram" -> "GebrGeoXmlParameters";
 * 	"GebrGeoXmlParameterGroup" -> "GebrGeoXmlParameters";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1..*"
 * 	]
 * 	"GebrGeoXmlProgramParameter" -> "GebrGeoXmlPropertyValue";
 * }
 * \enddot
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
 * 	"GebrGeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlProjectLine" [ URL = "\ref GebrGeoXmlProjectLine" ];
 * 	"GebrGeoXmlLineFlow" [ URL = "\ref GebrGeoXmlLineFlow" ];
 * 	"GebrGeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GebrGeoXmlParameter" [ URL = "\ref gebr-gui-parameter.h" ];
 * 	"GebrGeoXmlPropertyValue" [ URL = "\ref GebrGeoXmlPropertyValue" ];
 * 	"GebrGeoXmlEnumOption" [ URL = "\ref enum_option.h" ];
 * 	"GebrGeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlObject" -> "GebrGeoXmlSequence"
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlProjectLine";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlLineFlow";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlProgram";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlParameters";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlParameter";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlPropertyValue";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlEnumOption";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlValueSequence";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlCategory";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlLinePath";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlPropertyValue";
 * }
 * \enddot
 * Project home page: http://www.gebrproject.com/
 * \author
 * Braulio Barros de Oliveira (brauliobo@gmail.com)
 */

/* For more information, go to libgebr-geoxml's documentation at
 * http://gebr.sf.net/doc/geoxml
 */

/* include all geoxml library's headers. */
#include <geoxml/gebr-geoxml-clipboard.h>
#include <geoxml/gebr-geoxml-document.h>
#include <geoxml/gebr-geoxml-enum-option.h>
#include <geoxml/gebr-geoxml-error.h>
#include <geoxml/gebr-geoxml-flow.h>
#include <geoxml/gebr-geoxml-line.h>
#include <geoxml/gebr-geoxml-object.h>
#include <geoxml/gebr-geoxml-parameter-group.h>
#include <geoxml/gebr-geoxml-parameter.h>
#include <geoxml/gebr-geoxml-parameters.h>
#include <geoxml/gebr-geoxml-program-parameter.h>
#include <geoxml/gebr-geoxml-program.h>
#include <geoxml/gebr-geoxml-project.h>
#include <geoxml/gebr-geoxml-sequence.h>
#include <geoxml/gebr-geoxml-tmpl.h>
#include <geoxml/gebr-geoxml-validate.h>
#include <geoxml/gebr-geoxml-value-sequence.h>

#endif				//__GEBR_GEOXML_H
