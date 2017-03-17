#include <stdlib.h>
#include <unistd.h>

#include "jsonrpc-client.h"

static JsonrpcClient *client;
static GMainLoop *main_loop;

static void
close_cb (GObject      *object,
          GAsyncResult *result,
          gpointer      user_data)
{
  g_main_loop_quit (main_loop);
}

static gboolean
timeout_cb (gpointer data)
{
  jsonrpc_client_close_async (client, NULL, close_cb, NULL);
  return G_SOURCE_REMOVE;
}

static void
call_cb (GObject      *object,
         GAsyncResult *result,
         gpointer      user_data)
{
  JsonrpcClient *client = (JsonrpcClient *)object;
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) return_value = NULL;
  g_autofree gchar *str = NULL;

  g_assert (JSONRPC_IS_CLIENT (client));
  g_assert (G_IS_ASYNC_RESULT (result));

  if (!jsonrpc_client_call_finish (client, result, &return_value, &error))
    g_error ("%s", error->message);
}

static void
wait_cb (GObject      *object,
         GAsyncResult *result,
         gpointer      user_data)
{
  GSubprocess *subprocess = (GSubprocess *)object;
  g_autoptr(GError) error = NULL;

  g_assert (G_IS_SUBPROCESS (subprocess));
  g_assert (G_IS_ASYNC_RESULT (result));

  g_message ("rustls exited");

  exit (1);
}

static void
notification_cb (JsonrpcClient *client,
                 const gchar   *method,
                 GVariant      *params,
                 gpointer       user_data)
{
  g_autofree gchar *str = g_variant_print (params, TRUE);

  g_message ("(Notification): %s: %s", method, str);
}

gint
main (gint   argc,
      gchar *argv[])
{
  g_autoptr(GIOStream) io_stream = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GSubprocess) subprocess = NULL;
  g_auto(GVariantDict) params = { 0 };
  GInputStream *stdout_pipe;
  GOutputStream *stdin_pipe;

  main_loop = g_main_loop_new (NULL, FALSE);

  subprocess = g_subprocess_new (G_SUBPROCESS_FLAGS_STDIN_PIPE |
                                 G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                 G_SUBPROCESS_FLAGS_STDERR_SILENCE,
                                 &error,
                                 "rls",
                                 NULL);

  if (subprocess == NULL)
    {
      g_warning ("%s", error->message);
      return EXIT_FAILURE;
    }

  g_message ("rustls started");

  stdin_pipe = g_subprocess_get_stdin_pipe (subprocess);
  stdout_pipe = g_subprocess_get_stdout_pipe (subprocess);
  io_stream = g_object_new (G_TYPE_SIMPLE_IO_STREAM,
                            "input-stream", stdout_pipe,
                            "output-stream", stdin_pipe,
                            NULL);

  client = jsonrpc_client_new (io_stream);

  g_signal_connect (client,
                    "notification",
                    G_CALLBACK (notification_cb),
                    NULL);

  g_subprocess_wait_async (subprocess, NULL, wait_cb, NULL);

  g_autofree gchar *path = g_build_filename (g_get_home_dir (),
                                             "Projects",
                                             "rustls",
                                             "sample_project",
                                             NULL);

  g_variant_dict_init (&params, NULL);
  g_variant_dict_insert (&params, "processId", "i", getpid ());
  g_variant_dict_insert (&params, "rootPath", "s", path);
  g_variant_dict_insert_value (&params, "capabilities", g_variant_new ("a{sv}", NULL, NULL, NULL));

  jsonrpc_client_call_async (client, "initialize", g_variant_dict_end (&params), NULL, call_cb, NULL);

  g_timeout_add_seconds (5, timeout_cb, NULL);

  g_main_loop_run (main_loop);

  return EXIT_SUCCESS;
}
