/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

/*
 * File: cb_flowcomp.c
 * Callbacks to flow components
 */

#include <geoxml.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "cb_flowcomp.h"
#include "gebr.h"
#include "cb_flow.h"
#include "widgets.h"
#include "callbacks.h"
#include "ui_prop.h"

