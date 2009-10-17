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

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "gchannelsocket.h"
#include "gsocketprivate.h"
#include "gstreamsocketprivate.h"
#include "gsocketaddressprivate.h"

/*
 * prototypes
 */

static void
__g_channel_socket_new_connection(GebrCommChannelSocket * channel_socket);

/*
 * gobject stuff
 */

enum {
	LAST_PROPERTY
};

enum {
	LAST_SIGNAL
};
// static guint object_signals[LAST_SIGNAL];

static void
gebr_comm_channel_socket_class_init(GebrCommChannelSocketClass * class)
{
	/* virtual */
	class->parent.new_connection = (typeof(class->parent.new_connection))__g_channel_socket_new_connection;
}

static void
gebr_comm_channel_socket_init(GebrCommChannelSocket * channel_socket)
{
	channel_socket->parent.state = G_SOCKET_STATE_NOTLISTENING;
}

G_DEFINE_TYPE(GebrCommChannelSocket, gebr_comm_channel_socket, GEBR_COMM_SOCKET_TYPE)

/*
 * internal functions
 */

static void
__g_channel_socket_source_disconnected(GStreamSocket * source_socket, GStreamSocket * forward_socket)
{
	gebr_comm_socket_close(GEBR_COMM_SOCKET(source_socket));
	gebr_comm_socket_close(GEBR_COMM_SOCKET(forward_socket));
}

static void
__g_channel_socket_source_read(GStreamSocket * source_socket, GStreamSocket * forward_socket)
{
	GByteArray *	byte_array;

	byte_array = gebr_comm_socket_read_all(GEBR_COMM_SOCKET(source_socket));
	gebr_comm_socket_write(GEBR_COMM_SOCKET(forward_socket), byte_array);

	g_byte_array_free(byte_array, TRUE);
}

static void
__g_channel_socket_source_error(GStreamSocket * source_socket, enum GebrCommSocketError error, GStreamSocket * forward_socket)
{
	__g_channel_socket_source_disconnected(source_socket, forward_socket);
}

static void
__g_channel_socket_forward_disconnected(GStreamSocket * forward_socket, GStreamSocket * source_socket)
{
	__g_channel_socket_source_disconnected(source_socket, forward_socket);
}

static void
__g_channel_socket_forward_read(GStreamSocket * forward_socket, GStreamSocket * source_socket)
{
	__g_channel_socket_source_read(forward_socket, source_socket);
}

static void
__g_channel_socket_forward_error(GStreamSocket * forward_socket, enum GebrCommSocketError error, GStreamSocket * source_socket)
{
	__g_channel_socket_source_error(source_socket, error, forward_socket);
}

static void
__g_channel_socket_new_connection(GebrCommChannelSocket * channel_socket)
{
	GebrCommSocketAddress	peer_address;
	int		client_sockfd, sockfd;

	sockfd = _g_socket_get_fd(&channel_socket->parent);
	while ((client_sockfd = _gebr_comm_socket_address_accept(&peer_address,
	channel_socket->parent.address_type, sockfd)) != -1) {
		GStreamSocket *	source_socket;
		GStreamSocket *	forward_socket;

		source_socket = _g_stream_socket_new_connected(client_sockfd, channel_socket->parent.address_type);
		forward_socket = g_stream_socket_new();
		g_stream_socket_connect(forward_socket, &channel_socket->forward_address, FALSE);

		g_signal_connect(source_socket, "disconnected", (GCallback)
			__g_channel_socket_source_disconnected, forward_socket);
		g_signal_connect(source_socket, "ready-read", (GCallback)
			__g_channel_socket_source_read, forward_socket);
		g_signal_connect(source_socket, "error", (GCallback)
			__g_channel_socket_source_error, forward_socket);

		g_signal_connect(forward_socket, "disconnected", (GCallback)
			__g_channel_socket_forward_disconnected, source_socket);
		g_signal_connect(forward_socket, "ready-read", (GCallback)
			__g_channel_socket_forward_read, source_socket);
		g_signal_connect(forward_socket, "error", (GCallback)
			__g_channel_socket_forward_error, source_socket);
	}
}

/*
 * user functions
 */

GebrCommChannelSocket *
gebr_comm_channel_socket_new(void)
{
	return (GebrCommChannelSocket*)g_object_new(GEBR_COMM_CHANNEL_SOCKET_TYPE, NULL);
}

void
gebr_comm_channel_socket_free(GebrCommChannelSocket * channel_socket)
{
	gebr_comm_socket_close(&channel_socket->parent);
	g_object_unref(channel_socket);
}

gboolean
gebr_comm_channel_socket_start(GebrCommChannelSocket * channel_socket, GebrCommSocketAddress * listen_address,
	GebrCommSocketAddress * forward_address)
{
	if (!gebr_comm_socket_address_get_is_valid(listen_address) || !gebr_comm_socket_address_get_is_valid(forward_address))
		return FALSE;

	int			sockfd;
	struct sockaddr	*	sockaddr;
	gsize			sockaddr_size;
	GError *		error;

	/* initialization */
	error = NULL;
	sockfd = socket(_gebr_comm_socket_address_get_family(listen_address), SOCK_STREAM, 0);
	_g_socket_init(&channel_socket->parent, sockfd, listen_address->type);
	channel_socket->parent.state = G_SOCKET_STATE_NOTLISTENING;
	g_io_channel_set_flags(channel_socket->parent.io_channel, G_IO_FLAG_NONBLOCK, &error);
	_g_socket_enable_read_watch(&channel_socket->parent);

	/* bind and listen */
	_gebr_comm_socket_address_get_sockaddr(listen_address, &sockaddr, &sockaddr_size);
	if (bind(sockfd, sockaddr, sockaddr_size))
		return FALSE;
	if (listen(sockfd, 1024)) {
		channel_socket->parent.state = G_SOCKET_STATE_NOTLISTENING;
		return FALSE;
	}
	channel_socket->parent.state = G_SOCKET_STATE_LISTENING;
	channel_socket->forward_address = *forward_address;

	return TRUE;
}

GebrCommSocketAddress
gebr_comm_channel_socket_get_forward_address(GebrCommChannelSocket * channel_socket)
{
	return channel_socket->forward_address;
}
