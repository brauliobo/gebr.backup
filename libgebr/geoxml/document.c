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
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include "../config.h"

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <gdome.h>
#include <libxml/parser.h>

#include "defines.h"
#if ENABLE_TIDY
# if TIDY_HAVE_SUBDIR
#  include <tidy/tidy.h>
#  include <tidy/buffio.h>
# else
#  include <tidy.h>
#  include <buffio.h>
# endif
#endif

#include "document.h"
#include "document_p.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "sequence.h"
#include "flow.h"
#include "parameter.h"
#include "program-parameter.h"
#include "program.h"
#include "parameters_p.h"
#include "parameter_p.h"
#include "parameter_group.h"
#include "parameter_group_p.h"
#include "value_sequence.h"
#include "../gebr-expr.h"
#include "../utils.h"

#include "parameters.h"
/* global variables */
/**
 * \internal
 * Global variable.
 */
GdomeException exception;

/**
 * \internal
 */
struct gebr_geoxml_document {
	GdomeDocument *document;
};

/**
 * \internal
 */
typedef GdomeDocument *(*createDoc_func) (GdomeDOMImplementation *, char *, unsigned int, GdomeException *);

/**
 * \internal
 */
static GdomeDOMImplementation *dom_implementation;
/**
 * \internal
 */
static gint dom_implementation_ref_count = 0;
/**
 * \internal
 * Used at GebrGeoXmlObject's methods.
 */
GdomeDocument *clipboard_document = NULL;

static const gchar *dtd_directory = GEBR_GEOXML_DTD_DIR;

/**
 * \internal
 * Checks if \p version has the form '%d.%d.%d', ie three numbers separated by a dot.
 */
static gboolean gebr_geoxml_document_is_version_valid(const gchar * version);

/**
 * \internal
 * Checks if \p document version is less than or equal to GeBR's version.
 */
static gboolean gebr_geoxml_document_check_version(GebrGeoXmlDocument * document, const gchar * version);

/**
 * \internal
 */
static void gebr_geoxml_document_fix_header(GString * source, const gchar * tagname, const gchar * dtd_filename);

/**
 * \internal
 */
static void __gebr_geoxml_ref(void)
{
	if (!dom_implementation_ref_count) {
		dom_implementation = gdome_di_mkref();
		clipboard_document = gdome_di_createDocument(dom_implementation, NULL,
							     gdome_str_mkref("gebr-geoxml-clipboard"), NULL,
							     &exception);
	} else
		gdome_di_ref(dom_implementation, &exception);
	++dom_implementation_ref_count;
}

/**
 * \internal
 */
static void __gebr_geoxml_unref(void)
{
	gdome_di_unref(dom_implementation, &exception);
	--dom_implementation_ref_count;
	if (!dom_implementation) {
		gdome_doc_unref(clipboard_document, &exception);
		clipboard_document = NULL;
	}
}

/*
 * __gebr_geoxml_document_new_data:
 * Creates the #GebrGeoXmlDocumentData for this document.
 */
static void __gebr_geoxml_document_new_data(GebrGeoXmlDocument * document, const gchar * filename)
{
	GebrGeoXmlDocumentData *data = g_new(GebrGeoXmlDocumentData, 1);
	((GdomeDocument*)document)->user_data = data;
	data->filename = g_string_new(filename);
}

/**
 * \internal
 */
static GdomeDocument *__gebr_geoxml_document_clone_doc(GdomeDocument * source, GdomeDocumentType * document_type)
{
	if (source == NULL)
		return NULL;

	GdomeDocument *document;
	GdomeElement *root_element;
	GdomeElement *source_root_element;

	source_root_element = gdome_doc_documentElement(source, &exception);
	document = gdome_di_createDocument(dom_implementation,
					   NULL, gdome_el_nodeName(source_root_element, &exception), document_type,
					   &exception);
	if (document == NULL)
		return NULL;

	/* copy version attribute */
	root_element = gdome_doc_documentElement(document, &exception);
	__gebr_geoxml_set_attr_value(root_element, "version",
				     __gebr_geoxml_get_attr_value(source_root_element, "version"));

	/* import all elements */
	GdomeElement *element = __gebr_geoxml_get_first_element(source_root_element, "*");
	for (; element != NULL; element = __gebr_geoxml_next_element(element)) {
		GdomeNode *new_node = gdome_doc_importNode(document, (GdomeNode*)element, TRUE, &exception);
		gdome_el_appendChild(root_element, new_node, &exception);
	}

	return document;
}

/**
 * \internal
 */
