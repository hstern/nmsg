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

/* Import. */

#include "nmsg_port.h"

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nmsg/pbmod.h>
#include <nmsg/private.h>
#include <nmsg/res.h>
#include <nmsg/strbuf.h>

/* Macros. */

#define PBFIELD(pbuf, field, type) \
	((type *) &((char *) pbuf)[field->descr->offset])

#define PBFIELD_HAS(pbuf, field) \
	((protobuf_c_boolean *) &((char *) pbuf)[field->descr->quantifier_offset])

#define PBFIELD_Q(pbuf, field) \
	((int *) &((char *) pbuf)[field->descr->quantifier_offset])

#define PBFIELD_ONE_PRESENT(pbuf, field) \
	(field->descr->label == PROTOBUF_C_LABEL_REQUIRED || \
	 (field->descr->label == PROTOBUF_C_LABEL_OPTIONAL && \
	  *PBFIELD_HAS(pbuf, field) == true))

#define PBFIELD_REPEATED(field) \
	(field->descr->label == PROTOBUF_C_LABEL_REPEATED)

#define LINECMP(line, str) (strncmp(line, str, sizeof(str) - 1) == 0)

/* Forward. */

static bool is_automatic_pbmod(struct nmsg_pbmod *mod);
static void load_field_descriptors(struct nmsg_pbmod *mod);
static nmsg_res module_init(struct nmsg_pbmod *mod, void **clos);
static nmsg_res module_fini(struct nmsg_pbmod *mod, void **clos);
static nmsg_res module_pbuf_to_pres(struct nmsg_pbmod *mod,
				    Nmsg__NmsgPayload *np, char **pres,
				    const char *endline);
static nmsg_res module_pbuf_field_to_pres(struct nmsg_pbmod_field *field,
					  void *ptr,
					  struct nmsg_strbuf *sb,
					  const char *endline);
static nmsg_res module_pres_to_pbuf(struct nmsg_pbmod *mod, void *clos,
				    const char *pres);
static nmsg_res pres_to_pbuf_load(struct nmsg_pbmod_field *field,
				  struct nmsg_pbmod_clos *clos,
				  const char *value, void *ptr, int *qptr);
static nmsg_res module_pres_to_pbuf_finalize(struct nmsg_pbmod *mod, void *clos,
					     uint8_t **pbuf, size_t *sz);
static inline size_t sizeof_elt_in_repeated_array (ProtobufCType type);

/* Export. */

nmsg_res
nmsg_pbmod_init(struct nmsg_pbmod *mod, void **clos, int debug) {
	if (is_automatic_pbmod(mod)) {
		return (module_init(mod, clos));
	} else if (mod->init != NULL) {
		return (mod->init(clos, debug));
	} else {
		return (nmsg_res_notimpl);
	}
}

nmsg_res
nmsg_pbmod_fini(struct nmsg_pbmod *mod, void **clos) {
	if (is_automatic_pbmod(mod)) {
		return (module_fini(mod, clos));
	} else if (mod->fini != NULL) {
		return (mod->fini(clos));
	} else {
		return (nmsg_res_notimpl);
	}
}

nmsg_res
nmsg_pbmod_pbuf2pres(struct nmsg_pbmod *mod, Nmsg__NmsgPayload *np, char **pres,
		     const char *endline)
{
	if (is_automatic_pbmod(mod)) {
		return (module_pbuf_to_pres(mod, np, pres, endline));
	} else if (mod->pbuf2pres != NULL) {
		return (mod->pbuf2pres(np, pres, endline));
	} else {
		return (nmsg_res_notimpl);
	}
}

nmsg_res
nmsg_pbmod_pres2pbuf(struct nmsg_pbmod *mod, void *clos, const char *pres) {
	if (is_automatic_pbmod(mod)) {
		return (module_pres_to_pbuf(mod, clos, pres));
	} else if (mod->pres2pbuf != NULL) {
		return (mod->pres2pbuf(clos, pres));
	} else {
		return (nmsg_res_notimpl);
	}
}

