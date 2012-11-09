/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2012 Inria.  All rights reserved.
 * Copyright © 2009-2011 Université Bordeaux 1
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/xml.h>
#include <private/private.h>
#include <private/misc.h>
#include <private/debug.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

/*******************
 * Common routines *
 *******************/

static void hwloc_libxml2_error_callback(void * ctx __hwloc_attribute_unused, const char * msg __hwloc_attribute_unused, ...) { /* do nothing */ }

static void
hwloc_libxml2_disable_stderrwarnings(void)
{
  static int first = 1;
  if (first) {
    xmlSetGenericErrorFunc(NULL, hwloc__xml_verbose() ? xmlGenericError : hwloc_libxml2_error_callback);
    first = 0;
  }
}

/*******************
 * Import routines *
 *******************/

typedef struct hwloc__libxml_import_state_data_s {
  xmlNode *node; /* current libxml node, always valid */
  xmlNode *child; /* last processed child, or NULL if none yet */
  xmlAttr *attr; /* last processed attribute, or NULL if none yet */
} * hwloc__libxml_import_state_data_t;

static int
hwloc__libxml_import_next_attr(hwloc__xml_import_state_t state, char **namep, char **valuep)
{
  hwloc__libxml_import_state_data_t lstate = (void*) state->data;

  xmlAttr *attr;
  if (lstate->attr)
    attr = lstate->attr->next;
  else
    attr = lstate->node->properties;
  for (; attr; attr = attr->next)
    if (attr->type == XML_ATTRIBUTE_NODE) {
      /* use the first valid attribute content */
      xmlNode *subnode;
      for (subnode = attr->children; subnode; subnode = subnode->next) {
	if (subnode->type == XML_TEXT_NODE) {
	  if (subnode->content && subnode->content[0] != '\0' && subnode->content[0] != '\n') {
	    *namep = (char *) attr->name;
	    *valuep = (char *) subnode->content;
	    lstate->attr = attr;
	    return 0;
	  }
	} else {
	  if (hwloc__xml_verbose())
	    fprintf(stderr, "ignoring unexpected xml attr node type %u\n", subnode->type);
	}
      }
    } else {
      if (hwloc__xml_verbose())
	fprintf(stderr, "ignoring unexpected xml attr type %u\n", attr->type);
    }
  return -1;
}

static int
hwloc__libxml_import_find_child(hwloc__xml_import_state_t state,
				hwloc__xml_import_state_t childstate,
				char **tagp)
{
  hwloc__libxml_import_state_data_t lstate = (void*) state->data;
  hwloc__libxml_import_state_data_t lchildstate = (void*) childstate->data;
  xmlNode *child;
  childstate->parent = state;
  childstate->next_attr = state->next_attr;
  childstate->find_child = state->find_child;
  childstate->close_tag = state->close_tag;
  childstate->close_child = state->close_child;
  childstate->get_content = state->get_content;
  childstate->close_content = state->close_content;
  if (!lstate->child)
    return 0;
  child = lstate->child->next;
  for (; child; child = child->next)
    if (child->type == XML_ELEMENT_NODE) {
      lstate->child = lchildstate->node = child;
      lchildstate->child = child->children;
      lchildstate->attr = NULL;
      *tagp = (char*) child->name;
      return 1;
    } else if (child->type == XML_TEXT_NODE) {
      if (child->content && child->content[0] != '\0' && child->content[0] != '\n')
	if (hwloc__xml_verbose())
	  fprintf(stderr, "ignoring object text content %s\n", (const char*) child->content);
    } else if (child->type != XML_COMMENT_NODE) {
      if (hwloc__xml_verbose())
	fprintf(stderr, "ignoring unexpected xml node type %u\n", child->type);
    }

  return 0;
}

static int
hwloc__libxml_import_close_tag(hwloc__xml_import_state_t state __hwloc_attribute_unused)
{
  return 0;
}

