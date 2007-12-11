/*   G�BR - An environment for seismic processing.
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

#include <gtk/gtk.h>

#include <comm/gtcpserver.h>
#include <comm/gprocess.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "server.h"
#include "gebr.h"
#include "support.h"
#include "client.h"
#include "job.h"
#include "callbacks.h"

/*
 * Prototypes
 */

static void
server_connected(GTcpSocket * tcp_socket, struct server * server);

static void
server_disconnected(GTcpSocket * tcp_socket, struct server * server);

static void
server_read(GTcpSocket * tcp_socket, struct server * server);

static void
server_error(GTcpSocket * tcp_socket, enum GSocketError error, struct server * server);

/*
 * Internal functions
 */

static void
ssh_ask_server_port(struct server * server);

static void
ssh_run_server_finished(GProcess * process, struct server * server)
{
	/* now ask via ssh its port */
	ssh_ask_server_port(server);
	gebr_message(DEBUG, TRUE, TRUE, "ssh_run_server_finished");
// 	g_process_free(process);
}

static void
ssh_ask_port_read_stdout(GProcess * process, struct server * server)
{
	GString *	output;
	gchar *		strtol_endptr;
	guint16		port;

	output = g_process_read_stdout_string_all(process);
	port = strtol(output->str, &strtol_endptr, 10);
	if (errno != ERANGE)
		server->port = port;
	gebr_message(DEBUG, TRUE, TRUE, "ssh_ask_port_read_stdout: port read = %d, server port = %d", port, server->port);
	g_string_free(output, TRUE);
}

static void
ssh_ask_port_read_stderr(GProcess * process, struct server * server)
{
	GString *	error;

	if (g_process_is_running(process) == TRUE)
		g_process_kill(process);

	error = g_process_read_stderr_string_all(process);
	server->error = SERVER_ERROR_SSH_ASK_PORT;

	gebr_message(ERROR, TRUE, TRUE, _("Error contacting server at address %s via ssh: %s"),
		server->address->str, error->str);

	g_string_free(error, TRUE);
}

static void
ssh_ask_port_finished(GProcess * process, struct server * server)
{
	if (server->error != SERVER_ERROR_NONE)
		goto out;
	if (server->port) {
		GHostAddress *	host_address;

		/* connection is made to a localhost tunnel */
		host_address = g_host_address_new();
		g_host_address_set_ipv4_string(host_address, "127.0.0.1");

		server->state = SERVER_STATE_OPEN_TUNNEL;
		server->ssh_tunnel = ssh_tunnel_new(2125, server->address->str, server->port);

		server->state = SERVER_STATE_CONNECT;
		g_tcp_socket_connect(server->tcp_socket, host_address, server->ssh_tunnel->port);

		g_host_address_free(host_address);
	} else {
// 		GProcess *	server_process;
		GString *	cmd_line;

		if (server->state == SERVER_STATE_RUNNED_ASK_PORT)
			goto out;

		cmd_line = g_string_new(NULL);

		gebr_message(INFO, TRUE, TRUE, _("Launching server at %s"), server->address->str);

		/* run gebrd via ssh for remote hosts */
		if (g_ascii_strcasecmp(server->address->str, "127.0.0.1") == 0)
			g_string_printf(cmd_line, "bash -l -c gebrd&");
		else
			g_string_printf(cmd_line, "ssh -f -x %s 'gebrd'", server->address->str);

		system(cmd_line->str);
		server->state = SERVER_STATE_RUNNED_ASK_PORT;
		ssh_ask_server_port(server);

		g_string_free(cmd_line, TRUE);
	}

out:	g_process_free(process);
}

static void
ssh_ask_server_port(struct server * server)
{
	GProcess *	process;
	GString *	cmd_line;

	process = g_process_new();
	cmd_line = g_string_new(NULL);

	g_signal_connect(process, "ready-read-stdout",
			G_CALLBACK(ssh_ask_port_read_stdout), server);
	g_signal_connect(process, "ready-read-stderr",
			G_CALLBACK(ssh_ask_port_read_stderr), server);
	g_signal_connect(process, "finished",
			G_CALLBACK(ssh_ask_port_finished), server);

	/* ask via ssh port from server */
	g_string_printf(cmd_line, "ssh %s 'test -e ~/.gebr/run/gebrd.run && cat ~/.gebr/run/gebrd.run'",
		server->address->str);
	g_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
}

/*
 * Public functions
 */

