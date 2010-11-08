/*
 * Copyright (c) 2010 by Internet Systems Consortium, Inc. ("ISC")
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
#include "nmsg_port_net.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdlib.h>

#include "nmsg.h"
#include "private.h"

/* Export. */

nmsg_res
nmsg_sock_parse(int af, const char *addr, unsigned port,
		struct sockaddr_in *sai,
		struct sockaddr_in6 *sai6,
		struct sockaddr **sa,
		socklen_t *salen)
{
	switch (af) {

	case AF_INET:
		if (!inet_pton(AF_INET, addr, &sai->sin_addr))
			return (nmsg_res_parse_error);

		sai->sin_family = AF_INET;
		sai->sin_port = htons(port);
#ifdef HAVE_SA_LEN
		sai->sin_len = sizeof(struct sockaddr_in);
#endif
		*sa = (struct sockaddr *) sai;
		*salen = sizeof(struct sockaddr_in);
		break;

	case AF_INET6:
		if (!inet_pton(AF_INET6, addr, &sai6->sin6_addr))
			return (nmsg_res_parse_error);

		sai6->sin6_family = AF_INET6;
		sai6->sin6_port = htons(port);
#ifdef HAVE_SA_LEN
		sai6->sin6_len = sizeof(struct sockaddr_in6);
#endif
		*sa = (struct sockaddr *) sai6;
		*salen = sizeof(struct sockaddr_in6);
		break;

	default:
		assert(af != AF_INET && af != AF_INET6);
	}

	return (nmsg_res_success);
}

nmsg_res
nmsg_sock_parse_sockspec(const char *sockspec, int *af, char **addr,
			 unsigned *port_start, unsigned *port_end)
{
	Ustr *sock;
	Ustr *sock_addr = USTR_NULL;
	Ustr *sock_port = USTR_NULL;
	Ustr *sock_port_end = USTR_NULL;
	Ustr *sock_port_start = USTR_NULL;
	nmsg_res res;
	size_t offset = 0;
	uint8_t buf[16];
	unsigned ern = 0;

	res = nmsg_res_failure;

	sock = ustr_dup_cstr(sockspec);

	/* tokenize socket address */
	sock_addr = ustr_split_cstr(sock, &offset, "/", USTR_NULL, 0);
	if (sock_addr == USTR_NULL)
		goto out;

	/* parse socket address */
	if (inet_pton(AF_INET6, ustr_cstr(sock_addr), buf)) {
		*af = AF_INET6;
		*addr = strdup(ustr_cstr(sock_addr));
		if (*addr == NULL)
			goto out;
	} else if (inet_pton(AF_INET, ustr_cstr(sock_addr), buf)) {
		*af = AF_INET;
		*addr = strdup(ustr_cstr(sock_addr));
		if (*addr == NULL)
			goto out;
	}

	/* tokenize socket port range */
	sock_port = ustr_split_cstr(sock, &offset, "/", USTR_NULL, 0);
	if (sock_port == USTR_NULL)
		goto out;

	/* parse socket port range */
	offset = 0;
	sock_port_start = ustr_split_cstr(sock_port, &offset, "..", USTR_NULL, 0);
	if (sock_port == USTR_NULL)
		goto out;
	sock_port_end = ustr_split_cstr(sock_port, &offset, "..", USTR_NULL, 0);

	if (sock_port_start != USTR_NULL && sock_port_end != USTR_NULL) {
		/* socket port range consists of multiple ports */
		*port_start = ustr_parse_uint(sock_port_start, 0, 10, 0, &ern);
		if (ern)
			goto out;
		*port_end = ustr_parse_uint(sock_port_end, 0, 10, 0, &ern);
		if (ern)
			goto out;
	} else {
		/* socket port range is a single port */
		*port_start = ustr_parse_uint(sock_port, 0, 10, 0, &ern);
		*port_end = *port_start;
		if (ern)
			goto out;
	}

	/* parsed successfully */
	res = nmsg_res_success;

out:
	ustr_free(sock_port_end);
	ustr_free(sock_port_start);
	ustr_free(sock_port);
	ustr_free(sock_addr);
	ustr_free(sock);

	return (res);
}
