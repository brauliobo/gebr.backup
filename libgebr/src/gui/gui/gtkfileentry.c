/*   libgebr - GêBR Library
 *   Copyright(C) 2007 GÃªBR core team(http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtkfileentry.h"
#include "support.h"

/*
 * Prototypes
 */

static gboolean
__gtk_file_entry_expose(GtkWidget * widget, GdkEventExpose * event);

static void
gtk_file_entry_realize(GtkWidget *widget);

static void
gtk_file_entry_map(GtkWidget *widget);

static void
__gtk_file_entry_entry_changed(GtkEntry * entry, GtkFileEntry * file_entry);

static void
__gtk_file_entry_browse_button_clicked(GtkButton * button, GtkFileEntry * file_entry);

/*
 * gobject stuff
 */

enum {
	PATH_CHANGED = 0,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
gtk_file_entry_class_init(GtkFileEntryClass * class)
{
	GtkWidgetClass *	widget_class;

	widget_class = (GtkWidgetClass*)class;
	widget_class->expose_event = __gtk_file_entry_expose;
	widget_class->realize = gtk_file_entry_realize;
	widget_class->map = gtk_file_entry_map;

	/* signals */
	object_signals[PATH_CHANGED] = g_signal_new("path-changed",
		GTK_TYPE_FILE_ENTRY,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET(GtkFileEntryClass, path_changed),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
gtk_file_entry_init(GtkFileEntry * file_entry)
{
	GtkWidget *	hbox;
	GtkWidget *	entry;
	GtkWidget *	browse_button;

// 	GTK_WIDGET_SET_FLAGS(file_entry, GTK_CAN_FOCUS | GTK_RECEIVES_DEFAULT);
	GTK_WIDGET_SET_FLAGS(file_entry, GTK_NO_WINDOW | GTK_CAN_FOCUS);

	/* hbox */
	hbox = gtk_hbox_new(FALSE, 0);

	/* entry */
	entry = gtk_entry_new();
	file_entry->entry = entry;
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	g_signal_connect(GTK_ENTRY(entry), "changed",
		G_CALLBACK(__gtk_file_entry_entry_changed), file_entry);

	/* browse button */
	browse_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start(GTK_BOX(hbox), browse_button, FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(browse_button), "clicked",
		G_CALLBACK(__gtk_file_entry_browse_button_clicked), file_entry);

	file_entry->choose_directory = FALSE;

	gtk_widget_show_all(hbox);
	gtk_container_add(GTK_CONTAINER(file_entry), hbox);
}

G_DEFINE_TYPE(GtkFileEntry, gtk_file_entry, GTK_TYPE_BIN);

/*
 * Internal functions
 */

static gboolean
__gtk_file_entry_expose(GtkWidget * widget, GdkEventExpose * event)
{
	return TRUE;
}

static void
gtk_file_entry_realize(GtkWidget *widget)
{
	GtkFileEntry *	file_entry;
	GdkWindowAttr	attributes;
	gint		attributes_mask;
	gint		border_width;

	file_entry = GTK_FILE_ENTRY(widget);
	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	border_width = GTK_CONTAINER(widget)->border_width;

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x + border_width;
	attributes.y = widget->allocation.y + border_width;
	attributes.width = widget->allocation.width - border_width * 2;
	attributes.height = widget->allocation.height - border_width * 2;
	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
				GDK_BUTTON_RELEASE_MASK |
				GDK_ENTER_NOTIFY_MASK |
				GDK_LEAVE_NOTIFY_MASK);
	puts("gtk_file_entry_realize");
	attributes_mask = GDK_WA_X | GDK_WA_Y;

	widget->window = gtk_widget_get_parent_window (widget);
	g_object_ref (widget->window);

	file_entry->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
						&attributes, attributes_mask);
	gdk_window_set_user_data (file_entry->event_window, file_entry);

	widget->style = gtk_style_attach (widget->style, widget->window);
}

static void
gtk_file_entry_map(GtkWidget *widget)
{
	GtkFileEntry *	file_entry;
puts("gtk_file_entry_map");
	file_entry = GTK_FILE_ENTRY(widget);
	GTK_WIDGET_CLASS(gtk_file_entry_parent_class)->map(widget);

	if (file_entry->event_window)
		gdk_window_show(file_entry->event_window);
}

static void
__gtk_file_entry_entry_changed(GtkEntry * entry, GtkFileEntry * file_entry)
{
	g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
}

static void
__gtk_file_entry_browse_button_clicked(GtkButton * button, GtkFileEntry * file_entry)
{
	GtkWidget *	chooser_dialog;

	chooser_dialog = gtk_file_chooser_dialog_new(
		file_entry->choose_directory == FALSE ? _("Choose file") : _("Choose directory"),
		NULL,
		file_entry->choose_directory == FALSE ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);

	switch (gtk_dialog_run(GTK_DIALOG(chooser_dialog))) {
	case GTK_RESPONSE_OK:
		gtk_entry_set_text(GTK_ENTRY(file_entry->entry), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog)));
		g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(chooser_dialog));
}

/*
 * Library functions
 */

GtkWidget *
gtk_file_entry_new()
{
	return g_object_new(GTK_TYPE_FILE_ENTRY, NULL);
}

void
gtk_file_entry_set_choose_directory(GtkFileEntry * file_entry, gboolean choose_directory)
{
	file_entry->choose_directory = choose_directory;
}

gboolean
gtk_file_entry_get_choose_directory(GtkFileEntry * file_entry)
{
	return file_entry->choose_directory;
}

void
gtk_file_entry_set_path(GtkFileEntry * file_entry, const gchar * path)
{
	gtk_entry_set_text(GTK_ENTRY(file_entry->entry), path);
}

gchar *
gtk_file_entry_get_path(GtkFileEntry * file_entry)
{
	return (gchar*)gtk_entry_get_text(GTK_ENTRY(file_entry->entry));
}
