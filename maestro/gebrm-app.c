/*
 * gebrm-app.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Team
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
#include "gebrm-app.h"
#include "gebrm-daemon.h"
#include "gebrm-job.h"

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/utils.h>
#include <glib/gi18n.h>

/*
 * Global variables to implement GebrmAppSingleton methods.
 */
static GebrmAppFactory __factory = NULL;
static gpointer           __data = NULL;
static GebrmApp           *__app = NULL;

#define GEBRM_LIST_OF_SERVERS_PATH ".gebr/gebrm"
#define GEBRM_LIST_OF_SERVERS_FILENAME "servers.conf"

static void gebrm_config_save_server(GebrmDaemon *daemon);

static GebrmDaemon *gebrm_add_server_to_list(GebrmApp *app,
					     const gchar *addr,
					     const gchar *tags);

static void gebrm_config_delete_server(const gchar *serv);

static gboolean gebrm_remove_server_from_list(GebrmApp *app, const gchar *address);

gboolean gebrm_config_load_servers(GebrmApp *app, gchar *path);

G_DEFINE_TYPE(GebrmApp, gebrm_app, G_TYPE_OBJECT);

struct _GebrmAppPriv {
	GMainLoop *main_loop;
	GebrCommListenSocket *listener;
	GebrCommSocketAddress address;
	GList *connections;
	GList *daemons;

	// Job controller
	GHashTable *jobs;
};


// Refactor this method to GebrmJobController {{{
static GebrmJob *
gebrm_app_job_controller_find(GebrmApp *app, const gchar *id)
{
	return g_hash_table_lookup(app->priv->jobs, id);
}

static void
gebrm_app_job_controller_add(GebrmApp *app, GebrmJob *job)
{
	g_hash_table_insert(app->priv->jobs,
			    g_strdup(gebrm_job_get_id(job)),
			    job);
}
// }}}

static void
send_server_status_message(GebrCommProtocolSocket *socket,
			   GebrCommServer *server)
{
	const gchar *state = gebr_comm_server_state_to_string(server->state);
	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.ssta_def, 2,
					      server->address->str,
					      state);
}

static void
gebrm_app_job_controller_on_task_def(GebrmDaemon *daemon,
				     GebrmTask *task,
				     GebrmApp* app)
{
	const gchar *rid = gebrm_task_get_job_id(task);
	GebrmJob *job = gebrm_app_job_controller_find(app, rid);

	if (!job)
		g_return_if_reached();

	gebrm_job_append_task(job, task);
}

static void
gebrm_app_job_controller_on_issued(GebrmJob    *job,
				   const gchar *issues,
				   GebrmApp    *app)
{
	for (GList *i = app->priv->connections; i; i = i->next) {
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
						      gebr_comm_protocol_defs.iss_def, 2,
						      gebrm_job_get_id(job),
						      issues);
	}
}

static void
gebrm_app_job_controller_on_cmd_line_received(GebrmJob *job,
					      GebrmTask *task,
					      const gchar *cmd,
					      GebrmApp *app)
{
	gchar *frac;
	for (GList *i = app->priv->connections; i; i = i->next) {
		frac = g_strdup_printf("%d", gebrm_task_get_fraction(task));
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
						      gebr_comm_protocol_defs.cmd_def, 3,
						      gebrm_job_get_id(job),
						      frac,
						      cmd);
		g_free(frac);
	}
}

static void
gebrm_app_job_controller_on_output(GebrmJob *job,
				   GebrmTask *task,
				   const gchar *output,
				   GebrmApp *app)
{
	for (GList *i = app->priv->connections; i; i = i->next) {
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
						      gebr_comm_protocol_defs.out_def, 2,
						      gebrm_job_get_id(job),
						      output);
	}
}

static void
gebrm_app_job_controller_on_status_change(GebrmJob *job,
					  gint old_status,
					  gint new_status,
					  const gchar *parameter,
					  GebrmApp *app)
{
	for (GList *i = app->priv->connections; i; i = i->next) {
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
						      gebr_comm_protocol_defs.sta_def, 3,
						      gebrm_job_get_id(job),
						      gebr_comm_job_get_string_from_status(new_status),
						      parameter);
	}
}

static void
gebrm_app_daemon_on_state_change(GebrmDaemon *daemon,
				 GebrCommServerState state,
				 GebrmApp *app)
{
	for (GList *i = app->priv->connections; i; i = i->next)
		send_server_status_message(i->data, gebrm_daemon_get_server(daemon));
}

static void
gebrm_app_finalize(GObject *object)
{
	GebrmApp *app = GEBRM_APP(object);
	g_hash_table_unref(app->priv->jobs);
	g_list_free(app->priv->connections);
	g_list_free(app->priv->daemons);
	G_OBJECT_CLASS(gebrm_app_parent_class)->finalize(object);
}

