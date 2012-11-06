/*
 * gebr-comm-server.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2011 - GêBR Core Team (www.gebrproject.com)
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

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <config.h>

#include <libgebr/marshalers.h>
#include <libgebr/gebr-version.h>
#include "../libgebr-gettext.h"
#include <glib/gi18n-lib.h>
#include <libgebr/utils.h>

#include "gebr-comm-server.h"
#include "gebr-comm-listensocket.h"
#include "gebr-comm-protocol.h"
#include "gebr-comm-uri.h"
#include "gebr-comm-port-provider.h"

/*
 * Declarations
 */

typedef enum {
	ISTATE_NONE,
	ISTATE_PASS,
	ISTATE_QUESTION,
} InteractiveState;

struct _GebrCommServerPriv {
	gboolean is_maestro;

	gchar *gebr_id;

	/* Interactive state variables */
	gboolean is_interactive;
	InteractiveState istate;
	gchar *title;
	gchar *description;

	GHashTable *qa_cache;
};

G_DEFINE_TYPE(GebrCommServer, gebr_comm_server, G_TYPE_OBJECT);

enum {
	PASSWORD_REQUEST,
	QUESTION_REQUEST,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void	gebr_comm_server_log_message	(GebrCommServer *server,
						 GebrLogMessageType type,
						 const gchar * message, ...);

static gboolean	gebr_comm_ssh_parse_output	(GebrCommTerminalProcess *process,
						 GebrCommServer *server,
						 GString *output);

static void	gebr_comm_ssh_read		(GebrCommTerminalProcess *process,
						 GebrCommServer *server);

static void	gebr_comm_ssh_finished		(GebrCommTerminalProcess *process,
						 GebrCommServer *server);

static void	gebr_comm_server_disconnected_state(GebrCommServer *server,
						    enum gebr_comm_server_error error,
						    const gchar * message, ...);

static void	gebr_comm_server_change_state	(GebrCommServer *server,
						 GebrCommServerState state);

static void	gebr_comm_server_socket_connected(GebrCommProtocolSocket * socket,
						  GebrCommServer *server);

static void	gebr_comm_server_socket_disconnected(GebrCommProtocolSocket * socket,
						     GebrCommServer *server);

static void	gebr_comm_server_socket_process_request(GebrCommProtocolSocket * socket,
							GebrCommHttpMsg *request,
							GebrCommServer *server);

static void	gebr_comm_server_socket_process_response(GebrCommProtocolSocket *socket,
							 GebrCommHttpMsg *request,
							 GebrCommHttpMsg *response,
							 GebrCommServer *server);

static void	gebr_comm_server_socket_old_parse_messages(GebrCommProtocolSocket *socket,
							   GebrCommServer *server);

static void	gebr_comm_server_free_x11_forward(GebrCommServer *server);

static void	gebr_comm_server_free_for_reuse	(GebrCommServer *server);


static void
gebr_comm_server_init(GebrCommServer *server)
{
	server->priv = G_TYPE_INSTANCE_GET_PRIVATE(server,
						   GEBR_COMM_TYPE_SERVER,
						   GebrCommServerPriv);

	server->priv->istate = ISTATE_NONE;
	server->priv->is_interactive = FALSE;
}

static void
gebr_comm_server_class_init(GebrCommServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	signals[PASSWORD_REQUEST] =
		g_signal_new("password-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommServerClass, password_request),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__STRING_STRING,
			     G_TYPE_NONE, 2,
			     G_TYPE_STRING, G_TYPE_STRING);

	signals[QUESTION_REQUEST] =
		g_signal_new("question-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommServerClass, question_request),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__STRING_STRING,
			     G_TYPE_NONE, 2,
			     G_TYPE_STRING, G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(GebrCommServerPriv));
}

static gchar *
gebr_get_dafault_keys(void)
{
	const gchar *default_keys[] = {"id_rsa", "id_dsa", "identity", NULL};
	GString *keys = g_string_new(NULL);

	for (gint i = 0; default_keys[i]; i++) {
		gchar *default_key = g_build_filename(g_get_home_dir(), ".ssh", default_keys[i], NULL);

		if (g_file_test(default_key, G_FILE_TEST_EXISTS)) {
			gchar *cmd = g_strdup_printf(" -i %s", default_key);
			keys = g_string_append(keys, cmd);
			g_free(cmd);
		}

		g_free(default_key);
	}

	return g_string_free(keys, FALSE);
}

static gchar *
get_ssh_command_with_key(void)
{
	const gchar *default_keys = gebr_get_dafault_keys();
	gchar *basic_cmd;
	if (default_keys)
		basic_cmd = g_strdup_printf("ssh -o NoHostAuthenticationForLocalhost=yes %s", default_keys);
	else
		basic_cmd = g_strdup("ssh -o NoHostAuthenticationForLocalhost=yes");

	gchar *path = gebr_key_filename(FALSE);
	gchar *ssh_cmd;

	if (g_file_test(path, G_FILE_TEST_EXISTS))
		ssh_cmd = g_strconcat(basic_cmd, " -i ", path, NULL);
	else
		ssh_cmd = g_strdup(basic_cmd);

	g_free(path);
	g_free(basic_cmd);

	return ssh_cmd;
}

gchar *
gebr_comm_server_get_user(const gchar *address)
{
	gchar *addr_temp;

	addr_temp = g_strdup(address);

	return (gchar *) strsep(&addr_temp, "@");
}

GebrCommServerType gebr_comm_server_get_id(const gchar * name)
{
	if (strcmp(name, "regular") == 0)
		return GEBR_COMM_SERVER_TYPE_REGULAR;
	else if (strcmp(name, "moab") == 0)
		return GEBR_COMM_SERVER_TYPE_MOAB;
	else
		return GEBR_COMM_SERVER_TYPE_UNKNOWN;
}

const gchar *
gebr_comm_server_get_last_error(GebrCommServer *server)
{
	return server->last_error->str;
}

GebrCommServer *
gebr_comm_server_new(const gchar * _address,
		     const gchar *gebr_id,
		     const struct gebr_comm_server_ops *ops)
{
	GebrCommServer *server;
	server = g_object_new(GEBR_COMM_TYPE_SERVER, NULL);

	if (g_strcmp0(_address, "127.0.0.1") == 0
	    || g_strcmp0(_address, "localhost") == 0)
		server->address = g_string_new(g_get_host_name());
	else
		server->address = g_string_new(_address);

	server->priv->gebr_id = g_strdup(gebr_id);
	server->socket = gebr_comm_protocol_socket_new();
	server->port = 0;
	server->use_public_key = FALSE;
	server->password = NULL;
	server->x11_forward_process = NULL;
	server->x11_forward_unix = NULL;
	server->ops = ops;
	server->user_data = NULL;
	server->tunnel_pooling_source = 0;
	server->last_error = g_string_new(NULL);
	server->state = SERVER_STATE_UNKNOWN;
	server->error = SERVER_ERROR_UNKNOWN;

	server->priv->qa_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	gebr_comm_server_free_for_reuse(server);
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_NONE, "");