nmsg_res
nmsg_pbmod_pres2pbuf_finalize(struct nmsg_pbmod *mod, void *clos,
			      uint8_t **pbuf, size_t *sz)
{
	if (is_automatic_pbmod(mod)) {
		return (module_pres_to_pbuf_finalize(mod, clos, pbuf, sz));
	} else if (mod->pres2pbuf_finalize != NULL) {
		return (mod->pres2pbuf_finalize(clos, pbuf, sz));
	} else {
		return (nmsg_res_notimpl);
	}
}

nmsg_res
nmsg_pbmod_field2pbuf(struct nmsg_pbmod *mod, void *clos, const char *field,
		      const uint8_t *val, size_t len, uint8_t **pbuf,
		      size_t *sz)
{
	if (mod->field2pbuf != NULL)
		return (mod->field2pbuf(clos, field, val, len, pbuf, sz));
	else
		return (nmsg_res_notimpl);

}

/* Internal use. */

nmsg_res
_nmsg_pbmod_start(struct nmsg_pbmod *mod) {
	if (is_automatic_pbmod(mod)) {
		/* lookup field descriptors if necessary */
		if (mod->fields[0].descr == NULL)
			load_field_descriptors(mod);
	}

	return (nmsg_res_success);
}

/* Private. */

static nmsg_res
module_init(struct nmsg_pbmod *mod, void **cl) {
	struct nmsg_pbmod_clos **clos = (struct nmsg_pbmod_clos **) cl;

	/* allocate the closure */
	*clos = calloc(1, sizeof(struct nmsg_pbmod_clos));
	if (*clos == NULL) {
		return (nmsg_res_memfail);
	}

	/* allocate space for pointers to multiline buffers */
	(*clos)->strbufs = calloc(1, (sizeof(struct nmsg_strbuf)) *
				  mod->pbdescr->n_fields);
	if ((*clos)->strbufs == NULL) {
		free(*clos);
		return (nmsg_res_memfail);
	}

	return (nmsg_res_success);
}

static nmsg_res
module_fini(struct nmsg_pbmod *mod, void **cl) {
	struct nmsg_pbmod_clos **clos = (struct nmsg_pbmod_clos **) cl;
	struct nmsg_pbmod_field *field;

	/* deallocate serialized message buffer */
	free((*clos)->nmsg_pbuf);

	/* deallocate multiline buffers */
	for (field = mod->fields; field->descr != NULL; field++) {
		if (field->type == nmsg_pbmod_ft_mlstring) {
			struct nmsg_strbuf *sb;

			sb = &(*clos)->strbufs[field->descr->id - 1];
			free(sb->data);
		}
	}

	/* deallocate multiline buffer pointers */
	free((*clos)->strbufs);

	/* deallocate closure */
	free(*clos);
	*clos = NULL;

	return (nmsg_res_success);
}

static nmsg_res
module_pbuf_to_pres(struct nmsg_pbmod *mod, Nmsg__NmsgPayload *np, char **pres,
		    const char *endline)
{
	ProtobufCMessage *m;
	nmsg_res res;
	struct nmsg_pbmod_field *field;
	struct nmsg_strbuf *sb;

	/* unpack message */
	if (np->has_payload == 0)
		return (nmsg_res_failure);
	m = protobuf_c_message_unpack(mod->pbdescr, NULL, np->payload.len,
				      np->payload.data);

	/* allocate pres str buffer */
	sb = calloc(1, sizeof(*sb));
	if (sb == NULL)
		return (nmsg_res_memfail);

	/* convert each message field to presentation format */
	for (field = mod->fields; field->descr != NULL; field++) {
		void *ptr;

		if (PBFIELD_ONE_PRESENT(m, field)) {
			ptr = PBFIELD(m, field, void);

			res = module_pbuf_field_to_pres(field, ptr, sb, endline);
			if (res != nmsg_res_success) {
				nmsg_strbuf_free(&sb);
				return (res);
			}
		} else if (PBFIELD_REPEATED(field)) {
			size_t n, n_entries;

			n_entries = *PBFIELD_Q(m, field);
			for (n = 0; n < n_entries; n++) {
				ptr = PBFIELD(m, field, void);
				char *array = *(char **) ptr;
				size_t siz = sizeof_elt_in_repeated_array(field->descr->type);

				res = module_pbuf_field_to_pres(field, &array[n * siz], sb, endline);
				if (res != nmsg_res_success) {
					nmsg_strbuf_free(&sb);
					return (res);
				}
			}
		}
	}

	/* cleanup */
	*pres = sb->data;
	free(sb);
	protobuf_c_message_free_unpacked(m, NULL);

	return (nmsg_res_success);
}