static int
__gebr_geoxml_document_validate_doc(GdomeDocument ** document,
				    GebrGeoXmlDiscardMenuRefCallback discard_menu_ref)
{
	GString *source;
	GString *dtd_filename;

	GdomeDocument *tmp_doc;

	GdomeElement *root_element;
	const gchar *version;

	int ret;

	dtd_filename = g_string_new(NULL);
	if (gdome_doc_doctype(*document, &exception) != NULL) {
		ret = GEBR_GEOXML_RETV_DTD_SPECIFIED;
		goto out2;
	}

	/* initialization */
	root_element = gebr_geoxml_document_root_element(*document);

	/* If there is no version attribute, the document is invalid */
	version = gebr_geoxml_document_get_version((GebrGeoXmlDocument *) *document);
	if (!gebr_geoxml_document_is_version_valid(version)) {
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto out2;
	}

	/* Checks if the document's version is greater than GeBR's version */
	if (!gebr_geoxml_document_check_version((GebrGeoXmlDocument*)*document, version)) {
		ret = GEBR_GEOXML_RETV_NEWER_VERSION;
		goto out2;
	}

	/* Find the DTD spec file. If the file doesn't exists, it may mean that this document is from newer version. */
	gchar * tagname;
	tagname = g_strdup(gdome_el_nodeName(root_element, &exception)->str);
	g_string_printf(dtd_filename, "%s/%s-%s.dtd", dtd_directory, tagname, version);
	if (g_file_test(dtd_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
	    || g_access(dtd_filename->str, R_OK) < 0) {
		ret = GEBR_GEOXML_RETV_CANT_ACCESS_DTD;
		goto out2;
	}

	/* Inserts the document type into the xml so we can validate and specify the ID attributes and use
	 * gdome_doc_getElementById. */
	gchar *src;
	gdome_di_saveDocToMemoryEnc(dom_implementation, *document, &src, ENCODING, GDOME_SAVE_STANDARD, &exception);
	source = g_string_new(src);
	g_free(src);
	gebr_geoxml_document_fix_header(source, tagname, dtd_filename->str);

	/* Based on code by Daniel Veillard
	 * References:
	 *   http://xmlsoft.org/examples/index.html#parse2.c
	 *   http://xmlsoft.org/examples/parse2.c
	 */
	xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
	xmlDocPtr doc = xmlCtxtReadMemory(ctxt, source->str, source->len, NULL, NULL,
					  XML_PARSE_NOBLANKS | XML_PARSE_DTDATTR |        /* default DTD attributes */
					  XML_PARSE_NOENT |                               /* substitute entities */
					  XML_PARSE_DTDVALID);
	if (doc == NULL) {
		xmlFreeParserCtxt(ctxt);
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto out;
	} else {
		xmlFreeDoc(doc);
		if (ctxt->valid == 0) {
			xmlFreeParserCtxt(ctxt);
			ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
			goto out;
		}
	}

	tmp_doc = gdome_di_createDocFromMemory(dom_implementation, source->str, GDOME_LOAD_PARSING, &exception);

	g_free(tagname);

	if (tmp_doc == NULL) {
		ret = GEBR_GEOXML_RETV_NO_MEMORY;
		goto out;
	}
	gdome_doc_unref(*document, &exception);
	*document = tmp_doc;
	root_element = gebr_geoxml_document_root_element(tmp_doc);

	/*
	 * Success, now change to last version
	 */

	/**
	 * \internal
	 * Change group XML as declared in flow-0.3.5, project-0.3.2 and line-0.3.2
	 */
	void __port_to_new_group_semantics(void)
	{
		GdomeElement *element;
		gebr_foreach_gslist_hyg(element, __gebr_geoxml_get_elements_by_tag(root_element, "parameters"), parameters) {
			__gebr_geoxml_set_attr_value(element, "default-selection",
						     __gebr_geoxml_get_attr_value(element, "exclusive"));
			__gebr_geoxml_remove_attr(element, "exclusive");
			__gebr_geoxml_set_attr_value(element, "selection",
						     __gebr_geoxml_get_attr_value(element, "selected"));
			__gebr_geoxml_remove_attr(element, "selected");
		}

		gebr_foreach_gslist_hyg(element, __gebr_geoxml_get_elements_by_tag(root_element, "group"), group) {
			GdomeNode *new_instance;
			GebrGeoXmlParameters *template_instance;

			template_instance = GEBR_GEOXML_PARAMETERS(__gebr_geoxml_get_first_element(element, "parameters"));
			/* encapsulate template instance into template-instance element */
			GdomeElement *template_container;
			template_container = __gebr_geoxml_insert_new_element(element, "template-instance",
									      (GdomeElement*)template_instance);
			gdome_el_insertBefore_protected(template_container, (GdomeNode*)template_instance, NULL, &exception);

			/* move new instance after template instance */
			new_instance = gdome_el_cloneNode((GdomeElement*)template_instance, TRUE, &exception);
			GdomeNode *next = (GdomeNode*)__gebr_geoxml_next_element((GdomeElement*)template_container);
			gdome_n_insertBefore_protected((GdomeNode*)element, new_instance, next, &exception);
		}
	}

	/* document 0.1.x to 0.2.0 */
	if (strcmp(version, "0.2.0") < 0) {
		GdomeElement *element;
		GdomeElement *before;

		before = __gebr_geoxml_get_first_element(root_element, "email");
		before = __gebr_geoxml_next_element(before);

		element = __gebr_geoxml_insert_new_element(root_element, "date", before);
		__gebr_geoxml_insert_new_element(element, "created", NULL);
		__gebr_geoxml_insert_new_element(element, "modified", NULL);

		if (gebr_geoxml_document_get_type((GebrGeoXmlDocument *) *document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			__gebr_geoxml_insert_new_element(element, "lastrun", NULL);
	}
	/* document 0.2.0 to 0.2.1 */
	if (strcmp(version, "0.2.1") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GebrGeoXmlSequence *program;

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(*document), &program, 0);
			while (program != NULL) {
				if (__gebr_geoxml_get_first_element((GdomeElement *) program, "url") == NULL)
					__gebr_geoxml_insert_new_element((GdomeElement *) program, "url",
									 __gebr_geoxml_get_first_element((GdomeElement
													  *) program,
													 "parameters"));

				gebr_geoxml_sequence_next(&program);
			}
		}
	}
	/* document 0.2.1 to 0.2.2 */
	if (strcmp(version, "0.2.2") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *element;

			__gebr_geoxml_set_attr_value(root_element, "version", "0.2.2");

			__gebr_geoxml_foreach_element_with_tagname_r(root_element, "option", element, option) {
				gchar *value;

				value = g_strdup(__gebr_geoxml_get_element_value(element));
				__gebr_geoxml_set_element_value(element, "", __gebr_geoxml_create_TextNode);

				__gebr_geoxml_insert_new_element(element, "label", NULL);
				__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
								(element, "value", NULL), value,
								__gebr_geoxml_create_TextNode);

				g_free(value);
			}

			__gebr_geoxml_foreach_element_with_tagname_r(root_element, "range", element, range)
			    __gebr_geoxml_set_attr_value(element, "digits", "");
		}
	}
	/* document 0.2.2 to 0.2.3
	 * nothing changed. why? because the changes were about the removed group support
	 */
	if (strcmp(version, "0.2.3") < 0)
		__gebr_geoxml_set_attr_value(root_element, "version", "0.2.3");
	/* document 0.2.3 to 0.3.0 */
	if (strcmp(version, "0.3.0") < 0) {
		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.0");
		__gebr_geoxml_set_attr_value(root_element, "nextid", "n0");

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GebrGeoXmlSequence *program;

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(*document), &program, 0);
			while (program != NULL) {
				GebrGeoXmlParameters *parameters;
				GdomeElement *old_parameter;

				parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
				__gebr_geoxml_set_attr_value((GdomeElement *) parameters, "exclusive", "0");
				old_parameter = __gebr_geoxml_get_first_element((GdomeElement *) parameters, "*");
				while (old_parameter != NULL) {
					GdomeElement *next_parameter;
					GdomeElement *parameter;
					GdomeElement *property;

					GebrGeoXmlParameterType type;
					GdomeDOMString *tag_name;
					int i;

					type = GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
					tag_name = gdome_el_tagName(old_parameter, &exception);
					for (i = 1; i <= parameter_type_to_str_len; ++i)
						if (!strcmp(parameter_type_to_str[i], tag_name->str)) {
							type = (GebrGeoXmlParameterType)i;
							break;
						}

					parameter = __gebr_geoxml_insert_new_element((GdomeElement *) parameters,
										     "parameter", old_parameter);
					gdome_el_insertBefore_protected(parameter, (GdomeNode *)
							      __gebr_geoxml_get_first_element(old_parameter, "label"),
							      NULL, &exception);

					next_parameter = __gebr_geoxml_next_element(old_parameter);
					gdome_el_insertBefore_protected(parameter, (GdomeNode *) old_parameter, NULL, &exception);

					property = __gebr_geoxml_insert_new_element(old_parameter, "property",
										    (GdomeElement *)
										    gdome_el_firstChild(old_parameter,
													&exception));
					gdome_el_insertBefore_protected(property, (GdomeNode *)
							      __gebr_geoxml_get_first_element(old_parameter, "keyword"),
							      NULL, &exception);
					if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
						GdomeElement *value;
						GdomeDOMString *string;
						GdomeDOMString *separator;

						string = gdome_str_mkref("required");
						gdome_el_setAttribute(property, string,
								      gdome_el_getAttribute(old_parameter, string,
											    &exception), &exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_str_unref(string);

						value = __gebr_geoxml_get_first_element(old_parameter, "value");
						string = gdome_str_mkref("separator");
						separator = gdome_el_getAttribute(old_parameter, string, &exception);
						if (strlen(separator->str)) {
							gdome_el_setAttribute(property, string, separator, &exception);

							gebr_geoxml_program_parameter_set_parse_list_value
							    (GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE,
							     __gebr_geoxml_get_element_value(value));
							gebr_geoxml_program_parameter_set_parse_list_value
							    (GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE,
							     __gebr_geoxml_get_attr_value(value, "default"));
						} else {
							__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
											((GdomeElement *) property,
											 "value", NULL),
											__gebr_geoxml_get_element_value
											(value),
											__gebr_geoxml_create_TextNode);
							__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
											((GdomeElement *) property,
											 "default", NULL),
											__gebr_geoxml_get_attr_value
											(value, "default"),
											__gebr_geoxml_create_TextNode);
						}
						gdome_el_removeChild(old_parameter, (GdomeNode *) value, &exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_str_unref(string);
					} else {
						GdomeElement *state;

						state = __gebr_geoxml_get_first_element(old_parameter, "state");
						__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
										((GdomeElement *) property, "value",
										 NULL),
										__gebr_geoxml_get_element_value(state),
										__gebr_geoxml_create_TextNode);
						__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
										((GdomeElement *) property, "default",
										 NULL),
										__gebr_geoxml_get_attr_value(state,
													     "default"),
										__gebr_geoxml_create_TextNode);
						__gebr_geoxml_set_attr_value(property, "required", "no");

						gdome_el_removeChild(old_parameter, (GdomeNode *) state, &exception);
					}

					old_parameter = next_parameter;
				}

				gebr_geoxml_sequence_next(&program);
			}
		}
	}
	/* document 0.3.0 to 0.3.1 */
	if (strcmp(version, "0.3.1") < 0) {
		GdomeElement *dict_element;

		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.1");

		dict_element = __gebr_geoxml_insert_new_element(root_element, "dict",
								__gebr_geoxml_get_first_element(root_element, "date"));
		__gebr_geoxml_parameters_append_new(dict_element);

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_insert_new_element(root_element, "servers",
							 __gebr_geoxml_next_element(__gebr_geoxml_get_first_element
										    (root_element, "io")));
		}
	}
	/* 0.3.1 to 0.3.2 */ 
	if (strcmp(version, "0.3.2") < 0) {
		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.2");

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *element;

			gebr_foreach_gslist(element, __gebr_geoxml_get_elements_by_tag(root_element, "menu")) {
				if (discard_menu_ref != NULL)
					discard_menu_ref((GebrGeoXmlProgram *)gdome_el_parentNode(element, &exception), 
							__gebr_geoxml_get_element_value(element) ,
							atoi(__gebr_geoxml_get_attr_value(element, "index")));

				gdome_n_removeChild(gdome_el_parentNode(element, &exception), (GdomeNode *)element, &exception);
			}
		} else {
			/* removal of filename for project and lines */
			gdome_el_removeChild(root_element, (GdomeNode*)__gebr_geoxml_get_first_element(root_element, "filename"), &exception);

			__gebr_geoxml_remove_attr(root_element, "nextid");

			GSList * params;
			GSList * iter;

			params = __gebr_geoxml_get_elements_by_tag(root_element, "parameter");
			iter = params;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "id");
				iter = iter->next;
			}
			g_slist_free(params);

			GSList * refer;

			refer = __gebr_geoxml_get_elements_by_tag(root_element, "reference");
			iter = refer;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "idref");
				iter = iter->next;
			}
			g_slist_free(refer);

			__port_to_new_group_semantics();
		}
	}
	/* flow 0.3.2 to 0.3.3 */ 
	if (strcmp(version, "0.3.3") < 0) {
		// Backward compatible change, nothing to be done.
		// Added #IMPLIED 'version' attribute to 'program' tag.
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.3");
		else if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_LINE) {
			GdomeElement *pivot;
			pivot = __gebr_geoxml_get_first_element(root_element, "date");
			pivot = __gebr_geoxml_next_element(pivot);
			__gebr_geoxml_insert_new_element(root_element, "server-group", pivot);
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.3");
		}
	}

	/* flow 0.3.3 to 0.3.4 */ 
	if (strcmp(version, "0.3.4") < 0) {
		// Backward compatible change, nothing to be done.
		// Added #IMPLIED 'mpi' attribute to 'program' tag.
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.4");
	}
	/* flow 0.3.4 to 0.3.5 */ 
	if (strcmp(version, "0.3.5") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.5");

			/* remove flow filename */
			gdome_el_removeChild(root_element, (GdomeNode*)__gebr_geoxml_get_first_element(root_element, "filename"), &exception);

			__port_to_new_group_semantics();

			__gebr_geoxml_remove_attr(root_element, "nextid");

			GSList * params;
			GSList * iter;

			params = __gebr_geoxml_get_elements_by_tag(root_element, "parameter");
			iter = params;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "id");
				iter = iter->next;
			}
			g_slist_free(params);

			GSList * refer;

			refer = __gebr_geoxml_get_elements_by_tag(root_element, "reference");
			iter = refer;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "idref");
				iter = iter->next;
			}
			g_slist_free(refer);
		}
	}
	/* flow 0.3.5 to 0.3.6 */ 
	if (strcmp(version, "0.3.6") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.6");

			GdomeElement *element;

			gebr_foreach_gslist_hyg(element, __gebr_geoxml_get_elements_by_tag(root_element, "group"), group) {
				GdomeElement *parameter;
				GebrGeoXmlParameterGroup *group;
				GebrGeoXmlSequence *instance;
				GebrGeoXmlSequence *iter;
				gboolean first_instance = TRUE;

				parameter = (GdomeElement*)gdome_el_parentNode(element, &exception);
				group = GEBR_GEOXML_PARAMETER_GROUP(parameter);
				gebr_geoxml_parameter_group_get_instance(group, &instance, 0);

				for (; instance != NULL; gebr_geoxml_sequence_next(&instance)){
					gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &iter, 0);

					for (; iter != NULL; gebr_geoxml_sequence_next(&iter)){
						__gebr_geoxml_parameter_set_label((GebrGeoXmlParameter *) iter, "");

						if (first_instance){
							if( __gebr_geoxml_parameter_get_type((GebrGeoXmlParameter *) iter, FALSE) != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE){
								__gebr_geoxml_parameter_set_be_reference_with_value((GebrGeoXmlParameter *) iter);
							}
						}
					}
					first_instance = FALSE;
				}
			}
		}
	}

	if (strcmp(version, "0.3.7") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GHashTable * keys_to_canonized = NULL;
			GebrGeoXmlSequence * program = NULL;

			GdomeElement *io;
			GdomeElement *el;
			GdomeElement *new_io;
			GdomeElement *lastrun;
			GdomeElement *server;
			GdomeElement *servers;

			gebr_geoxml_document_canonize_dict_parameters(
					(GebrGeoXmlDocument *) *document,
					&keys_to_canonized);

			io = __gebr_geoxml_get_first_element (root_element, "io");
			servers = __gebr_geoxml_next_element (io);
			server = __gebr_geoxml_get_first_element (servers, "server");

			if (!server) {
				server = __gebr_geoxml_insert_new_element (root_element, "server", servers);
				__gebr_geoxml_set_attr_value(server, "address", "127.0.0.1");
				new_io = __gebr_geoxml_insert_new_element(server, "io", NULL);
				el = __gebr_geoxml_insert_new_element(new_io, "input", NULL);
				__gebr_geoxml_set_element_value(el, "", __gebr_geoxml_create_TextNode);
				el = __gebr_geoxml_insert_new_element(new_io, "output", NULL);
				__gebr_geoxml_set_element_value(el, "", __gebr_geoxml_create_TextNode);
				el = __gebr_geoxml_insert_new_element(new_io, "error", NULL);
				__gebr_geoxml_set_element_value(el, "", __gebr_geoxml_create_TextNode);
				lastrun = __gebr_geoxml_insert_new_element(server, "lastrun", NULL);
				__gebr_geoxml_set_element_value(lastrun, "", __gebr_geoxml_create_TextNode);
			} else {
				server = (GdomeElement*)gdome_el_cloneNode(server, TRUE, &exception);
				gdome_el_insertBefore_protected(root_element,
								(GdomeNode*)server,
								(GdomeNode*)servers,
								&exception);
			}

			gdome_el_removeChild(root_element, (GdomeNode*) io, &exception);
			gdome_el_removeChild(root_element, (GdomeNode*) servers, &exception);

			gebr_geoxml_flow_get_program(
					(GebrGeoXmlFlow *) *document,
					&program, 0);

			for (; program != NULL; gebr_geoxml_sequence_next(&program))
			{
				gebr_geoxml_program_foreach_parameter(
						GEBR_GEOXML_PROGRAM(program),
						gebr_geoxml_program_parameter_update_old_dict_value,
						keys_to_canonized);

			}
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.7");
		}
	}

	/* CHECKS (may impact performance) */
	if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
		GdomeElement *element;

		/* accept only on/off for flags */
		gebr_foreach_gslist(element, __gebr_geoxml_get_elements_by_tag(root_element, "flag")) {
			GdomeElement *value_element;
			GdomeElement *default_value_element;

			value_element = __gebr_geoxml_get_first_element(element, "value");
			default_value_element = __gebr_geoxml_get_first_element(element, "default");

			if (strcmp(__gebr_geoxml_get_element_value(value_element), "on") &&
			    strcmp(__gebr_geoxml_get_element_value(value_element), "off"))
				__gebr_geoxml_set_element_value(value_element, "off", __gebr_geoxml_create_TextNode);
			if (strcmp(__gebr_geoxml_get_element_value(default_value_element), "on") &&
			    strcmp(__gebr_geoxml_get_element_value(default_value_element), "off"))
				__gebr_geoxml_set_element_value(default_value_element, "off",
								__gebr_geoxml_create_TextNode);
		}
	}

	ret = GEBR_GEOXML_RETV_SUCCESS;