	g_signal_connect(server->socket, "connected",
			 G_CALLBACK(gebr_comm_server_socket_connected), server);
	g_signal_connect(server->socket, "disconnected",
			 G_CALLBACK(gebr_comm_server_socket_disconnected), server);
	g_signal_connect(server->socket, "process-request",
			 G_CALLBACK(gebr_comm_server_socket_process_request), server);
	g_signal_connect(server->socket, "process-response",
			 G_CALLBACK(gebr_comm_server_socket_process_response), server);
	g_signal_connect(server->socket, "old-parse-messages",
			 G_CALLBACK(gebr_comm_server_socket_old_parse_messages), server);

	return server;
}

void
gebr_comm_server_free(GebrCommServer *server)
{
	g_string_free(server->last_error, TRUE);
	gebr_comm_server_free_for_reuse(server);
	g_string_free(server->address, TRUE);
	g_free(server->password);
	g_free(server->memory);
	g_free(server->model_name);
	g_object_unref(server->socket);
	g_object_unref(server);
}

static void
on_comm_port_defined(GebrCommPortProvider *self,
		     guint port,
		     GebrCommServer *server)
{
	GebrCommSocketAddress socket_address;
	socket_address = gebr_comm_socket_address_ipv4_local(port);
	gebr_comm_protocol_socket_connect(server->socket, &socket_address, FALSE);
}

static void
on_comm_port_error(GebrCommPortProvider *self, GError *error)
{
	g_critical("Error when launching gebrm/gebrd: %s", error->message);
}

