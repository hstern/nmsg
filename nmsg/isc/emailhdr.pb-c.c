/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#include "emailhdr.pb-c.h"
size_t nmsg__isc__emailhdr__get_packed_size
                     (const Nmsg__Isc__Emailhdr *message)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__emailhdr__descriptor);
  return protobuf_c_message_get_packed_size ((ProtobufCMessage*)(message));
}
size_t nmsg__isc__emailhdr__pack
                     (const Nmsg__Isc__Emailhdr *message,
                      uint8_t       *out)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__emailhdr__descriptor);
  return protobuf_c_message_pack ((ProtobufCMessage*)message, out);
}
size_t nmsg__isc__emailhdr__pack_to_buffer
                     (const Nmsg__Isc__Emailhdr *message,
                      ProtobufCBuffer *buffer)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__emailhdr__descriptor);
  return protobuf_c_message_pack_to_buffer ((ProtobufCMessage*)message, buffer);
}
Nmsg__Isc__Emailhdr *
       nmsg__isc__emailhdr__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Nmsg__Isc__Emailhdr *)
     protobuf_c_message_unpack (&nmsg__isc__emailhdr__descriptor,
                                allocator, len, data);
}
void   nmsg__isc__emailhdr__free_unpacked
                     (Nmsg__Isc__Emailhdr *message,
                      ProtobufCAllocator *allocator)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &nmsg__isc__emailhdr__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor nmsg__isc__emailhdr__field_descriptors[3] =
{
  {
    "user",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_UINT32,
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Emailhdr, n_user),
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Emailhdr, user),
    NULL
  },
  {
    "truncated",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_BOOL,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Emailhdr, truncated),
    NULL
  },
  {
    "headers",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_BYTES,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Nmsg__Isc__Emailhdr, headers),
    NULL
  },
};
static const unsigned nmsg__isc__emailhdr__field_indices_by_name[] = {
  2,   /* field[2] = headers */
  1,   /* field[1] = truncated */
  0,   /* field[0] = user */
};
static const ProtobufCIntRange nmsg__isc__emailhdr__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor nmsg__isc__emailhdr__descriptor =
{
  PROTOBUF_C_MESSAGE_DESCRIPTOR_MAGIC,
  "nmsg.isc.Emailhdr",
  "Emailhdr",
  "Nmsg__Isc__Emailhdr",
  "nmsg.isc",
  sizeof(Nmsg__Isc__Emailhdr),
  3,
  nmsg__isc__emailhdr__field_descriptors,
  nmsg__isc__emailhdr__field_indices_by_name,
  1,  nmsg__isc__emailhdr__number_ranges
};
