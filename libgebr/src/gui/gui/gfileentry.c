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

#include "gfileentry.h"
#include "support.h"

/*
 * gobject stuff
 */

enum {
	PATH_CHANGED,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
gtk_file_entry_class_init(GtkFileEntryClass * class)
{
	/* virtual */
	class->connected = NULL;
	class->disconnected = NULL;
	class->new_connection = NULL;

	/* signals */
	object_signals[PATH_CHANGED] = g_signal_new("path-changed",
		G_FILE_ENTRY_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GtkFileEntryClass, path_changed),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}
r
static void
gtk_file_entry_init(GtkFileEntry * file_entry)
{
	GtkFileEntry *	file_entry;

	file_entry = g_malloc(sizeof(_GtkFileEntry));
	file_entry.hbox = gtk_hbox_new(FALSE, 10);

	/* entry */
	file_entry.entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX (file_entry.hbox), file_entry.entry, TRUE, TRUE, 0);

	/* browse button */
	file_entry.browse_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start(GTK_BOX (file_entry.hbox), file_entry.browse_button, FALSE, TRUE, 0);
	g_signal_connect (GTK_OBJECT (file_entry.browse_button), "clicked",
			  G_CALLBACK (file_entry_browse_button_clicked), file_entry.entry);

	return file_entry;
}

G_DEFINE_TYPE(GtkFileEntry, gtk_file_entry, GTK_TYPE_WIDGET)

/*
 * Internal functions
 */

static void
__gtk_file_entry_browse_button_clicked(GtkWidget * button, GtkFileEntry * file_entry)
{
	GtkWidget *	chooser_dialog;

	chooser_dialog = gtk_file_chooser_dialog_new(	_("Choose file"), NULL,
							GTK_FILE_CHOOSER_ACTION_SAVE,
							GTK_STOCK_OK, GTK_RESPONSE_OK,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							NULL);

	switch (gtk_dialog_run(GTK_DIALOG(chooser_dialog))) {
	case GTK_RESPONSE_OK:
		gtk_entry_set_text(GTK_ENTRY(entry), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog)));
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(chooser_dialog));
}

/*
 * Library functions
 */

GtkFileEntry *
gtk_file_entry_new()
{
	return (GtkFileEntry*)g_object_new(G_FILE_ENTRY_TYPE, NULL);
}

void
gtk_file_entry_free(GtkFileEntry * file_entry)
{

}

void
gtk_file_entry_set_choose_directory(GtkFileEntry * file_entry, gboolean choose_directory)
{

}

gboolean
gtk_file_entry_get_choose_directory(GtkFileEntry * file_entry, gboolean choose_directory)
{

}
