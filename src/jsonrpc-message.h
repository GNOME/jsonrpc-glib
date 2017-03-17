/* jsonrpc-message.h
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JSONRPC_MESSAGE_H
#define JSONRPC_MESSAGE_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
  char bytes[8];
} JsonrpcMessageMagic;

typedef struct
{
  JsonrpcMessageMagic magic;
} JsonrpcMessageAny;

typedef struct
{
  JsonrpcMessageMagic magic;
  const char *val;
} JsonrpcMessagePutString;

typedef struct
{
  JsonrpcMessageMagic magic;
  const char **valptr;
} JsonrpcMessageGetString;

typedef struct
{
  JsonrpcMessageMagic magic;
  gint32 val;
} JsonrpcMessagePutInt32;

typedef struct
{
  JsonrpcMessageMagic magic;
  gint32 *valptr;
} JsonrpcMessageGetInt32;

typedef struct
{
  JsonrpcMessageMagic magic;
  gboolean val;
} JsonrpcMessagePutBoolean;

typedef struct
{
  JsonrpcMessageMagic magic;
  gboolean *valptr;
} JsonrpcMessageGetBoolean;

typedef struct
{
  JsonrpcMessageMagic magic;
  double val;
} JsonrpcMessagePutDouble;

typedef struct
{
  JsonrpcMessageMagic magic;
  double *valptr;
} JsonrpcMessageGetDouble;

typedef struct
{
  JsonrpcMessageMagic magic;
  GVariantIter **iterptr;
} JsonrpcMessageGetIter;

typedef struct
{
  JsonrpcMessageMagic magic;
  GVariantDict **dictptr;
} JsonrpcMessageGetDict;

typedef struct
{
  JsonrpcMessageMagic magic;
  GVariant **variantptr;
} JsonrpcMessageGetVariant;

#define _JSONRPC_MESSAGE_PUT_STRING_MAGIC  "@!@|PUTS"
#define _JSONRPC_MESSAGE_GET_STRING_MAGIC  "@!@|GETS"
#define _JSONRPC_MESSAGE_PUT_INT32_MAGIC   "@!@|PUTI"
#define _JSONRPC_MESSAGE_GET_INT32_MAGIC   "@!@|GETI"
#define _JSONRPC_MESSAGE_PUT_BOOLEAN_MAGIC "@!@|PUTB"
#define _JSONRPC_MESSAGE_GET_BOOLEAN_MAGIC "@!@|GETB"
#define _JSONRPC_MESSAGE_PUT_DOUBLE_MAGIC  "@!@|PUTD"
#define _JSONRPC_MESSAGE_GET_DOUBLE_MAGIC  "@!@|GETD"
#define _JSONRPC_MESSAGE_GET_ITER_MAGIC    "@!@|GETT"
#define _JSONRPC_MESSAGE_GET_DICT_MAGIC    "@!@|GETC"
#define _JSONRPC_MESSAGE_GET_VARIANT_MAGIC "@!@|GETV"

#define JSONRPC_MESSAGE_NEW(...) \
  jsonrpc_message_new(__VA_ARGS__, NULL)
#define JSONRPC_MESSAGE_PARSE(message, ...) \
  jsonrpc_message_parse(message, __VA_ARGS__, NULL)
#define JSONRPC_MESSAGE_PARSE_ARRAY(iter, ...) \
  jsonrpc_message_parse_array(iter, __VA_ARGS__, NULL)

#define JSONRPC_MESSAGE_PUT_STRING(_val) \
  &(JsonrpcMessagePutString) { .magic = _JSONRPC_MESSAGE_PUT_STRING_MAGIC, .val = _val }
#define JSONRPC_MESSAGE_GET_STRING(_valptr) \
  &(JsonrpcMessageGetString) { .magic = _JSONRPC_MESSAGE_GET_STRING_MAGIC, .valptr = _valptr }

#define JSONRPC_MESSAGE_PUT_INT32(_val) \
  &(JsonrpcMessagePutInt32) { .magic = _JSONRPC_MESSAGE_PUT_INT32_MAGIC, .val = _val }
#define JSONRPC_MESSAGE_GET_INT32(_valptr) \
  &(JsonrpcMessageGetInt32) { .magic = _JSONRPC_MESSAGE_GET_INT32_MAGIC, .valptr = _valptr }

#define JSONRPC_MESSAGE_PUT_BOOLEAN(_val) \
  &(JsonrpcMessagePutBoolean) { .magic = _JSONRPC_MESSAGE_PUT_BOOLEAN_MAGIC, .val = _val }
#define JSONRPC_MESSAGE_GET_BOOLEAN(_valptr) \
  &(JsonrpcMessageGetBoolean) { .magic = _JSONRPC_MESSAGE_GET_BOOLEAN_MAGIC, .valptr = _valptr }

#define JSONRPC_MESSAGE_PUT_DOUBLE(_val) \
  &(JsonrpcMessagePutDouble) { .magic = _JSONRPC_MESSAGE_PUT_DOUBLE_MAGIC, .val = _val }
#define JSONRPC_MESSAGE_GET_DOUBLE(_valptr) \
  &(JsonrpcMessageGetDouble) { .magic = _JSONRPC_MESSAGE_GET_DOUBLE_MAGIC, .valptr = _valptr }

#define JSONRPC_MESSAGE_GET_ITER(_valptr) \
  &(JsonrpcMessageGetIter) { .magic = _JSONRPC_MESSAGE_GET_ITER_MAGIC, .iterptr = _valptr }

#define JSONRPC_MESSAGE_GET_DICT(_valptr) \
  &(JsonrpcMessageGetDict) { .magic = _JSONRPC_MESSAGE_GET_DICT_MAGIC, .dictptr = _valptr }

#define JSONRPC_MESSAGE_GET_VARIANT(_valptr) \
  &(JsonrpcMessageGetVariant) { .magic = _JSONRPC_MESSAGE_GET_VARIANT_MAGIC, .variantptr = _valptr }

GVariant *jsonrpc_message_new         (gpointer first_param, ...) G_GNUC_NULL_TERMINATED;
gboolean  jsonrpc_message_parse       (GVariant *message, ...) G_GNUC_NULL_TERMINATED;
gboolean  jsonrpc_message_parse_array (GVariantIter *iter, ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* JSONRPC_MESSAGE_H */
