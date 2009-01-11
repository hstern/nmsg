/*
 * Copyright (c) 2009 by Internet Systems Consortium, Inc. ("ISC")
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

/* Import. */

#include "nmsg_port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <zlib.h>

#include "private.h"
#include "zbuf.h"

nmsg_zbuf
nmsg_zbuf_deflate_init(void) {
	int zret;
	struct nmsg_zbuf *zb;

	zb = malloc(sizeof(*zb));
	if (zb == NULL)
		return (NULL);

	zb->type = nmsg_zbuf_type_deflate;
	zb->zs.zalloc = Z_NULL;
	zb->zs.zfree = Z_NULL;
	zb->zs.opaque = Z_NULL;

	zret = deflateInit(&zb->zs, Z_DEFAULT_COMPRESSION);
	if (zret != Z_OK) {
		free(zb);
		return (NULL);
	}

	return (zb);
}

nmsg_zbuf
nmsg_zbuf_inflate_init(void) {
	int zret;
	struct nmsg_zbuf *zb;

	zb = malloc(sizeof(*zb));
	if (zb == NULL)
		return (NULL);

	zb->type = nmsg_zbuf_type_inflate;
	zb->zs.zalloc = Z_NULL;
	zb->zs.zfree = Z_NULL;
	zb->zs.opaque = Z_NULL;
	zb->zs.avail_in = 0;
	zb->zs.next_in = Z_NULL;

	zret = inflateInit(&zb->zs);
	if (zret != Z_OK) {
		free(zb);
		return (NULL);
	}

	return (zb);
}

void
nmsg_zbuf_destroy(nmsg_zbuf *zb) {
	if ((*zb)->type == nmsg_zbuf_type_deflate)
		deflateEnd(&(*zb)->zs);
	else if ((*zb)->type == nmsg_zbuf_type_inflate)
		inflateEnd(&(*zb)->zs);
	free(*zb);
	*zb = NULL;
}

nmsg_res
nmsg_zbuf_deflate(nmsg_zbuf zb, size_t len, void *buf,
		  size_t *zlen, void **zbuf)
{
	int zret;
	size_t size, rem;
	unsigned char *output;

	size = len + 64;
	rem = size & 63;
	if (rem != 0)
		size += (64 - rem);
	output = malloc(size);
	if (output == NULL)
		return (nmsg_res_memfail);

	zb->zs.avail_in = len;
	zb->zs.next_in = buf;
	zb->zs.avail_out = size;
	zb->zs.next_out = output;

	zret = deflate(&zb->zs, Z_FINISH);
	assert(zret == Z_STREAM_END);
	assert(zb->zs.avail_in == 0);
	*zlen = size - zb->zs.avail_out;
	assert(deflateReset(&zb->zs) == Z_OK);

	fprintf(stderr, "zlen=%zd\n", *zlen);
	*zbuf = output;

	return (nmsg_res_success);
}
