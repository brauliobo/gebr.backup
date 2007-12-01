/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team(http://ui_flow_browse.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui_flow_browse..h"
#include "gebr.h"
#include "support.h"
#include "ui_help.h"

/*
 * Function: add_flow_browse
 * Assembly the flow browse page.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_flow_browse
flow_browse_setup_ui(void)
{
	struct ui_flow_browse.	ui_flow_browse;
	GtkWidget *		page;
	GtkWidget *		hpanel;
	GtkWidget *		scrolledwin;
	GtkTreeSelection *	selection;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;
	gchar *			label;

	label = _("Flows");

	/* Create flow browse page */
	page = gtk_vbox_new(FALSE, 0);

	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(page), hpanel);

	/*
	 * Left side: flow list
	 */
	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolledwin, FALSE, FALSE);
	gtk_widget_set_size_request(scrolledwin, 180, -1);

	ui_flow_browse.flow_store = gtk_list_store_new(FB_N_COLUMN,
					G_TYPE_STRING,  /* Name(title for libgeoxml) */
					G_TYPE_STRING); /* Filename */

	ui_flow_browse.flow_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_browse.flow_store));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(GTK_OBJECT(renderer), "edited",
			GTK_SIGNAL_FUNC(flow_rename), NULL);
	g_signal_connect(GTK_OBJECT(renderer), "edited",
			GTK_SIGNAL_FUNC(flow_browse_info_update), NULL);

	col = gtk_tree_view_column_new_with_attributes(label, renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, FB_TITLE);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse.flow_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", FB_TITLE);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse.flow_view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);

	gtk_container_add(GTK_CONTAINER(scrolledwin), ui_flow_browse.flow_view);

	g_signal_connect(GTK_OBJECT(ui_flow_browse.flow_view), "cursor-changed",
			GTK_SIGNAL_FUNC(flow_load), NULL);
	g_signal_connect(GTK_OBJECT(ui_flow_browse.flow_view), "cursor-changed",
			GTK_SIGNAL_FUNC(flow_browse_info_update();), NULL);

	/*
	 * Right side: flow info
	 */
	{
		GtkWidget *	frame;
		GtkWidget *	infopage;

		frame = gtk_frame_new(_("Details"));
		gtk_paned_pack2(GTK_PANED(hpanel), frame, TRUE, FALSE);

		infopage = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(frame), infopage);

		/* Title */
		ui_flow_browse.flow_info.title = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.title), 0, 0);
		gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse.flow_info.title, FALSE, TRUE, 0);

		/* Description */
		ui_flow_browse.flow_info.description = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.description), 0, 0);
		gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse.flow_info.description, FALSE, TRUE, 10);

		/* I/O */
		GtkWidget *table;
		table = gtk_table_new(3, 2, FALSE);
		gtk_box_pack_start(GTK_BOX(infopage), table, FALSE, TRUE, 0);

		ui_flow_browse.flow_info.input_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.input_label), 0, 0);
		gtk_table_attach(GTK_TABLE(table), ui_flow_browse.flow_info.input_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

		ui_flow_browse.flow_info.input = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.input), 0, 0);
		gtk_table_attach(GTK_TABLE(table), ui_flow_browse.flow_info.input, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

		ui_flow_browse.flow_info.output_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.output_label), 0, 0);
		gtk_table_attach(GTK_TABLE(table), ui_flow_browse.flow_info.output_label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

		ui_flow_browse.flow_info.output = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.output), 0, 0);
		gtk_table_attach(GTK_TABLE(table), ui_flow_browse.flow_info.output, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

		ui_flow_browse.flow_info.error_label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.error_label), 0, 0);
		gtk_table_attach(GTK_TABLE(table), ui_flow_browse.flow_info.error_label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

		ui_flow_browse.flow_info.error = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.error), 0, 0);
		gtk_table_attach(GTK_TABLE(table), ui_flow_browse.flow_info.error, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

		/* Help */
		ui_flow_browse.flow_info.help = gtk_button_new_from_stock(GTK_STOCK_INFO);
		gtk_box_pack_end(GTK_BOX(infopage), ui_flow_browse.flow_info.help, FALSE, TRUE, 0);
		g_signal_connect(GTK_OBJECT(ui_flow_browse.flow_info.help), "clicked",
				GTK_SIGNAL_FUNC(flow_browse_show_help), flow);

		/* Author */
		ui_flow_browse.flow_info.author = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(ui_flow_browse.flow_info.author), 0, 0);
		gtk_box_pack_end(GTK_BOX(infopage), ui_flow_browse.flow_info.author, FALSE, TRUE, 0);
	}

	return ui_flow_browse;
}

/*
 * Function: flow_browse_info_update
 * Update information shown about the selected flow
 */
void
flow_browse_info_update(void)
{
	if (gebr.flow == NULL) {
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.title), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.description), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.input_label), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.input), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.output_label), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.output), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.error_label), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.error), "");
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.author), "");

		g_object_set(ui_flow_browse.flow_info.help, "sensitive", FALSE, NULL);
		return;
	}

	gchar *		markup;
	GString *	text;

	/* Title in bold */
	markup = g_markup_printf_escaped("<b>%s</b>", geoxml_document_get_title(GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(ui_flow_browse.flow_info.title), markup);
	g_free(markup);

	/* Description in italic */
	markup = g_markup_printf_escaped("<i>%s</i>", geoxml_document_get_description(GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(ui_flow_browse.flow_info.description), markup);
	g_free(markup);

	/* I/O labels */
	markup = g_markup_printf_escaped("<b>"_("Input:")"</b>");
	gtk_label_set_markup(GTK_LABEL(ui_flow_browse.flow_info.input_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>"_("Output:")"</b>");
	gtk_label_set_markup(GTK_LABEL(ui_flow_browse.flow_info.output_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>"_("Error log:")"</b>");
	gtk_label_set_markup(GTK_LABEL(ui_flow_browse.flow_info.error_label), markup);
	g_free(markup);

	/* Input file */
	if (strlen(geoxml_flow_io_get_input(flow)) > 0)
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.input), geoxml_flow_io_get_input(flow));
	else
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.input), "(none)");
	/* Output file */
	if (strlen(geoxml_flow_io_get_output(flow)) > 0)
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.output), geoxml_flow_io_get_output(flow));
	else
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.output), "(none)");
	/* Error file */
	if(strlen(geoxml_flow_io_get_error(flow)) > 0)
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.error), geoxml_flow_io_get_error(flow));
	else
		gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.error), "(none)");

	/* Author and email */
	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
			geoxml_document_get_author(GEOXML_DOC(gebr.flow)),
			geoxml_document_get_email(GEOXML_DOC(gebr.flow)));
	gtk_label_set_text(GTK_LABEL(ui_flow_browse.flow_info.author), text->text);
	g_string_free(text, TRUE);

	/* Info button */
	g_object_set(ui_flow_browse.flow_info.help, "sensitive", TRUE, NULL);
}

void
flow_browse_show_help(void)
{
	help_show((gchar*)geoxml_document_get_help(GEOXML_DOC(gebr.flow)),
		_("Flow help"), (gchar*)geoxml_document_get_filename(GEOXML_DOC(gebr.flow)));
}