static void
on_comm_port_password(GebrCommPortProvider *self,
		      GebrCommSsh *ssh,
		      gboolean retry,
		      GebrCommServer *server)
{
	GString *string;
	GString *password;

	// FIXME: This is a workaround to a problem faced when using this
	// method for other SSH connections other than the server connection
	// itself. See gebr_comm_server_forward_remote_port() for an example.
	//
	// If the server is already connected, the cached password is correct
	// so we can just write it into the process.
	gboolean is_connected = (server->state == SERVER_STATE_CONNECT
				 || server->state == SERVER_STATE_LOGGED);

	gboolean has_password = server->password && *server->password;

	if (is_connected || (has_password && !server->tried_existant_pass)) {
		gebr_comm_ssh_set_password(ssh, server->password);
		server->tried_existant_pass = TRUE;
		return;
	}

	string = g_string_new(NULL);
	if (!retry)
		g_string_printf(string, _("Machine '%s' needs SSH login."),
				server->address->str);
	else
		g_string_printf(string, _("Wrong password for machine '%s', please try again."),
				server->address->str);

	if (server->priv->is_interactive) {
		server->priv->istate = ISTATE_PASS;

		if (server->priv->title)
			g_free(server->priv->title);

		if (server->priv->description)
			g_free(server->priv->description);

		server->priv->title = g_strdup(_("Please, enter password"));
		server->priv->description = g_strdup(string->str);

		g_signal_emit(server, signals[PASSWORD_REQUEST], 0,
			      server->priv->title,
			      server->priv->description);
	} else {
		password = server->ops->ssh_login(server, _("SSH login:"), string->str,
						  server->user_data);
		if (password == NULL) {
			g_free(server->password);
			server->password = NULL;

			gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, _("No password provided."));
			gebr_comm_ssh_kill(ssh);
		} else {
			gebr_comm_server_set_password(server, password->str);
			gebr_comm_ssh_set_password(ssh, password->str);
			g_string_free(password, TRUE);
		}
		server->tried_existant_pass = FALSE;
	}

	g_string_free(string, TRUE);
}

static void
on_comm_port_question(GebrCommPortProvider *self,
		      GebrCommSsh *ssh,
		      const gchar *question,
		      GebrCommServer *server)
{
	if (server->priv->is_interactive) {
		if (server->priv->title)
			g_free(server->priv->title);

		if (server->priv->description)
			g_free(server->priv->description);

		server->priv->title = g_strdup(_("Please, answer the question"));
		server->priv->description = g_strdup(question);

		server->priv->istate = ISTATE_QUESTION;
		g_signal_emit(server, signals[QUESTION_REQUEST], 0,
			      server->priv->title,
			      server->priv->description);
	} else {
		gchar *cached_answer = g_hash_table_lookup(server->priv->qa_cache, question);
		gboolean answer;

		if (!cached_answer) {
			answer = server->ops->ssh_question(server,
							   _("SSH host key question:"),
							   question, server->user_data);

			g_hash_table_insert(server->priv->qa_cache,
					    g_strdup(question),
					    g_strdup(answer?"yes":"no"));
		} else {
			answer = g_strcmp0(cached_answer, "yes") == 0;
		}

		gebr_comm_ssh_answer_question(ssh, answer);

		if (!answer)
			gebr_comm_server_disconnected_state(server,
							    SERVER_ERROR_SSH,
							    _("SSH host key rejected."));
	}
}

const gchar *
get_real_addr(const gchar *addr)
{
	if (g_strcmp0(g_get_host_name(), addr) == 0)
		return "127.0.0.1";
	else
		return addr;
}

void gebr_comm_server_connect(GebrCommServer *server,
			      gboolean maestro)
{
	GebrCommPortType port_type;

	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("%p: Launching machine at '%s'."),
				     server->socket, server->address->str);

	gebr_comm_server_free_for_reuse(server);
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_NONE, "");
	gebr_comm_server_change_state(server, SERVER_STATE_RUN);

	server->tried_existant_pass = FALSE;
	server->priv->is_maestro = maestro;

	if (maestro)
		port_type = GEBR_COMM_PORT_TYPE_MAESTRO;
	else
		port_type = GEBR_COMM_PORT_TYPE_DAEMON;

	const gchar *addr = get_real_addr(server->address->str);
	GebrCommPortProvider *port_provider =
		gebr_comm_port_provider_new(port_type, addr);
	g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_comm_port_defined), server);
	g_signal_connect(port_provider, "error", G_CALLBACK(on_comm_port_error), server);
	g_signal_connect(port_provider, "password", G_CALLBACK(on_comm_port_password), server);
	g_signal_connect(port_provider, "question", G_CALLBACK(on_comm_port_question), server);
	gebr_comm_port_provider_start(port_provider);
}