static nmsg_res
module_pbuf_field_to_pres(struct nmsg_pbmod_field *field,
			  void *ptr,
			  struct nmsg_strbuf *sb,
			  const char *endline)
{
	ProtobufCBinaryData *bdata;
	unsigned i;

	switch (field->type) {
	case nmsg_pbmod_ft_string:
		bdata = (ProtobufCBinaryData *) ptr;
		nmsg_strbuf_append(sb, "%s: %s%s",
				   field->descr->name,
				   bdata->data,
				   endline);
		break;
	case nmsg_pbmod_ft_mlstring:
		bdata = (ProtobufCBinaryData *) ptr;
		nmsg_strbuf_append(sb, "%s:%s%s.%s",
				   field->descr->name,
				   endline,
				   bdata->data,
				   endline);
		break;
	case nmsg_pbmod_ft_enum: {
		ProtobufCEnumDescriptor *enum_descr;
		bool enum_found;
		unsigned enum_value;

		enum_found = false;
		enum_descr = (ProtobufCEnumDescriptor *) field->descr->descriptor;

		enum_value = *((unsigned *) ptr);
		for (i = 0; i < enum_descr->n_values; i++) {
			if ((unsigned) enum_descr->values[i].value == enum_value) {
				nmsg_strbuf_append(sb, "%s: %s%s",
						   field->descr->name,
						   enum_descr->values[i].name,
						   endline);
				enum_found = true;
			}
		}
		if (enum_found == false) {
			nmsg_strbuf_append(sb, "%s: <UNKNOWN ENUM>%s",
					   field->descr->name,
					   endline);
		}
		break;
	}
	case nmsg_pbmod_ft_ip: {
		char sip[INET6_ADDRSTRLEN];

		bdata = (ProtobufCBinaryData *) ptr;
		if (bdata->len == 4) {
			if (inet_ntop(AF_INET, bdata->data, sip, sizeof(sip))) {
				nmsg_strbuf_append(sb, "%s: %s%s",
						   field->descr->name,
						   sip, endline);
			}
		} else if (bdata->len == 16) {
			if (inet_ntop(AF_INET6, bdata->data, sip, sizeof(sip))) {
				nmsg_strbuf_append(sb, "%s: %s%s",
						   field->descr->name,
						   sip, endline);
			}
		} else {
			nmsg_strbuf_append(sb, "%s: <INVALID IP len=%zd>%s",
					   field->descr->name, bdata->len,
					   endline);
		}
		break;
	}
	case nmsg_pbmod_ft_uint16:
		nmsg_strbuf_append(sb, "%s: %hu%s",
				   field->descr->name,
				   *((uint16_t *) ptr), endline);
		break;
	case nmsg_pbmod_ft_uint32:
		nmsg_strbuf_append(sb, "%s: %u%s",
				   field->descr->name,
				   *((uint32_t *) ptr), endline);
		break;
	} /* end switch */

	return (nmsg_res_success);
}

