/*
 * Copyright (c) 2008, 2009 by Internet Systems Consortium, Inc. ("ISC")
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

#ifndef NMSG_PRIVATE_H
#define NMSG_PRIVATE_H

#include "nmsg_port.h"

#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <zlib.h>

#include "nmsg.h"

#define NMSG_FRAG_GC_INTERVAL	30
#define NMSG_MSG_MODULE_PREFIX	"nmsg_msg"

typedef enum {
	nmsg_modtype_pbuf
} nmsg_modtype;

typedef enum {
	nmsg_stream_type_file,
	nmsg_stream_type_sock
} nmsg_stream_type;

typedef enum {
	nmsg_pcap_type_file,
	nmsg_pcap_type_live
} nmsg_pcap_type;

struct nmsg_buf;
struct nmsg_dlmod;
struct nmsg_frag;
struct nmsg_frag_tree;
struct nmsg_input;
struct nmsg_output;
struct nmsg_msgmod_clos;
struct nmsg_pcap;
struct nmsg_pres;
struct nmsg_stream_input;
struct nmsg_stream_output;

typedef nmsg_res (*nmsg_input_read_fp)(struct nmsg_input *,
				       Nmsg__NmsgPayload **);
typedef nmsg_res (*nmsg_input_read_loop_fp)(struct nmsg_input *, int,
					    nmsg_cb_payload, void *);
typedef nmsg_res (*nmsg_output_write_fp)(struct nmsg_output *,
					 Nmsg__NmsgPayload *);

/* nmsg_frag: used by nmsg_stream_input */
struct nmsg_frag {
	RB_ENTRY(nmsg_frag)	link;
	uint32_t		id;
	unsigned		last;
	unsigned		rem;
	struct timespec		ts;
	ProtobufCBinaryData	*frags;
};

/* nmsg_frag_tree: used by nmsg_stream_input */
struct nmsg_frag_tree {
	RB_HEAD(frag_ent, nmsg_frag)  head;
};

/* nmsg_buf: used by nmsg_stream_input, nmsg_stream_output */
struct nmsg_buf {
	int			fd;
	size_t			bufsz;
	u_char			*data;	/* allocated data starts here */
	u_char			*pos;	/* position of next buffer read */
	u_char			*end;	/* one byte beyond valid data */
};

/* nmsg_pcap: used by nmsg_input */
struct nmsg_pcap {
	int			datalink;
	pcap_t			*handle;
	struct nmsg_ipreasm	*reasm;
	u_char			*new_pkt;

	pcap_t			*user;
	char			*userbpft;
	struct bpf_program	userbpf;

	nmsg_pcap_type		type;
};

/* nmsg_pres: used by nmsg_input and nmsg_output */
struct nmsg_pres {
	pthread_mutex_t		lock;
	FILE			*fp;
	bool			flush;
	nmsg_msgmodset_t	ms;
	char			*endline;
};

/* nmsg_stream_input: used by nmsg_input */
struct nmsg_stream_input {
	nmsg_stream_type	type;
	struct nmsg_buf		*buf;
	Nmsg__Nmsg		*nmsg;
	unsigned		np_index;
	struct nmsg_frag_tree	nft;
	struct pollfd		pfd;
	struct timespec		now;
	struct timespec		lastgc;
	unsigned		nfrags;
	unsigned		flags;
	nmsg_zbuf_t		zb;
	u_char			*zb_tmp;
	unsigned		source;
	unsigned		operator;
	unsigned		group;
};

/* nmsg_stream_output: used by nmsg_output */
struct nmsg_stream_output {
	pthread_mutex_t		lock;
	nmsg_stream_type	type;
	struct nmsg_buf		*buf;
	Nmsg__Nmsg		*nmsg;
	size_t			estsz;
	nmsg_rate_t		rate;
	bool			buffered;
	nmsg_zbuf_t		zb;
	u_char			*zb_tmp;
	unsigned		source;
	unsigned		operator;
	unsigned		group;
};

/* nmsg_callback_output: used by nmsg_output */
struct nmsg_callback_output {
	nmsg_cb_payload		cb;
	void			*user;
};

/* nmsg_input */
struct nmsg_input {
	nmsg_input_type		type;
	nmsg_msgmod_t		msgmod;
	void			*clos;
	union {
		struct nmsg_stream_input  *stream;
		struct nmsg_pcap	  *pcap;
		struct nmsg_pres	  *pres;
	};
	nmsg_input_read_fp	read_fp;
	nmsg_input_read_loop_fp	read_loop_fp;
};

/* nmsg_output */
struct nmsg_output {
	nmsg_output_type	type;
	union {
		struct nmsg_stream_output	*stream;
		struct nmsg_pres		*pres;
		struct nmsg_callback_output	*callback;
	};
	nmsg_output_write_fp	write_fp;
};

/* messages */
struct nmsg_message {
	nmsg_msgmod_t		mod;
	ProtobufCMessage	*message;
	Nmsg__NmsgPayload	*payload;
};

/* dlmod / msgmod / msgmodset */

struct nmsg_dlmod {
	ISC_LINK(struct nmsg_dlmod)	link;
	nmsg_modtype			type;
	char				*path;
	void				*handle;
};

typedef enum nmsg_msgmod_clos_mode {
	nmsg_msgmod_clos_m_keyval,
	nmsg_msgmod_clos_m_multiline
} nmsg_msgmod_clos_mode;

struct nmsg_msgmod_clos {
	char			*nmsg_pbuf;
	size_t			estsz;
	nmsg_msgmod_clos_mode	mode;
	struct nmsg_msgmod_field	*field;
	struct nmsg_strbuf	*strbufs;
};

/* from buf.c */

struct nmsg_buf *
_nmsg_buf_new(size_t sz);

ssize_t
_nmsg_buf_used(struct nmsg_buf *buf);

ssize_t
_nmsg_buf_avail(struct nmsg_buf *buf);

void
_nmsg_buf_destroy(struct nmsg_buf **buf);

void
_nmsg_buf_reset(struct nmsg_buf *buf);

/* from dlmod.c */

struct nmsg_dlmod *
_nmsg_dlmod_init(const char *path);

void
_nmsg_dlmod_destroy(struct nmsg_dlmod **dlmod);

/* from msgmod.c */

nmsg_res
_nmsg_msgmod_start(struct nmsg_msgmod *mod);

/* from message.c */
nmsg_res
_nmsg_message_serialize(struct nmsg_message *msg);

nmsg_res
_nmsg_message_init_payload(struct nmsg_message *msg);

#endif /* NMSG_PRIVATE_H */