void gebr_comm_server_disconnect(GebrCommServer *server)
{
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_NONE, "");
	gebr_comm_server_free_for_reuse(server);
	gebr_comm_protocol_socket_disconnect(server->socket);
}

gboolean
gebr_comm_server_is_logged(GebrCommServer *server)
{
	return server->socket->protocol->logged;
}

void
gebr_comm_server_set_logged(GebrCommServer *server)
{
	server->socket->protocol->logged = TRUE;
	gebr_comm_server_change_state(server, SERVER_STATE_LOGGED);
}

gboolean gebr_comm_server_is_local(GebrCommServer *server)
{
	return strcmp(server->address->str, "127.0.0.1") == 0 ? TRUE : strcmp(server->address->str, "localhost") == 0 ? TRUE : FALSE;
}

void gebr_comm_server_kill(GebrCommServer *server)
{
	GebrCommTerminalProcess *process;

	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Stopping machine at '%s'."),
				     server->address->str);

	server->tried_existant_pass = FALSE;
	process = gebr_comm_terminal_process_new();
	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), server);
	g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_finished), server);

	const gchar *bin;
	if (gebr_comm_server_is_maestro(server))
		bin = "gebrm";
	else
		bin = "gebrd";

	gchar *kill = g_strdup_printf("fuser -sk -15 $(cat $HOME/.gebr/%s/$HOSTNAME/lock)/tcp", bin);
	GString *cmd_line = g_string_new(NULL);

	if (gebr_comm_server_is_local(server))
		g_string_printf(cmd_line, "bash -c '%s'", kill);
	else {
		gchar *ssh_cmd = get_ssh_command_with_key();
		g_string_printf(cmd_line, "%s -x %s '%s'", ssh_cmd, server->address->str, kill);
		g_free(ssh_cmd);
	}

	gebr_comm_terminal_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
	g_free(kill);
}

static gchar *
get_x11_unix_file(void)
{
	const gchar *display = g_getenv("DISPLAY");

	if (!display)
		return NULL;

	guint16 display_number;
	if (sscanf(strchr(display, ':'), ":%hu.", &display_number) != 1)
		return NULL;

	return g_strdup_printf("/tmp/.X11-unix/X%hu", display_number);
}

static void
on_x11_port_defined(GebrCommPortProvider *self,
		    guint port,
		    GebrCommServer *server)
{
	gchar *x11_file = get_x11_unix_file();
	server->x11_forward_unix = gebr_comm_process_new();
	GString *cmdline = g_string_new(NULL);
	g_string_printf(cmdline, "gebr-comm-socketchannel %d %s", port, x11_file);
	gebr_comm_process_start(server->x11_forward_unix, cmdline);
	g_string_free(cmdline, TRUE);
}

static void
on_x11_port_error(GebrCommPortProvider *self,
		  GError *error,
		  GebrCommServer *server)
{
	g_critical("Error when forwarding x11: %s", error->message);
}

void
gebr_comm_server_forward_x11(GebrCommServer *server, guint16 remote_display)
{
	GebrCommPortProvider *port_provider =
		gebr_comm_port_provider_new(GEBR_COMM_PORT_TYPE_X11, server->address->str);
	gebr_comm_port_provider_set_display(port_provider, remote_display);
	g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_x11_port_defined), server);
	g_signal_connect(port_provider, "error", G_CALLBACK(on_x11_port_error), server);
	g_signal_connect(port_provider, "password", G_CALLBACK(on_comm_port_password), server);
	g_signal_connect(port_provider, "question", G_CALLBACK(on_comm_port_question), server);
	gebr_comm_port_provider_start(port_provider);
}

void
gebr_comm_server_close_x11_forward(GebrCommServer *server)
{
	if (server->x11_forward_process) {
		gebr_comm_terminal_process_kill(server->x11_forward_process);
		gebr_comm_terminal_process_free(server->x11_forward_process);
		server->x11_forward_process = NULL;
	}
}

/**
 * \internal
 */
