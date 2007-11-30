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

#include "log.h"

/*
 * Internal functions
 */

log_messages_free_each(struct log_message * message)
{
	g_string_free(message, TRUE);
	g_free(log_message);
}

/*
 * Library functions
 */

struct log *
log_open(const gchar * path)
{
	struct log *	log;
	GIOStatus	io_status;

	log = g_malloc(sizeof(log));
	*log = (struct log) {
		.io_channel = g_io_channel_new_file(path, "r+"),
		.messages = NULL
	};

	while (1) {
		GString *	tmp;
		GError *	error;

		error = NULL;
		tmp = g_string_new(NULL);
		io_status = g_io_channel_read_line_string(log->io_channel, &tmp, NULL, &error);
		if (io_status != G_IO_STATUS_EOF) {
			struct log_message * log
		} else {
			g_string_free(tmp, TRUE);
			break;
		}
	}
}

void
log_close(struct log * log)
{
	g_io_channel_unref(log->io_channel);

	g_list_foreach(log->messages, (GFunc)log_messages_free_each, NULL);
	g_list_free(log->messages);

	g_free(log);
}

void
log_add_message(struct log * log, enum log_message_type type, GString * message)
{

}