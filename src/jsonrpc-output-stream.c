/* jsonrpc-output-stream.c
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
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

#define G_LOG_DOMAIN "jsonrpc-output-stream"

#include <string.h>

#include "jsonrpc-output-stream.h"

G_DEFINE_TYPE (JsonrpcOutputStream, jsonrpc_output_stream, G_TYPE_DATA_OUTPUT_STREAM)

static void
jsonrpc_output_stream_class_init (JsonrpcOutputStreamClass *klass)
{
}

static void
jsonrpc_output_stream_init (JsonrpcOutputStream *self)
{
}

static GBytes *
jsonrpc_output_stream_create_bytes (JsonrpcOutputStream  *self,
                                    JsonNode             *node,
                                    GError              **error)
{
  g_autofree gchar *str = NULL;
  gsize len;

  g_assert (JSONRPC_IS_OUTPUT_STREAM (self));
  g_assert (node != NULL);

  if (!JSON_NODE_HOLDS_OBJECT (node) && !JSON_NODE_HOLDS_ARRAY (node))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVAL,
                   "node must be an array or object");
      return FALSE;
    }

  str = json_to_string (node, FALSE);
  len = strlen (str);
  /*
   * So that we can have a somewhat readable stream when debugging,
   * we add a trailing \n after the rpc. We use the trailing \0 and
   * replace it with \n.
   */
  str[len++] = '\n';

  return g_bytes_new_take (g_steal_pointer (&str), len);
}

JsonrpcOutputStream *
jsonrpc_output_stream_new (GOutputStream *base_stream)
{
  return g_object_new (JSONRPC_TYPE_OUTPUT_STREAM,
                       "base-stream", base_stream,
                       NULL);
}

gboolean
jsonrpc_output_stream_write_message (JsonrpcOutputStream  *self,
                                     JsonNode             *node,
                                     GCancellable         *cancellable,
                                     GError              **error)
{
  g_autoptr(GBytes) bytes = NULL;
  const guint8 *buffer;
  gsize n_written = 0;
  gsize len;

  g_return_val_if_fail (JSONRPC_IS_OUTPUT_STREAM (self), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable), FALSE);

  if (NULL == (bytes = jsonrpc_output_stream_create_bytes (self, node, error)))
    return FALSE;

  buffer = g_bytes_get_data (bytes, &len);

  if (!g_output_stream_write_all (G_OUTPUT_STREAM (self), buffer, len, &n_written, cancellable, error))
    return FALSE;

  if (n_written != len)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_CLOSED,
                   "Failed to write all bytes to peer");
      return FALSE;
    }

  return TRUE;
}

static void
jsonrpc_output_stream_write_message_cb (GObject      *object,
                                        GAsyncResult *result,
                                        gpointer      user_data)
{
  GOutputStream *stream = (GOutputStream *)object;
  g_autoptr(GError) error = NULL;
  g_autoptr(GTask) task = user_data;
  GBytes *bytes;
  gsize n_written;

  g_assert (G_IS_OUTPUT_STREAM (stream));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  if (!g_output_stream_write_all_finish (stream, result, &n_written, &error))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  bytes = g_task_get_task_data (task);

  if (g_bytes_get_size (bytes) != n_written)
    {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_CLOSED,
                               "Failed to write all bytes to peer");
      return;
    }

  g_task_return_boolean (task, TRUE);
}

void
jsonrpc_output_stream_write_message_async (JsonrpcOutputStream *self,
                                           JsonNode            *node,
                                           GCancellable        *cancellable,
                                           GAsyncReadyCallback  callback,
                                           gpointer             user_data)
{
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GTask) task = NULL;
  const guint8 *data;
  GError *error = NULL;
  gsize len;

  g_return_if_fail (JSONRPC_IS_OUTPUT_STREAM (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, jsonrpc_output_stream_write_message_async);

  if (NULL == (bytes = jsonrpc_output_stream_create_bytes (self, node, &error)))
    {
      g_task_return_error (task, error);
      return;
    }

  g_task_set_task_data (task, g_bytes_ref (bytes), (GDestroyNotify)g_bytes_unref);

  data = g_bytes_get_data (bytes, &len);

  g_output_stream_write_all_async (G_OUTPUT_STREAM (self),
                                   data,
                                   len,
                                   G_PRIORITY_DEFAULT,
                                   cancellable,
                                   jsonrpc_output_stream_write_message_cb,
                                   g_steal_pointer (&task));
}

gboolean
jsonrpc_output_stream_write_message_finish (JsonrpcOutputStream  *self,
                                            GAsyncResult         *result,
                                            GError              **error)
{
  g_return_val_if_fail (JSONRPC_IS_OUTPUT_STREAM (self), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}