static void
gebr_comm_server_log_message(GebrCommServer *server, GebrLogMessageType type,
			     const gchar * message, ...)
{
	va_list argp;
	va_start(argp, message);
	gchar *string = g_strdup_vprintf(message, argp);
	gchar *msg = g_strdup_printf("%s: %s", server->address->str, string);
	server->ops->log_message(server, type, msg, server->user_data);
	g_free(string);
	g_free(msg);
	va_end(argp);
}

static void
write_pass_in_process(GebrCommTerminalProcess *process,
		      const gchar *pass)
{
	gsize len = strlen(pass);
	GString p = {(gchar*)pass, len, len};
	gebr_comm_terminal_process_write_string(process, &p);
	p.str = "\n";
	p.len = p.allocated_len = 1;
	gebr_comm_terminal_process_write_string(process, &p);
}

/*
 * Return TRUE if callee should not proceed.
 */
static gboolean
gebr_comm_ssh_parse_output(GebrCommTerminalProcess *process,
			   GebrCommServer *server,
			   GString * output)
{
	if (output->len <= 2)
		return TRUE;

	gchar *start = g_strrstr(output->str, GEBR_PORT_PREFIX);
	if (start) {
		g_string_erase(output, 0, start - output->str + strlen(GEBR_PORT_PREFIX));
		return FALSE;
	}

	for (gint i = 0; output->str[i]; i++) {
		if (!g_ascii_isdigit(output->str[i]))
			break;
		return FALSE;
	}

	if (output->str[output->len - 2] == ':') {
		GString *string;
		GString *password;

		// FIXME: This is a workaround to a problem faced when using this
		// method for other SSH connections other than the server connection
		// itself. See gebr_comm_server_forward_remote_port() for an example.
		//
		// If the server is already connected, the cached password is correct
		// so we can just write it into the process.
		gboolean is_connected = (server->state == SERVER_STATE_CONNECT
					 || server->state == SERVER_STATE_LOGGED);

		gboolean has_password = server->password && *server->password;

		if (is_connected || (has_password && !server->tried_existant_pass)) {
			write_pass_in_process(process, server->password);
			server->tried_existant_pass = TRUE;
			goto out;
		}

		string = g_string_new(NULL);
		if (server->tried_existant_pass == FALSE)
			g_string_printf(string, _("Machine '%s' needs SSH login.\n\n%s"),
					server->address->str, output->str);
		else
			g_string_printf(string, _("Wrong password for machine '%s', please try again.\n\n%s"),
					server->address->str, output->str);

		if (server->priv->is_interactive) {
			server->priv->istate = ISTATE_PASS;

			if (server->priv->title)
				g_free(server->priv->title);

			if (server->priv->description)
				g_free(server->priv->description);

			server->priv->title = g_strdup(_("Please, enter password"));
			server->priv->description = g_strdup(string->str);

			g_signal_emit(server, signals[PASSWORD_REQUEST], 0,
				      server->priv->title,
				      server->priv->description);
		} else {
			password = server->ops->ssh_login(server, _("SSH login:"), string->str,
							  server->user_data);
			if (password == NULL) {
				g_free(server->password);
				server->password = NULL;

				gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, _("No password provided."));
				gebr_comm_terminal_process_kill(process);
			} else {
				gebr_comm_server_set_password(server, password->str);
				write_pass_in_process(process, password->str);
				g_string_free(password, TRUE);
			}
			server->tried_existant_pass = FALSE;
		}

		g_string_free(string, TRUE);
	} else if (output->str[output->len - 2] == '?') {
		if (server->priv->is_interactive) {
			if (server->priv->title)
				g_free(server->priv->title);

			if (server->priv->description)
				g_free(server->priv->description);

			server->priv->title = g_strdup(_("Please, answer the question"));
			server->priv->description = g_strdup(output->str);

			server->priv->istate = ISTATE_QUESTION;
			g_signal_emit(server, signals[QUESTION_REQUEST], 0,
				      server->priv->title,
				      server->priv->description);
		} else {
			GString *answer = g_string_new(NULL);
			gchar *cached_answer = g_hash_table_lookup(server->priv->qa_cache, output->str);

			if (!cached_answer) {
				if (server->ops->ssh_question(server, _("SSH host key question:"),
				                              output->str, server->user_data) == FALSE) {
					g_string_assign(answer, "no\n");
					gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, _("SSH host key rejected."));
				} else
					g_string_assign(answer, "yes\n");

				g_hash_table_insert(server->priv->qa_cache,
				                    g_strdup(output->str),
				                    g_strdup(answer->str));
			} else
				g_string_assign(answer, cached_answer);

			gebr_comm_terminal_process_write_string(process, answer);
			g_string_free(answer, TRUE);
		}
	} else if (g_str_has_prefix(output->str, "@@@")) {
		gebr_g_string_replace(output, "\r\n", "\n");
		g_string_erase(output, strlen(output->str)-1, 1); //last \n

		gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, _("SSH error: %s"), output->str);
		gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Received SSH error for machine '%s': %s"),
					     server->address->str, output->str);
	} else if (output->str[output->len - 4] == '.') {
		gebr_g_string_replace(output, "\r\n", "\n");
		g_string_erase(output, strlen(output->str)-1, 1); //last \n

		gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Received SSH message of machine '%s': %s"),
					     server->address->str, output->str);
	} else if (!strcmp(output->str, "yes\r\n")) {
		goto out;
	} else {
		/* check for known error prefixes */
		if (g_str_has_prefix(output->str, "ssh:") || g_str_has_prefix(output->str, "channel ")) {
			gebr_g_string_replace(output, "\r\n", "\n");
			g_string_erase(output, strlen(output->str)-1, 1); //last \n

			gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, 
							    _("SSH reported the following error: \n%s"), output->str);
			gebr_comm_server_log_message(server, GEBR_LOG_ERROR, _("SSH reported the following error for machine '%s': %s"),
						     server->address->str, output->str);
			return TRUE;
		}
	}

