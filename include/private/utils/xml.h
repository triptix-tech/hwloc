/* -*- encoding: utf-8 -*- */
/*
 * Copyright © 2017-2018 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#ifndef _UTILS_XML_H_
#define _UTILS_XML_H_

#include <netloc.h>
#include <private/netloc.h>

#ifdef HWLOC_HAVE_LIBXML2

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

typedef xmlNsPtr xml_ns_ptr;

typedef xmlChar xml_char;

typedef xmlNodePtr xml_node_ptr;

typedef xmlDocPtr xml_doc_ptr;

#define xml_parser_cleanup()                    \
    do {                                        \
        if (getenv("HWLOC_LIBXML_CLEANUP"))     \
            xmlCleanupParser();                 \
    } while (0)

#define XML_LIB_CHECK_VERSION LIBXML_TEST_VERSION

#else

/* XML writing is made with netloc/xml.c and netloc/nolibxml.c */

typedef void * xml_ns_ptr;

typedef char xml_char;
#define BAD_CAST (xml_char *)

struct xml_node_t;
typedef struct xml_node_t * xml_node_ptr;

struct xml_doc_t;
typedef struct xml_doc_t * xml_doc_ptr;

#define xml_parser_cleanup()  do { /* do nothing */ } while (0)

#define XML_LIB_CHECK_VERSION do { /* do nothing */ } while (0)

/******************************************************************************/
/* Private API for XML file import */
xml_doc_ptr xml_node_read_file(const char *path);
xml_node_ptr xml_doc_get_root_element(const xml_doc_ptr doc);
char *xml_node_get_name(const xml_node_ptr node);
size_t xml_node_get_num_children(const xml_node_ptr node);
int xml_node_has_child_data(const xml_node_ptr node);
xml_node_ptr xml_node_get_child(const xml_node_ptr node, const unsigned int idx);
xml_char *xml_node_get_content(const xml_node_ptr node);
char *xml_node_attr_get(xml_node_ptr node, const char *attrname);

#endif /* HWLOC_HAVE_LIBXML2 */

/******************************************************************************/
/* Public API for XML file export */

extern void xml_char_free(xml_char *string);

extern xml_char *xml_char_strdup(const char *string);

extern xml_node_ptr
xml_node_new(xml_ns_ptr namespace __netloc_attribute_unused,
             const xml_char *name);

extern void xml_node_free(xml_node_ptr node);

extern void xml_node_attr_add(xml_node_ptr node, const xml_char *name,
                              const xml_char *value);

extern void xml_node_attr_cpy_add(xml_node_ptr pnode,
                                  const xml_char *name,
                                  const char *value);

extern void xml_node_child_add(xml_node_ptr parent, xml_node_ptr child);

extern xml_node_ptr
xml_node_child_new(xml_node_ptr parent,
                   xml_ns_ptr namespace __netloc_attribute_unused,
                   const xml_char *name, const xml_char *content);

extern void xml_node_merge(xml_node_ptr dest, xml_node_ptr src);

extern int xml_node_has_child(const xml_node_ptr node);

extern xml_doc_ptr xml_doc_new(const xml_char *version);

extern void xml_doc_free(xml_doc_ptr doc);

extern xml_node_ptr xml_doc_set_root_element(xml_doc_ptr doc,
                                             const xml_node_ptr node);

extern int
xml_doc_write(const char *outpath, const xml_doc_ptr doc, const char *enc,
              const int format __netloc_attribute_unused);

extern void
xml_dtd_subset_create(xml_doc_ptr doc, const xml_char *name,
                      const xml_char *externid __netloc_attribute_unused,
                      const xml_char *systemid);

/******************************************************************************/
/* Public API for XML file import */

extern xml_doc_ptr xml_reader_init(const char *path);

extern int xml_reader_clean_and_out(xml_doc_ptr doc);

#endif /* _UTILS_XML_H_ */