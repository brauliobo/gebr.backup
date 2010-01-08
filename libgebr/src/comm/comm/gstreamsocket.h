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
 *
 *   Inspired on Qt 4.3 version of QStreamSocket, by Trolltech
 */

#ifndef __GEBR_COMM_STREAM_SOCKET_H
#define __GEBR_COMM_STREAM_SOCKET_H

#include "gsocket.h"
#include "gsocketaddress.h"

G_BEGIN_DECLS GType gebr_comm_stream_socket_get_type(void);

#define GEBR_COMM_STREAM_SOCKET_TYPE		(gebr_comm_stream_socket_get_type())
#define GEBR_COMM_STREAM_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_STREAM_SOCKET_TYPE, GStreamSocket))
#define GEBR_COMM_STREAM_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_STREAM_SOCKET_TYPE, GStreamSocketClass))
#define GEBR_COMM_IS_STREAM_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_STREAM_SOCKET_TYPE))
#define GEBR_COMM_IS_STREAM_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_STREAM_SOCKET_TYPE))
#define GEBR_COMM_STREAM_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_STREAM_SOCKET_TYPE, GStreamSocketClass))

typedef struct _GStreamSocket GStreamSocket;
typedef struct _GStreamSocketClass GStreamSocketClass;

struct _GStreamSocket {
	GebrCommSocket parent;
};
struct _GStreamSocketClass {
	GebrCommSocketClass parent;

	/* signals */
	void (*connected) (GStreamSocket * self);
	void (*disconnected) (GStreamSocket * self);
};

/*
 * user functions
 */

GStreamSocket *gebr_comm_stream_socket_new(void);

gboolean
gebr_comm_stream_socket_connect(GStreamSocket * stream_socket, GebrCommSocketAddress * socket_address, gboolean wait);

void gebr_comm_stream_socket_disconnect(GStreamSocket * stream_socket);

GebrCommSocketAddress gebr_comm_stream_socket_peer_address(GStreamSocket * stream_socket);

G_END_DECLS
#endif				//__GEBR_COMM_STREAM_SOCKET_H