static nmsg_res
module_pres_to_pbuf(struct nmsg_pbmod *mod, void *cl, const char *pres) {
	ProtobufCMessage *m;
	char *line = NULL, *name = NULL, *saveptr = NULL;
	const char *value = NULL;
	int *qptr;
	nmsg_res res;
	size_t len;
	struct nmsg_pbmod_clos *clos = (struct nmsg_pbmod_clos *) cl;
	struct nmsg_pbmod_field *field;
	void *ptr;

	/* initialize the in-memory protobuf message if necessary */
	if (clos->nmsg_pbuf == NULL) {
		ProtobufCMessage *base;

		clos->nmsg_pbuf = calloc(1, mod->pbdescr->sizeof_message);
		if (clos->nmsg_pbuf == NULL) {
			return (nmsg_res_memfail);
		}
		base = (ProtobufCMessage *) &(clos->nmsg_pbuf)[0];
		base->descriptor = mod->pbdescr;
		clos->mode = nmsg_pbmod_clos_m_keyval;
	}

	/* convenience reference */
	m = (ProtobufCMessage *) clos->nmsg_pbuf;

	/* return pbuf ready if the end-of-message marker was seen */
	if (pres[0] == '\n' && clos->mode == nmsg_pbmod_clos_m_keyval) {
		clos->mode = nmsg_pbmod_clos_m_keyval;
		return (nmsg_res_pbuf_ready);
	}

	/* single line data types, and the type tag of a multiline string */
	if (clos->mode == nmsg_pbmod_clos_m_keyval) {
		/* make a copy of the line */
		len = strlen(pres);
		line = alloca(len);
		memcpy(line, pres, len);

		/* trim the newline at the end of the line */
		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		/* find the key */
		name = strtok_r(line, ":", &saveptr);
		if (name == NULL)
			return (nmsg_res_parse_error);

		/* find the field named by this key */
		for (field = mod->fields; field->descr != NULL; field++) {
			if (strcmp(name, field->descr->name) == 0)
				break;
		}

		if (field->descr == NULL)
			return (nmsg_res_parse_error);

		/* find the value */
		if (field->type != nmsg_pbmod_ft_mlstring)
			value = strtok_r(NULL, " ", &saveptr);
	} else if (clos->mode == nmsg_pbmod_clos_m_multiline) {
		field = clos->field;
		value = pres;
	}

	/* load the value */
	if (PBFIELD_REPEATED(field)) {
		/* XXX */
	} else {
		ptr = PBFIELD(m, field, void);
		qptr = PBFIELD_Q(m, field);

		res = pres_to_pbuf_load(field, clos, value, ptr, qptr);
		if (res != nmsg_res_success)
			return (res);
	}

	return (nmsg_res_success);
}

