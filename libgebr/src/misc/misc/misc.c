/*   libgebr - GêBR Library
 *   Copyright (C) 2007 GÃªBR core team (http://gebr.sourceforge.net)
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

#include <stdlib.h>
#include <string.h>

#include "misc.h"

/**
 * mkstemp has the exigency that the XXXXXX comes in the end of the template.
 * This function returns an static allocated string which will be used to create
 * temporary filename, possibly with an extension, which is not easily possible with mkstemp
 */
gchar *
make_temp_filename(void)
{
	static gchar	template[7];

	strcmp(template, "XXXXXX");
	mkstemp(template);

	return template;
}
