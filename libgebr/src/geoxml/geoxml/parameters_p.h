/*   libgebr - G�BR Library
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_GEOXML_PARAMETERS_P_H
#define __LIBGEBR_GEOXML_PARAMETERS_P_H

/**
 * If \p parameters is a group and it was instantiated only once,
 * then adjust the number of parameters of a instance (npar)
 */
gboolean
__geoxml_parameters_adjust_group_npar(GeoXmlParameters * parameters, glong adjust);

/**
 * \internal
 * Create a new parameter with type \p type.
 */
GeoXmlParameter *
__geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type, gboolean adjust_npar);

/**
 * \internal
 * Change all \p parameters' values to \p value,
 * including default values.
 */
void
__geoxml_parameters_reset(GeoXmlParameters * parameters, gboolean recursive);

#endif //__LIBGEBR_GEOXML_PARAMETERS_P_H