static nmsg_res
pres_to_pbuf_load(struct nmsg_pbmod_field *field, struct nmsg_pbmod_clos *clos,
		  const char *value, void *ptr, int *qptr)
{
	unsigned i;

	switch (field->type) {
	case nmsg_pbmod_ft_enum: {
		bool enum_found;
		ProtobufCEnumDescriptor *enum_descr;

		enum_found = false;
		enum_descr = (ProtobufCEnumDescriptor *) field->descr->descriptor;
		for (i = 0; i < enum_descr->n_values; i++) {
			if (strcmp(enum_descr->values[i].name, value) == 0) {
				enum_found = true;
				*(unsigned *) ptr = enum_descr->values[i].value;

				if (field->descr->label == PROTOBUF_C_LABEL_OPTIONAL)
					*qptr = 1;
				clos->estsz += 4;
				break;
			}
		}
		if (enum_found == false)
			return (nmsg_res_parse_error);
		break;
	}
	case nmsg_pbmod_ft_string: {
		ProtobufCBinaryData *bdata;

		bdata = ptr;
		bdata->data = (uint8_t *) strdup(value);
		if (bdata->data == NULL) {
			return (nmsg_res_memfail);
		}
		bdata->len = strlen(value) + 1; /* \0 terminated */
		clos->estsz += strlen(value);

		if (field->descr->label == PROTOBUF_C_LABEL_OPTIONAL)
			*qptr = 1;
		break;
	}
	case nmsg_pbmod_ft_ip: {
		ProtobufCBinaryData *bdata;
		char addr[16];

		bdata = ptr;
		if (inet_pton(AF_INET, value, addr) == 1) {
			bdata->data = malloc(4);
			if (bdata->data == NULL) {
				return (nmsg_res_memfail);
			}
			bdata->len = 4;
			memcpy(bdata->data, addr, 4);
			clos->estsz += 4;
		} else if (inet_pton(AF_INET6, value, addr) == 1) {
			bdata->len = 16;
			bdata->data = malloc(16);
			if (bdata->data == NULL) {
				return (nmsg_res_memfail);
			}
			memcpy(bdata->data, addr, 16);
			clos->estsz += 16;
		} else {
			return (nmsg_res_parse_error);
		}

		if (field->descr->label == PROTOBUF_C_LABEL_OPTIONAL)
			*qptr = 1;

		break;
	}
	case nmsg_pbmod_ft_uint16: {
		char *t;
		long intval;

		intval = strtoul(value, &t, 0);
		if (*t != '\0' || intval > UINT16_MAX)
			return (nmsg_res_parse_error);
		*(uint16_t *) ptr = (uint16_t) intval;
		if (field->descr->label == PROTOBUF_C_LABEL_OPTIONAL)
			*qptr = 1;
		break;
	}
	case nmsg_pbmod_ft_uint32: {
		char *t;
		long intval;

		intval = strtoul(value, &t, 0);
		if (*t != '\0' || intval > UINT32_MAX)
			return (nmsg_res_parse_error);
		*(uint32_t *) ptr = (uint32_t) intval;
		if (field->descr->label == PROTOBUF_C_LABEL_OPTIONAL)
			*qptr = 1;
		break;
	}

	case nmsg_pbmod_ft_mlstring:
	/* if we are in keyval mode and the field type is multiline,
	 * there is no value data to read yet */
		if (clos->mode == nmsg_pbmod_clos_m_keyval) {
			clos->field = field;
			clos->mode = nmsg_pbmod_clos_m_multiline;
		} else if (clos->mode == nmsg_pbmod_clos_m_multiline) {
			struct nmsg_strbuf *sb;
			size_t len = strlen(value);

			/* load the saved field */
			field = clos->field;

			/* locate our buffer */
			sb = &clos->strbufs[field->descr->id - 1];

			/* check if this is the end */
			if (LINECMP(value, ".\n")) {
				ProtobufCBinaryData *bdata;

				bdata = ptr;
				bdata->len = nmsg_strbuf_len(sb);
				bdata->data = (uint8_t *) sb->data;
				if (field->descr->label == PROTOBUF_C_LABEL_OPTIONAL)
					*qptr = 1;

				clos->mode = nmsg_pbmod_clos_m_keyval;
			} else {
				nmsg_strbuf_append(sb, "%s", value);
				clos->estsz += len;
			}
		}
		break;
	} /* end switch */

	return (nmsg_res_success);
}

