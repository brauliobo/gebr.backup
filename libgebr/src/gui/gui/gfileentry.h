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

#ifndef __LIBGEBR_GUI_GTK_FILE_ENTRY_H
#define __LIBGEBR_GUI_GTK_FILE_ENTRY_H

G_BEGIN_DECLS

GType
gtk_file_entry_get_type(void);

#define GTK_FILE_ENTRY_TYPE		(gtk_file_entry_get_type())
#define GTK_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_FILE_ENTRY_TYPE, GtkFileEntry))
#define GTK_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_FILE_ENTRY_TYPE, GtkFileEntryClass))
#define GTK_IS_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_FILE_ENTRY_TYPE))
#define GTK_IS_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_FILE_ENTRY_TYPE))
#define GTK_FILE_ENTRY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_FILE_ENTRY_TYPE, GtkFileEntryClass))

typedef struct _GtkFileEntry		GtkFileEntry;
typedef struct _GtkFileEntryClass	GtkFileEntryClass;

typedef struct _GtkFileEntry {
	GWidget		parent;

	GtkWidget *	hbox;

	GtkWidget *	entry;
	GtkWidget *	browse_button;
};
struct _GtkFileEntryClass {
	GWidgetClass		parent;

	/* signals */
	void			(*path_changed)(GtkFileEntry * self);
};

GtkFileEntry *
gtk_file_entry_new();

void
gtk_file_entry_free(GtkFileEntry * file_entry);

void
gtk_file_entry_set_choose_directory(GtkFileEntry * file_entry, gboolean choose_directory);

gboolean
gtk_file_entry_get_choose_directory(GtkFileEntry * file_entry, gboolean choose_directory);

G_END_DECLS

#endif //__LIBGEBR_GUI_GTK_FILE_ENTRY_H