static void
hwloc__libxml_import_close_child(hwloc__xml_import_state_t state __hwloc_attribute_unused)
{
  /* nothing to do */
}

static int
hwloc__libxml_import_get_content(hwloc__xml_import_state_t state,
				 char **beginp, size_t expected_length)
{
  hwloc__libxml_import_state_data_t lstate = (void*) state->data;
  xmlNode *child;
  size_t length;

  child = lstate->node->children;
  if (!child)
    return 0;
  if (child->type != XML_TEXT_NODE)
    return 0;

  length = strlen((char *) child->content);
  if (length != expected_length)
    return -1;
  *beginp = (char *) child->content;
  return 1;
}

static void
hwloc__libxml_import_close_content(hwloc__xml_import_state_t state __hwloc_attribute_unused)
{
  /* nothing to do */
}

static int
hwloc_libxml_look_init(struct hwloc_xml_backend_data_s *bdata,
		       struct hwloc__xml_import_state_s *state)
{
  hwloc__libxml_import_state_data_t lstate = (void*) state->data;
  xmlNode* root_node;
  xmlDtd *dtd;

  assert(sizeof(*lstate) <= sizeof(state->data));

  dtd = xmlGetIntSubset((xmlDoc*) bdata->data);
  if (!dtd) {
    if (hwloc__xml_verbose())
      fprintf(stderr, "Loading XML topology without DTD\n");
  } else if (strcmp((char *) dtd->SystemID, "hwloc.dtd")) {
    if (hwloc__xml_verbose())
      fprintf(stderr, "Loading XML topology with wrong DTD SystemID (%s instead of %s)\n",
	      (char *) dtd->SystemID, "hwloc.dtd");
  }

  root_node = xmlDocGetRootElement((xmlDoc*) bdata->data);

  if (strcmp((const char *) root_node->name, "topology") && strcmp((const char *) root_node->name, "root")) {
    /* root node should be in "topology" class (or "root" if importing from < 1.0) */
    if (hwloc__xml_verbose())
      fprintf(stderr, "ignoring object of class `%s' not at the top the xml hierarchy\n", (const char *) root_node->name);
    goto failed;
  }

  state->next_attr = hwloc__libxml_import_next_attr;
  state->find_child = hwloc__libxml_import_find_child;
  state->close_tag = hwloc__libxml_import_close_tag;
  state->close_child = hwloc__libxml_import_close_child;
  state->get_content = hwloc__libxml_import_get_content;
  state->close_content = hwloc__libxml_import_close_content;
  state->parent = NULL;
  lstate->node = root_node;
  lstate->child = root_node->children;
  lstate->attr = NULL;
  return 0; /* success */

 failed:
  return -1; /* failed */
}

/********************
 * Backend routines *
 ********************/

static void
hwloc_libxml_backend_exit(struct hwloc_xml_backend_data_s *bdata)
{
  xmlFreeDoc((xmlDoc*)bdata->data);
}

int
hwloc_libxml_backend_init(struct hwloc_xml_backend_data_s *bdata,
			  const char *xmlpath, const char *xmlbuffer, int xmlbuflen)
{
  xmlDoc *doc = NULL;

  LIBXML_TEST_VERSION;
  hwloc_libxml2_disable_stderrwarnings();

  errno = 0; /* set to 0 so that we know if libxml2 changed it */

  if (xmlpath)
    doc = xmlReadFile(xmlpath, NULL, 0);
  else if (xmlbuffer)
    doc = xmlReadMemory(xmlbuffer, xmlbuflen, "", NULL, 0);

  if (!doc) {
    if (!errno)
      /* libxml2 read the file fine, but it got an error during parsing */
    errno = EINVAL;
    return -1;
  }

  bdata->look_init = hwloc_libxml_look_init;
  bdata->look_failed = NULL;
  bdata->backend_exit = hwloc_libxml_backend_exit;
  bdata->data = doc;
  return 0;
}

/*******************
 * Export routines *
 *******************/

