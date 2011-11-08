/*   GeBR - An environment for seismic processing.
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

#include "gebr-task.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "gebr-task.h"
#include "gebr.h"
#include "server.h"
#include "gebr-job-control.h"
#include "server.h"
#include "gebr-marshal.h"

enum {
	OUTPUT,
	STATUS_CHANGE,
	N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

struct _GebrTaskPriv {
	GebrServer *server;
	enum JobStatus status;
	gchar *rid;
	gint frac;
	gint total;
	gdouble percentage;
	GString *start_date;
	GString *finish_date;
	GString *issues;
	GString *cmd_line;
	GString *moab_jid;
	GString *queue_id;
	GString *output;
	GString *n_procs;
	GString *niceness;
	GString *last_run_date;
};

G_DEFINE_TYPE(GebrTask, gebr_task, G_TYPE_OBJECT);

static GHashTable *tasks_map = NULL;

static GHashTable *
get_tasks_map(void)
{
	if (!tasks_map)
		tasks_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return tasks_map;
}

static void
gebr_task_free(GebrTask *task)
{
	g_free(task->priv->rid);
	g_string_free(task->priv->start_date, TRUE);
	g_string_free(task->priv->finish_date, TRUE);
	g_string_free(task->priv->issues, TRUE);
	g_string_free(task->priv->cmd_line, TRUE);
	g_string_free(task->priv->moab_jid, TRUE);
	g_string_free(task->priv->queue_id, TRUE);
	g_string_free(task->priv->output, TRUE);
	g_string_free(task->priv->niceness, TRUE);
	g_string_free(task->priv->last_run_date, TRUE);
}

static void
gebr_task_finalize(GObject *object)
{
	GebrTask *task = GEBR_TASK(object);
	gebr_task_free(task);
	G_OBJECT_CLASS(gebr_task_parent_class)->finalize(object);
}

static void gebr_task_init(GebrTask *task)
{
	task->priv = G_TYPE_INSTANCE_GET_PRIVATE(task,
	                                         GEBR_TYPE_TASK,
	                                         GebrTaskPriv);

	task->priv->output = g_string_new(NULL);
	task->priv->start_date = g_string_new(NULL);
	task->priv->finish_date = g_string_new(NULL);
	task->priv->issues = g_string_new(NULL);
	task->priv->cmd_line = g_string_new(NULL);
	task->priv->moab_jid = g_string_new(NULL);
	task->priv->queue_id = g_string_new(NULL);
	task->priv->n_procs = g_string_new(NULL);
	task->priv->niceness = g_string_new(NULL);
	task->priv->last_run_date = g_string_new(NULL);
}

static void gebr_task_class_init(GebrTaskClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_task_finalize;

	signals[OUTPUT] =
		g_signal_new("output",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GebrTaskClass, output),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[STATUS_CHANGE] =
		g_signal_new("status-change",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GebrTaskClass, status_change),
			     NULL, NULL,
			     gebr_cclosure_marshal_VOID__INT_INT_STRING,
			     G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(GebrTaskPriv));
}

static gchar *
build_task_id(const gchar *rid,
	      const gchar *frac)
{
	return g_strconcat(rid, ":", frac, NULL);
}

static gchar *
gebr_task_get_id(GebrTask *task)
{
	return g_strdup_printf("%s:%d:%d",
			       task->priv->rid,
			       task->priv->frac,
			       task->priv->total);
}

GebrTask *
gebr_task_new(GebrServer  *server,
	      const gchar *rid,
	      const gchar *_frac)
{
	gchar *frac, *total;
	GebrTask *task = g_object_new(GEBR_TYPE_TASK, NULL);

	frac = g_strdup(_frac);
	total = strchr(frac, ':');
	total[0] = '\0';
	total++;

	task->priv->status = JOB_STATUS_INITIAL; 
	task->priv->server = server;
	task->priv->rid = g_strdup(rid);
	task->priv->frac = atoi(frac);
	task->priv->total = atoi(total);

	g_free(frac);

	g_hash_table_insert(get_tasks_map(), gebr_task_get_id(task), task);

	return task;
}

void
gebr_task_init_details(GebrTask *task,
		       GString  *status,
		       GString  *start_date,
		       GString  *finish_date,
		       GString  *issues,
		       GString  *cmd_line,
		       GString  *queue,
		       GString  *moab_jid,
		       GString  *output,
		       GString  *n_procs,
		       GString  *niceness,
		       GString  *last_run_date)
{
	task->priv->status = job_translate_status(status);
	g_string_assign(task->priv->start_date, start_date->str);
	g_string_assign(task->priv->finish_date, finish_date->str);
	g_string_assign(task->priv->issues, issues->str);
	g_string_assign(task->priv->cmd_line, cmd_line->str);
	g_string_assign(task->priv->moab_jid, moab_jid->str);
	g_string_assign(task->priv->queue_id, queue->str);
	g_string_assign(task->priv->output, output->str);
	g_string_assign(task->priv->n_procs, n_procs->str);
	g_string_assign(task->priv->niceness, niceness->str);
	g_string_assign(task->priv->last_run_date, last_run_date->str);
}

enum JobStatus job_translate_status(GString * status)
{
	enum JobStatus translated_status;

	if (!strcmp(status->str, "unknown"))
		translated_status = JOB_STATUS_INITIAL;
	else if (!strcmp(status->str, "queued"))
		translated_status = JOB_STATUS_QUEUED;
	else if (!strcmp(status->str, "failed"))
		translated_status = JOB_STATUS_FAILED;
	else if (!strcmp(status->str, "running"))
		translated_status = JOB_STATUS_RUNNING;
	else if (!strcmp(status->str, "finished"))
		translated_status = JOB_STATUS_FINISHED;
	else if (!strcmp(status->str, "canceled"))
		translated_status = JOB_STATUS_CANCELED;
	else if (!strcmp(status->str, "requeued"))
		translated_status = JOB_STATUS_REQUEUED;
	else if (!strcmp(status->str, "issued"))
		translated_status = JOB_STATUS_ISSUED;
	else
		translated_status = JOB_STATUS_INITIAL;

	return translated_status;
}

GebrTask *
gebr_task_find(const gchar *rid, const gchar *frac)
{
	g_return_val_if_fail(rid != NULL && frac != NULL, NULL);

	gchar *tid = build_task_id(rid, frac);
	GebrTask *task = g_hash_table_lookup(get_tasks_map(), tid);
	g_free(tid);
	return task;
}

void
gebr_task_get_fraction(GebrTask *task, gint *frac, gint *total)
{
	g_return_if_fail(frac != NULL || total != NULL);

	if (frac)
		*frac = task->priv->frac;

	if (total)
		*total = task->priv->total;
}

enum JobStatus
gebr_task_get_status(GebrTask *task)
{
	return task->priv->status;
}

void
gebr_task_emit_output_signal(GebrTask *task,
			     const gchar *output)
{
	g_string_append(task->priv->output, output);
	g_signal_emit(task, signals[OUTPUT], 0, output);
}

void
gebr_task_emit_status_changed_signal(GebrTask *task,
				     enum JobStatus new_status,
				     const gchar *parameter)
{
	enum JobStatus old_status;

	old_status = task->priv->status;
	task->priv->status = new_status;

	switch (new_status)
	{
	case JOB_STATUS_RUNNING:
		g_string_assign(task->priv->start_date, parameter);
		break;
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_CANCELED:
		g_string_assign(task->priv->finish_date, parameter);
		break;
	default:
		break;
	}

	g_signal_emit(task, signals[STATUS_CHANGE], 0, old_status, new_status, parameter);
}

const gchar *
gebr_task_get_cmd_line(GebrTask *task)
{
	return task->priv->cmd_line->str;
}

const gchar *
gebr_task_get_start_date(GebrTask *task)
{
	return task->priv->start_date->len > 0 ? task->priv->start_date->str : NULL;
}

const gchar *
gebr_task_get_finish_date(GebrTask *task)
{
	return task->priv->finish_date->str;
}

const gchar *
gebr_task_get_issues(GebrTask *task)
{
	return task->priv->issues->str;
}
const gchar *
gebr_task_get_output(GebrTask *task)
{
	return task->priv->output->str;
}

void
gebr_task_close(GebrTask *task, const gchar *rid)
{
	gchar *tid = gebr_task_get_id(task);
	if (gebr_comm_server_is_logged(task->priv->server->comm))
		gebr_comm_protocol_socket_oldmsg_send(task->priv->server->comm->socket, FALSE,
						      gebr_comm_protocol_defs.clr_def, 1,
						      rid);

	g_hash_table_remove(get_tasks_map(), tid);
	g_free(tid);
}

void
gebr_task_kill(GebrTask *task)
{
	gebr_comm_protocol_socket_oldmsg_send(task->priv->server->comm->socket, FALSE,
					      gebr_comm_protocol_defs.kil_def, 1,
					      task->priv->rid);
}

GebrServer *
gebr_task_get_server(GebrTask *task)
{
	return task->priv->server;
}

const gchar *
gebr_task_get_queue(GebrTask *task)
{
	return task->priv->queue_id->str;
}

const gchar *
gebr_task_get_nprocs(GebrTask *task)
{
	return task->priv->n_procs->str;
}

const gchar *
gebr_task_get_niceness(GebrTask *task)
{
	return task->priv->niceness->str;
}

const gchar *
gebr_task_get_last_run_date(GebrTask *task)
{
	return task->priv->last_run_date->str;
}

void
gebr_task_set_percentage(GebrTask *task, gdouble perc)
{
	task->priv->percentage = perc;
}

gdouble
gebr_task_get_percentage(GebrTask *task)
{
	return task->priv->percentage;
}
