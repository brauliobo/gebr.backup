/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebr_guiproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <regex.h>
#include <webkit/webkit.h>
#include <glib.h>
#include <geoxml.h>

#include "gebr-gui-html-viewer-widget.h"
#include "../../intl.h"
#include "../../utils.h"
#include "../../defines.h"
#include "gebr-gui-utils.h"
#include "gebr-gui-js.h"

#define CSS_LINK "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://"LIBGEBR_DATA_DIR"/gebr.css\" />"

enum {
	TITLE_READY,
	PRINT_REQUESTED,
	LAST_SIGNAL
};

static guint signals[ LAST_SIGNAL ] = { 0 };

typedef struct _GebrGuiHtmlViewerWidgetPrivate GebrGuiHtmlViewerWidgetPrivate;

struct _GebrGuiHtmlViewerWidgetPrivate {
	GtkWidget * web_view;
	GebrGeoXmlObject *object;
	GebrGuiHtmlViewerCustomTab callback;
	gchar * label;
	GString *content;
};

#define GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, GebrGuiHtmlViewerWidgetPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, GebrGuiHtmlViewerWidget *self);

static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame *frame,
							WebKitNetworkRequest *request, GebrGuiHtmlViewerWidget *self);

static void gebr_gui_html_viewer_widget_finalize(GObject *object);

G_DEFINE_TYPE(GebrGuiHtmlViewerWidget, gebr_gui_html_viewer_widget, GTK_TYPE_VBOX);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_html_viewer_widget_class_init(GebrGuiHtmlViewerWidgetClass * klass)
{
	GObjectClass * gobject_class;
	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_gui_html_viewer_widget_finalize;

	/**
	 * GebrGuiHtmlViewerWidget::title-ready:
	 * @widget: This #GebrGuiHtmlViewerWidget
	 * @title: The title.
	 *
	 * This signal is fired when the title is ready to be set. The title is passed by parameter @title, and its
	 * value may depend on context. If this viewer is showing the help of a geoxml-menu, the title is the same as
	 * the menu. Otherwise, the title is defined by the &lt;title&gt; tag inside the Html.
	 */
	signals[ TITLE_READY ] =
		g_signal_new("title-ready",
			     GEBR_GUI_TYPE_HTML_VIEWER_WIDGET,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiHtmlViewerWidgetClass, title_ready),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE,
			     1,
			     G_TYPE_STRING);

	/**
	 * GebrGuiHtmlViewerWidget::print-requested:
	 * Emitted when the user requests a print job.
	 */
	signals[ PRINT_REQUESTED ] =
		g_signal_new("print-requested",
			     GEBR_GUI_TYPE_HTML_VIEWER_WIDGET,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiHtmlViewerWidgetClass, print_requested),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	g_type_class_add_private(klass, sizeof(GebrGuiHtmlViewerWidgetPrivate));
}

static void gebr_gui_html_viewer_widget_init(GebrGuiHtmlViewerWidget * self)
{
	GtkWidget * scrolled_window;
	GebrGuiHtmlViewerWidgetPrivate * priv;

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	priv->object = NULL;
	priv->web_view = webkit_web_view_new();
	priv->callback = NULL;
	priv->content = g_string_new (NULL);

	g_signal_connect(priv->web_view, "navigation-requested", G_CALLBACK(on_navigation_requested), self);
	g_signal_connect(priv->web_view, "load-finished", G_CALLBACK(on_load_finished), self);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), priv->web_view);
	gtk_box_pack_start(GTK_BOX(self), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show_all(scrolled_window);
}