out:	return TRUE;
}

/**
 * \internal
 * Simple ssh messages parser, like login questions and warnings
 */
static void gebr_comm_ssh_read(GebrCommTerminalProcess * process, GebrCommServer *server)
{
	GString *output;

	output = gebr_comm_terminal_process_read_string_all(process);
	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_read: %s", output->str);

	if (gebr_comm_ssh_parse_output(process, server, output) == TRUE)
		goto out;

out:	g_string_free(output, TRUE);
}

/**
 * \internal
 */

static void gebr_comm_ssh_finished(GebrCommTerminalProcess * process, GebrCommServer *server)
{
	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_finished");
	gebr_comm_terminal_process_free(process);
}

/**
 * \internal
 */
static void gebr_comm_server_disconnected_state(GebrCommServer *server,
						enum gebr_comm_server_error error,
						const gchar * message, ...)
{
	if (error != SERVER_ERROR_UNKNOWN) {
		server->error = error;
		va_list argp;
		va_start(argp, message);
		gchar *string = g_strdup_vprintf(message, argp);
		g_string_assign(server->last_error, string);
		g_free(string);
		va_end(argp);
	}

	/* take care not to free the process here cause this function
	 * maybe be used by Process's read callback */
	server->port = 0;
	server->socket->protocol->logged = FALSE;
	gebr_comm_server_change_state(server, SERVER_STATE_DISCONNECTED);
}

static void gebr_comm_server_change_state(GebrCommServer *server, GebrCommServerState state)
{
	g_debug("State of machine %s changed from %s to %s",
		server->address->str,
		gebr_comm_server_state_to_string(server->state),
		gebr_comm_server_state_to_string(state));

	if (server->state != SERVER_STATE_UNKNOWN) {
		if (state == SERVER_STATE_DISCONNECTED)
			g_queue_clear(server->socket->protocol->waiting_ret_hashs);
		server->state = state;
		server->ops->state_changed(server, server->user_data);
	} else
		server->state = state;
}

/*
 * get this X session magic cookie
 */
gchar *
get_xauth_cookie(const gchar *display_number)
{
	if (!display_number)
		return g_strdup("");

	gchar *mcookie_str = g_new(gchar, 33);
	GString *cmd_line = g_string_new(NULL);

	g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display_number);

	g_debug("GET XATUH COOKIE WITH COMMAND: %s", cmd_line->str);

	/* WORKAROUND: if xauth is already executing it will lock
	 * the auth file and it will fail to retrieve the m-cookie.
	 * So, as a workaround, we try to get the m-cookie many times.
	 */
	gint i;
	for (i = 0; i < 5; i++) {
		FILE *output_fp = popen(cmd_line->str, "r");
		if (fscanf(output_fp, "%32s", mcookie_str) != 1)
			usleep(100*1000);
		else {
			pclose(output_fp);
			break;
		}
		pclose(output_fp);
	}

	if (i == 5)
		strcpy(mcookie_str, "");

	g_debug("===== COOKIE ARE %s", mcookie_str);

	g_string_free(cmd_line, TRUE);

	return mcookie_str;
}