out:
	g_string_free(source, TRUE);
out2:
	g_string_free(dtd_filename, TRUE);
	return ret;
}

/**
 * \internal
 */
static int __gebr_geoxml_document_load(GebrGeoXmlDocument ** document, const gchar * src, createDoc_func func,
				       gboolean validate, GebrGeoXmlDiscardMenuRefCallback discard_menu_ref)
{
	GdomeDocument *doc;
	int ret;

	/* create the implementation. */
	__gebr_geoxml_ref();

	/* load */
	doc = func(dom_implementation, (gchar *) src, GDOME_LOAD_PARSING, &exception);
	if (doc == NULL) {
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto err;
	}

	if (validate) {
		ret = __gebr_geoxml_document_validate_doc(&doc, discard_menu_ref);
		if (ret != GEBR_GEOXML_RETV_SUCCESS) {
			gdome_doc_unref((GdomeDocument *) doc, &exception);
			goto err;
		}
	}

	*document = (GebrGeoXmlDocument *) doc;
	__gebr_geoxml_document_new_data(*document, "");
	return GEBR_GEOXML_RETV_SUCCESS;

 err:	__gebr_geoxml_unref();
	*document = NULL;
	return ret;
}

/**
 * \internal
 */
static int filename_check_access(const gchar * filename)
{
	if (filename == NULL)
		return GEBR_GEOXML_RETV_FILE_NOT_FOUND;
	if (!g_file_test(filename, G_FILE_TEST_EXISTS)) 
		return GEBR_GEOXML_RETV_FILE_NOT_FOUND;
	if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_access(filename, R_OK) < 0) 
		return GEBR_GEOXML_RETV_PERMISSION_DENIED;

	return GEBR_GEOXML_RETV_SUCCESS;
}