static void gebr_gui_html_viewer_widget_finalize(GObject *object)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(object);
	
	g_free(priv->label);
	g_string_free (priv->content, TRUE);

	G_OBJECT_CLASS (gebr_gui_html_viewer_widget_parent_class)->finalize (object);
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void create_generate_links_index_function(JSContextRef context)
{
	static const gchar * script =
		"function GenerateLinksIndex(links, is_programs_help) {"
			"var navbar = document.getElementsByClassName('navigation')[0];"
			"if (!navbar) return;"
			"var linklist = navbar.getElementsByTagName('ul')[0];"
			"if (!linklist) return;"
			"if (!is_programs_help) {"
				"var session = document.createElement('li');"
				"session.innerHTML = '<h2>Programs</h2>';"
				"linklist.appendChild(session);"
			"}"
			"for (var i = 0; i < links.length; ++i) {"
				"var link = document.createElement('a');"
				"link.setAttribute('href', links[i][1]);"
				"link.innerHTML = links[i][0];"
				"var li = document.createElement('li');"
				"li.appendChild(link);"
				"linklist.appendChild(li);"
			"}"
		"}";

	gebr_js_evaluate(context, script);
}

static void generate_links_index(JSContextRef context, const gchar * links, gboolean is_programs_help)
{
	gchar * script;
	const gchar * program;

	program = is_programs_help? "true":"false";
	script = g_strdup_printf("GenerateLinksIndex(%s,%s);", links, program);
	gebr_js_evaluate(context, script);
	g_free(script);
}

static gchar * js_get_document_title(JSContextRef context)
{
	JSValueRef value;
	GString * title;
	value = gebr_js_evaluate(context, "document.title");
	title = gebr_js_value_get_string(context, value);
	return g_string_free(title, FALSE);
}

static void on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	GebrGeoXmlObjectType type;
	JSContextRef context;
	gchar * title;

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	context = webkit_web_frame_get_global_context(frame);

	if (priv->object) {
		type = gebr_geoxml_object_get_type(priv->object);
		create_generate_links_index_function(context);

		if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
			generate_links_index(context, "[['<b>Menu</b>', 'gebr://menu']]", TRUE);
		} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
			GString * list;
			GebrGeoXmlSequence *program;

			list = g_string_new("[");
			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(priv->object), &program, 0);

			for (gint i = 0; program != NULL; gebr_geoxml_sequence_next(&program), ++i)
				g_string_append_printf(list, "%s['%s', 'gebr://prog%d']",
						       i == 0? "":",",
						       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)), i);
			g_string_append(list, "]");
			generate_links_index(context, list->str, FALSE);
			g_string_free(list, TRUE);
		}
	}

	title = js_get_document_title(context);

	g_signal_emit(self, signals[ TITLE_READY ], 0, title);
	g_free (title);
}

static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame *frame,
							WebKitNetworkRequest *request, GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	const gchar * uri = webkit_network_request_get_uri(request);

	if (priv->object && g_str_has_prefix(uri, "gebr://")) {
		const gchar *help;
		GebrGeoXmlObject *object;
		if (!strcmp(uri, "gebr://menu")) {
			if (gebr_geoxml_object_get_type(priv->object) == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			GebrGeoXmlDocument *menu = gebr_geoxml_object_get_owner_document(priv->object);
			help = gebr_geoxml_document_get_help(menu);
			object = GEBR_GEOXML_OBJECT(menu);
		} else {
			if (gebr_geoxml_object_get_type(priv->object) != GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			int program_index = -1;
			sscanf(strstr(uri, "prog"), "prog%d", &program_index);
			GebrGeoXmlSequence *program;
			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(priv->object), &program, program_index);
			if (program == NULL) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Invalid link"), _("Sorry, couldn't find program."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}

			help = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(program));
			object = GEBR_GEOXML_OBJECT(program);
		}

		if (!strlen(help)) {
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("No help available"), _("Sorry, the help is empty."));
			return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
		}

		gebr_gui_html_viewer_widget_generate_links(self, object);
		gebr_gui_html_viewer_widget_show_html(self, help);
		return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
	}
	if (g_str_has_prefix(uri, "file://") || g_str_has_prefix(uri, "about:"))
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;

	gebr_gui_show_uri(uri);

	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

