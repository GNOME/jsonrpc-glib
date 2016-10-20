/* jsonrpc-client.c
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

#define G_LOG_DOMAIN "jsonrpc-client"

#include "jsonrpc-client.h"
#include "jsonrpc-input-stream.h"
#include "jsonrpc-output-stream.h"

typedef struct
{
  /*
   * The invocations field contains a hashtable that maps request ids to
   * the GTask that is awaiting their completion. The tasks are removed
   * from the hashtable automatically upon completion by connecting to
   * the GTask::completed signal. When reading a message from the input
   * stream, we use the request id as a string to lookup the inflight
   * invocation. The result is passed as the result of the task.
   */
  GHashTable *invocations;

  /*
   * We hold an extra reference to the GIOStream pair to make things
   * easier to construct and ensure that the streams are in tact in
   * case they are poorly implemented.
   */
  GIOStream *io_stream;

  /*
   * The input_stream field contains our wrapper input stream around the
   * underlying input stream provided by JsonrpcClient::io-stream. This
   * allows us to conveniently write JsonNode instances.
   */
  JsonrpcInputStream *input_stream;

  /*
   * The output_stream field contains our wrapper output stream around the
   * underlying output stream provided by JsonrpcClient::io-stream. This
   * allows us to convieniently read JsonNode instances.
   */
  JsonrpcOutputStream *output_stream;

  /*
   * Every JSONRPC invocation needs a request id. This is a monotonic
   * integer that we encode as a string to the server.
   */
  gint64 sequence;
} JsonrpcClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (JsonrpcClient, jsonrpc_client, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_IO_STREAM,
  N_PROPS
};

enum {
  NOTIFICATION,
  N_SIGNALS
};

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

static gboolean
is_jsonrpc_reply (JsonNode *node)
{
  JsonObject *object;
  const gchar *value;

  return JSON_NODE_HOLDS_OBJECT (node) &&
         NULL != (object = json_node_get_object (node)) &&
         json_object_has_member (object, "jsonrpc") &&
         NULL != (value = json_object_get_string_member (object, "jsonrpc")) &&
         (g_strcmp0 (value, "2.0") == 0);
}

static gboolean
is_jsonrpc_notification (JsonNode *node)
{
  JsonObject *object;
  const gchar *value;

  g_assert (JSON_NODE_HOLDS_OBJECT (node));

  object = json_node_get_object (node);

  return !json_object_has_member (object, "id") &&
         json_object_has_member (object, "method") &&
         NULL != (value = json_object_get_string_member (object, "method")) &&
         value != NULL && *value != '\0';
}

static gboolean
is_jsonrpc_result (JsonNode *node)
{
  JsonObject *object;
  JsonNode *field;

  g_assert (JSON_NODE_HOLDS_OBJECT (node));

  object = json_node_get_object (node);

  return json_object_has_member (object, "id") &&
         NULL != (field = json_object_get_member (object, "id")) &&
         JSON_NODE_HOLDS_VALUE (field) &&
         json_node_get_string (field) != NULL &&
         json_object_has_member (object, "result");
}

static gboolean
unwrap_jsonrpc_error (JsonNode     *node,
                      const gchar **id,
                      GError      **error)
{
  JsonObject *object;
  JsonObject *err_obj;
  JsonNode *field;

  g_assert (node != NULL);
  g_assert (id != NULL);
  g_assert (error != NULL);

  if (!JSON_NODE_HOLDS_OBJECT (node))
    return FALSE;

  object = json_node_get_object (node);

  if (json_object_has_member (object, "id") &&
      NULL != (field = json_object_get_member (object, "id")) &&
      JSON_NODE_HOLDS_VALUE (field) &&
      json_node_get_string (field) != NULL)
    *id = json_node_get_string (field);
  else
    *id = NULL;

  if (json_object_has_member (object, "error") &&
      NULL != (field = json_object_get_member (object, "error")) &&
      JSON_NODE_HOLDS_OBJECT (field) &&
      NULL != (err_obj = json_node_get_object (field)))
    {
      const gchar *message;
      gint code;

      message = json_object_get_string_member (object, "message");
      code = json_object_get_int_member (object, "code");

      if (message == NULL || *message == '\0')
        message = "Unknown error occurred";

      g_set_error_literal (error, JSONRPC_CLIENT_ERROR, code, message);

      return TRUE;
    }

  return FALSE;
}

