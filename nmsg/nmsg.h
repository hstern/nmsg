#ifndef NMSG_H
#define NMSG_H

/*
 * Copyright (c) 2008 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*****
 ***** Module Info
 *****/

/*! \file nmsg.h
 * \brief Base nmsg support header.
 *
 * This header ensures that needed constants, protobuf functions, result
 * codes, vendor definitions, and opaque pointer types are defined.
 */

#ifdef __cplusplus
extern "C" {
#endif

/***
 *** Types
 ***/

typedef struct nmsg_buf *nmsg_buf;
typedef struct nmsg_fma *nmsg_fma;
typedef struct nmsg_io *nmsg_io;
typedef struct nmsg_pbmod *nmsg_pbmod;
typedef struct nmsg_pbmodset *nmsg_pbmodset;
typedef struct nmsg_pcap *nmsg_pcap;
typedef struct nmsg_pres *nmsg_pres;
typedef struct nmsg_rate *nmsg_rate;
typedef struct nmsg_reasm_ip *nmsg_ipreasm;
typedef struct nmsg_zbuf *nmsg_zbuf;

typedef enum nmsg_res nmsg_res;

struct nmsg_idname {
	unsigned	id;	/*%< ID number */
	const char	*name;	/*%< Human readable name */
};

/***
 *** Imports
 ***/

#include <nmsg/protobuf-c.h>
#include <nmsg/nmsg.pb-c.h>

#include <nmsg/asprintf.h>
#include <nmsg/constants.h>
#include <nmsg/fma.h>
#include <nmsg/input.h>
#include <nmsg/io.h>
#include <nmsg/ipdg.h>
#include <nmsg/ipreasm.h>
#include <nmsg/output.h>
#include <nmsg/payload.h>
#include <nmsg/pbmod.h>
#include <nmsg/pbmodset.h>
#include <nmsg/rate.h>
#include <nmsg/res.h>
#include <nmsg/strbuf.h>
#include <nmsg/tree.h>
#include <nmsg/ts.h>
#include <nmsg/vendors.h>
#include <nmsg/zbuf.h>

/***
 *** Constants
 ***/

/*%
 * Sentinel at the end of an nmsg_idname array.
 */
#define NMSG_IDNAME_END		{ 0, NULL }

#ifdef __cplusplus
}
#endif

#endif /* NMSG_H */
