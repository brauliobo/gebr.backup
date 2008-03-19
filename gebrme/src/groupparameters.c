/*   GÍBR ME - GÍBR Menu Editor
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include "groupparameters.h"
#include "support.h"
#include "parameter.h"

/*
 * Internal stuff
 */
void
group_parameters_data_free(GtkObject * expander, struct group_parameters_data * data)
{
	g_free(data);
}

/*
 * Public functions
 */
GtkWidget *
group_parameters_create_ui(GeoXmlParameterGroup * parameter_group, gboolean hidden)
{
	GtkWidget *			group_parameters_expander;
	GtkWidget *			group_parameters_label_widget;
	GtkWidget *			group_parameters_label;
	GtkWidget *			group_parameters_vbox;
	GtkWidget *			widget;
	GtkWidget *			depth_hbox;

	GeoXmlSequence *		i;
	struct group_parameters_data *	data;

	group_parameters_expander = gtk_expander_new("");
	gtk_expander_set_expanded (GTK_EXPANDER (group_parameters_expander), hidden);
	gtk_widget_show(group_parameters_expander);
	depth_hbox = create_depth(group_parameters_expander);

	data = g_malloc(sizeof(struct group_parameters_data));
	data->parameters.is_group = TRUE;
	data->parameters.parameters = geoxml_parameter_group_get_parameters(parameter_group);
	g_signal_connect(group_parameters_expander, "destroy",
		GTK_SIGNAL_FUNC(group_parameters_data_free), data);

	group_parameters_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(depth_hbox), group_parameters_vbox, TRUE, TRUE, 0);
	gtk_widget_show(group_parameters_vbox);

	group_parameters_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(group_parameters_expander), group_parameters_label_widget);
	gtk_widget_show(group_parameters_label_widget);
	gtk_expander_hacked_define(group_parameters_expander, group_parameters_label_widget);
	group_parameters_label = gtk_label_new(_("Parameters"));
	gtk_widget_show(group_parameters_label);
	gtk_box_pack_start(GTK_BOX(group_parameters_label_widget), group_parameters_label, FALSE, TRUE, 0);
	widget = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(group_parameters_label_widget), widget, FALSE, TRUE, 5);
	g_signal_connect(widget, "clicked",
		GTK_SIGNAL_FUNC(parameter_add), data);
	g_object_set(G_OBJECT(widget),
		"user-data", group_parameters_vbox,
		"relief", GTK_RELIEF_NONE,
		NULL);

	data->group = parameter_group;
	data->radio_group = NULL;
	i = geoxml_parameters_get_first_parameter(data->parameters.parameters);
	while (i != NULL) {
		gtk_box_pack_start(GTK_BOX(group_parameters_vbox),
			parameter_create_ui(GEOXML_PARAMETER(i), (struct parameters_data *)data, hidden), FALSE, TRUE, 0);

		geoxml_sequence_next(&i);
	}

	return group_parameters_expander;
}