static void
jsonrpc_client_panic (JsonrpcClient *self,
                      const GError  *error)
{
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);
  g_autoptr(GHashTable) invocations = NULL;
  GHashTableIter iter;
  GTask *task;

  g_assert (JSONRPC_IS_CLIENT (self));
  g_assert (error != NULL);

  g_warning ("%s", error->message);

  /* Steal the tasks so that we don't have to worry about reentry. */
  invocations = g_steal_pointer (&priv->invocations);
  priv->invocations = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  /*
   * Clear our input and output streams so that new calls
   * fail immediately due to not being connected.
   */
  g_clear_object (&priv->input_stream);
  g_clear_object (&priv->output_stream);

  /*
   * Now notify all of the in-flight invocations that they failed due
   * to an unrecoverable error.
   */
  g_hash_table_iter_init (&iter, invocations);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&task))
    g_task_return_error (task, g_error_copy (error));
}

static gboolean
jsonrpc_client_check_ready (JsonrpcClient *self,
                            GTask         *task)
{
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);

  g_assert (JSONRPC_IS_CLIENT (self));
  g_assert (G_IS_TASK (task));

  if (priv->output_stream == NULL || priv->input_stream == NULL)
    {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_NOT_CONNECTED,
                               "No stream available to deliver invocation");
      return FALSE;
    }

  return TRUE;
}

static void
jsonrpc_client_constructed (GObject *object)
{
  JsonrpcClient *self = (JsonrpcClient *)object;
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);
  GInputStream *input_stream;
  GOutputStream *output_stream;

  G_OBJECT_CLASS (jsonrpc_client_parent_class)->constructed (object);

  if (priv->io_stream == NULL)
    {
      g_warning ("%s requires a GIOStream to communicate. Disabling.",
                 G_OBJECT_TYPE_NAME (self));
      return;
    }

  input_stream = g_io_stream_get_input_stream (priv->io_stream);
  output_stream = g_io_stream_get_output_stream (priv->io_stream);

  priv->input_stream = jsonrpc_input_stream_new (input_stream);
  priv->output_stream = jsonrpc_output_stream_new (output_stream);
}

static void
jsonrpc_client_finalize (GObject *object)
{
  JsonrpcClient *self = (JsonrpcClient *)object;
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);

  g_clear_pointer (&priv->invocations, g_hash_table_unref);

  g_clear_object (&priv->input_stream);
  g_clear_object (&priv->output_stream);
  g_clear_object (&priv->io_stream);

  G_OBJECT_CLASS (jsonrpc_client_parent_class)->finalize (object);
}

static void
jsonrpc_client_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  JsonrpcClient *self = JSONRPC_CLIENT (object);
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_IO_STREAM:
      priv->io_stream = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
jsonrpc_client_class_init (JsonrpcClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = jsonrpc_client_constructed;
  object_class->finalize = jsonrpc_client_finalize;
  object_class->set_property = jsonrpc_client_set_property;

  properties [PROP_IO_STREAM] =
    g_param_spec_object ("io-stream",
                         "IO Stream",
                         "The stream to communicate over",
                         G_TYPE_IO_STREAM,
                         (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [NOTIFICATION] =
    g_signal_new ("notification",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonrpcClientClass, notification),
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                  JSON_TYPE_ARRAY);
}

static void
jsonrpc_client_init (JsonrpcClient *self)
{
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);

  priv->invocations = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

static gchar *
jsonrpc_client_next_sequence (JsonrpcClient *self)
{
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);

  g_return_val_if_fail (JSONRPC_IS_CLIENT (self), NULL);

  return g_strdup_printf ("%"G_GINT64_FORMAT, ++priv->sequence);
}

JsonrpcClient *
jsonrpc_client_new (GIOStream *io_stream)
{
  return g_object_new (JSONRPC_TYPE_CLIENT,
                       "io-stream", io_stream,
                       NULL);
}

static void
jsonrpc_client_call_notify_completed (GTask      *task,
                                      GParamSpec *pspec,
                                      gpointer    user_data)
{
  JsonrpcClientPrivate *priv;
  JsonrpcClient *self;
  const gchar *id;

  g_assert (G_IS_TASK (task));
  g_assert (pspec != NULL);
  g_assert (g_str_equal (pspec->name, "completed"));

  self = g_task_get_source_object (task);
  priv = jsonrpc_client_get_instance_private (self);
  id = g_task_get_task_data (task);

  g_hash_table_remove (priv->invocations, id);
}