struct server *
server_new(const gchar * _address)
{
	GtkTreeIter	iter;
	gchar *		address;

	struct server *	server;

	address = g_ascii_strcasecmp(_address, "127.0.0.1") == 0
		? _("Local server") : (gchar*)_address;

	/* initialize */
	server = g_malloc(sizeof(struct server));
	/* add to the list of servers */
	gtk_list_store_append(gebr.ui_server_list->store, &iter);
	gtk_list_store_set(gebr.ui_server_list->store, &iter,
			SERVER_STATUS_ICON, gebr.pixmaps.stock_disconnect,
			SERVER_ADDRESS, address,
			SERVER_POINTER, server,
			-1);
	/* fill struct */
	*server = (struct server) {
		.tcp_socket = g_tcp_socket_new(),
		.protocol = protocol_new(),
		.address = g_string_new(_address),
		.port = 0,
		.iter = iter
	};

	g_signal_connect(server->tcp_socket, "connected",
			G_CALLBACK(server_connected), server);
	g_signal_connect(server->tcp_socket, "disconnected",
			G_CALLBACK(server_disconnected), server);
	g_signal_connect(server->tcp_socket, "ready-read",
			G_CALLBACK(server_read), server);
	g_signal_connect(server->tcp_socket, "error",
			G_CALLBACK(server_error), server);

	server_connect(server);

	return server;
}

void
server_free(struct server * server)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* delete all jobs at server */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	while (valid) {
		struct job *	job;
		GtkTreeIter	this;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				JC_STRUCT, &job,
				-1);
		this = iter;
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);

		if (job->server == server)
			job_delete(job);
	}

	if (server->port)
		ssh_tunnel_free(server->ssh_tunnel);
	g_string_free(server->address, TRUE);
	g_socket_close(G_SOCKET(server->tcp_socket));
	protocol_free(server->protocol);
	g_free(server);
}

void
server_connect(struct server * server)
{
	/* initiate the marathon to communicate to server */
	server->error = SERVER_ERROR_NONE;
	server->state = SERVER_STATE_ASK_PORT;
	ssh_ask_server_port(server);
}

gboolean
server_is_logged(struct server * server)
{
	return server->protocol->logged;
}

void
server_connected(GTcpSocket * tcp_socket, struct server * server)
{
	gchar		hostname[100];
	gchar *		display;
	GString *	mcookie_cmd;
	FILE *		output_fp;
	gchar		line[1024];
	gchar **	splits;

	/* initialization */
	mcookie_cmd = g_string_new(NULL);

	/* hostname and display */
	gethostname(hostname, 100);
	display = getenv("DISPLAY");

	/* TODO: port to GProcess using blocking calls */
	/* get this X session magic cookie */
	g_string_printf(mcookie_cmd, "xauth list %s", display);
	output_fp = popen(mcookie_cmd->str, "r");
	fread(line, 1, 1024, output_fp);
	/* split output and get only the magic cookie */
	splits = g_strsplit_set(line, " \n", 6);
	pclose(output_fp);

	protocol_send_data(server->protocol, server->tcp_socket,
		protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, display, splits[4]);

	/* frees */
	g_string_free(mcookie_cmd, TRUE);
	g_strfreev(splits);
}

static void
server_disconnected(GTcpSocket * tcp_socket, struct server * server)
{
	server->port = 0;
	server->protocol->logged = FALSE;
	gtk_list_store_set(gebr.ui_server_list->store, &server->iter,
			SERVER_STATUS_ICON, gebr.pixmaps.stock_disconnect,
			-1);

	gebr_message(WARNING, TRUE, TRUE, "Server '%s' disconnected",
		     server->address->str);
}

static void
server_read(GTcpSocket * tcp_socket, struct server * server)
{
	GString *	data;

	data = g_socket_read_string_all(G_SOCKET(tcp_socket));
	gebr_message(DEBUG, FALSE, TRUE, "read from server '%s'",
		     server->address->str);
	g_print("server_read:\n%s\n", data->str);

	protocol_receive_data(server->protocol, data);
	client_parse_server_messages(server);

	g_string_free(data, TRUE);
}

static void
server_error(GTcpSocket * tcp_socket, enum GSocketError error, struct server * server)
{
	/* FIXME: fix signal on GTcpSocket */
//	gebr_message(ERROR, FALSE, TRUE, _("Connection error '%s' on server '%s'"), error, server->address->str);
}

void
server_run_flow(struct server * server)
{
	GeoXmlFlow *		flow_wnh; /* wnh: with no help */
	GeoXmlProgram *		program;
	GeoXmlSequence *	sequence;
	gchar *			xml;

	/* removes flow's help */
	flow_wnh = GEOXML_FLOW(geoxml_document_clone(GEOXML_DOC(gebr.flow)));
	geoxml_document_set_help(GEOXML_DOC(flow_wnh), "");
	/* removes programs' help */
	geoxml_flow_get_program(flow_wnh, &program, 0);
	sequence = GEOXML_SEQUENCE(program);
	while (sequence != NULL) {
		program = GEOXML_PROGRAM(sequence);
		geoxml_program_set_help(program, "");
		geoxml_sequence_next(&sequence);
	}

	/* get the xml */
	geoxml_document_to_string(GEOXML_DOC(flow_wnh), &xml);

	/* finally... */
	protocol_send_data(server->protocol, server->tcp_socket, protocol_defs.run_def, 1, xml);

	g_free(xml);
	geoxml_document_free(GEOXML_DOC(flow_wnh));
}