static void
gebrm_app_class_init(GebrmAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gebrm_app_finalize;
	g_type_class_add_private(klass, sizeof(GebrmAppPriv));
}

static void
gebrm_app_init(GebrmApp *app)
{
	app->priv = G_TYPE_INSTANCE_GET_PRIVATE(app,
						GEBRM_TYPE_APP,
						GebrmAppPriv);
	app->priv->main_loop = g_main_loop_new(NULL, FALSE);
	app->priv->connections = NULL;
	app->priv->daemons = NULL;
	app->priv->jobs = g_hash_table_new_full(g_str_hash, g_str_equal,
						g_free, NULL);

	gchar *path = g_build_filename(g_get_home_dir(),
				       GEBRM_LIST_OF_SERVERS_PATH,
				       GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
	gebrm_config_load_servers(app, path);
	g_free(path);

}

void
gebrm_app_singleton_set_factory(GebrmAppFactory fac,
				gpointer data)
{
	__factory = fac;
	__data = data;
}

GebrmApp *
gebrm_app_singleton_get(void)
{
	if (__factory)
		return (*__factory)(__data);

	if (!__app)
		__app = gebrm_app_new();

	return __app;
}

static GebrmDaemon *
gebrm_add_server_to_list(GebrmApp *app,
			 const gchar *address,
			 const gchar *tags)
{
	GebrmDaemon *daemon;

	for (GList *i = app->priv->daemons; i; i = i->next) {
		daemon = i->data;
		if (g_strcmp0(address, gebrm_daemon_get_address(daemon)) == 0)
			return NULL;
	}

	daemon = gebrm_daemon_new(address);
	g_signal_connect(daemon, "state-change",
			 G_CALLBACK(gebrm_app_daemon_on_state_change), app);
	g_signal_connect(daemon, "task-define",
			 G_CALLBACK(gebrm_app_job_controller_on_task_def), app);

	gchar **tagsv = tags ? g_strsplit(tags, ",", -1) : NULL;
	if (tagsv) {
		for (int i = 0; tagsv[i]; i++)
			gebrm_daemon_add_tag(daemon, tagsv[i]);
	}

	app->priv->daemons = g_list_prepend(app->priv->daemons, daemon);
	gebrm_daemon_connect(daemon);

	return daemon;
}

static GList *
get_comm_servers_list(GebrmApp *app)
{
	GList *servers = NULL;
	GebrCommServer *server;
	for (GList *i = app->priv->daemons; i; i = i->next) {
		g_object_get(i->data, "server", &server, NULL);
		servers = g_list_prepend(servers, server);
	}

	return servers;
}

static gboolean
gebrm_remove_server_from_list(GebrmApp *app, const gchar *addr)
{
	for (GList *i = app->priv->daemons; i; i = i->next) {
		GebrmDaemon *daemon = i->data;
		if (g_strcmp0(gebrm_daemon_get_address(daemon), addr) == 0) {
			app->priv->daemons = g_list_delete_link(app->priv->daemons, i);
			gebrm_daemon_disconnect(daemon);
			g_object_unref(daemon);
			return TRUE;
		}
	}
	return FALSE;
}

typedef struct {
	GebrmApp *app;
	GebrmJob *job;
} AppAndJob;

static void
on_execution_response(GebrCommRunner *runner,
		      gpointer data)
{
	AppAndJob *aap = data;

	gchar *infile, *outfile, *logfile;
	gebrm_job_get_io(aap->job, &infile, &outfile, &logfile);

	gebrm_job_set_total_tasks(aap->job, gebr_comm_runner_get_total(runner));
	gebrm_job_set_servers_list(aap->job, gebr_comm_runner_get_servers_list(runner));
	gebrm_job_set_nprocs(aap->job, gebr_comm_runner_get_nprocs(runner));

	for (GList *i = aap->app->priv->connections; i; i = i->next) {
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
						      gebr_comm_protocol_defs.job_def, 13,
						      gebrm_job_get_id(aap->job),
						      gebrm_job_get_nprocs(aap->job),
						      gebrm_job_get_servers_list(aap->job),
						      gebrm_job_get_hostname(aap->job),
						      gebrm_job_get_title(aap->job),
						      gebrm_job_get_queue(aap->job),
						      gebrm_job_get_nice(aap->job),
						      infile,
						      outfile,
						      logfile,
						      gebrm_job_get_submit_date(aap->job),
						      gebrm_job_get_server_group(aap->job),
						      gebrm_job_get_exec_speed(aap->job));
	}

	gebr_validator_free(gebr_comm_runner_get_validator(runner));
	gebr_comm_runner_free(runner);
	g_free(aap);
}