static void
jsonrpc_client_call_write_cb (GObject      *object,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  JsonrpcOutputStream *stream = (JsonrpcOutputStream *)object;
  g_autoptr(GTask) task = user_data;
  g_autoptr(GError) error = NULL;

  g_assert (JSONRPC_IS_OUTPUT_STREAM (stream));
  g_assert (G_IS_TASK (task));

  if (!jsonrpc_output_stream_write_message_finish (stream, result, &error))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  /* We don't need to complete the task because it will get completed when the
   * server replies with our reply. This is performed using an asynchronous
   * read that will pump through the messages.
   */
}

static void
jsonrpc_client_call_read_cb (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  JsonrpcInputStream *stream = (JsonrpcInputStream *)object;
  g_autoptr(JsonrpcClient) self = user_data;
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(GError) error = NULL;
  gboolean needs_chained_read;
  const gchar *id;

  g_assert (JSONRPC_IS_INPUT_STREAM (stream));
  g_assert (JSONRPC_IS_CLIENT (self));

  if (!jsonrpc_input_stream_read_message_finish (stream, result, &node, &error))
    {
      /*
       * If we fail to read a message, that means we couldn't even receive
       * a message describing the error. All we can do in this case is panic
       * and shutdown the whole client.
       */
      jsonrpc_client_panic (self, error);
      return;
    }

  /*
   * Before we make further progress on this task, determine if we will
   * need to follow up this read request with a chained read for the next
   * message. If there is more than one task in our invocation hashtable,
   * we need to do more reads.
   */
  needs_chained_read = g_hash_table_size (priv->invocations) > 1;

  /*
   * If the message is malformed, we'll also need to perform another read.
   * We do this to try to be relaxed against failures. That seems to be
   * the JSONRPC way, although I'm not sure I like the idea.
   */
  if (!is_jsonrpc_reply (node))
    {
      g_warning ("Received malformed response from peer");
      needs_chained_read = TRUE;
      goto chain_read;
    }

  /*
   * If the response does not have an "id" field, then it is a "notification"
   * and we need to emit the "notificiation" signal.
   */
  if (is_jsonrpc_notification (node))
    {
      JsonObject *obj;
      g_autoptr(JsonNode) empty_params = NULL;
      const gchar *method_name;
      JsonNode *params;

      obj = json_node_get_object (node);
      method_name = json_object_get_string_member (obj, "method");
      params = json_object_get_member (obj, "params");

      if (params == NULL)
        params = empty_params = json_node_new (JSON_NODE_ARRAY);

      g_signal_emit (self, signals [NOTIFICATION], 0, method_name, params);

      goto maybe_chain_read;
    }

  if (is_jsonrpc_result (node))
    {
      JsonObject *obj;
      JsonNode *res;
      GTask *task;

      obj = json_node_get_object (node);
      id = json_object_get_string_member (obj, "id");
      res = json_object_get_member (obj, "result");

      task = g_hash_table_lookup (priv->invocations, id);

      if (task != NULL)
        {
          g_task_return_pointer (task, json_node_ref (res), (GDestroyNotify)json_node_unref);
          goto maybe_chain_read;
        }

      g_warning ("Reply to missing or invalid task");

      goto chain_read;
    }

  /*
   * If we got an error destined for one of our inflight invocations, then
   * we need to dispatch it now.
   */
  if (unwrap_jsonrpc_error (node, &id, &error))
    {
      if (id != NULL)
        {
          GTask *task = g_hash_table_lookup (priv->invocations, id);

          if (task != NULL)
            {
              g_task_return_error (task, g_steal_pointer (&error));
              goto maybe_chain_read;
            }
        }

      g_warning ("%s", error->message);
    }

  /*
   * We failed and we don't really know why. Try again and block waiting
   * for another reply from the peer.
   */
  needs_chained_read = TRUE;

maybe_chain_read:
  if (!needs_chained_read)
    return;

chain_read:
  if (priv->input_stream != NULL)
    jsonrpc_input_stream_read_message_async (priv->input_stream,
                                             NULL,
                                             jsonrpc_client_call_read_cb,
                                             g_steal_pointer (&self));
}

static void
jsonrpc_client_call_sync_cb (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  JsonrpcClient *self = (JsonrpcClient *)object;
  GTask *task = user_data;
  g_autoptr(JsonNode) return_value = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (JSONRPC_IS_CLIENT (self));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  if (!jsonrpc_client_call_finish (self, result, &return_value, &error))
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_pointer (task, g_steal_pointer (&return_value), (GDestroyNotify)json_node_unref);
}