static nmsg_res
module_pres_to_pbuf_finalize(struct nmsg_pbmod *mod, void *cl,
			     uint8_t **pbuf, size_t *sz)
{
	ProtobufCMessage *m;
	struct nmsg_pbmod_clos *clos = (struct nmsg_pbmod_clos *) cl;
	struct nmsg_pbmod_field *field;
	struct nmsg_strbuf *sb;

	/* guarantee a minimum allocation */
	if (clos->estsz < 64)
		clos->estsz = 64;

	/* convenience reference */
	m = (ProtobufCMessage *) clos->nmsg_pbuf;

	/* allocate a buffer for the serialized message */
	*pbuf = malloc(2 * clos->estsz);
	if (*pbuf == NULL) {
		free(clos->nmsg_pbuf);
		return (nmsg_res_memfail);
	}

	/* null terminate multiline strings */
	for (field = mod->fields; field->descr != NULL; field++) {
		if (field->descr->type == PROTOBUF_C_TYPE_BYTES &&
		    field->type == nmsg_pbmod_ft_mlstring &&
		    PBFIELD_ONE_PRESENT(m, field))
		{
			ProtobufCBinaryData *bdata;

			bdata = PBFIELD(m, field, ProtobufCBinaryData);
			bdata->len += 1;
		}
	}

	/* serialize the message */
	*sz = protobuf_c_message_pack((const ProtobufCMessage *) clos->nmsg_pbuf,
				      *pbuf);

	/* deallocate any byte arrays field members */
	for (field = mod->fields; field->descr != NULL; field++) {
		if (field->descr->type == PROTOBUF_C_TYPE_BYTES &&
		    *PBFIELD_HAS(m, field) == true)
		{
			/* for mlstring's, bdata->data is only a pointer to
			 * the inside of a strbuf */
			if (field->type != nmsg_pbmod_ft_mlstring) {
				ProtobufCBinaryData *bdata;

				bdata = PBFIELD(m, field, ProtobufCBinaryData);
				free(bdata->data);
			} else {
				sb = &clos->strbufs[field->descr->id - 1];
				nmsg_strbuf_reset(sb);
			}
		}
	}

	/* cleanup */
	free(clos->nmsg_pbuf);
	clos->nmsg_pbuf = NULL;
	clos->estsz = 0;

	return (nmsg_res_success);
}

static void
load_field_descriptors(struct nmsg_pbmod *mod) {
	const ProtobufCFieldDescriptor *pbfield;
	struct nmsg_pbmod_field *field;
	unsigned i;

	/* lookup the field descriptors by name */
	for (field = mod->fields; field->name != NULL; field++) {
		bool descr_found = false;

		for (i = 0; i < mod->pbdescr->n_fields; i++) {
			pbfield = &mod->pbfields[i];
			if (strcmp(pbfield->name, field->name) == 0) {
				descr_found = true;
				field->descr = pbfield;
				break;
			}
		}
		assert(descr_found == true);
	}
}

static bool
is_automatic_pbmod(struct nmsg_pbmod *mod) {
	if (mod->init == NULL &&
	    mod->fini == NULL &&
	    mod->pbuf2pres == NULL &&
	    mod->pres2pbuf == NULL &&
	    mod->pres2pbuf_finalize == NULL &&
	    mod->field2pbuf == NULL &&
	    mod->pbdescr != NULL &&
	    mod->pbfields != NULL &&
	    mod->fields != NULL)
	{
		return (true);
	}
	return (false);
}

/* from protobuf-c.c */

static inline size_t sizeof_elt_in_repeated_array (ProtobufCType type)
{
  switch (type)
    {
    case PROTOBUF_C_TYPE_SINT32:
    case PROTOBUF_C_TYPE_INT32:
    case PROTOBUF_C_TYPE_UINT32:
    case PROTOBUF_C_TYPE_SFIXED32:
    case PROTOBUF_C_TYPE_FIXED32:
    case PROTOBUF_C_TYPE_FLOAT:
    case PROTOBUF_C_TYPE_ENUM:
      return 4;
    case PROTOBUF_C_TYPE_SINT64:
    case PROTOBUF_C_TYPE_INT64:
    case PROTOBUF_C_TYPE_UINT64:
    case PROTOBUF_C_TYPE_SFIXED64:
    case PROTOBUF_C_TYPE_FIXED64:
    case PROTOBUF_C_TYPE_DOUBLE:
      return 8;
    case PROTOBUF_C_TYPE_BOOL:
      return sizeof (protobuf_c_boolean);
    case PROTOBUF_C_TYPE_STRING:
    case PROTOBUF_C_TYPE_MESSAGE:
      return sizeof (void *);
    case PROTOBUF_C_TYPE_BYTES:
      return sizeof (ProtobufCBinaryData);
    }
  PROTOBUF_C_ASSERT_NOT_REACHED ();
  return 0;
}