static void
gebr_comm_server_socket_connected(GebrCommProtocolSocket * socket,
				  GebrCommServer *server)
{
	const gchar *display;
	gchar *display_number = NULL;
	const gchar *hostname = g_get_host_name();


	display = getenv("DISPLAY");
	if (!display)
		display = "";
	else
		display_number = strchr(display, ':');

	gebr_comm_server_change_state(server, SERVER_STATE_CONNECT);

	if (server->priv->is_maestro) {
		gchar *mcookie_str = get_xauth_cookie(display_number);
		GTimeVal gebr_time;
		g_get_current_time(&gebr_time);
		gchar *gebr_time_iso = g_time_val_to_iso8601(&gebr_time);
		gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
						      gebr_comm_protocol_defs.ini_def, 4,
						      gebr_version(),
						      mcookie_str,
						      server->priv->gebr_id,
						      gebr_time_iso);
		g_free(mcookie_str);
		g_free(gebr_time_iso);
	} else {
		gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
						      gebr_comm_protocol_defs.ini_def, 2,
						      gebr_comm_protocol_get_version(),
						      hostname);
	}

}

static void
gebr_comm_server_socket_disconnected(GebrCommProtocolSocket *socket,
				     GebrCommServer *server)
{
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_UNKNOWN, "");
	gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Machine '%s' disconnected"), 
				     server->address->str);
}

static void
gebr_comm_server_socket_process_request(GebrCommProtocolSocket *socket,
					GebrCommHttpMsg *request,
					GebrCommServer *server)
{
	server->ops->process_request(server, request, server->user_data);
}

static void
gebr_comm_server_socket_process_response(GebrCommProtocolSocket *socket,
					 GebrCommHttpMsg *request,
					 GebrCommHttpMsg *response,
					 GebrCommServer *server)
{
	server->ops->process_response(server, request, response, server->user_data);
}

static void
gebr_comm_server_socket_old_parse_messages(GebrCommProtocolSocket *socket,
					   GebrCommServer *server)
{
	server->ops->parse_messages(server, server->user_data);
}

/**
 * \internal
 * Free (if necessary) server->x11_forward_process for reuse
 */
static void gebr_comm_server_free_x11_forward(GebrCommServer *server)
{
	if (server->tunnel_pooling_source) {
		g_source_remove(server->tunnel_pooling_source);
		server->tunnel_pooling_source = 0;
	}

	if (server->x11_forward_process != NULL) {
		gebr_comm_terminal_process_kill(server->x11_forward_process);
		gebr_comm_terminal_process_free(server->x11_forward_process);
		server->x11_forward_process = NULL;
	}
	if (server->x11_forward_unix != NULL) {
		gebr_comm_process_free(server->x11_forward_unix);
		server->x11_forward_unix = NULL;
	}
}

/**
 * \internal
 */
static void gebr_comm_server_free_for_reuse(GebrCommServer *server)
{
	if (server->priv->qa_cache)
		g_hash_table_remove_all(server->priv->qa_cache);

	gebr_comm_protocol_reset(server->socket->protocol);
	gebr_comm_server_free_x11_forward(server);

	if (server->process) {
		gebr_comm_terminal_process_free(server->process);
		server->process = NULL;
	}
}

static const gchar *state_hash[] = {
	"unknown",
	"disconnected",
	"run",
	"open_tunnel",
	"connect",
	"logged",
	NULL
};

const gchar *
gebr_comm_server_state_to_string(GebrCommServerState state)
{
	return state_hash[state];
}

GebrCommServerState
gebr_comm_server_state_from_string(const gchar *string)
{
	for (int i = 0; state_hash[i]; i++)
		if (g_strcmp0(state_hash[i], string) == 0)
			return (GebrCommServerState) i;

	return SERVER_STATE_UNKNOWN;
}

GebrCommServerState
gebr_comm_server_get_state(GebrCommServer *server)
{
	return server->state;
}

void
gebr_comm_server_set_password(GebrCommServer *server, const gchar *pass)
{
	server->password = g_strdup(pass);

	if (server->priv->is_interactive
	    && server->priv->istate == ISTATE_PASS)
		write_pass_in_process(server->process, pass);
}