/*
 * private functions and variables
 */

GebrGeoXmlDocument *gebr_geoxml_document_new(const gchar * name, const gchar * version)
{
	GdomeDocument *document;
	GdomeElement *root_element;
	GdomeElement *element;

	/* create the implementation. */
	__gebr_geoxml_ref();
	document = gdome_di_createDocument(dom_implementation, NULL, gdome_str_mkref(name), NULL, &exception);
	__gebr_geoxml_document_new_data((GebrGeoXmlDocument *)document, "");

	/* document (root) element */
	root_element = gdome_doc_documentElement(document, &exception);
	__gebr_geoxml_set_attr_value(root_element, "version", version);

	/* elements (as specified in DTD) */
	__gebr_geoxml_insert_new_element(root_element, "title", NULL);
	__gebr_geoxml_insert_new_element(root_element, "description", NULL);
	__gebr_geoxml_insert_new_element(root_element, "help", NULL);
	__gebr_geoxml_insert_new_element(root_element, "author", NULL);
	__gebr_geoxml_insert_new_element(root_element, "email", NULL);
	element = __gebr_geoxml_insert_new_element(root_element, "dict", NULL);
	__gebr_geoxml_parameters_append_new(element);
	element = __gebr_geoxml_insert_new_element(root_element, "date", NULL);
	__gebr_geoxml_insert_new_element(element, "created", NULL);
	__gebr_geoxml_insert_new_element(element, "modified", NULL);

	return (GebrGeoXmlDocument *) document;
}

