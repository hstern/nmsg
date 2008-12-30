#ifndef PROTOBUF_C_linkpair_2eproto__INCLUDED
#define PROTOBUF_C_linkpair_2eproto__INCLUDED

#include <nmsg/protobuf-c.h>

PROTOBUF_C_BEGIN_DECLS


typedef struct _Nmsg__Isc__Linkpair Nmsg__Isc__Linkpair;


/* --- enums --- */

typedef enum _Nmsg__Isc__Linktype {
  NMSG__ISC__LINKTYPE__anchor = 0,
  NMSG__ISC__LINKTYPE__redirect = 1
} Nmsg__Isc__Linktype;

/* --- messages --- */

struct  _Nmsg__Isc__Linkpair
{
  ProtobufCMessage base;
  Nmsg__Isc__Linktype type;
  ProtobufCBinaryData src;
  ProtobufCBinaryData dst;
  protobuf_c_boolean truncated;
  protobuf_c_boolean has_headers;
  ProtobufCBinaryData headers;
};
#define NMSG__ISC__LINKPAIR__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&nmsg__isc__linkpair__descriptor) \
    , 0, {0,NULL}, {0,NULL}, 0, 0,{0,NULL} }


/* Nmsg__Isc__Linkpair methods */
void   nmsg__isc__linkpair__init
                     (Nmsg__Isc__Linkpair         *message);
size_t nmsg__isc__linkpair__get_packed_size
                     (const Nmsg__Isc__Linkpair   *message);
size_t nmsg__isc__linkpair__pack
                     (const Nmsg__Isc__Linkpair   *message,
                      uint8_t             *out);
size_t nmsg__isc__linkpair__pack_to_buffer
                     (const Nmsg__Isc__Linkpair   *message,
                      ProtobufCBuffer     *buffer);
Nmsg__Isc__Linkpair *
       nmsg__isc__linkpair__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   nmsg__isc__linkpair__free_unpacked
                     (Nmsg__Isc__Linkpair *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Nmsg__Isc__Linkpair_Closure)
                 (const Nmsg__Isc__Linkpair *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    nmsg__isc__linktype__descriptor;
extern const ProtobufCMessageDescriptor nmsg__isc__linkpair__descriptor;

PROTOBUF_C_END_DECLS


#endif  /* PROTOBUF_linkpair_2eproto__INCLUDED */
