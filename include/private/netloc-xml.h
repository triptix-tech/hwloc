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

#ifndef _NETLOC_XML_PRIVATE_H_
#define _NETLOC_XML_PRIVATE_H_

#include <netloc.h>
#include <private/netloc.h>

/**
 * Load the netloc machine as described in the xml file pointed by \ref path.
 *
 * On error, *pmachine is not set.
 *
 * \param path A valid path to the XML file describing the requested topology
 * \param pmachine A valid reference to an unitialized machine pointer
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR_NOENT if \ref path is not valid
 * \returns NETLOC_ERROR on error
 */
extern int
netloc_machine_xml_load(const char *path, netloc_machine_t **pmachine);

/**
 * Check whether warnings and errors should be reported in stderr in
 * addition to the return codes. If the build is not for in debug
 * mode, messages are displayed in stderr, except HWLOC_XML_VERBOSE
 * environment variable is set to 0. For standard build, the messages
 * are displayed if HWLOC_XML_VERBOSE environment variable is set to
 * 1. This value is checked once. Modifying this value during the
 * execution of the code will not change the behaviour of the library.
 *
 * \returns 1 if error and warning messages should be displayed
 * \returns 0 otherwise
 */
extern int netloc__xml_verbose(void);

#endif /* _NETLOC_XML_PRIVATE_H_ */