/*
 * library functions.
 */

int gebr_geoxml_document_load(GebrGeoXmlDocument ** document, const gchar * path, gboolean validate,
			      GebrGeoXmlDiscardMenuRefCallback discard_menu_ref)
{
	int ret;
	if ((ret = filename_check_access(path))) {
		*document = NULL;
		return ret;
	}

	ret = __gebr_geoxml_document_load(document, path, (createDoc_func)gdome_di_createDocFromURI, validate,
					  discard_menu_ref);
	if (ret)
		return ret;

	gchar * filename = g_path_get_basename(path);
	gebr_geoxml_document_set_filename(*document, filename);
	g_free(filename);

	return ret;
}

int gebr_geoxml_document_load_buffer(GebrGeoXmlDocument ** document, const gchar * xml)
{
	return __gebr_geoxml_document_load(document, xml, (createDoc_func)gdome_di_createDocFromMemory, TRUE, NULL);
}

void gebr_geoxml_document_free(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return;

	GebrGeoXmlDocumentData *data;
	data = _gebr_geoxml_document_get_data(document);
	g_string_free(data->filename, TRUE);
	g_free(data);
	gdome_doc_unref((GdomeDocument *) document, &exception);
	__gebr_geoxml_unref();
}

GebrGeoXmlDocument *gebr_geoxml_document_clone(GebrGeoXmlDocument * source)
{
	if (source == NULL)
		return NULL;

	gchar *xml;
	GebrGeoXmlDocumentData *data;
	GebrGeoXmlDocument *document;

	data = _gebr_geoxml_document_get_data(source);
	gebr_geoxml_document_to_string(source, &xml);
	gebr_geoxml_document_load_buffer(&document, xml);
	__gebr_geoxml_document_new_data(document, data->filename->str);
	g_free(xml);

	return document;
}

