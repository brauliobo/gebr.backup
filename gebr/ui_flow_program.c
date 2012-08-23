/*
 * ui_flow_program.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Team
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "ui_flow_program.h"

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-version.h>
#include <gebr/ui_flow_program.h>
#include <libgebr/comm/gebr-comm.h>


struct _GebrUiFlowProgramPriv {
	GebrGeoXmlProgram *program;
	gboolean never_opened;
	GebrGeoXmlProgramStatus status;
	GebrIExprError error_id;
	// inserir ID que deve ser um indentificador unico de um programa
};

/*----------------------------------------------------------------------------------------------*/

G_DEFINE_TYPE(GebrUiFlowProgram, gebr_ui_flow_program, G_TYPE_OBJECT);


static void
gebr_ui_flow_program_finalize(GObject *object)
{
	G_OBJECT_CLASS(gebr_ui_flow_program_parent_class)->finalize(object);
}

static void
gebr_ui_flow_program_class_init(GebrUiFlowProgramClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gebr_ui_flow_program_finalize;
	g_type_class_add_private(klass, sizeof(GebrUiFlowProgramPriv));
}


static void
gebr_ui_flow_program_init(GebrUiFlowProgram *prog)
{
	prog->priv = G_TYPE_INSTANCE_GET_PRIVATE(prog,
	                                       GEBR_TYPE_UI_FLOW_PROGRAM,
	                                       GebrUiFlowProgramPriv);
}

GebrUiFlowProgram *
gebr_ui_flow_program_new(GebrGeoXmlProgram *program)
{
	GebrUiFlowProgram *ui_prog = g_object_new(GEBR_TYPE_UI_FLOW_PROGRAM, NULL);

	ui_prog->priv->program = program;
	ui_prog->priv->status = gebr_geoxml_program_get_status(program);
	gebr_geoxml_program_get_error_id(program, &(ui_prog->priv->error_id));

	return ui_prog;
}

/*----------------------------------------------------------------------------------------------*/

void
gebr_ui_flow_program_set_xml (GebrUiFlowProgram *program,
                              GebrGeoXmlProgram *prog_xml)
{
	program->priv->program = prog_xml;
}

GebrGeoXmlProgram *
gebr_ui_flow_program_get_xml (GebrUiFlowProgram *program)
{
	return program->priv->program;
}

void
gebr_ui_flow_program_set_flag_opened (GebrUiFlowProgram *program,
                                      gboolean never_opened)
{
	program->priv->never_opened = never_opened;
}

gboolean
gebr_ui_flow_program_get_flag_opened (GebrUiFlowProgram *program)
{
	return program->priv->never_opened;
}

void
gebr_ui_flow_program_set_status (GebrUiFlowProgram *program,
                            GebrGeoXmlProgramStatus status)
{
	program->priv->status = status;
}

GebrGeoXmlProgramStatus
gebr_ui_flow_program_get_status (GebrUiFlowProgram *program)
{
	return program->priv->status;
}

void
gebr_ui_flow_program_set_error_id (GebrUiFlowProgram *program,
                              GebrIExprError error_id)
{
	program->priv->error_id = error_id;
}

GebrIExprError
gebr_ui_flow_program_get_error_id (GebrUiFlowProgram *program)
{
	return program->priv->error_id;
}

const gchar *
gebr_ui_flow_program_get_error_tooltip(GebrUiFlowProgram *program)
{
	GebrIExprError errorid = gebr_ui_flow_program_get_error_id(program);
	const gchar *error_message;

	switch (errorid) {
	case GEBR_IEXPR_ERROR_SYNTAX:
	case GEBR_IEXPR_ERROR_TOOBIG:
	case GEBR_IEXPR_ERROR_RUNTIME:
	case GEBR_IEXPR_ERROR_INVAL_TYPE:
	case GEBR_IEXPR_ERROR_TYPE_MISMATCH:
		error_message = _("This program has an invalid expression");
		break;
	case GEBR_IEXPR_ERROR_EMPTY_EXPR:
		error_message = _("A required parameter is unfilled");
		break;
	case GEBR_IEXPR_ERROR_UNDEF_VAR:
	case GEBR_IEXPR_ERROR_UNDEF_REFERENCE:
		error_message = _("An undefined variable is being used");
		break;
	case GEBR_IEXPR_ERROR_INVAL_VAR:
	case GEBR_IEXPR_ERROR_BAD_REFERENCE:
	case GEBR_IEXPR_ERROR_CYCLE:
		error_message = _("A badly defined variable is being used");
		break;
	case GEBR_IEXPR_ERROR_PATH:
		error_message = _("This program has cleaned their paths");
		break;
	case GEBR_IEXPR_ERROR_BAD_MOVE:
	case GEBR_IEXPR_ERROR_INITIALIZE:
	default:
		error_message = "";
		break;
	}

	return error_message;
}