/**
 * jsonrpc_client_call:
 * @self: A #JsonrpcClient
 * @method: the name of the method to call
 * @params: (transfer full) (nullable): A #JsonNode of parameters or %NULL
 * @cancellable: (nullable): A #GCancellable or %NULL
 * @return_value: (nullable) (out): A location for a #JsonNode.
 *
 * Synchronously calls @method with @params on the remote peer.
 *
 * This function takes ownership of @params.
 *
 * once a reply has been received, or failure, this function will return.
 * If successful, @return_value will be set with the reslut field of
 * the response.
 *
 * Returns; %TRUE on success; otherwise %FALSE and @error is set.
 */
gboolean
jsonrpc_client_call (JsonrpcClient  *self,
                     const gchar    *method,
                     JsonNode       *params,
                     GCancellable   *cancellable,
                     JsonNode      **return_value,
                     GError        **error)
{
  g_autoptr(GTask) task = NULL;
  g_autoptr(GMainContext) main_context = NULL;
  g_autoptr(JsonNode) local_return_value = NULL;
  gboolean ret;

  g_return_val_if_fail (JSONRPC_IS_CLIENT (self), FALSE);
  g_return_val_if_fail (method != NULL, FALSE);
  g_return_val_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable), FALSE);

  main_context = g_main_context_ref_thread_default ();

  task = g_task_new (NULL, NULL, NULL, NULL);
  g_task_set_source_tag (task, jsonrpc_client_call);

  jsonrpc_client_call_async (self,
                             method,
                             params,
                             cancellable,
                             jsonrpc_client_call_sync_cb,
                             task);

  while (!g_task_get_completed (task))
    g_main_context_iteration (main_context, TRUE);

  local_return_value = g_task_propagate_pointer (task, error);
  ret = local_return_value != NULL;

  if (return_value != NULL)
    *return_value = g_steal_pointer (&local_return_value);

  return ret;
}

/**
 * jsonrpc_client_call_async:
 * @self: A #JsonrpcClient
 * @method: the name of the method to call
 * @params: (transfer full) (nullable): A #JsonNode of parameters or %NULL
 * @cancellable: (nullable): A #GCancellable or %NULL
 * @callback: a callback to executed upon completion
 * @user_data: user data for @callback
 *
 * Asynchronously calls @method with @params on the remote peer.
 *
 * This function takes ownership of @params.
 *
 * Upon completion or failure, @callback is executed and it should
 * call jsonrpc_client_call_finish() to complete the request and release
 * any memory held.
 */
void
jsonrpc_client_call_async (JsonrpcClient       *self,
                           const gchar         *method,
                           JsonNode            *params,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);
  g_autoptr(JsonObject) object = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(GTask) task = NULL;
  g_autofree gchar *id = NULL;

  g_return_if_fail (JSONRPC_IS_CLIENT (self));
  g_return_if_fail (method != NULL);
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, jsonrpc_client_call_async);

  if (!jsonrpc_client_check_ready (self, task))
    return;

  g_signal_connect (task,
                    "notify::completed",
                    G_CALLBACK (jsonrpc_client_call_notify_completed),
                    NULL);

  id = jsonrpc_client_next_sequence (self);

  if (params == NULL)
    params = json_node_new (JSON_NODE_NULL);

  object = json_object_new ();

  json_object_set_string_member (object, "jsonrpc", "2.0");
  json_object_set_string_member (object, "id", id);
  json_object_set_string_member (object, "method", method);
  json_object_set_member (object, "params", params);

  node = json_node_new (JSON_NODE_OBJECT);
  json_node_take_object (node, g_steal_pointer (&object));

  g_hash_table_insert (priv->invocations, g_steal_pointer (&id), g_object_ref (task));

  jsonrpc_output_stream_write_message_async (priv->output_stream,
                                             node,
                                             cancellable,
                                             jsonrpc_client_call_write_cb,
                                             g_steal_pointer (&task));

  /*
   * The user is not allowed to mix asynchronous and synchronous writes.
   * So if there is not an asynchronous read in flight, we need to start
   * one now. If there is, when that request finishes, it will start the
   * next message read for us as long as there are tasks in the invocation
   * hashtable.
   */

  if (g_hash_table_size (priv->invocations) == 1)
    {
      jsonrpc_input_stream_read_message_async (priv->input_stream,
                                               NULL,
                                               jsonrpc_client_call_read_cb,
                                               g_object_ref (self));
    }
}