GebrGeoXmlDocumentType gebr_geoxml_document_get_type(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;

	GdomeElement *root_element;

	root_element = gdome_doc_documentElement((GdomeDocument *) document, &exception);

	if (strcmp("flow", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
	else if (strcmp("line", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_LINE;
	else if (strcmp("project", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_PROJECT;

	return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
}

const gchar *gebr_geoxml_document_get_version(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value(gdome_doc_documentElement((GdomeDocument *) document, &exception),
					    "version");
}

int gebr_geoxml_document_validate(const gchar * filename)
{
	int ret;
	if ((ret = filename_check_access(filename)))
		return ret;

	GebrGeoXmlDocument *document;

	ret = gebr_geoxml_document_load(&document, filename, TRUE, NULL);
	gebr_geoxml_document_free(document);

	return ret;
}

int gebr_geoxml_document_save(GebrGeoXmlDocument * document, const gchar * path)
{
	gzFile zfp;
	char *xml;
	int ret = 0;
	FILE *fp;

	if (document == NULL)
		return FALSE;

	gebr_geoxml_document_to_string(document, &xml);

	if ((gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) && g_str_has_suffix(path, ".mnu")) {
		fp = fopen(path, "w");
		if (fp == NULL) {
			return GEBR_GEOXML_RETV_PERMISSION_DENIED;
		}

#if ENABLE_TIDY
		Bool ok;
		TidyBuffer output;
		TidyBuffer errbuf;
		TidyDoc tdoc = tidyCreate ();

		tidyBufInit (&output);
		tidyBufInit (&errbuf);
		ok = tidyOptSetBool (tdoc, TidyXmlTags, yes);
		ok = ok && tidyOptSetValue (tdoc, TidyIndentContent, "auto");
		ok = ok && tidyOptSetValue (tdoc, TidyCharEncoding, "UTF8");
		ok = ok && tidyOptSetValue (tdoc, TidyWrapLen, "0");
		ok = ok && tidyOptSetBool(tdoc, TidyLiteralAttribs, yes);

		if (ok) {
			ret = tidySetErrorBuffer (tdoc, &errbuf);
			if (ret >= 0)
				ret = tidyParseString (tdoc, xml);
			if (ret >= 0)
				ret = tidyCleanAndRepair (tdoc);
			if (ret >= 0)
				ret = tidySaveBuffer (tdoc, &output);
			if (ret >= 0)
				ret = fwrite (output.bp, sizeof(gchar),
					      strlen((const gchar *)output.bp), fp);
		}
		tidyBufFree(&output);
		tidyBufFree(&errbuf);
		tidyRelease(tdoc);
#else
		ret = fwrite(xml, sizeof(gchar), strlen(xml), fp);
#endif

		fclose(fp);
	} else {
		zfp = gzopen(path, "w");

		if (zfp == NULL && errno != 0)
			return GEBR_GEOXML_RETV_PERMISSION_DENIED;

		ret = gzwrite(zfp, xml, strlen(xml));
		gzclose(zfp);
	}

	if (!ret) {
		gchar * filename = g_path_get_basename(path);
		gebr_geoxml_document_set_filename(document, filename);
		g_free(filename);
	}

	return ret ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_PERMISSION_DENIED;
}

int gebr_geoxml_document_to_string(GebrGeoXmlDocument * document, gchar ** xml_string)
{
	if (document == NULL)
		return FALSE;

	GdomeBoolean ret;
	GdomeDocument * clone;

	/* clone to remove DOCTYPE */
	clone = __gebr_geoxml_document_clone_doc((GdomeDocument*)document, NULL);
	ret = gdome_di_saveDocToMemoryEnc(dom_implementation, clone, xml_string, ENCODING,
					  GDOME_SAVE_LIBXML_INDENT, &exception);
	gdome_doc_unref(clone, &exception);

	return ret ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_NO_MEMORY;
}

void gebr_geoxml_document_set_filename(GebrGeoXmlDocument * document, const gchar * filename)
{
	if (document == NULL || filename == NULL)
		return;
	g_string_assign(_gebr_geoxml_document_get_data(document)->filename, filename);
}

void gebr_geoxml_document_set_title(GebrGeoXmlDocument * document, const gchar * title)
{
	if (document == NULL || title == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "title", title, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_author(GebrGeoXmlDocument * document, const gchar * author)
{
	if (document == NULL || author == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "author", author, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_email(GebrGeoXmlDocument * document, const gchar * email)
{
	if (document == NULL || email == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "email", email, __gebr_geoxml_create_TextNode);
}

GebrGeoXmlParameters *gebr_geoxml_document_get_dict_parameters(GebrGeoXmlDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);

	return (GebrGeoXmlParameters *)
	    __gebr_geoxml_get_first_element(__gebr_geoxml_get_first_element
					    (gebr_geoxml_document_root_element(document), "dict"), "parameters");
}

gboolean
gebr_geoxml_document_canonize_dict_parameters(GebrGeoXmlDocument * document,
					      GHashTable 	** list_copy)
{
	g_return_val_if_fail(document != NULL, FALSE);
	g_return_val_if_fail(list_copy != NULL, FALSE);

	static GHashTable * vars_list = NULL;

	GebrGeoXmlSequence * parameters = NULL;
	gint i = 0;
	GHashTable * values_to_canonized = NULL;

	values_to_canonized = g_hash_table_new_full(g_str_hash, g_str_equal,
						    g_free, g_free);

	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
	{
		if (vars_list != NULL)
			g_hash_table_destroy(vars_list);
		vars_list = NULL;
	}

	if (vars_list == NULL)
		vars_list = g_hash_table_new_full(g_str_hash, g_str_equal,
						  g_free, g_free);

	parameters = gebr_geoxml_parameters_get_first_parameter(
			gebr_geoxml_document_get_dict_parameters(document));

	for (i = 0; parameters != NULL; gebr_geoxml_sequence_next(&parameters), i++)
	{
		gchar * new_value = NULL;
		const gchar * key = gebr_geoxml_program_parameter_get_keyword(
				GEBR_GEOXML_PROGRAM_PARAMETER(parameters));

		GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(
				GEBR_GEOXML_PARAMETER(parameters));

		switch(type)
		{
		case	GEBR_GEOXML_PARAMETER_TYPE_STRING:
			break;
		case	GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
			break;
		default:
			gebr_geoxml_parameter_set_type(
					GEBR_GEOXML_PARAMETER(parameters),
					GEBR_GEOXML_PARAMETER_TYPE_FLOAT);
		}

		gchar * spaces = g_strdup(key);
		if(g_strcmp0(g_strstrip(spaces),"") == 0)
		{
			g_free(spaces);
			continue;
		}
		g_free(spaces);

		gebr_str_canonical_var_name(key, &new_value, NULL);
		gchar * duplicated_key = NULL;

		duplicated_key = g_hash_table_lookup(vars_list, key);
		if (duplicated_key)
		{
			gebr_geoxml_program_parameter_set_keyword(
					GEBR_GEOXML_PROGRAM_PARAMETER(parameters),
					(const gchar *)duplicated_key);
			continue;
		}

		duplicated_key = g_hash_table_lookup(values_to_canonized,
						     new_value);

		if(!duplicated_key)
		{
			g_hash_table_insert(values_to_canonized,
					    g_strdup(new_value),
					    g_strdup(key));

			g_hash_table_insert(vars_list,
					    g_strdup(key),
					    g_strdup(new_value));
		}
		else
		{	
			if(g_strcmp0(key, duplicated_key) == 0)
			{
				g_hash_table_insert(vars_list,
						    g_strdup(key),
						    g_strdup(new_value));

			}
			else
			{

				gint j = 1;
				gchar * concat = g_strdup_printf("%s_%d", new_value, j);

				while (g_hash_table_lookup(values_to_canonized,
						     concat) != NULL)
				{
					g_free(concat);
					concat = NULL;
					j += 1;
					concat = g_strdup_printf("%s_%d", new_value, j);
				}

				g_free(new_value);
				new_value = concat;

				g_hash_table_insert(vars_list,
						    g_strdup(key),
						    g_strdup(new_value));

				g_hash_table_insert(values_to_canonized,
						    g_strdup(new_value),
						    g_strdup(key));
			}
		}

		gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameters),
							  (const gchar *)new_value);
	}

	*list_copy = vars_list;
	g_hash_table_destroy(values_to_canonized); 
	return TRUE;
}


void gebr_geoxml_document_set_date_created(GebrGeoXmlDocument * document, const gchar * created)
{
	if (document == NULL || created == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(document), "date"), "created", created,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_date_modified(GebrGeoXmlDocument * document, const gchar * modified)
{
	if (document == NULL || modified == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(document), "date"), "modified", modified,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_description(GebrGeoXmlDocument * document, const gchar * description)
{
	if (document == NULL || description == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "description", description, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_help(GebrGeoXmlDocument * document, const gchar * help)
{
	if (document == NULL || help == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "help", help, __gebr_geoxml_create_CDATASection);
}

const gchar *gebr_geoxml_document_get_filename(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return _gebr_geoxml_document_get_data(document)->filename->str;
}

const gchar *gebr_geoxml_document_get_title(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value_non_rec(gebr_geoxml_document_root_element(document), "title");
}

const gchar *gebr_geoxml_document_get_author(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "author");
}

const gchar *gebr_geoxml_document_get_email(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "email");
}

const gchar *gebr_geoxml_document_get_date_created(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(document), "date"), "created");
}

const gchar *gebr_geoxml_document_get_date_modified(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(document), "date"), "modified");
}

const gchar *gebr_geoxml_document_get_description(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "description");
}

const gchar *gebr_geoxml_document_get_help(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "help");
}

static gboolean gebr_geoxml_document_is_version_valid(const gchar * version)
{
	if (!version)
		return FALSE;

	guint major, minor, micro;
	return sscanf(version, "%d.%d.%d", &major, &minor, &micro) == 3;
}

static gboolean gebr_geoxml_document_check_version(GebrGeoXmlDocument * document, const gchar * version)
{
	guint major1, minor1, micro1;
	guint major2, minor2, micro2;
	const gchar * doc_version;

	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		doc_version = GEBR_GEOXML_FLOW_VERSION;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		doc_version = GEBR_GEOXML_LINE_VERSION;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		doc_version = GEBR_GEOXML_PROJECT_VERSION;
		break;
	default:
		return FALSE;
	}

	if (sscanf(version, "%d.%d.%d", &major1, &minor1, &micro1) != 3)
		return FALSE;

	if (sscanf(doc_version, "%d.%d.%d", &major2, &minor2, &micro2) != 3)
		return FALSE;

	return major2 < major1
		|| (major2 == major1 && minor2 < minor1)
		|| (major2 == major1 && minor2 == minor1 && micro2 < micro1) ? FALSE : TRUE;
}

static void gebr_geoxml_document_fix_header(GString * source, const gchar * tagname, const gchar * dtd_filename)
{
	gssize c = 0;
	gchar * doctype;

	while (source->str[c] != '>')
		c++;

	doctype = g_strdup_printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>"
				  "<!DOCTYPE %s SYSTEM \"%s\">", tagname, dtd_filename);
	g_string_erase(source, 0, c + 1);
	g_string_prepend(source, doctype);
	g_free(doctype);
}

void
gebr_geoxml_document_merge_dicts(GebrGeoXmlDocument *first, ...)
{
	va_list args;
	GebrGeoXmlDocument *doc;
	GebrGeoXmlParameters *dict;

	g_return_if_fail(first != NULL);

	va_start(args, first);
	doc = va_arg(args, GebrGeoXmlDocument*);

	if (!doc)
		return;

	dict = gebr_geoxml_document_get_dict_parameters(first);

	while (doc) {
		GebrGeoXmlSequence *seq = gebr_geoxml_document_get_dict_parameter(doc);
		gebr_geoxml_parameters_append_parameter(dict, GEBR_GEOXML_PARAMETER_TYPE_GROUP);
		while (seq) {
			const gchar *value;
			const gchar *keyword;
			const gchar *comment;
			GebrGeoXmlParameterType type;
			GebrGeoXmlParameter *param =  GEBR_GEOXML_PARAMETER(seq);
			GebrGeoXmlProgramParameter *pparam = GEBR_GEOXML_PROGRAM_PARAMETER(seq);

			// copy data
			value = gebr_geoxml_program_parameter_get_first_value(pparam, FALSE);
			keyword = gebr_geoxml_program_parameter_get_keyword(pparam);
			comment = gebr_geoxml_parameter_get_label(param);
			type = gebr_geoxml_parameter_get_type(param);

			// paste data
			GebrGeoXmlParameter *copy;
			copy = gebr_geoxml_parameters_append_parameter(dict, type);
			param = GEBR_GEOXML_PARAMETER(copy);
			pparam = GEBR_GEOXML_PROGRAM_PARAMETER(copy);
			gebr_geoxml_program_parameter_set_keyword(pparam, keyword);
			gebr_geoxml_program_parameter_set_first_value(pparam, FALSE, value);
			gebr_geoxml_parameter_set_label(param, comment);

			gebr_geoxml_sequence_next(&seq);
		}
		doc = va_arg(args, GebrGeoXmlDocument*);
	}
	va_end(args);
}

gboolean
gebr_geoxml_document_split_dict(GebrGeoXmlDocument *first, ...)
{
	va_list args;
	gboolean retval = TRUE;
	GebrGeoXmlSequence *seq;
	GebrGeoXmlSequence *clean = NULL;
	GebrGeoXmlDocument *doc = NULL;
	GebrGeoXmlParameterType type;

	va_start(args, first);

	/**
	 * A document can contain dictionary information of its parents
	 * documents, ie a line can contain its projects variables, and a flow
	 * its line and project variables. Internally, they are separated by
	 * parameters of type "GROUP". See the example below
	 *
	 * FLOW
	 *   DICT-PARAMS
	 *     flow parameter
	 *     flow parameter
	 *     ...
	 *     PARAMETER_TYPE_GROUP
	 *     line parameter
	 *     line parameter
	 *     ...
	 *     PARAMETER_TYPE_GROUP
	 *     proj parameter
	 *     proj parameter
	 *     ...
	 */

	seq = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(first));
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlParameter *param = GEBR_GEOXML_PARAMETER(seq);
		type = gebr_geoxml_parameter_get_type(param);
		if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
			doc = va_arg(args, GebrGeoXmlDocument*);
			if (!doc) {
				/* The list of documents isn't large enough to
				 * hold the parameters. */
				retval = FALSE;
				goto clean;
			}
			if (!clean)
				clean = seq;
			continue;
		} else if (!doc)
			continue; // skip flow parameters

		// Time to split them
		const gchar *value;
		const gchar *keyword;
		const gchar *comment;
		GebrGeoXmlParameter *copy;
		GebrGeoXmlProgramParameter *pparam = GEBR_GEOXML_PROGRAM_PARAMETER(seq);

		value = gebr_geoxml_program_parameter_get_first_value(pparam, FALSE);
		keyword = gebr_geoxml_program_parameter_get_keyword(pparam);
		comment = gebr_geoxml_parameter_get_label(param);
		copy = gebr_geoxml_document_set_dict_keyword(doc, type, keyword, value);
		gebr_geoxml_parameter_set_label(copy, comment);
	}

