/* test-server.c
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <gio/gio.h>
#include <gio/gunixoutputstream.h>
#include <gio/gunixinputstream.h>
#include <jsonrpc-glib.h>
#include <unistd.h>

static void
handle_notification (JsonrpcServer *server,
                     JsonrpcClient *client,
                     const gchar   *method,
                     JsonNode      *params,
                     gpointer       user_data)
{
  gint *count = user_data;

  g_assert (JSONRPC_IS_SERVER (server));
  g_assert (JSONRPC_IS_CLIENT (client));
  g_assert (method != NULL);
  g_assert (params != NULL);

  (*count)++;
}

static gboolean
handle_call (JsonrpcServer *server,
             JsonrpcClient *client,
             const gchar   *method,
             JsonNode      *id,
             JsonNode      *params,
             gpointer       user_data)
{
  const gchar *rootPath = NULL;
  g_autoptr(JsonNode) return_value = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (JSON_NODE_HOLDS_VALUE (id));

  JCON_EXTRACT (params, "rootPath", JCONE_STRING (rootPath));

  g_assert_cmpint (1, ==, json_node_get_int (id));
  g_assert_cmpstr (method, ==, "initialize");

  return_value = JCON_NEW ("foo", "bar");

  jsonrpc_client_reply (client,
                        json_node_ref (id),
                        g_steal_pointer (&return_value),
                        NULL,
                        &error);
  g_assert_no_error (error);

  return TRUE;
}

static void
test_basic (void)
{
  g_autoptr(JsonrpcServer) server = NULL;
  g_autoptr(JsonrpcClient) client = NULL;
  g_autoptr(GInputStream) input_a = NULL;
  g_autoptr(GInputStream) input_b = NULL;
  g_autoptr(GOutputStream) output_a = NULL;
  g_autoptr(GOutputStream) output_b = NULL;
  g_autoptr(GIOStream) stream_a = NULL;
  g_autoptr(GIOStream) stream_b = NULL;
  g_autoptr(JsonNode) message = NULL;
  g_autoptr(JsonNode) return_value = NULL;
  g_autoptr(GError) error = NULL;
  gint pair_a[2];
  gint pair_b[2];
  gint count = 0;
  gint r;

  r = pipe2 (pair_a, O_CLOEXEC);
  g_assert_cmpint (r, ==, 0);

  r = pipe2 (pair_b, O_CLOEXEC);
  g_assert_cmpint (r, ==, 0);

  input_a = g_unix_input_stream_new (pair_a[0], TRUE);
  input_b = g_unix_input_stream_new (pair_b[0], TRUE);
  output_a = g_unix_output_stream_new (pair_a[1], TRUE);
  output_b = g_unix_output_stream_new (pair_b[1], TRUE);

  stream_a = g_simple_io_stream_new (input_a, output_b);
  stream_b = g_simple_io_stream_new (input_b, output_a);

  client = jsonrpc_client_new (stream_a);

  server = jsonrpc_server_new ();
  jsonrpc_server_accept_io_stream (server, stream_b);

  g_signal_connect (server,
                    "handle-call",
                    G_CALLBACK (handle_call),
                    NULL);

  g_signal_connect (server,
                    "notification",
                    G_CALLBACK (handle_notification),
                    &count);

  r = jsonrpc_client_notification (client, "testNotification", json_node_new (JSON_NODE_NULL), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (r, ==, TRUE);

  message = JCON_NEW ("rootPath", JCON_STRING ("."));

  r = jsonrpc_client_call (client,
                           "initialize",
                           g_steal_pointer (&message),
                           NULL,
                           &return_value,
                           &error);
  g_assert_no_error (error);
  g_assert_cmpint (r, ==, TRUE);
  g_assert (return_value != NULL);

  g_assert_cmpint (count, ==, 1);
}

gint
main (gint   argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/Jsonrpc/Server/basic", test_basic);
  return g_test_run ();
}