gboolean
jsonrpc_client_call_finish (JsonrpcClient  *self,
                            GAsyncResult   *result,
                            JsonNode      **return_value,
                            GError        **error)
{
  g_autoptr(JsonNode) local_return_value = NULL;
  gboolean ret;

  g_return_val_if_fail (JSONRPC_IS_CLIENT (self), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  local_return_value = g_task_propagate_pointer (G_TASK (result), error);
  ret = local_return_value != NULL;

  if (return_value != NULL)
    *return_value = g_steal_pointer (&local_return_value);

  return ret;
}

GQuark
jsonrpc_client_error_quark (void)
{
  return g_quark_from_static_string ("jsonrpc-client-error-quark");
}

static void
jsonrpc_client_notification_write_cb (GObject      *object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  JsonrpcOutputStream *stream = (JsonrpcOutputStream *)object;
  g_autoptr(GTask) task = user_data;
  g_autoptr(GError) error = NULL;

  g_assert (JSONRPC_IS_OUTPUT_STREAM (stream));
  g_assert (G_IS_TASK (task));

  if (!jsonrpc_output_stream_write_message_finish (stream, result, &error))
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_boolean (task, TRUE);
}

/**
 * jsonrpc_client_notification:
 * @self: A #JsonrpcClient
 * @method: the name of the method to call
 * @params: (transfer full) (nullable): A #JsonNode of parameters or %NULL
 * @cancellable: (nullable): A #GCancellable or %NULL
 *
 * Synchronously calls @method with @params on the remote peer.
 * This function will not wait or expect a reply from the peer.
 *
 * This function takes ownership of @params.
 *
 * Returns; %TRUE on success; otherwise %FALSE and @error is set.
 */
gboolean
jsonrpc_client_notification (JsonrpcClient  *self,
                             const gchar    *method,
                             JsonNode       *params,
                             GCancellable   *cancellable,
                             GError        **error)
{
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);
  g_autoptr(JsonObject) object = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(GTask) task = NULL;

  g_return_val_if_fail (JSONRPC_IS_CLIENT (self), FALSE);
  g_return_val_if_fail (method != NULL, FALSE);
  g_return_val_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable), FALSE);

  if (params == NULL)
    params = json_node_new (JSON_NODE_NULL);

  object = json_object_new ();

  json_object_set_string_member (object, "jsonrpc", "2.0");
  json_object_set_string_member (object, "method", method);
  json_object_set_member (object, "params", params);

  node = json_node_new (JSON_NODE_OBJECT);
  json_node_take_object (node, g_steal_pointer (&object));

  return jsonrpc_output_stream_write_message (priv->output_stream, node, cancellable, error);
}

/**
 * jsonrpc_client_notification_async:
 * @self: A #JsonrpcClient
 * @method: the name of the method to call
 * @params: (transfer full) (nullable): A #JsonNode of parameters or %NULL
 * @cancellable: (nullable): A #GCancellable or %NULL
 *
 * Asynchronously calls @method with @params on the remote peer.
 * This function will not wait or expect a reply from the peer.
 *
 * This function is useful when the caller wants to be notified that
 * the bytes have been delivered to the underlying stream. This does
 * not indicate that the peer has received them.
 *
 * This function takes ownership of @params.
 */
void
jsonrpc_client_notification_async (JsonrpcClient       *self,
                                   const gchar         *method,
                                   JsonNode            *params,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  JsonrpcClientPrivate *priv = jsonrpc_client_get_instance_private (self);
  g_autoptr(JsonObject) object = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(GTask) task = NULL;

  g_return_if_fail (JSONRPC_IS_CLIENT (self));
  g_return_if_fail (method != NULL);
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  if (params == NULL)
    params = json_node_new (JSON_NODE_NULL);

  object = json_object_new ();

  json_object_set_string_member (object, "jsonrpc", "2.0");
  json_object_set_string_member (object, "method", method);
  json_object_set_member (object, "params", params);

  node = json_node_new (JSON_NODE_OBJECT);
  json_node_take_object (node, g_steal_pointer (&object));

  jsonrpc_output_stream_write_message_async (priv->output_stream,
                                             node,
                                             cancellable,
                                             jsonrpc_client_notification_write_cb,
                                             g_steal_pointer (&task));
}

gboolean
jsonrpc_client_notification_finish (JsonrpcClient  *self,
                                    GAsyncResult   *result,
                                    GError        **error)
{
  g_return_val_if_fail (JSONRPC_IS_CLIENT (self), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}