clean:
	while (clean)
	{
		GebrGeoXmlSequence *aux = clean;
		gebr_geoxml_sequence_next(&clean);
		gebr_geoxml_sequence_remove(aux);
	}

	va_end(args);
	return retval;
}

static gboolean
document_has_dictkey (GebrGeoXmlDocument *doc, const gchar *name, gchar **out_value)
{
	GebrGeoXmlParameters *params = gebr_geoxml_document_get_dict_parameters (doc);

	GebrGeoXmlSequence *seq;
	gebr_geoxml_parameters_get_parameter (params, &seq, 0);
	for (; seq != NULL; gebr_geoxml_sequence_next (&seq)) {
		GebrGeoXmlProgramParameter *param = GEBR_GEOXML_PROGRAM_PARAMETER (seq);
		const gchar *keyword = gebr_geoxml_program_parameter_get_keyword (param);

		if (g_strcmp0 (name, keyword) == 0) {
			*out_value = g_strdup (gebr_geoxml_program_parameter_get_first_value (param, FALSE));
			return TRUE;
		}
	}

	return FALSE;
}

gboolean
gebr_geoxml_document_is_dictkey_defined (const gchar *name,
					 gchar **out_value,
					 GebrGeoXmlDocument *first_doc,
					 ...)
{
	g_return_val_if_fail (first_doc != NULL, FALSE);

	gboolean key_found = FALSE;

	va_list args;
	va_start (args, first_doc);
	GebrGeoXmlDocument *doc = first_doc;

	while (doc) {
		gchar *value;
		if (document_has_dictkey (doc, name, &value)) {
			key_found = TRUE;
			if (out_value)
				*out_value = value;
			break;
		}

		doc = va_arg(args, GebrGeoXmlDocument*);
	}
	va_end (args);

	return key_found;
}