static void
on_client_request(GebrCommProtocolSocket *socket,
		  GebrCommHttpMsg *request,
		  GebrmApp *app)
{
	g_debug("URL: %s", request->url->str);

	if (request->method == GEBR_COMM_HTTP_METHOD_PUT) {
		if (g_str_has_prefix(request->url->str, "/server/")) {
			gchar *addr = request->url->str + strlen("/server/");
			GebrmDaemon *d = gebrm_add_server_to_list(app, addr, NULL);
			gebrm_config_save_server(d);
		}
		else if (g_str_has_prefix(request->url->str, "/disconnect/")) {
			const gchar *addr = request->url->str + strlen("/server/");
			gebrm_remove_server_from_list(app, addr);
			gebrm_config_delete_server(addr);
			g_debug(">> on %s, disconecting %s", __func__, addr) 	;
		}
		else if (g_str_has_prefix(request->url->str, "/run")) {
			GebrCommJsonContent *json;
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar **params = g_strsplit(tmp, ";", -1);
			gchar *parent_id, *speed, *nice, *group, *host;

			g_debug("I will run this flow:");

			parent_id = strchr(params[0], '=') + 1;
			speed     = strchr(params[1], '=') + 1;
			nice      = strchr(params[2], '=') + 1;
			group     = strchr(params[3], '=') + 1;
			host      = strchr(params[4], '=') + 1;

			json = gebr_comm_json_content_new(request->content->str);
			GString *value = gebr_comm_json_content_to_gstring(json);
			write(STDOUT_FILENO, value->str, MIN(value->len, 100));
			puts("");

			GebrGeoXmlProject **pproj = g_new(GebrGeoXmlProject*, 1);
			GebrGeoXmlLine **pline = g_new(GebrGeoXmlLine*, 1);
			GebrGeoXmlFlow **pflow = g_new(GebrGeoXmlFlow*, 1);

			gebr_geoxml_document_load_buffer((GebrGeoXmlDocument **)pflow, value->str);
			*pproj = gebr_geoxml_project_new();
			*pline = gebr_geoxml_line_new();

			GebrValidator *validator = gebr_validator_new((GebrGeoXmlDocument **)pflow,
								      (GebrGeoXmlDocument **)pline,
								      (GebrGeoXmlDocument **)pproj);

			GList *servers = get_comm_servers_list(app);
			GebrCommRunner *runner = gebr_comm_runner_new(GEBR_GEOXML_DOCUMENT(*pflow),
								      servers,
								      parent_id, speed, nice, group,
								      validator);
			g_list_free(servers);

			gchar *title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(*pflow));

			GebrmJobInfo info = { 0, };
			info.title = title;
			info.hostname = host;
			info.parent_id = parent_id;
			info.servers = "";
			info.nice = nice;
			info.input = gebr_geoxml_flow_io_get_input(*pflow);
			info.output = gebr_geoxml_flow_io_get_output(*pflow);
			info.error = gebr_geoxml_flow_io_get_error(*pflow);
			info.group = group;
			info.speed = speed;

			g_free(title);

			GebrmJob *job = gebrm_job_new();

			g_signal_connect(job, "status-change",
					 G_CALLBACK(gebrm_app_job_controller_on_status_change), app);
			g_signal_connect(job, "issued",
					 G_CALLBACK(gebrm_app_job_controller_on_issued), app);
			g_signal_connect(job, "cmd-line-received",
					 G_CALLBACK(gebrm_app_job_controller_on_cmd_line_received), app);
			g_signal_connect(job, "output",
					 G_CALLBACK(gebrm_app_job_controller_on_output), app);

			gebrm_job_init_details(job, &info);
			gebrm_app_job_controller_add(app, job);

			AppAndJob *aap = g_new(AppAndJob, 1);
			aap->app = app;
			aap->job = job;
			gebr_comm_runner_set_ran_func(runner, on_execution_response, aap);
			gebr_comm_runner_run_async(runner, gebrm_job_get_id(job));
		}
	}
}