typedef struct hwloc__libxml_export_state_data_s {
  xmlNodePtr current_node; /* current node to output */
} * hwloc__libxml_export_state_data_t;

static void
hwloc__libxml_export_new_child(hwloc__xml_export_state_t parentstate,
			       hwloc__xml_export_state_t state,
			       const char *name)
{
  hwloc__libxml_export_state_data_t lpdata = (void *) parentstate->data;
  hwloc__libxml_export_state_data_t ldata = (void *) state->data;

  state->parent = parentstate;
  state->new_child = parentstate->new_child;
  state->new_prop = parentstate->new_prop;
  state->add_content = parentstate->add_content;
  state->end_object = parentstate->end_object;

  ldata->current_node = xmlNewChild(lpdata->current_node, NULL, BAD_CAST name, NULL);
}

static void
hwloc__libxml_export_new_prop(hwloc__xml_export_state_t state, const char *name, const char *value)
{
  hwloc__libxml_export_state_data_t ldata = (void *) state->data;
  xmlNewProp(ldata->current_node, BAD_CAST name, BAD_CAST value);
}

static void
hwloc__libxml_export_end_object(hwloc__xml_export_state_t state __hwloc_attribute_unused, const char *name __hwloc_attribute_unused)
{
  /* nothing to do */
}

static void
hwloc__libxml_export_add_content(hwloc__xml_export_state_t state, const char *buffer, size_t length)
{
  hwloc__libxml_export_state_data_t ldata = (void *) state->data;
  xmlNodeAddContentLen(ldata->current_node, BAD_CAST buffer, length);
}

static xmlDocPtr
hwloc__libxml2_prepare_export(hwloc_topology_t topology)
{
  struct hwloc__xml_export_state_s state;
  hwloc__libxml_export_state_data_t data = (void *) state.data;
  xmlDocPtr doc = NULL;       /* document pointer */
  xmlNodePtr root_node = NULL; /* root pointer */

  assert(sizeof(*data) <= sizeof(state.data));

  LIBXML_TEST_VERSION;
  hwloc_libxml2_disable_stderrwarnings();

  /* Creates a new document, a node and set it as a root node. */
  doc = xmlNewDoc(BAD_CAST "1.0");
  root_node = xmlNewNode(NULL, BAD_CAST "topology");
  xmlDocSetRootElement(doc, root_node);

  /* Creates a DTD declaration. Isn't mandatory. */
  (void) xmlCreateIntSubset(doc, BAD_CAST "topology", NULL, BAD_CAST "hwloc.dtd");

  state.new_child = hwloc__libxml_export_new_child;
  state.new_prop = hwloc__libxml_export_new_prop;
  state.add_content = hwloc__libxml_export_add_content;
  state.end_object = hwloc__libxml_export_end_object;

  data->current_node = root_node;

  hwloc__xml_export_object (&state, topology, hwloc_get_root_obj(topology));

  return doc;
}

int
hwloc_libxml_export_file(hwloc_topology_t topology, const char *filename)
{
  xmlDocPtr doc;
  int ret;

  errno = 0; /* set to 0 so that we know if libxml2 changed it */

  doc = hwloc__libxml2_prepare_export(topology);
  ret = xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
  xmlFreeDoc(doc);

  if (ret < 0) {
    if (!errno)
      /* libxml2 likely got an error before doing I/O */
      errno = EINVAL;
    return ret;
  }
  return 0;
}

int
hwloc_libxml_export_buffer(hwloc_topology_t topology, char **xmlbuffer, int *buflen)
{
  xmlDocPtr doc = hwloc__libxml2_prepare_export(topology);
  xmlDocDumpFormatMemoryEnc(doc, (xmlChar **)xmlbuffer, buflen, "UTF-8", 1);
  xmlFreeDoc(doc);
  return 0;
}

void
hwloc_libxml_free_buffer(void *xmlbuffer)
{
  xmlFree(BAD_CAST xmlbuffer);
}