gboolean
gebr_geoxml_document_validate_expr(const gchar *expr,
				   GebrGeoXmlDocument *flow,
				   GebrGeoXmlDocument *line,
				   GebrGeoXmlDocument *proj,
				   GError **err)
{
	gboolean success = TRUE;
	GHashTable *ht;

	if (*expr == '\0')
		return TRUE;

	ht = g_hash_table_new_full (g_str_hash,
				    g_str_equal,
				    g_free,
				    g_free);

	// Check if @expr is using any undefined variable
	GList *vars = gebr_expr_extract_vars (expr);

	GHashTable *var_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	gchar * hash_key = NULL;
	gint type = 0;

	GebrGeoXmlSequence *i = gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters(flow));
	for (; i != NULL; gebr_geoxml_sequence_next(&i)) {
		const gchar * key = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(i));
		type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(i));
		hash_key = g_strdup(key);
		g_hash_table_insert(var_hash, hash_key, GINT_TO_POINTER(type));
	}

	GRegex *regex;
	GMatchInfo *info;

	regex = g_regex_new ("^[a-z][a-z0-9_]*$", 0, 0, NULL);

	for (GList *i = vars; i; i = i->next) {
		gchar *value;
		gchar *name = i->data;

		g_regex_match (regex, name, 0, &info);
		if (!g_match_info_matches (info))
		{
			g_match_info_free (info);
			success = FALSE;
			g_set_error (err, gebr_expr_error_quark(),
				     GEBR_EXPR_ERROR_UNDEFINED_VAR,
				     _("Invalid name for variable %s"),
				     name);
			goto out;
		}
		g_match_info_free (info);

		if (g_hash_table_lookup_extended(var_hash, name, NULL, (gpointer)&type))
			if (type != GEBR_GEOXML_PARAMETER_TYPE_INT &&
			    type != GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
			{
				success = FALSE;
				g_set_error (err, gebr_expr_error_quark(),
					     GEBR_EXPR_ERROR_UNDEFINED_VAR,
					     "Incompatible type for variable %s", name);
				goto out;
			}

		if (!gebr_geoxml_document_is_dictkey_defined (name, &value,
							      flow, line, proj,
							      NULL))
		{
			success = FALSE;
			g_set_error (err, gebr_expr_error_quark(),
				     GEBR_EXPR_ERROR_UNDEFINED_VAR,
				     "Undefined variable name %s", name);
			goto out;
		}
		if(g_strcmp0(name,"iter") == 0)
			value = g_strdup("0");

		g_hash_table_insert (ht, g_strdup (name), value);
	}

	GError *error = NULL;
	GebrExpr *expression = gebr_expr_new (&error);

	// Check if expression is well-formed
	if (!expression) {
		g_propagate_error (err, error);
		success = FALSE;
		gebr_expr_free (expression);
		goto out;
	}

	void f(gchar *var_name, gchar *var_value) {
		if (error)
			return;
		gebr_expr_set_var (expression, var_name, var_value, &error);
	}

	g_hash_table_foreach (ht, (GHFunc) f, NULL);
	if (error) {
		g_propagate_error (err, error);
		success = FALSE;
		gebr_expr_free (expression);
		goto out;
	}

	success = gebr_expr_eval(expression, expr, NULL, &error);
	if (!success)
		g_propagate_error (err, error);
	gebr_expr_free(expression);

out:
	g_hash_table_destroy(var_hash);

	g_regex_unref (regex);
	g_hash_table_unref (ht);
	g_list_foreach (vars, (GFunc) g_free, NULL);
	g_list_free (vars);

	return success;
}

gboolean
gebr_geoxml_document_validate_str (const gchar *str,
				   GebrGeoXmlDocument *flow,
				   GebrGeoXmlDocument *line,
				   GebrGeoXmlDocument *proj,
				   GError **err)
{
	gboolean success = TRUE;
	GRegex *regex;
	GMatchInfo *info;

	if (*str == '\0')
		return TRUE;

	// Check if @str is using any undefined variable
	GList *vars = NULL; 
	GebrExprError error = gebr_str_expr_extract_vars (str, &vars);
	if (error == GEBR_EXPR_ERROR_NONE){

		regex = g_regex_new ("^[a-z][a-z0-9_]*$", 0, 0, NULL);

		for (GList *i = vars; i; i = i->next) {
			gchar *value;
			gchar *name = i->data;
			g_regex_match (regex, name, 0, &info);
			if (!g_match_info_matches (info))
			{
				success = FALSE;
				g_match_info_free (info);
				g_set_error (err, gebr_expr_error_quark(),
					     GEBR_EXPR_ERROR_UNDEFINED_VAR,
					     _("Invalid name for variable %s"),
					     name);
				goto out;
			}
			g_match_info_free (info);

			if (!gebr_geoxml_document_is_dictkey_defined (name, &value,
								      flow, line, proj,
								      NULL))
			{
				success = FALSE;
				g_set_error (err, gebr_expr_error_quark(),
					     GEBR_EXPR_ERROR_UNDEFINED_VAR,
					     "Undefined variable name %s", name);
				goto out;
			}
		}
	}
	else{
		GString * err_msg = g_string_new(NULL);

		switch(error){
		case GEBR_EXPR_ERROR_SYNTAX:
			g_string_assign(err_msg, "Syntax Error");
			break;

		case GEBR_EXPR_ERROR_EMPTY_VAR:
			g_string_assign(err_msg, "Empty Variable");
			break;

		case GEBR_EXPR_ERROR_STATE_UNKNOWN:
			g_string_assign(err_msg, "State Unknown");
			break;

		default:
			g_string_assign(err_msg, "Other Error");
			break;
		}

		success = FALSE;
		g_set_error_literal(err, gebr_expr_error_quark(),
				    error,
				    err_msg->str);
		g_string_free(err_msg, TRUE);
		goto out;
	}

out:
	g_regex_unref (regex);
	g_list_foreach (vars, (GFunc) g_free, NULL);
	g_list_free (vars);

	return success;
}

GebrGeoXmlSequence *
gebr_geoxml_document_get_dict_parameter(GebrGeoXmlDocument *doc)
{
	g_return_val_if_fail(doc != NULL, NULL);
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameters *params;

	params = gebr_geoxml_document_get_dict_parameters(doc);
	gebr_geoxml_parameters_get_parameter(params, &seq, 0);
	return seq;
}

GebrGeoXmlParameter *
gebr_geoxml_document_set_dict_keyword(GebrGeoXmlDocument *doc,
				      GebrGeoXmlParameterType type,
				      const gchar *keyword,
				      const gchar *value)
{
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *params;

	g_return_val_if_fail(doc != NULL, NULL);
	g_return_val_if_fail(keyword != NULL, NULL);
	g_return_val_if_fail(value != NULL, NULL);
	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_STRING ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_INT,
			     NULL);

	params = gebr_geoxml_document_get_dict_parameters(doc);
	param = gebr_geoxml_parameters_append_parameter(params, type);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param), keyword);
	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param),
						      FALSE, value);

	return param;
}

void gebr_geoxml_document_set_dtd_dir(const gchar *path)
{
	dtd_directory = path;
}
