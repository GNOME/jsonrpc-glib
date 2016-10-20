/* jsonrpc-input-stream.c
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

#define G_LOG_DOMAIN "jsonrpc-input-stream"

#include "jsonrpc-input-stream.h"

G_DEFINE_TYPE (JsonrpcInputStream, jsonrpc_input_stream, G_TYPE_DATA_INPUT_STREAM)

static void
jsonrpc_input_stream_class_init (JsonrpcInputStreamClass *klass)
{
}

static void
jsonrpc_input_stream_init (JsonrpcInputStream *self)
{
}

JsonrpcInputStream *
jsonrpc_input_stream_new (GInputStream *base_stream)
{
  return g_object_new (JSONRPC_TYPE_INPUT_STREAM,
                       "base-stream", base_stream,
                       NULL);
}

gboolean
jsonrpc_input_stream_read_message (JsonrpcInputStream  *self,
                                   GCancellable        *cancellable,
                                   JsonNode           **node,
                                   GError             **error)
{
  g_autoptr(JsonParser) parser = NULL;
  JsonNode *root;

  g_return_val_if_fail (JSONRPC_IS_INPUT_STREAM (self), FALSE);
  g_return_val_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable), FALSE);

  parser = json_parser_new ();

  if (!json_parser_load_from_stream (parser, G_INPUT_STREAM (self), cancellable, error))
    return FALSE;

  root = json_parser_get_root (parser);

  if (!JSON_NODE_HOLDS_OBJECT (root) || !JSON_NODE_HOLDS_ARRAY (root))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_DATA,
                   "Implausable error from json parser");
      return FALSE;
    }

  *node = json_node_ref (root);

  return TRUE;
}

static void
jsonrpc_input_stream_read_message_cb (GObject      *object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  JsonParser *parser = (JsonParser *)object;
  g_autoptr(GTask) task = user_data;
  g_autoptr(GError) error = NULL;
  JsonNode *root;

  g_assert (JSON_IS_PARSER (parser));
  g_assert (G_IS_TASK (task));

  if (!json_parser_load_from_stream_finish (parser, result, &error))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  root = json_parser_get_root (parser);

  if (root == NULL)
    g_error ("Fooy");

  if (!JSON_NODE_HOLDS_OBJECT (root) && !JSON_NODE_HOLDS_ARRAY (root))
    {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_DATA,
                               "Implausable error from json parser");
      return;
    }

  g_task_return_pointer (task, json_node_ref (root), (GDestroyNotify)json_node_unref);
}

void
jsonrpc_input_stream_read_message_async (JsonrpcInputStream  *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;
  g_autoptr(JsonParser) parser = NULL;

  g_return_if_fail (JSONRPC_IS_INPUT_STREAM (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, jsonrpc_input_stream_read_message_async);

  parser = json_parser_new ();

  json_parser_load_from_stream_async (parser,
                                      G_INPUT_STREAM (self),
                                      cancellable,
                                      jsonrpc_input_stream_read_message_cb,
                                      g_steal_pointer (&task));
}

gboolean
jsonrpc_input_stream_read_message_finish (JsonrpcInputStream  *self,
                                          GAsyncResult        *result,
                                          JsonNode           **object,
                                          GError             **error)
{
  g_return_val_if_fail (JSONRPC_IS_INPUT_STREAM (self), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}