void pre_process_html (GebrGuiHtmlViewerWidgetPrivate * priv)
{
	regex_t regexp;
	regmatch_t matchptr;

	if (!priv->content->len)
		return;

	regcomp(&regexp, "<link[^<]*gebr.css[^<]*>", REG_NEWLINE | REG_ICASE);
	if (!regexec(&regexp, priv->content->str, 1, &matchptr, 0)) {
		/*
		 * If we found a link for GeBR's style sheet, we must update its path.
		 */
		gssize start = matchptr.rm_so;
		gssize length = matchptr.rm_eo - matchptr.rm_so;
		g_string_erase(priv->content, start, length);
		g_string_insert(priv->content, start, CSS_LINK);
	} else if (priv->object != NULL) {
		/*
		 * Otherwise, we only include the CSS link if we have a
		 * GeoXml object associated with this priv->content view, because
		 * this mean we are viewing a Flow or Program.
		 */
		regcomp(&regexp, "<head>", REG_NEWLINE | REG_ICASE);
		if (!regexec(&regexp, priv->content->str, 1, &matchptr, 0)) {
			gssize start = matchptr.rm_eo;
			g_string_insert(priv->content, start, CSS_LINK);
		}
	}
}

static GtkWidget * on_create_custom_widget(GtkPrintOperation * print_op, GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	if (priv->callback)
		return priv->callback(self); 

	return NULL;
}

static void on_apply_custom_widget(GtkPrintOperation * print_op, GtkWidget * widget, GebrGuiHtmlViewerWidget * self)
{
	g_signal_emit(self, signals[ PRINT_REQUESTED ], 0);
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

GtkWidget *gebr_gui_html_viewer_widget_new(void)
{
	return  g_object_new(GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, NULL);
}

void gebr_gui_html_viewer_widget_print(GebrGuiHtmlViewerWidget * self)
{
	WebKitWebFrame * frame;
	GtkPrintOperation * print_op;
	GebrGuiHtmlViewerWidgetPrivate * priv;
	GError * error = NULL;

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	print_op = gtk_print_operation_new();
	g_signal_connect(print_op, "create-custom-widget", G_CALLBACK(on_create_custom_widget), self);
	g_signal_connect(print_op, "custom-widget-apply", G_CALLBACK(on_apply_custom_widget), self);
	gtk_print_operation_set_custom_tab_label(print_op, priv->label);

	frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(priv->web_view));
	webkit_web_frame_print_full(frame, print_op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, &error);

	if (error) {
		g_warning("%s", error->message);
		g_clear_error(&error);
	}
	g_object_unref(print_op);
}

void gebr_gui_html_viewer_widget_show_html(GebrGuiHtmlViewerWidget * self, const gchar * content)
{
	GebrGuiHtmlViewerWidgetPrivate * priv;
	GString *tmp_file;

	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	tmp_file = gebr_make_temp_filename("XXXXXX.html");
	g_string_assign (priv->content, content);

	pre_process_html (priv);

	/* some webkit versions crash to open an empty file... */
	if (!priv->content->len)
		g_string_assign(priv->content, " ");

	/* write current priv->content to temporary file */
	FILE *fp;
	fp = fopen(tmp_file->str, "w");
	fputs(priv->content->str, fp);
	fclose(fp);

	webkit_web_view_open(WEBKIT_WEB_VIEW(priv->web_view), tmp_file->str);

	g_string_free(tmp_file, TRUE);
}

void gebr_gui_html_viewer_widget_generate_links(GebrGuiHtmlViewerWidget *self, GebrGeoXmlObject * object)
{
	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	priv->object = object;
}

void gebr_gui_html_viewer_widget_set_custom_tab (GebrGuiHtmlViewerWidget *self,
						 const gchar *label,
						 GebrGuiHtmlViewerCustomTab callback)
{
	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WIDGET(self));

	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	priv->callback = callback;
	priv->label = g_strdup(label);
}

const gchar * gebr_gui_html_viewer_widget_get_html (GebrGuiHtmlViewerWidget *self)
{
	GebrGuiHtmlViewerWidgetPrivate *priv;

	g_return_val_if_fail (GEBR_GUI_IS_HTML_VIEWER_WIDGET (self), NULL);

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE (self);
	return priv->content->str;
}
