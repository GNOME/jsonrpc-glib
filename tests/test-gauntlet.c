/* test-gauntlet.c
 *
 * Copyright © 2018 Christian Hergert <chergert@redhat.com>
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

#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <glib-unix.h>
#include <jsonrpc-glib.h>
#include <signal.h>

static GMainLoop *main_loop;

static void
server_handle_call (JsonrpcClient *server,
                    const gchar   *method,
                    GVariant      *id,
                    GVariant      *params,
                    gpointer       user_data)
{
  g_assert (JSONRPC_IS_CLIENT (server));
  g_assert (g_str_equal (method, "do/something"));
  g_assert (id != NULL);
  g_assert (g_variant_is_of_type (id, G_VARIANT_TYPE_INT64));
  g_assert (params != NULL);
  g_assert (g_variant_is_of_type (params, G_VARIANT_TYPE_INT64));
  g_assert (g_variant_get_int64 (params) == 42);
  g_assert (user_data == NULL);

  jsonrpc_client_close (server, NULL, NULL);
}

static void
call_cb (GObject      *object,
         GAsyncResult *result,
         gpointer      user_data)
{
  g_autoptr(GVariant) res = NULL;
  g_autoptr(GError) error = NULL;
  gboolean r;

  r = jsonrpc_client_call_finish (JSONRPC_CLIENT (object), result, &res, &error);

  if (r == FALSE)
    {
      g_assert_cmpint (error->domain, ==, G_IO_ERROR);
#if 0
      /* We can't really guarantee this, given the ways the socket errors
       * can be propagated.
       */
      g_assert (error->code == 0 || error->code == G_IO_ERROR_NOT_CONNECTED);
#endif
    }

  g_main_loop_quit (main_loop);
}

static void
test_gauntlet (void)
{
  g_autoptr(JsonrpcClient) client = NULL;
  g_autoptr(JsonrpcClient) server = NULL;
  g_autoptr(GInputStream) input_a = NULL;
  g_autoptr(GInputStream) input_b = NULL;
  g_autoptr(GOutputStream) output_a = NULL;
  g_autoptr(GOutputStream) output_b = NULL;
  g_autoptr(GIOStream) stream_a = NULL;
  g_autoptr(GIOStream) stream_b = NULL;
  g_autoptr(GError) error = NULL;
  gboolean r;
  gint pair_a[2];
  gint pair_b[2];

  signal (SIGPIPE, SIG_IGN);

  main_loop = g_main_loop_new (NULL, FALSE);

  r = g_unix_open_pipe (pair_a, FD_CLOEXEC, &error);
  g_assert_no_error (error);
  g_assert_cmpint (r, ==, TRUE);

  r = g_unix_open_pipe (pair_b, FD_CLOEXEC, &error);
  g_assert_no_error (error);
  g_assert_cmpint (r, ==, TRUE);

  input_a = g_unix_input_stream_new (pair_a[0], TRUE);
  input_b = g_unix_input_stream_new (pair_b[0], TRUE);
  output_a = g_unix_output_stream_new (pair_a[1], TRUE);
  output_b = g_unix_output_stream_new (pair_b[1], TRUE);

  stream_a = g_simple_io_stream_new (input_a, output_b);
  stream_b = g_simple_io_stream_new (input_b, output_a);

  client = jsonrpc_client_new (stream_a);
  jsonrpc_client_start_listening (client);

  server = jsonrpc_client_new (stream_b);
  g_signal_connect (server,
                    "handle-call",
                    G_CALLBACK (server_handle_call),
                    NULL);
  jsonrpc_client_start_listening (server);

  /* Try a call, peer will close instead of replying */
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (42), NULL, NULL, NULL);
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (42), NULL, NULL, NULL);
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (42), NULL, NULL, NULL);
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (42), NULL, call_cb, NULL);
  g_main_loop_run (main_loop);

  /* Try a call, peer is closed, expect failure */
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (42), NULL, NULL, NULL);
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (42), NULL, NULL, NULL);
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (42), NULL, NULL, NULL);
  jsonrpc_client_call_async (client, "do/something", g_variant_new_int32 (43), NULL, call_cb, NULL);
  g_main_loop_run (main_loop);
}

gint
main (gint   argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/Jsonrpc/Client/gauntlet", test_gauntlet);
  return g_test_run ();
}