void
gebr_comm_server_answer_question(GebrCommServer *server,
				 gboolean response)
{
	GString *answer = g_string_new(response ? "yes\n" : "no\n");

	if (server->priv->is_interactive
	    && server->priv->istate == ISTATE_QUESTION)
		gebr_comm_terminal_process_write_string(server->process, answer);
}

void
gebr_comm_server_set_interactive(GebrCommServer *server, gboolean setting)
{
	server->priv->is_interactive = setting;
}

void
gebr_comm_server_emit_interactive_state_signals(GebrCommServer *server)
{
	g_return_if_fail(server->priv->is_interactive);

	switch (server->priv->istate)
	{
	case ISTATE_NONE:
		break;
	case ISTATE_PASS:
		g_signal_emit(server, signals[PASSWORD_REQUEST], 0,
			      server->priv->title,
			      server->priv->description);
		break;
	case ISTATE_QUESTION:
		g_signal_emit(server, signals[QUESTION_REQUEST], 0,
			      server->priv->title,
			      server->priv->description);
		break;
	}
}

static GebrCommTerminalProcess *
gebr_comm_server_forward_port(GebrCommServer *server,
			      guint16 port1,
			      guint16 port2,
			      const gchar *addr,
			      gboolean is_local)
{
	g_return_val_if_fail(server->state == SERVER_STATE_CONNECT
			     || server->state == SERVER_STATE_LOGGED, NULL);

	GebrCommTerminalProcess *proc = gebr_comm_terminal_process_new();

	g_signal_connect(proc, "ready-read",
			 G_CALLBACK(gebr_comm_ssh_read), server);

	gchar *ssh_cmd = get_ssh_command_with_key();
	GString *string = g_string_new(NULL);
	g_string_printf(string, "%s -x -%s %d:%s:%d %s -N",
			ssh_cmd,
	                is_local ? "L" : "R",
			port1,
			addr,
			port2,
			server->address->str);

	g_debug("Simple forward: %s", string->str);

	gebr_comm_terminal_process_start(proc, string);
	g_string_free(string, TRUE);
	g_free(ssh_cmd);

	return proc;
}

GebrCommTerminalProcess *
gebr_comm_server_forward_remote_port(GebrCommServer *server,
				     guint16 remote_port,
				     guint16 local_port)
{
	return gebr_comm_server_forward_port(server, remote_port, local_port, "127.0.0.1", FALSE);
}

GebrCommTerminalProcess *
gebr_comm_server_forward_local_port(GebrCommServer *server,
				    guint16 remote_port,
				    guint16 local_port,
				    const gchar *addr)
{
	return gebr_comm_server_forward_port(server, local_port, remote_port, addr, TRUE);
}

gboolean
gebr_comm_server_append_key(GebrCommServer *server,
                            void * finished_callback,
                            gpointer user_data)
{
	gchar *path = gebr_key_filename(TRUE);
	gchar *public_key;

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		g_free(path);
		return FALSE;
	}

	g_debug("Append gebr.key on PATH %s", path);

	// FIXME: please handle GError of the g_file_get_contents
	g_file_get_contents(path, &public_key, NULL, NULL);
	public_key[strlen(public_key) - 1] = '\0'; // Erase new line

	GebrCommTerminalProcess *process;
	server->process = process = gebr_comm_terminal_process_new();

	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), server);
	if (finished_callback)
		g_signal_connect(process, "finished", G_CALLBACK(finished_callback), user_data);
	g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_finished), server);

	gchar *ssh_cmd = get_ssh_command_with_key();
	GString *cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s '%s' -o StrictHostKeyChecking=no "
	                "'umask 077; test -d $HOME/.ssh || mkdir $HOME/.ssh ; echo \"%s (%s)\" >> $HOME/.ssh/authorized_keys'",
	                ssh_cmd, server->address->str, public_key, gebr_comm_server_is_maestro(server) ? "gebr" : "gebrm");

	gebr_comm_terminal_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
	g_free(path);
	g_free(ssh_cmd);
	g_free(public_key);

	return TRUE;
}

void
gebr_comm_server_set_use_public_key(GebrCommServer *server,
                                    gboolean use_key)
{
	server->use_public_key = use_key;
}

gboolean
gebr_comm_server_get_use_public_key(GebrCommServer *server)
{
	return server->use_public_key;
}

gboolean
gebr_comm_server_is_maestro(GebrCommServer *server)
{
	return server->priv->is_maestro;
}