static void
gebrm_config_save_server(GebrmDaemon *daemon)
{
	GKeyFile *servers = g_key_file_new ();
	gchar *dir = g_build_filename(g_get_home_dir(),
				      GEBRM_LIST_OF_SERVERS_PATH, NULL);
	if (!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 755);

	gchar *path = g_build_filename(dir, GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
	gchar *tags = gebrm_daemon_get_tags(daemon);

	g_key_file_load_from_file(servers, path, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(servers, gebrm_daemon_get_address(daemon),
			      "tags", tags);

	gchar *content = g_key_file_to_data(servers, NULL, NULL);
	if (content)
		g_file_set_contents(path, content, -1, NULL);

	g_free(content);
	g_free(path);
	g_free(tags);
	g_key_file_free(servers);
}

static void
gebrm_config_delete_server(const gchar *serv)
{
	gchar *dir, *path, *subdir, *server;
	gchar *final_list_str;
	GKeyFile *servers;
	gboolean ret;

	server = g_strcmp0(serv, "127.0.0.1") ? g_strdup(serv): g_strdup("localhost");
	g_debug("Adding server %s to file", server);

	servers = g_key_file_new ();

	dir = g_build_path ("/", g_get_home_dir (), GEBRM_LIST_OF_SERVERS_PATH, NULL);
	if(!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 755);

	subdir = g_strconcat(GEBRM_LIST_OF_SERVERS_PATH, GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
	path = g_build_path ("/", g_get_home_dir (), subdir, NULL);

	g_key_file_load_from_file (servers, path, G_KEY_FILE_NONE, NULL);

	if(g_key_file_has_group(servers,server))
		g_debug("File doesn't have:%s", server);

	g_key_file_remove_group(servers, server, NULL);
	   
	if(g_key_file_has_group(servers,server))
		g_debug("Already has:%s", server);


	final_list_str= g_key_file_to_data (servers, NULL, NULL);
	ret = g_file_set_contents (path, final_list_str, -1, NULL);
	g_free (server);
	g_free (final_list_str);
	g_free (path);
	g_free (subdir);
	g_key_file_free (servers);
}

gboolean
gebrm_config_load_servers(GebrmApp *app, gchar *path)
{

	GKeyFile *servers = g_key_file_new();
	gchar **groups;
	gboolean succ = g_key_file_load_from_file(servers, path, G_KEY_FILE_NONE, NULL);
	if (succ) {
		groups = g_key_file_get_groups(servers, NULL);
		for (int i = 0; groups[i]; i++) {
			gchar *tags = g_key_file_get_string(servers, groups[i], "tags", NULL);
			gebrm_add_server_to_list(app, groups[i], tags);
		}
		g_key_file_free (servers);
	}
	return TRUE;
}

static void
on_client_disconnect(GebrCommProtocolSocket *socket,
		     GebrmApp *app)
{
	g_debug("Client disconnected!");
}

static void
on_new_connection(GebrCommListenSocket *listener,
		  GebrmApp *app)
{
	GebrCommStreamSocket *client;

	g_debug("New connection!");

	while ((client = gebr_comm_listen_socket_get_next_pending_connection(listener))) {
		GebrCommProtocolSocket *protocol =
			gebr_comm_protocol_socket_new_from_socket(client);

		app->priv->connections = g_list_prepend(app->priv->connections, protocol);

		for (GList *i = app->priv->daemons; i; i = i->next) {
			GebrCommServer *server;
			g_object_get(i->data, "server", &server, NULL);
			send_server_status_message(protocol, server);
		}

		g_signal_connect(protocol, "disconnected",
				 G_CALLBACK(on_client_disconnect), app);
		g_signal_connect(protocol, "process-request",
				 G_CALLBACK(on_client_request), app);
	}
}

GebrmApp *
gebrm_app_new(void)
{
	return g_object_new(GEBRM_TYPE_APP, NULL);
}

gboolean
gebrm_app_run(GebrmApp *app)
{
	GError *error = NULL;

	GebrCommSocketAddress address = gebr_comm_socket_address_ipv4_local(0);
	app->priv->listener = gebr_comm_listen_socket_new();

	g_signal_connect(app->priv->listener, "new-connection",
			 G_CALLBACK(on_new_connection), app);
	
	if (!gebr_comm_listen_socket_listen(app->priv->listener, &address)) {
		g_critical("Failed to start listener");
		return FALSE;
	}

	app->priv->address =
		gebr_comm_socket_get_address(GEBR_COMM_SOCKET(app->priv->listener));
	guint16 port = gebr_comm_socket_address_get_ip_port(&app->priv->address);
	gchar *portstr = g_strdup_printf("%d", port);

	g_file_set_contents(gebrm_main_get_lock_file(),
			    portstr, -1, &error);

	if (error) {
		g_critical("Could not create lock: %s", error->message);
		return FALSE;
	}

	/* success, send port */
	g_debug("Server started at %u port",
		gebr_comm_socket_address_get_ip_port(&app->priv->address));

	g_main_loop_run(app->priv->main_loop);

	return TRUE;
}

const gchar *
gebrm_main_get_lock_file(void)
{
	static gchar *lock = NULL;

	if (!lock) {
		gchar *fname = g_strconcat("lock-", g_get_host_name(), NULL);
		gchar *dirname = g_build_filename(g_get_home_dir(), ".gebr",
						  "gebrm", NULL);
		if(!g_file_test(dirname, G_FILE_TEST_EXISTS))
			g_mkdir_with_parents(dirname, 0755);
		lock = g_build_filename(g_get_home_dir(), ".gebr", "gebrm", fname, NULL);
		g_free(dirname);
		g_free(fname);
	}

	return lock;
}
