/* GstSwitch
 * Copyright (C) 2012,2013 Duzy Chan <code@duzy.info>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <gst/gst.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "../tools/gstswitchclient.h"
#include "../tools/gstcomposite.h"
#include "../tools/gstcase.h"
#include "../logutils.h"

#define ASSERT_TEST_PIPELINE_ERRORS 0
#define TEST_RECORDING_DATA 0
#define TEST_DISPLAY_VIDEO_RESULT 0
#if TEST_DISPLAY_VIDEO_RESULT
#define VIDEOSINK "xvimagesink"
#else
#define VIDEOSINK "fakesink"
#endif //TEST_DISPLAY_VIDEO_RESULT
#define AUDIOSINK "alsasink"

gboolean verbose = FALSE;

static GMutex children_lock = { 0 };

static GList *children = NULL;

enum
{
#if ENABLE_LOW_RESOLUTION
  W = LOW_RES_W, H = LOW_RES_H,
#else
  W = 1280, H = 720,
#endif
};

gboolean infinity_open_file = FALSE;

static struct
{
  gboolean enable_test_controller;
  gboolean enable_test_composite_mode;
  gboolean enable_test_video;
  gboolean enable_test_audio;
  gboolean enable_test_ui;
  gboolean enable_test_random;
  gboolean enable_test_switching;
  gboolean enable_test_fuzz;
  gboolean enable_test_checking_timestamps;
  gboolean enable_test_multiple_clients;
  gboolean test_external_server;
  gboolean test_external_ui;
  gboolean valgrind;
  gboolean print_debug_messages;
} opts = {
.enable_test_controller = FALSE,.enable_test_composite_mode =
      FALSE,.enable_test_video = FALSE,.enable_test_audio =
      FALSE,.enable_test_ui = FALSE,.enable_test_random =
      FALSE,.enable_test_switching = FALSE,.enable_test_fuzz =
      FALSE,.enable_test_checking_timestamps = FALSE,.test_external_server =
      FALSE,.test_external_ui = FALSE,.valgrind =
      FALSE,.print_debug_messages = FALSE,};

static GOptionEntry option_entries[] = {
  {"enable-test-controller", 0, 0, G_OPTION_ARG_NONE,
      &opts.enable_test_controller, "Enable testing controller", NULL},
  {"enable-test-composite-mode", 0, 0, G_OPTION_ARG_NONE,
        &opts.enable_test_composite_mode, "Enable testing composite mode",
      NULL},
  {"enable-test-video", 0, 0, G_OPTION_ARG_NONE, &opts.enable_test_video,
      "Enable testing video", NULL},
  {"enable-test-audio", 0, 0, G_OPTION_ARG_NONE, &opts.enable_test_audio,
      "Enable testing audio", NULL},
  {"enable-test-ui", 0, 0, G_OPTION_ARG_NONE, &opts.enable_test_ui,
      "Enable testing UI", NULL},
  {"enable-test-random", 0, 0, G_OPTION_ARG_NONE, &opts.enable_test_random,
      "Enable testing random input streams", NULL},
  {"enable-test-switching", 0, 0, G_OPTION_ARG_NONE,
      &opts.enable_test_switching, "Enable testing switching", NULL},
  {"enable-test-fuzz", 0, 0, G_OPTION_ARG_NONE, &opts.enable_test_fuzz,
      "Enable testing fuzz input", NULL},
  {"enable-test-checking-timestamps", 0, 0, G_OPTION_ARG_NONE,
        &opts.enable_test_checking_timestamps,
      "Enable testing checking timestamps", NULL},
  {"enable-test-multiple-clients", 0, 0, G_OPTION_ARG_NONE,
        &opts.enable_test_multiple_clients, "Enable testing multiple clients",
      NULL},
  {"test-external-server", 0, 0, G_OPTION_ARG_NONE, &opts.test_external_server,
      "Testing external server", NULL},
  {"test-external-ui", 0, 0, G_OPTION_ARG_NONE, &opts.test_external_ui,
      "Testing external ui", NULL},
  {"valgrind", 0, 0, G_OPTION_ARG_NONE, &opts.valgrind, "Use valgrind", NULL},
  {"print-debug-messages", 0, 0, G_OPTION_ARG_NONE, &opts.print_debug_messages,
      "Print debug messages", NULL},
  {NULL}
};

typedef struct _testcase testcase;
struct _testcase
{
  const gchar *name;
  gboolean free_name;
  GMainLoop *mainloop;
  GstElement *pipeline;
  GMutex lock;
  GThread *thread;
  GString *desc;
  gint timer;
  gint live_seconds;
  gint error_count;
  void (*null_func) (testcase * t, gpointer data);
  gpointer null_func_data;
  gboolean errors_are_ok;
  gboolean pass;
};

static void
testcase_quit (testcase * t)
{
  if (GST_IS_ELEMENT (t->pipeline))
    gst_element_set_state (t->pipeline, GST_STATE_NULL);
  g_main_loop_quit (t->mainloop);
}

static void
testcase_fail (testcase * t)
{
  testcase_quit (t);
  g_test_fail ();
}

static void
testcase_ok (testcase * t)
{
  testcase_quit (t);
}

static void
testcase_state_change (testcase * t, GstState oldstate, GstState newstate,
    GstState pending)
{
  GstStateChange statechange = GST_STATE_TRANSITION (oldstate, newstate);
  switch (statechange) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (GST_IS_ELEMENT (t->pipeline))
        gst_element_set_state (t->pipeline, GST_STATE_PAUSED);
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (GST_IS_ELEMENT (t->pipeline))
        gst_element_set_state (t->pipeline, GST_STATE_PLAYING);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      /*
         INFO ("quit: %s\n", t->name);
       */
      if (t->null_func)
        t->null_func (t, t->null_func_data);
      testcase_ok (t);
      break;
    default:
      break;
  }
}

static void
testcase_error_message (testcase * t, GError * error, const gchar * info)
{
#if 0
  ERROR ("%s: %s", t->name, error->message);
  ERROR ("debug:\n%s\n", info);
#endif

  t->error_count += 1;

  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_TOO_LAZY));
  g_assert (!g_error_matches (error, GST_CORE_ERROR,
          GST_CORE_ERROR_NOT_IMPLEMENTED));
  g_assert (!g_error_matches (error, GST_CORE_ERROR,
          GST_CORE_ERROR_STATE_CHANGE));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_PAD));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_THREAD));
  g_assert (!g_error_matches (error, GST_CORE_ERROR,
          GST_CORE_ERROR_NEGOTIATION));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_EVENT));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_SEEK));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_CAPS));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_TAG));
  g_assert (!g_error_matches (error, GST_CORE_ERROR,
          GST_CORE_ERROR_MISSING_PLUGIN));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_CLOCK));
  g_assert (!g_error_matches (error, GST_CORE_ERROR, GST_CORE_ERROR_DISABLED));
  g_assert (!g_error_matches (error, GST_CORE_ERROR,
          GST_CORE_ERROR_NUM_ERRORS));
}

static gboolean
testcase_pipeline_message (GstBus * bus, GstMessage * message, gpointer data)
{
  testcase *t = (testcase *) data;
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED:
    {
      if (GST_ELEMENT (message->src) == t->pipeline) {
        GstState oldstate, newstate, pending;
        gst_message_parse_state_changed (message, &oldstate, &newstate,
            &pending);
        testcase_state_change (t, oldstate, newstate, pending);
      }
    }
      break;
    case GST_MESSAGE_ERROR:
    {
      GError *error = NULL;
      gchar *info = NULL;
      gst_message_parse_error (message, &error, &info);
      testcase_error_message (t, error, info);
      g_error_free (error);
      g_free (info);
    }
      break;
    case GST_MESSAGE_EOS:
    {
      if (GST_IS_ELEMENT (t->pipeline))
        gst_element_set_state (t->pipeline, GST_STATE_NULL);
      testcase_ok (t);
    }
      break;
    default:
    {
      /*
         INFO ("%s: %s", GST_OBJECT_NAME (message->src), GST_MESSAGE_TYPE_NAME (message));
       */
    }
      break;
  }
  return TRUE;
}

#if 0
static void
testcase_client_socket_removed (GstElement * element,
    GSocket * socket, testcase * t)
{
  g_socket_close (socket, NULL);
}
#endif

static gboolean
testcase_launch_pipeline (testcase * t)
{
  GError *error = NULL;
  GstBus *bus;

  if (!t->desc) {
    ERROR ("invalid test case");
    testcase_fail (t);
    return FALSE;
  }

  t->pipeline = (GstElement *) gst_parse_launch (t->desc->str, &error);

  if (error) {
    ERROR ("%s: %s", t->name, error->message);
    testcase_fail (t);
    return FALSE;
  }
#if 0
  {
    GstElement *tcp_sink = gst_bin_get_by_name (t->pipeline, "tcp_sink");
    if (tcp_sink) {
      g_signal_connect (tcp_sink, "client-socket-removed",
          G_CALLBACK (testcase_client_socket_removed), t);
      gst_object_unref (tcp_sink);
    }
  }
#endif

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (t->pipeline), TRUE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (t->pipeline));
  gst_bus_add_watch (bus, testcase_pipeline_message, t);
  gst_element_set_state (t->pipeline, GST_STATE_READY);
  gst_object_unref (bus);
  return TRUE;
}

static void
child_quit (GPid pid, gint status, gpointer data)
{
  GOutputStream *ostream = G_OUTPUT_STREAM (data);
  GError *error = NULL;

  INFO ("quit %d", pid);
  {
    g_mutex_lock (&children_lock);
    children = g_list_remove (children, (gconstpointer) GINT_TO_POINTER(pid));
    g_mutex_unlock (&children_lock);
  }

  g_output_stream_flush (ostream, NULL, &error);
  g_assert_no_error (error);
  g_output_stream_close (ostream, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (ostream);
}

static void
child_stdout (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  char buf[1024];
  GError *error = NULL;
  gsize bytes_read;
  GOutputStream *ostream = G_OUTPUT_STREAM (data);

  INFO ("out");

  if (condition & G_IO_IN) {
    gsize bytes_written;
    GIOStatus status =
        g_io_channel_read_chars (channel, buf, sizeof (buf), &bytes_read,
        &error);
    g_assert_no_error (error);
    g_output_stream_write_all (ostream, buf, bytes_read, &bytes_written, NULL,
        &error);
    g_assert_no_error (error);
    g_assert_cmpint (bytes_read, ==, bytes_written);
    (void) status;
  }
  if (condition & G_IO_HUP) {
    g_output_stream_flush (ostream, NULL, &error);
    g_assert_no_error (error);
  }
}

static void
child_stderr (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  //INFO ("err");
}

typedef struct _log_dumper
{
  gint fd;
  GOutputStream *ostream;
} log_dumper;
static gpointer
dump_log (gpointer data)
{
  enum
  { max_len = 1024 };
  log_dumper *l = (log_dumper *) data;
  gchar buffer[max_len];
  ssize_t len = 0;
  GError *error = NULL;
  gsize bytes_written;

  do {
    len = read (l->fd, buffer, max_len);
    if (len < 0) {
      ERROR ("%s", strerror (errno));
      break;
    }
    if (!G_IS_OUTPUT_STREAM (l->ostream))
      break;
    g_output_stream_write_all (l->ostream, buffer, len, &bytes_written, NULL,
        &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, bytes_written);
  } while (TRUE);

  close (l->fd);

  g_free (l);
  g_thread_unref (g_thread_self ());
  return NULL;
}

static GPid
launch (const gchar * name, ...)
{
  GPid pid = 0;
  GError *error = NULL;
  GMainContext *context;
  GIOChannel *channel;
  GSource *source;
  gint fd_in = 0, fd_out = 0, fd_err = 0;
  GFileOutputStream *outstream;
  GFile *outfile;
  gchar *outfilename;
  gchar **argv;
  gboolean ok;
  GPtrArray *array = g_ptr_array_new ();
  const gchar *arg;
  va_list va;
  va_start (va, name);
  for (arg = name; arg; arg = va_arg (va, const gchar *))
      g_ptr_array_add (array, g_strdup (arg));
  va_end (va);
  g_ptr_array_add (array, NULL);
  argv = (gchar **) g_ptr_array_free (array, FALSE);

  g_print ("cmdline: %s\n", g_strjoinv (" \\\n ", argv));

  if (!opts.print_debug_messages) {
    ok = g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
        NULL, NULL, &pid, &fd_in, &fd_out, &fd_err, &error);
  } else {
    (void) fd_in, (void) fd_out, (void) fd_err;
    ok = g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
        NULL, NULL, &pid, NULL, NULL, NULL, &error);
  }

  if (ok) {
    g_mutex_lock (&children_lock);
    children = g_list_append (children, (gpointer) GINT_TO_POINTER(pid));
    g_mutex_unlock (&children_lock);
  }

  g_free (argv);

  g_assert_no_error (error);
  g_assert (ok);

  context = g_main_context_default ();

  outfilename = g_strdup_printf ("test-server-%d.log", pid);
  outfile = g_file_new_for_path (outfilename);
  outstream = g_file_create (outfile, G_FILE_CREATE_NONE, NULL, &error);

  g_free (outfilename);
  g_assert_no_error (error);
  g_assert (outfile);
  g_assert (outstream);

  if (fd_out) {
#if 0
    channel = g_io_channel_unix_new (fd_out);
    source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
    g_source_set_callback (source, (GSourceFunc) child_stdout, outstream, NULL);
    g_source_attach (source, context);
    g_source_unref (source);
    g_io_channel_unref (channel);
#else
    (void) source, (void) channel, (void) context, (void) child_stdout;
    log_dumper *p = g_new0 (log_dumper, 1);
    p->fd = fd_out;
    p->ostream = G_OUTPUT_STREAM (outstream);
    g_thread_new ("log-dumper", dump_log, p);
#endif
  }

  (void) child_stderr;

  /*
     channel = g_io_channel_unix_new (fd_err);
     source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
     g_source_set_callback (source, (GSourceFunc) child_stderr, NULL, NULL);
     g_source_attach (source, context);
     g_source_unref (source);
     g_io_channel_unref (channel);
   */

  g_child_watch_add (pid, child_quit, outstream);

  //g_main_context_unref (context);
  return pid;
}

static GPid
launch_server ()
{
  GPid pid;

  if (opts.valgrind) {
    pid = launch ("/usr/bin/valgrind",
        //"-v",
        "--show-reachable=yes",
        //"--trace-children=yes"
        "--track-origins=yes",
        "--track-fds=yes",
        "--tool=memcheck",
        "--leak-check=full",
        "--leak-resolution=high",
        "--num-callers=20",
        "--log-file=test-switch-server-valgrind.log",
        "--suppressions=../../gstreamer/common/gst.supp",
        "--suppressions=../../gst-plugins-base/tests/check/gst-plugins-base.supp",
        "--suppressions=../../gst-plugins-good/tests/check/gst-plugins-good.supp",
        "--suppressions=../../gst-plugins-bad/tests/check/gst-plugins-bad.supp",
        "--suppressions=../../gst-plugins-ugly/tests/check/gst-plugins-ugly.supp",
        "--suppressions=faac.supp",
        "../tools/gst-switch-srv", "-v",
        "--gst-debug-no-color", "--record=test-recording.data", NULL);
  } else {
    pid = launch ("../tools/gst-switch-srv", "-v",
        "--gst-debug-no-color", "--record=test-recording.data", NULL);
  }

  sleep (5);
  INFO ("server %d", pid);
  return pid;
}

static GPid
launch_ui ()
{
  GPid pid = launch ("../tools/gst-switch-ui", "-v",
      "--gst-debug-no-color",
      NULL);
  INFO ("ui %d", pid);
  return pid;
}

static void
close_pid (GPid pid)
{
  //kill (pid, SIGKILL);
  kill (pid, SIGTERM);
  g_spawn_close_pid (pid);
  sleep (2);                    /* give a second for cleaning up */
}

static gboolean
testcase_second_timer (testcase * t)
{
  t->live_seconds -= 1;

  if (t->live_seconds == 0) {
    testcase_ok (t);
  }

  return 0 < t->live_seconds ? TRUE : FALSE;
}

static gpointer
testcase_run (testcase * t)
{
  g_mutex_init (&t->lock);

  g_print ("Running %s (%d seconds)\n", t->name, t->live_seconds);
  t->pass = TRUE;               // assume everything is ok
  t->mainloop = g_main_loop_new (NULL, TRUE);
  if (testcase_launch_pipeline (t)) {
    if (0 < t->live_seconds) {
      t->timer = g_timeout_add (1000, (GSourceFunc) testcase_second_timer, t);
    }
    g_main_loop_run (t->mainloop);
    gst_object_unref (t->pipeline);
  } else {
    ERROR ("launch failed");
    g_test_fail ();
  }
  g_string_free (t->desc, FALSE);
  if (t->timer)
    g_source_remove (t->timer);
  //t->mainloop = NULL;
  t->pipeline = NULL;
  t->desc = NULL;
  t->timer = 0;

  if (0 < t->error_count) {
    if (t->errors_are_ok) {
      t->pass = t->pass && TRUE;
    } else {
      t->pass = FALSE;
      ERROR ("%s: %d errors", t->name, t->error_count);
    }
  } else {
    t->pass = t->pass && TRUE;
  }

  if (t->free_name) {
    g_free ((gpointer) t->name);
    t->name = NULL;
  }

  g_mutex_lock (&t->lock);
  g_thread_unref (t->thread);
  t->thread = NULL;
  g_mutex_unlock (&t->lock);
  return NULL;
}

static void
testcase_run_thread (testcase * t)
{
  t->thread = g_thread_new (t->name, (GThreadFunc) testcase_run, t);
}

static void
testcase_join (testcase * t)
{
  GThread *thread = NULL;
  g_mutex_lock (&t->lock);
  if (t->thread) {
    thread = t->thread;
    g_thread_ref (thread);
  }
  g_mutex_unlock (&t->lock);
  if (thread)
    g_thread_join (thread);
}

/*
static void
testcase_end (testcase *t)
{
  if (t->mainloop)
    g_main_loop_quit (t->mainloop);
  //testcase_join (t);
}
*/

typedef struct _testclient
{
  GstSwitchClient base;
  GThread *thread;
  GThread *thread_test1;
  GThread *thread_test2;
  GThread *thread_test3;
  GThread *thread_test4;
  gboolean enable_thread_test1;
  gboolean enable_thread_test2;
  gboolean enable_thread_test3;
  gboolean enable_thread_test4;
  GMainLoop *mainloop;
  gint audio_port0;
  gint audio_port;
  gint audio_port_count;
  gint compose_port0;
  gint compose_port;
  gint compose_port_count;
  gint encode_port0;
  gint encode_port;
  gint encode_port_count;
  gint new_mode_count;
  gint preview_port_1;
  gint preview_port_2;
  gint preview_port_3;
  gint preview_port_4;
  gint preview_type_1;
  gint preview_type_2;
  gint preview_type_3;
  gint preview_type_4;
  gint preview_port_count;
  guint seconds;
  testcase sinka;
  GMutex sink0_lock;
  GList *sink0;
  testcase sink1;
  testcase sink2;
  testcase sink3;
  testcase sink4;
  gboolean enable_test_sinks;
  GMutex expected_compose_count_lock;
  gint expected_compose_count;
} testclient;

typedef struct _testclientClass
{
  GstSwitchClientClass baseclass;
} testclientClass;

GType testclient_get_type (void);

#define TYPE_TESTCLIENT (testclient_get_type ())
#define TESTCLIENT(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_TESTCLIENT, testclient))
#define TESTCLIENTCLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), TYPE_TESTCLIENT, testclientClass))
G_DEFINE_TYPE (testclient, testclient, GST_TYPE_SWITCH_CLIENT);

static gint clientcount = 0;
static void
testclient_init (testclient * client)
{
  ++clientcount;
  client->mainloop = NULL;
  //INFO ("client init");

  g_mutex_init (&client->sink0_lock);
  g_mutex_init (&client->expected_compose_count_lock);

  client->thread = NULL;
  client->sink0 = NULL;
  client->expected_compose_count = 0;
  client->enable_test_sinks = FALSE;
}

static void
testclient_finalize (testclient * client)
{
  --clientcount;
  //INFO ("client finalize");

  g_mutex_clear (&client->sink0_lock);
  g_mutex_clear (&client->expected_compose_count_lock);
}

/*
static gboolean
testclient_has_sink0 (testclient *client)
{
  gboolean b = FALSE;
  g_mutex_lock (&client->sink0_lock);
  if (client->sink0) b = TRUE;
  g_mutex_unlock (&client->sink0_lock);
  return b;
}
*/

static gpointer
testclient_test_pip (testclient * client)
{
  usleep (100000);
  while (client->thread) {
    usleep (50000);
    if (gst_switch_client_is_connected (GST_SWITCH_CLIENT (client))) {
    }
  }
  return NULL;
}

static gpointer
testclient_test_mode (testclient * client)
{
  gboolean ok;
  gint mode = 0;
  usleep (100000);
  while (client->thread) {
    usleep (50000);
    if (gst_switch_client_is_connected (GST_SWITCH_CLIENT (client))) {
      usleep (1000000 * 1);
      if (3 < mode)
        mode = 0;
      ok = gst_switch_client_set_composite_mode (GST_SWITCH_CLIENT (client),
          mode);
      if (!ok) {
#if 0
        WARN ("failed changing mode: %d", mode);
#endif
      } else {
        g_mutex_lock (&client->expected_compose_count_lock);
        client->expected_compose_count += 1;
        g_mutex_unlock (&client->expected_compose_count_lock);
      }
      mode += 1;
    } else {
      usleep (50000);
    }
  }
  return NULL;
}

static gpointer
testclient_test_mode_2 (testclient * client)
{
  gboolean ok;
  gint mode = 0;
  usleep (100000);
  while (client->thread) {
    usleep (10000);
    if (gst_switch_client_is_connected (GST_SWITCH_CLIENT (client))) {
      if (3 < mode)
        mode = 0;
      ok = gst_switch_client_set_composite_mode (GST_SWITCH_CLIENT (client),
          mode);
      if (!ok) {
#if 0
        WARN ("failed changing mode: %d", mode);
#endif
      } else {
        g_mutex_lock (&client->expected_compose_count_lock);
        client->expected_compose_count += 1;
        g_mutex_unlock (&client->expected_compose_count_lock);
      }
      mode += 1;
    } else {
      usleep (50000);
    }
  }
  return NULL;
}

static void
testclient_end (testclient * client)
{
  g_main_loop_quit (client->mainloop);
}

static void
testclient_connection_closed (testclient * client, GError * error)
{
  INFO ("closed: %s", error ? error->message : "");
  testclient_end (client);
}

static void
testclient_remove_sink0 (testcase * t, gpointer data)
{
  testclient *client = (testclient *) data;
  //INFO ("remove: %s", t->name);
  g_mutex_lock (&client->sink0_lock);
  client->sink0 = g_list_remove (client->sink0, t);
  g_mutex_unlock (&client->sink0_lock);
}

static void
testclient_set_compose_port (testclient * client, gint port)
{
  //INFO ("set-compose-port: %d", port);
  client->compose_port = port;
  client->compose_port_count += 1;
  g_assert_cmpint (client->compose_port, !=, 0);
  //g_assert_cmpint (client->compose_port0, ==, client->compose_port);
  if (client->enable_test_sinks && 0 < client->sink1.live_seconds) {
    testcase *sink0 = g_new0 (testcase, 1);
    sink0->live_seconds = client->sink1.live_seconds;
    sink0->name =
        g_strdup_printf ("test-compose-output-%d-%d", port,
        client->compose_port_count);
    sink0->free_name = TRUE;
    sink0->desc = g_string_new ("");
    g_string_append_printf (sink0->desc, "tcpclientsrc port=%d ",
        client->compose_port);
    g_string_append_printf (sink0->desc, "! gdpdepay ");
    g_string_append_printf (sink0->desc, "! videoconvert ");
    g_string_append_printf (sink0->desc, "! " VIDEOSINK);
    testcase_run_thread (sink0);
    g_mutex_lock (&client->sink0_lock);
    sink0->null_func = testclient_remove_sink0;
    sink0->null_func_data = client;
    client->sink0 = g_list_append (client->sink0, sink0);
    g_mutex_unlock (&client->sink0_lock);
  }
}

static void
testclient_set_encode_port (testclient * client, gint port)
{
  //INFO ("set-encode-port: %d", port);
  client->encode_port = port;
  client->encode_port_count += 1;
  g_assert_cmpint (client->encode_port, !=, 0);
  //g_assert_cmpint (client->encode_port0, ==, client->encode_port);
}

static void
testclient_new_mode (testclient * client, gint mode)
{
  //INFO ("set-encode-port: %d", port);
  client->new_mode_count += 1;
  g_assert_cmpint (mode, >=, COMPOSE_MODE_NONE);
  g_assert_cmpint (mode, <=, COMPOSE_MODE_DUAL_EQUAL);
}

static void
testclient_set_audio_port (testclient * client, gint port)
{
  //INFO ("set-audio-port: %d", port);
  client->audio_port = port;
  client->audio_port_count += 1;
  /*
     if (client->preview_port_3) {
     g_assert_cmpint (client->audio_port, ==, client->preview_port_3);
     }
   */
}

static void
testclient_add_preview_port (testclient * client, gint port, gint type)
{
  //INFO ("add-preview-port: %d, %d", port, type);
  client->preview_port_count += 1;
  switch (client->preview_port_count) {
    case 1:
      client->preview_port_1 = port;
      client->preview_type_1 = type;
      //g_assert_cmpint (type, ==, GST_SERVE_VIDEO_STREAM);
      if (client->enable_test_sinks) {
        g_assert (client->sink1.thread == NULL);
        client->sink1.live_seconds = client->seconds;
        client->sink1.name = g_strdup_printf ("test-preview-output-%d", port);
        client->sink1.free_name = TRUE;
        client->sink1.desc = g_string_new ("");
        g_string_append_printf (client->sink1.desc, "tcpclientsrc port=%d ",
            client->preview_port_1);
        g_string_append_printf (client->sink1.desc, "! gdpdepay ");
#if 0
        g_string_append_printf (client->sink1.desc, "! videoconvert ");
        g_string_append_printf (client->sink1.desc, "! " VIDEOSINK);
#else
        g_string_append_printf (client->sink1.desc, "! fakesink ");
#endif
        testcase_run_thread (&client->sink1);
      }
      break;
    case 2:
      client->preview_port_2 = port;
      client->preview_type_2 = type;
      //g_assert_cmpint (type, ==,GST_SERVE_VIDEO_STREAM);
      if (client->enable_test_sinks) {
        g_assert (client->sink2.thread == NULL);
        client->sink2.live_seconds = client->seconds;
        client->sink2.name = g_strdup_printf ("test-preview-output-%d", port);
        client->sink2.free_name = TRUE;
        client->sink2.desc = g_string_new ("");
        g_string_append_printf (client->sink2.desc, "tcpclientsrc port=%d ",
            client->preview_port_2);
        g_string_append_printf (client->sink2.desc, "! gdpdepay ");
#if 0
        g_string_append_printf (client->sink2.desc, "! videoconvert ");
        g_string_append_printf (client->sink2.desc, "! " VIDEOSINK);
#else
        g_string_append_printf (client->sink2.desc, "! fakesink ");
#endif
        testcase_run_thread (&client->sink2);
      }
      break;
    case 3:
      client->preview_port_3 = port;
      client->preview_type_3 = type;
      /*
         if (client->audio_port) {
         g_assert_cmpint (client->audio_port, ==, client->preview_port_3);
         }
       */
      //g_assert_cmpint (type, ==, GST_SERVE_AUDIO_STREAM);
      if (client->enable_test_sinks) {
        g_assert (client->sink3.thread == NULL);
        client->sink3.live_seconds = client->seconds;
        client->sink3.name = g_strdup_printf ("test-preview-output-%d", port);
        client->sink3.free_name = TRUE;
        client->sink3.desc = g_string_new ("");
        g_string_append_printf (client->sink3.desc, "tcpclientsrc port=%d ",
            client->preview_port_3);
        g_string_append_printf (client->sink3.desc, "! gdpdepay ");
#if 0
        g_string_append_printf (client->sink3.desc, "! goom2k1 ");
        g_string_append_printf (client->sink3.desc, "! videoconvert ");
        g_string_append_printf (client->sink3.desc, "! " VIDEOSINK);
#else
        g_string_append_printf (client->sink3.desc, "! fakesink ");
#endif
        testcase_run_thread (&client->sink3);
      }
      break;
    case 4:
      client->preview_port_4 = port;
      client->preview_type_4 = type;
      //g_assert_cmpint (type, ==, GST_SERVE_AUDIO_STREAM);
      if (client->enable_test_sinks) {
        g_assert (client->sink4.thread == NULL);
        client->sink4.live_seconds = client->seconds;
        client->sink4.name = g_strdup_printf ("test-preview-output-%d", port);
        client->sink4.free_name = TRUE;
        client->sink4.desc = g_string_new ("");
        g_string_append_printf (client->sink4.desc, "tcpclientsrc port=%d ",
            client->preview_port_4);
        g_string_append_printf (client->sink4.desc, "! gdpdepay ");
#if 0
        g_string_append_printf (client->sink4.desc, "! goom2k1 ");
        g_string_append_printf (client->sink4.desc, "! videoconvert ");
        g_string_append_printf (client->sink4.desc, "! " VIDEOSINK);
#else
        g_string_append_printf (client->sink4.desc, "! fakesink ");
#endif
        testcase_run_thread (&client->sink4);
      }
      break;
  }
}

static void
testclient_class_init (testclientClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstSwitchClientClass *client_class = GST_SWITCH_CLIENT_CLASS (klass);
  object_class->finalize = (GObjectFinalizeFunc) testclient_finalize;
  client_class->connection_closed = (GstSwitchClientConnectionClosedFunc)
      testclient_connection_closed;
  client_class->set_compose_port = (GstSwitchClientSetComposePortFunc)
      testclient_set_compose_port;
  client_class->set_encode_port = (GstSwitchClientSetEncodePortFunc)
      testclient_set_encode_port;
  client_class->set_audio_port = (GstSwitchClientSetAudioPortFunc)
      testclient_set_audio_port;
  client_class->add_preview_port = (GstSwitchClientAddPreviewPortFunc)
      testclient_add_preview_port;
  client_class->new_mode_online = (GstSwitchClientNewModeOnlineFunc)
      testclient_new_mode;
}

static gpointer
testclient_run (gpointer data)
{
  testclient *client = (testclient *) data;
  gboolean connect_ok = FALSE;
  client->mainloop = g_main_loop_new (NULL, TRUE);

  connect_ok =
      gst_switch_client_connect (GST_SWITCH_CLIENT (client), CLIENT_ROLE_UI, "tcp:host=127.0.0.1,port=5000");
  g_assert (connect_ok);

  client->compose_port0 =
      gst_switch_client_get_compose_port (GST_SWITCH_CLIENT (client));
  client->encode_port0 =
      gst_switch_client_get_encode_port (GST_SWITCH_CLIENT (client));
  client->audio_port0 =
      gst_switch_client_get_audio_port (GST_SWITCH_CLIENT (client));
  g_assert_cmpint (client->compose_port0, !=, 0);
  g_assert_cmpint (client->encode_port0, !=, 0);
  if (!opts.test_external_server) {
    /* audio is not allocated initially
     */
    //g_assert_cmpint (client->audio_port0, ==, 0);
  }

  testclient_set_compose_port (client, client->compose_port0);

  g_main_loop_run (client->mainloop);
  return NULL;
}

static void
testclient_run_thread (testclient * client)
{
  client->thread = g_thread_new ("testclient", testclient_run, client);
  if (client->enable_thread_test1)
    client->thread_test1 =
        g_thread_new ("testclient-pip", (GThreadFunc) testclient_test_pip,
        client);
  if (client->enable_thread_test2)
    client->thread_test2 =
        g_thread_new ("testclient-mode", (GThreadFunc) testclient_test_mode,
        client);
  if (client->enable_thread_test3)
    client->thread_test3 =
        g_thread_new ("testclient-mode_hight_1",
        (GThreadFunc) testclient_test_mode_2, client);
  if (client->enable_thread_test4)
    client->thread_test4 =
        g_thread_new ("testclient-mode_hight_2",
        (GThreadFunc) testclient_test_mode_2, client);
}

static void
testclient_join (testclient * client)
{
  GList *sink0;

  g_thread_join (client->thread);
  g_thread_unref (client->thread);
  client->thread = NULL;

  if (client->thread_test1) {
    g_thread_join (client->thread_test1);
    g_thread_unref (client->thread_test1);
  }
  if (client->thread_test2) {
    g_thread_join (client->thread_test2);
    g_thread_unref (client->thread_test2);
  }
  if (client->thread_test3) {
    g_thread_join (client->thread_test3);
    g_thread_unref (client->thread_test3);
  }
  if (client->thread_test4) {
    g_thread_join (client->thread_test4);
    g_thread_unref (client->thread_test4);
  }
  client->thread_test1 = NULL;
  client->thread_test2 = NULL;
  client->thread_test3 = NULL;
  client->thread_test4 = NULL;

  g_mutex_lock (&client->sink0_lock);
  for (sink0 = client->sink0; sink0; sink0 = g_list_next (sink0)) {
    testcase_join (((testcase *) (sink0->data)));
  }
  g_list_free_full (client->sink0, g_free);
  client->sink0 = NULL;

  g_mutex_unlock (&client->sink0_lock);
  testcase_join (&client->sink1);
  testcase_join (&client->sink2);
  testcase_join (&client->sink3);
  testcase_join (&client->sink4);

  if (client->encode_port != 0)
    g_assert_cmpint (client->encode_port0, ==, client->encode_port);
  if (client->compose_port != 0)
    g_assert_cmpint (client->compose_port0, ==, client->compose_port);

#if ASSERT_TEST_PIPELINE_ERRORS
  g_assert (client->sink1.pass);
  g_assert (client->sink2.pass);
  g_assert (client->sink3.pass);
  g_assert (client->sink4.pass);
#endif
}

static void
test_controller (void)
{
  enum
  { seconds = 30 };
  GPid server_pid = 0;
  testclient *client;
  testcase video_source1 = { "test-video-source1", 0 };
  testcase video_source2 = { "test-video-source2", 0 };
  testcase audio_source1 = { "test-audio-source1", 0 };
  testcase audio_source2 = { "test-audio-source2", 0 };

  g_print ("\n");

  if (opts.test_external_ui) {
    ERROR ("Testing controller for external UI is not allowed!");
    return;
  }

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (1);                  /* give a second for server to be online */
  }

  client = TESTCLIENT (g_object_new (TYPE_TESTCLIENT, NULL));
  client->seconds = seconds;
  client->enable_test_sinks = TRUE;
  client->enable_thread_test1 = TRUE;
  client->enable_thread_test2 = TRUE;
  client->enable_thread_test3 = FALSE;
  client->enable_thread_test4 = FALSE;
  testclient_run_thread (client);
  g_assert_cmpint (clientcount, ==, 1);

  {
    {
      video_source1.live_seconds = seconds;
      video_source1.desc = g_string_new ("");
      g_string_append_printf (video_source1.desc, "videotestsrc pattern=%d ",
          0);
      g_string_append_printf (video_source1.desc,
          "! video/x-raw,width=%d,height=%d ", W, H);
      //g_string_append_printf (video_source1.desc, "! textoverlay font-desc=\"Sans 50\" text=\"v1\" ");
      g_string_append_printf (video_source1.desc,
          "! timeoverlay font-desc=\"Verdana bold 50\" ");
      g_string_append_printf (video_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

      video_source2.live_seconds = seconds;
      video_source2.desc = g_string_new ("");
      g_string_append_printf (video_source2.desc, "videotestsrc pattern=%d ",
          1);
      g_string_append_printf (video_source2.desc,
          "! video/x-raw,width=%d,height=%d ", W, H);
      //g_string_append_printf (video_source1.desc, "! textoverlay font-desc=\"Sans 50\" text=\"v2\" ");
      g_string_append_printf (video_source2.desc,
          "! timeoverlay font-desc=\"Verdana bold 50\" ");
      g_string_append_printf (video_source2.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

      audio_source1.live_seconds = seconds;
      audio_source1.desc = g_string_new ("");
      g_string_append_printf (audio_source1.desc,
          "audiotestsrc freq=110 wave=%d ", 2);
      g_string_append_printf (audio_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=4000");

      audio_source2.live_seconds = seconds;
      audio_source2.desc = g_string_new ("");
      g_string_append_printf (audio_source2.desc,
          "audiotestsrc freq=110 wave=%d ", 4);
      g_string_append_printf (audio_source2.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=4000");

      testcase_run_thread (&video_source1);
      usleep (500);
      testcase_run_thread (&video_source2);
      usleep (500);
      testcase_run_thread (&audio_source1);
      usleep (500);
      testcase_run_thread (&audio_source2);
      testcase_join (&video_source1);
      testcase_join (&video_source2);
      testcase_join (&audio_source1);
      testcase_join (&audio_source2);

#if ASSERT_TEST_PIPELINE_ERRORS
      g_assert (video_source1.pass);
      g_assert (video_source2.pass);
      g_assert (audio_source1.pass);
      g_assert (audio_source2.pass);
#endif

      if (0 < video_source1.error_count ||
          0 < video_source2.error_count ||
          0 < audio_source1.error_count || 0 < audio_source2.error_count) {
        g_test_fail ();
      }

      if (0 < client->compose_port_count) {
        //g_assert_cmpint (client->compose_port_count, >=, 1);
        g_assert_cmpint (client->compose_port, ==, client->compose_port0);
      }
      if (0 < client->encode_port_count) {
        g_assert_cmpint (client->encode_port, ==, client->encode_port0);
      }
      if (0 < client->audio_port_count) {
        //g_assert_cmpint (client->audio_port_count, >=, 1);
        g_assert_cmpint (client->audio_port, !=, 0);
      }
      if (0 < client->preview_port_count) {
        g_assert_cmpint (client->preview_port_1, !=, 0);
        g_assert_cmpint (client->preview_port_2, !=, 0);
        g_assert_cmpint (client->preview_port_3, !=, 0);
        g_assert_cmpint (client->preview_port_4, !=, 0);
        g_assert_cmpint (client->preview_port_count, ==, 4);
      }
    }
  }

  sleep (5), testclient_end (client);
  testclient_join (client);
  //g_assert_cmpint (client->compose_port_count, ==, client->expected_compose_count);
  g_assert_cmpint (client->expected_compose_count - client->new_mode_count, <=,
      1);
  g_object_unref (client);
  g_assert_cmpint (clientcount, ==, 0);

  if (!opts.test_external_server) {
    close_pid (server_pid);
#if TEST_RECORDING_DATA
    {
      testcase play = { "play-test-record", 0 };
      GFile *file = g_file_new_for_path ("test-recording.data");
      g_assert (g_file_query_exists (file, NULL));
      play.desc = g_string_new ("filesrc location=test-recording.data ");
      g_string_append_printf (play.desc, "! avidemux name=dm ");
      g_string_append_printf (play.desc,
          "dm.audio_0 ! queue ! faad ! audioconvert ! alsasink ");
      g_string_append_printf (play.desc,
          "dm.video_0 ! queue ! vp8dec ! videoconvert ! " VIDEOSINK " ");
      testcase_run_thread (&play);
      testcase_join (&play);
      g_assert_cmpint (play.error_count, ==, 0);
      g_object_unref (file);
    }
#endif
  }
}

static void
test_composite_mode (void)
{
  int seconds = 60 * (infinity_open_file ? 5 : 0.5);
  GPid server_pid = 0;
  testclient *client;
  testcase video_source1 = { "test-video-source1", 0 };
  testcase video_source2 = { "test-video-source2", 0 };
  testcase audio_source1 = { "test-audio-source1", 0 };
  testcase audio_source2 = { "test-audio-source2", 0 };

  g_print ("\n");

  if (opts.test_external_ui) {
    ERROR ("Testing composite mode for external UI is not allowed!");
    return;
  }

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (1);                  /* give a second for server to be online */
  }

  client = TESTCLIENT (g_object_new (TYPE_TESTCLIENT, NULL));
  client->seconds = seconds;
  client->enable_test_sinks = TRUE;
  client->enable_thread_test1 = TRUE;
  client->enable_thread_test2 = TRUE;
  client->enable_thread_test3 = TRUE;
  client->enable_thread_test4 = TRUE;
  testclient_run_thread (client);
  g_assert_cmpint (clientcount, ==, 1);

  {
    {
      video_source1.live_seconds = seconds;
      video_source1.desc = g_string_new ("");
      g_string_append_printf (video_source1.desc, "videotestsrc pattern=%d ",
          0);
      g_string_append_printf (video_source1.desc,
          "! video/x-raw,width=%d,height=%d ", W, H);
      //g_string_append_printf (video_source1.desc, "! textoverlay font-desc=\"Sans 50\" text=\"v1\" ");
      g_string_append_printf (video_source1.desc,
          "! timeoverlay font-desc=\"Verdana bold 50\" ");
      g_string_append_printf (video_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

      video_source2.live_seconds = seconds;
      video_source2.desc = g_string_new ("");
      g_string_append_printf (video_source2.desc, "videotestsrc pattern=%d ",
          1);
      g_string_append_printf (video_source2.desc,
          "! video/x-raw,width=%d,height=%d ", W, H);
      //g_string_append_printf (video_source1.desc, "! textoverlay font-desc=\"Sans 50\" text=\"v2\" ");
      g_string_append_printf (video_source2.desc,
          "! timeoverlay font-desc=\"Verdana bold 50\" ");
      g_string_append_printf (video_source2.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

      audio_source1.live_seconds = seconds;
      audio_source1.desc = g_string_new ("");
      g_string_append_printf (audio_source1.desc,
          "audiotestsrc freq=110 wave=%d ", 2);
      g_string_append_printf (audio_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=4000");

      audio_source2.live_seconds = seconds;
      audio_source2.desc = g_string_new ("");
      g_string_append_printf (audio_source2.desc,
          "audiotestsrc freq=110 wave=%d ", 4);
      g_string_append_printf (audio_source2.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=4000");

      testcase_run_thread (&video_source1);
      usleep (500);
      testcase_run_thread (&video_source2);
      usleep (500);
      testcase_run_thread (&audio_source1);
      usleep (500);
      testcase_run_thread (&audio_source2);
      testcase_join (&video_source1);
      testcase_join (&video_source2);
      testcase_join (&audio_source1);
      testcase_join (&audio_source2);

#if ASSERT_TEST_PIPELINE_ERRORS
      g_assert (video_source1.pass);
      g_assert (video_source2.pass);
      g_assert (audio_source1.pass);
      g_assert (audio_source2.pass);
#endif

      if (0 < video_source1.error_count ||
          0 < video_source2.error_count ||
          0 < audio_source1.error_count || 0 < audio_source2.error_count) {
        g_test_fail ();
      }

      g_assert_cmpint (client->compose_port, ==, client->compose_port0);
      g_assert_cmpint (client->compose_port_count, >=, 1);
      if (0 < client->encode_port_count && client->encode_port0 != 0) {
        g_assert_cmpint (client->encode_port, ==, client->encode_port0);
      }
      if (0 < client->audio_port_count && client->audio_port0 != 0) {
        g_assert_cmpint (client->audio_port, ==, client->audio_port0);
      }
      g_assert_cmpint (client->audio_port_count, >=, 1);
      g_assert_cmpint (client->preview_port_1, !=, 0);
      g_assert_cmpint (client->preview_port_2, !=, 0);
      g_assert_cmpint (client->preview_port_3, !=, 0);
      g_assert_cmpint (client->preview_port_4, !=, 0);
      g_assert_cmpint (client->preview_port_count, ==, 4);
    }
  }

  sleep (10), testclient_end (client);
  testclient_join (client);
  //g_assert_cmpint (client->compose_port_count, ==, client->expected_compose_count);
  //g_assert_cmpint (client->new_mode_count, ==, client->expected_compose_count);
  g_assert_cmpint (client->expected_compose_count - client->new_mode_count, <=,
      1);
  g_object_unref (client);
  g_assert_cmpint (clientcount, ==, 0);

  if (!opts.test_external_server) {
    close_pid (server_pid);
#if TEST_RECORDING_DATA
    {
      testcase play = { "play-test-record", 0 };
      GFile *file = g_file_new_for_path ("test-recording.data");
      g_assert (g_file_query_exists (file, NULL));
      play.desc = g_string_new ("filesrc location=test-recording.data ");
      g_string_append_printf (play.desc, "! avidemux name=dm ");
      g_string_append_printf (play.desc,
          "dm.audio_0 ! queue ! faad ! audioconvert ! alsasink ");
      g_string_append_printf (play.desc,
          "dm.video_0 ! queue ! vp8dec ! videoconvert ! " VIDEOSINK " ");
      testcase_run_thread (&play);
      testcase_join (&play);
      g_assert_cmpint (play.error_count, ==, 0);
      g_object_unref (file);
    }
#endif
  }
}

static void
test_video (void)
{
  const gint seconds = 30;
  GPid server_pid = 0;
  testcase source1 = { "test-video-source1", 0 };
  testcase source2 = { "test-video-source2", 0 };
  testcase source3 = { "test-video-source3", 0 };
  testcase sink0 = { "test_video_compose_sink", 0 };
  testcase sink1 = { "test_video_preview_sink1", 0 };
  testcase sink2 = { "test_video_preview_sink2", 0 };
  testcase sink3 = { "test_video_preview_sink3", 0 };
  const gchar *textoverlay = "textoverlay "
      "font-desc=\"Sans 60\" " "auto-resize=true " "shaded-background=true ";

  g_print ("\n");
  g_assert (!source1.thread);
  g_assert (!source2.thread);
  g_assert (!source3.thread);
  g_assert (!sink0.thread);
  g_assert (!sink1.thread);
  g_assert (!sink2.thread);
  g_assert (!sink3.thread);

  source1.live_seconds = seconds;
  source1.desc = g_string_new ("videotestsrc pattern=0 ");
  g_string_append_printf (source1.desc, "! video/x-raw,width=%d,height=%d ", W,
      H);
  g_string_append_printf (source1.desc, "! %s text=source1 ", textoverlay);
  g_string_append_printf (source1.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (source1.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  source2.live_seconds = seconds;
  source2.desc = g_string_new ("videotestsrc pattern=1 ");
  g_string_append_printf (source2.desc, "! video/x-raw,width=%d,height=%d ", W,
      H);
  g_string_append_printf (source2.desc, "! %s text=source2 ", textoverlay);
  g_string_append_printf (source2.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (source2.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  source3.live_seconds = seconds;
  source3.desc = g_string_new ("videotestsrc pattern=15 ");
  g_string_append_printf (source3.desc, "! video/x-raw,width=%d,height=%d ", W,
      H);
  g_string_append_printf (source3.desc, "! %s text=source3 ", textoverlay);
  g_string_append_printf (source3.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (source3.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  sink0.live_seconds = seconds;
  sink0.desc = g_string_new ("tcpclientsrc port=3001 ");
  g_string_append_printf (sink0.desc, "! gdpdepay ");
  g_string_append_printf (sink0.desc, "! videoconvert ");
  g_string_append_printf (sink0.desc, "! " VIDEOSINK);

  sink1.live_seconds = seconds;
  sink1.desc = g_string_new ("tcpclientsrc port=3003 ");
  g_string_append_printf (sink1.desc, "! gdpdepay ");
  g_string_append_printf (sink1.desc, "! videoconvert ");
  g_string_append_printf (sink1.desc, "! " VIDEOSINK);

  sink2.live_seconds = seconds;
  sink2.desc = g_string_new ("tcpclientsrc port=3004 ");
  g_string_append_printf (sink2.desc, "! gdpdepay ");
  g_string_append_printf (sink2.desc, "! videoconvert ");
  g_string_append_printf (sink2.desc, "! " VIDEOSINK);

  sink3.live_seconds = seconds;
  sink3.desc = g_string_new ("tcpclientsrc port=3005 ");
  g_string_append_printf (sink3.desc, "! gdpdepay ");
  g_string_append_printf (sink3.desc, "! videoconvert ");
  g_string_append_printf (sink3.desc, "! " VIDEOSINK);

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (2);                  /* give a second for server to be online */
  }

  testcase_run_thread (&source1);
  sleep (1);                    /* give a second for source1 to be online */
  testcase_run_thread (&source2);
  testcase_run_thread (&source3);
  sleep (1);                    /* give a second for sources to be online */
  testcase_run_thread (&sink0);
  testcase_run_thread (&sink1);
  testcase_run_thread (&sink2);
  testcase_run_thread (&sink3);
  testcase_join (&source1);
  testcase_join (&source2);
  testcase_join (&source3);
  testcase_join (&sink0);
  testcase_join (&sink1);
  testcase_join (&sink2);
  testcase_join (&sink3);

#if ASSERT_TEST_PIPELINE_ERRORS
  g_assert (source1.pass);
  g_assert (source2.pass);
  g_assert (source3.pass);
  g_assert (sink0.pass);
  g_assert (sink1.pass);
  g_assert (sink2.pass);
  g_assert (sink3.pass);
#endif

  if (!opts.test_external_server)
    close_pid (server_pid);

  g_assert (source1.desc == NULL);
  g_assert (source1.pipeline == NULL);

  g_assert (source2.desc == NULL);
  g_assert (source2.pipeline == NULL);

  g_assert (source3.desc == NULL);
  g_assert (source3.pipeline == NULL);

  g_assert (sink0.desc == NULL);
  g_assert (sink0.pipeline == NULL);

  g_assert (sink1.desc == NULL);
  g_assert (sink1.pipeline == NULL);

  g_assert (sink2.desc == NULL);
  g_assert (sink2.pipeline == NULL);

  g_assert (sink3.desc == NULL);
  g_assert (sink3.pipeline == NULL);

  if (!opts.test_external_server) {
#if TEST_RECORDING_DATA
    GFile *file = g_file_new_for_path ("test-recording.data");
    g_assert (g_file_query_exists (file, NULL));
    g_object_unref (file);
#endif
  }
}

static void
test_video_recording_result (void)
{
  g_print ("\n");
  if (!opts.test_external_server) {
#if TEST_RECORDING_DATA
    GFile *file = g_file_new_for_path ("test-recording.data");
    GError *error = NULL;
    g_assert (g_file_query_exists (file, NULL));
    g_assert (g_file_delete (file, NULL, &error));
    g_assert (error == NULL);
    g_assert (!g_file_query_exists (file, NULL));
    g_object_unref (file);
#endif
  }
}

static void
test_audio (void)
{
  const gint seconds = 20;
  testcase source1 = { "test-audio-source1", 0 };
  testcase source2 = { "test-audio-source2", 0 };
  testcase source3 = { "test-audio-source3", 0 };
  testcase sink1 = { "test_audio_preview_sink1", 0 };
  testcase sink2 = { "test_audio_preview_sink2", 0 };
  testcase sink3 = { "test_audio_preview_sink3", 0 };
  GPid server_pid = 0;
  const gchar *textoverlay = "textoverlay "
      "font-desc=\"Sans 60\" " "auto-resize=true " "shaded-background=true ";

  g_print ("\n");
  g_assert (!source1.thread);
  g_assert (!source2.thread);
  g_assert (!source3.thread);
  g_assert (!sink1.thread);
  g_assert (!sink2.thread);
  g_assert (!sink3.thread);

  source1.live_seconds = seconds;
  source1.desc = g_string_new ("audiotestsrc freq=110 wave=2 ");
  g_string_append_printf (source1.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  source2.live_seconds = seconds;
  source2.desc = g_string_new ("audiotestsrc freq=110 wave=2 ");
  g_string_append_printf (source2.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  source3.live_seconds = seconds;
  source3.desc = g_string_new ("audiotestsrc freq=110 wave=2 ");
  g_string_append_printf (source3.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  sink1.live_seconds = seconds;
  sink1.desc = g_string_new ("tcpclientsrc port=3003 ");
  //g_string_append_printf (sink1.desc, "! gdpdepay ! faad ! goom2k1 ");
  g_string_append_printf (sink1.desc, "! gdpdepay ! faad ! monoscope ");
  g_string_append_printf (sink1.desc, "! %s text=audio1 ", textoverlay);
  g_string_append_printf (sink1.desc, "! videoconvert ");
  g_string_append_printf (sink1.desc, "! " VIDEOSINK);

  sink2.live_seconds = seconds;
  sink2.desc = g_string_new ("tcpclientsrc port=3004 ");
  //g_string_append_printf (sink2.desc, "! gdpdepay ! faad ! goom2k1 ");
  g_string_append_printf (sink2.desc, "! gdpdepay ! faad ! monoscope ");
  g_string_append_printf (sink2.desc, "! %s text=audio2 ", textoverlay);
  g_string_append_printf (sink2.desc, "! videoconvert ");
  g_string_append_printf (sink2.desc, "! " VIDEOSINK);

  sink3.live_seconds = seconds;
  sink3.desc = g_string_new ("tcpclientsrc port=3005 ");
  //g_string_append_printf (sink3.desc, "! gdpdepay ! faad ! goom2k1 ");
  g_string_append_printf (sink3.desc, "! gdpdepay ! faad ! monoscope ");
  g_string_append_printf (sink3.desc, "! %s text=audio3 ", textoverlay);
  g_string_append_printf (sink3.desc, "! videoconvert ");
  g_string_append_printf (sink3.desc, "! " VIDEOSINK);

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (3);                  /* give a second for server to be online */
  }

  testcase_run_thread (&source1);
  testcase_run_thread (&source2);
  testcase_run_thread (&source3);
  sleep (2);                    /* give a second for audios to be online */
  if (!opts.test_external_ui) {
    testcase_run_thread (&sink1);
    testcase_run_thread (&sink2);
    testcase_run_thread (&sink3);
  }
  testcase_join (&source1);
  testcase_join (&source2);
  testcase_join (&source3);
  if (!opts.test_external_ui) {
    testcase_join (&sink1);
    testcase_join (&sink2);
    testcase_join (&sink3);
  }
#if ASSERT_TEST_PIPELINE_ERRORS
  g_assert (source1.pass);
  g_assert (source2.pass);
  g_assert (source3.pass);
  g_assert (sink1.pass);
  g_assert (sink2.pass);
  g_assert (sink3.pass);
#endif

  if (!opts.test_external_server)
    close_pid (server_pid);

  g_assert_cmpint (source1.timer, ==, 0);
  g_assert (source1.desc == NULL);
  g_assert (source1.pipeline == NULL);

  g_assert_cmpint (source2.timer, ==, 0);
  g_assert (source2.desc == NULL);
  g_assert (source2.pipeline == NULL);

  g_assert_cmpint (source3.timer, ==, 0);
  g_assert (source3.desc == NULL);
  g_assert (source3.pipeline == NULL);

  if (!opts.test_external_server) {
#if TEST_RECORDING_DATA
    GFile *file = g_file_new_for_path ("test-recording.data");
    g_assert (g_file_query_exists (file, NULL));
    g_object_unref (file);
#endif
  }
}

static void
test_audio_recording_result (void)
{
  g_print ("\n");
  if (!opts.test_external_server) {
#if TEST_RECORDING_DATA
    GFile *file = g_file_new_for_path ("test-recording.data");
    GError *error = NULL;
    g_assert (g_file_query_exists (file, NULL));
    g_assert (g_file_delete (file, NULL, &error));
    g_assert (error == NULL);
    g_assert (!g_file_query_exists (file, NULL));
    g_object_unref (file);
#endif
  }
}

static void
test_ui (void)
{
  const gint seconds = 60 * 5;
  GPid server_pid = 0;
  GPid ui_pid = 0;
  testcase video_source1 = { "test-video-source1", 0 };
  testcase video_source2 = { "test-video-source2", 0 };
  testcase video_source3 = { "test-video-source3", 0 };
  testcase audio_source1 = { "test-audio-source1", 0 };
  testcase audio_source2 = { "test-audio-source2", 0 };
  testcase audio_source3 = { "test-audio-source3", 0 };
  const gchar *textoverlay = "textoverlay "
      "font-desc=\"Sans 60\" " "auto-resize=true " "shaded-background=true ";

  g_print ("\n");

  video_source1.live_seconds = seconds;
  video_source1.desc = g_string_new ("videotestsrc pattern=0 ");
  g_string_append_printf (video_source1.desc,
      "! video/x-raw,width=%d,height=%d ", W, H);
  g_string_append_printf (video_source1.desc, "! %s text=video1 ", textoverlay);
  g_string_append_printf (video_source1.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (video_source1.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  video_source2.live_seconds = seconds;
  video_source2.desc = g_string_new ("videotestsrc pattern=1 ");
  g_string_append_printf (video_source2.desc,
      "! video/x-raw,width=%d,height=%d ", W, H);
  g_string_append_printf (video_source2.desc, "! %s text=video2 ", textoverlay);
  g_string_append_printf (video_source2.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (video_source2.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  video_source3.live_seconds = seconds;
  video_source3.desc = g_string_new ("videotestsrc pattern=15 ");
  g_string_append_printf (video_source3.desc,
      "! video/x-raw,width=%d,height=%d ", W, H);
  g_string_append_printf (video_source3.desc, "! %s text=video3 ", textoverlay);
  g_string_append_printf (video_source3.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (video_source3.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  audio_source1.live_seconds = seconds;
  audio_source1.desc = g_string_new ("audiotestsrc freq=110 ");
  //g_string_append_printf (audio_source1.desc, "! audio/x-raw ");
  g_string_append_printf (audio_source1.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  audio_source2.live_seconds = seconds;
  audio_source2.desc = g_string_new ("audiotestsrc freq=110 ");
  g_string_append_printf (audio_source2.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  audio_source3.live_seconds = seconds;
  audio_source3.desc = g_string_new ("audiotestsrc freq=110 ");
  g_string_append_printf (audio_source3.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (3);                  /* give a second for server to be online */
  }

  if (!opts.test_external_ui) {
    ui_pid = launch_ui ();
    g_assert_cmpint (ui_pid, !=, 0);
    sleep (2);                  /* give a second for ui to be ready */
  }

  testcase_run_thread (&video_source1); //sleep (1);
  testcase_run_thread (&video_source2); //sleep (1);
  testcase_run_thread (&video_source3); //sleep (1);
  testcase_run_thread (&audio_source1); //sleep (1);
  testcase_run_thread (&audio_source2); //sleep (1);
  testcase_run_thread (&audio_source3); //sleep (1);
  testcase_join (&video_source1);
  testcase_join (&video_source2);
  testcase_join (&video_source3);
  testcase_join (&audio_source1);
  testcase_join (&audio_source2);
  testcase_join (&audio_source3);

#if ASSERT_TEST_PIPELINE_ERRORS
  g_assert (video_source1.pass);
  g_assert (video_source2.pass);
  g_assert (video_source3.pass);
  g_assert (audio_source1.pass);
  g_assert (audio_source2.pass);
  g_assert (audio_source3.pass);
#endif

  if (!opts.test_external_ui)
    close_pid (ui_pid);
  if (!opts.test_external_server)
    close_pid (server_pid);
}

static void
test_recording_result (void)
{
  g_print ("\n");
  if (!opts.test_external_server) {
#if TEST_RECORDING_DATA
    GFile *file = g_file_new_for_path ("test-recording.data");
    GError *error = NULL;
    g_assert (g_file_query_exists (file, NULL));
    g_assert (g_file_delete (file, NULL, &error));
    g_assert (error == NULL);
    g_assert (!g_file_query_exists (file, NULL));
    g_object_unref (file);
#endif
  }
}

static gpointer
test_random_1 (gpointer d)
{
  testcase video_source1 = { "test-video-source1", 0 };
  testcase audio_source0 = { "test-audio-source0", 0 };
  testcase audio_source1 = { "test-audio-source1", 0 };
  const gchar *textoverlay = "textoverlay "
      "font-desc=\"Sans 60\" " "auto-resize=true " "shaded-background=true ";
  gint n, m, i;

  audio_source0.live_seconds = 60;
  audio_source0.desc = g_string_new ("");
  g_string_append_printf (audio_source0.desc, "audiotestsrc freq=110 wave=2 ");
  g_string_append_printf (audio_source0.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");
  testcase_run_thread (&audio_source0);
  sleep (2);

  for (i = m = 0; m < 3; ++m) {
    for (n = 0; n < 3; ++n, ++i) {
      video_source1.live_seconds = 5;
      video_source1.name = g_strdup_printf ("test-video-source1-%d", i);
      video_source1.desc = g_string_new ("");
      g_string_append_printf (video_source1.desc, "videotestsrc pattern=%d ",
          rand () % 20);
      g_string_append_printf (video_source1.desc,
          "! video/x-raw,width=%d,height=%d ", W, H);
      g_string_append_printf (video_source1.desc, "! %s text=video1-%d ",
          textoverlay, n);
      g_string_append_printf (video_source1.desc,
          "! timeoverlay font-desc=\"Verdana bold 50\" ");
      g_string_append_printf (video_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

      audio_source1.live_seconds = 5;
      audio_source1.name = g_strdup_printf ("test-audio-source1-%d", i);
      audio_source1.desc = g_string_new ("");
      g_string_append_printf (audio_source1.desc,
          "audiotestsrc freq=110 wave=%d ", rand () % 12);
      g_string_append_printf (audio_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=4000");

      testcase_run_thread (&video_source1);
      testcase_run_thread (&audio_source1);
      testcase_join (&video_source1);
      testcase_join (&audio_source1);

#if ASSERT_TEST_PIPELINE_ERRORS
      g_assert (video_source1.pass);
      g_assert (audio_source1.pass);
#endif

      g_free ((void *) video_source1.name);
      g_free ((void *) audio_source1.name);
    }
  }

  testcase_join (&audio_source0);
  return NULL;
}

static gpointer
test_random_2 (gpointer d)
{
  testcase video_source1 = { "test-video-source1", 0 };
  testcase audio_source1 = { "test-audio-source1", 0 };
  const gchar *textoverlay = "textoverlay "
      "font-desc=\"Sans 60\" " "auto-resize=true " "shaded-background=true ";
  gint n, m, i;

  g_print ("\n");

  for (i = m = 0; m < 3; ++m) {
    for (n = 0; n < 3; ++n, ++i) {
      video_source1.live_seconds = 10;
      video_source1.name = g_strdup_printf ("test-video-source2-%d", i);
      video_source1.desc = g_string_new ("");
      g_string_append_printf (video_source1.desc, "videotestsrc pattern=%d ",
          rand () % 20);
      g_string_append_printf (video_source1.desc,
          "! video/x-raw,width=%d,height=%d ", W, H);
      g_string_append_printf (video_source1.desc, "! %s text=video1-%d ",
          textoverlay, n);
      g_string_append_printf (video_source1.desc,
          "! timeoverlay font-desc=\"Verdana bold 50\" ");
      g_string_append_printf (video_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

      audio_source1.live_seconds = 10;
      audio_source1.name = g_strdup_printf ("test-audio-source2-%d", i);
      audio_source1.desc = g_string_new ("");
      g_string_append_printf (audio_source1.desc,
          "audiotestsrc freq=110 wave=%d ", rand () % 12);
      g_string_append_printf (audio_source1.desc,
          "! gdppay ! tcpclientsink name=tcp_sink port=4000");

      testcase_run_thread (&video_source1);
      testcase_run_thread (&audio_source1);
      testcase_join (&video_source1);
      testcase_join (&audio_source1);

#if ASSERT_TEST_PIPELINE_ERRORS
      g_assert (video_source1.pass);
      g_assert (audio_source1.pass);
#endif

      g_free ((void *) video_source1.name);
      g_free ((void *) audio_source1.name);
    }
  }
  return NULL;
}

static void
test_random (void)
{
  GPid server_pid = 0;
  GPid ui_pid = 0;
  GThread *t1, *t2;

  g_print ("\n");

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (2);                  /* give a second for server to be online */
  }

  if (!opts.test_external_ui) {
    ui_pid = launch_ui ();
    g_assert_cmpint (ui_pid, !=, 0);
    sleep (1);                  /* give a second for ui to be ready */
  }

  t1 = g_thread_new ("random-1", test_random_1, NULL);
  sleep (1);
  t2 = g_thread_new ("random-2", test_random_2, NULL);

  g_thread_join (t1);
  g_thread_join (t2);
  g_thread_unref (t1);
  g_thread_unref (t2);

  if (!opts.test_external_ui)
    close_pid (ui_pid);
  if (!opts.test_external_server)
    close_pid (server_pid);
}

static void
test_switching (void)
{
  const gint seconds = 60 * 3;
  GPid server_pid = 0;
  GPid ui_pid = 0;
  testcase video_source1 = { "test-video-source1", 0 };
  testcase video_source2 = { "test-video-source2", 0 };
  testcase video_source3 = { "test-video-source3", 0 };
  testcase audio_source1 = { "test-audio-source1", 0 };
  testcase audio_source2 = { "test-audio-source2", 0 };
  testcase audio_source3 = { "test-audio-source3", 0 };

  const gchar *textoverlay = "textoverlay "
      "font-desc=\"Sans 60\" " "auto-resize=true " "shaded-background=true ";

  g_print ("\n");

  video_source1.live_seconds = seconds;
  video_source1.desc = g_string_new ("videotestsrc pattern=0 ");
  g_string_append_printf (video_source1.desc,
      "! video/x-raw,width=%d,height=%d ", W, H);
  g_string_append_printf (video_source1.desc, "! %s text=video1 ", textoverlay);
  g_string_append_printf (video_source1.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (video_source1.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  video_source2.live_seconds = seconds;
  video_source2.desc = g_string_new ("videotestsrc pattern=1 ");
  g_string_append_printf (video_source2.desc,
      "! video/x-raw,width=%d,height=%d ", W, H);
  g_string_append_printf (video_source2.desc, "! %s text=video2 ", textoverlay);
  g_string_append_printf (video_source2.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (video_source2.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  video_source3.live_seconds = seconds;
  video_source3.desc = g_string_new ("videotestsrc pattern=15 ");
  g_string_append_printf (video_source3.desc,
      "! video/x-raw,width=%d,height=%d ", W, H);
  g_string_append_printf (video_source3.desc, "! %s text=video3 ", textoverlay);
  g_string_append_printf (video_source3.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (video_source3.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  audio_source1.live_seconds = seconds;
  audio_source1.desc = g_string_new ("audiotestsrc freq=110 wave=2 ");
  //g_string_append_printf (audio_source1.desc, "! audio/x-raw ");
  g_string_append_printf (audio_source1.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  audio_source2.live_seconds = seconds;
  audio_source2.desc = g_string_new ("audiotestsrc freq=110 ");
  g_string_append_printf (audio_source2.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  audio_source3.live_seconds = seconds;
  audio_source3.desc = g_string_new ("audiotestsrc freq=110 ");
  g_string_append_printf (audio_source3.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=4000");

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (3);                  /* give a second for server to be online */
  }

  if (!opts.test_external_ui) {
    ui_pid = launch_ui ();
    g_assert_cmpint (ui_pid, !=, 0);
    sleep (2);                  /* give a second for ui to be ready */
  }

  testcase_run_thread (&video_source1); //sleep (1);
  testcase_run_thread (&video_source2); //sleep (1);
  testcase_run_thread (&video_source3); //sleep (1);
  testcase_run_thread (&audio_source1); //sleep (1);
  testcase_run_thread (&audio_source2); //sleep (1);
  testcase_run_thread (&audio_source3); //sleep (1);
  testcase_join (&video_source1);
  testcase_join (&video_source2);
  testcase_join (&video_source3);
  testcase_join (&audio_source1);
  testcase_join (&audio_source2);
  testcase_join (&audio_source3);

#if ASSERT_TEST_PIPELINE_ERRORS
  g_assert (video_source1.pass);
  g_assert (video_source2.pass);
  g_assert (video_source3.pass);
  g_assert (audio_source1.pass);
  g_assert (audio_source2.pass);
  g_assert (audio_source3.pass);
#endif

  if (!opts.test_external_ui)
    close_pid (ui_pid);
  if (!opts.test_external_server)
    close_pid (server_pid);
}

static gpointer
test_fuzz_feed (gpointer data)
{
  enum
  { seconds = 3 };
  gboolean *quit = (gboolean *) data;
  sleep (5);
  const gchar *names[] = {
    "/dev/zero",
    "./data/fuzz-input-01.data",
    "./data/fuzz-input-02.data",
  };
  const gint len = sizeof (names) / sizeof (names[0]);
  srand (time (NULL));
  while (!*quit) {
    testcase source1 = { "test-video-fuzz-source", 0 };
    testcase source2 = { "test-audio-fuzz-source", 0 };

    source1.live_seconds = seconds;
    source1.errors_are_ok = TRUE;
    source1.desc = g_string_new ("");
    g_string_append_printf (source1.desc, "filesrc location=%s ",
        names[rand () % len]);
    //g_string_append_printf (source1.desc, "! gdppay ");
    g_string_append_printf (source1.desc,
        "! tcpclientsink name=tcp_sink port=3000 ");

    source2.live_seconds = seconds;
    source2.errors_are_ok = TRUE;
    source2.desc = g_string_new ("");
    g_string_append_printf (source2.desc, "filesrc location=%s ",
        names[rand () % len]);
    //g_string_append_printf (source2.desc, "! gdppay ");
    g_string_append_printf (source2.desc,
        "! tcpclientsink name=tcp_sink port=4000 ");

    testcase_run_thread (&source1);
    testcase_run_thread (&source2);
    testcase_join (&source1);
    testcase_join (&source2);

#if ASSERT_TEST_PIPELINE_ERRORS
    g_assert (source1.pass);
    g_assert (source2.pass);
#endif

    usleep (5000);
  }
  return NULL;
}

static void
test_fuzz (void)
{
  const gint seconds = 60 * 0.5;
  GPid server_pid = 0;
  testcase source1 = { "test-video-good-source1", 0 };
  testcase source2 = { "test-video-good-source2", 0 };
  testcase source3 = { "test-video-good-source3", 0 };
  testcase sink0 = { "test_video_compose_sink", 0 };
  testcase sink1 = { "test_video_preview_sink1", 0 };
  testcase sink2 = { "test_video_preview_sink2", 0 };
  testcase sink3 = { "test_video_preview_sink3", 0 };
  const gchar *textoverlay = "textoverlay "
      "font-desc=\"Sans 60\" " "auto-resize=true " "shaded-background=true ";

  GThread *feed;
  gboolean feed_quit = FALSE;

  g_print ("\n");
  g_assert (!source1.thread);
  g_assert (!source2.thread);
  g_assert (!source3.thread);
  g_assert (!sink0.thread);
  g_assert (!sink1.thread);
  g_assert (!sink2.thread);
  g_assert (!sink3.thread);

  source1.live_seconds = seconds;
  source1.desc = g_string_new ("videotestsrc pattern=0 ");
  g_string_append_printf (source1.desc, "! video/x-raw,width=%d,height=%d ", W,
      H);
  g_string_append_printf (source1.desc, "! %s text=source1 ", textoverlay);
  g_string_append_printf (source1.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (source1.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  source2.live_seconds = seconds;
  source2.desc = g_string_new ("videotestsrc pattern=1 ");
  g_string_append_printf (source2.desc, "! video/x-raw,width=%d,height=%d ", W,
      H);
  g_string_append_printf (source2.desc, "! %s text=source2 ", textoverlay);
  g_string_append_printf (source2.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (source2.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  source3.live_seconds = seconds;
  source3.desc = g_string_new ("videotestsrc pattern=15 ");
  g_string_append_printf (source3.desc, "! video/x-raw,width=%d,height=%d ", W,
      H);
  g_string_append_printf (source3.desc, "! %s text=source3 ", textoverlay);
  g_string_append_printf (source3.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ");
  g_string_append_printf (source3.desc,
      "! gdppay ! tcpclientsink name=tcp_sink port=3000 ");

  sink0.live_seconds = seconds;
  sink0.desc = g_string_new ("tcpclientsrc port=3001 ");
  g_string_append_printf (sink0.desc, "! gdpdepay ");
  g_string_append_printf (sink0.desc, "! videoconvert ");
  g_string_append_printf (sink0.desc, "! " VIDEOSINK);

  sink1.live_seconds = seconds;
  sink1.desc = g_string_new ("tcpclientsrc port=3003 ");
  g_string_append_printf (sink1.desc, "! gdpdepay ");
  g_string_append_printf (sink1.desc, "! videoconvert ");
  g_string_append_printf (sink1.desc, "! " VIDEOSINK);

  sink2.live_seconds = seconds;
  sink2.desc = g_string_new ("tcpclientsrc port=3004 ");
  g_string_append_printf (sink2.desc, "! gdpdepay ");
  g_string_append_printf (sink2.desc, "! videoconvert ");
  g_string_append_printf (sink2.desc, "! " VIDEOSINK);

  sink3.live_seconds = seconds;
  sink3.desc = g_string_new ("tcpclientsrc port=3005 ");
  g_string_append_printf (sink3.desc, "! gdpdepay ");
  g_string_append_printf (sink3.desc, "! videoconvert ");
  g_string_append_printf (sink3.desc, "! " VIDEOSINK);

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (2);                  /* give a second for server to be online */
  }

  feed = g_thread_new ("fuzz-feed", test_fuzz_feed, &feed_quit);

  testcase_run_thread (&source1);
  sleep (1);                    /* give a second for source1 to be online */
  testcase_run_thread (&source2);
  testcase_run_thread (&source3);
  sleep (1);                    /* give a second for sources to be online */
  testcase_run_thread (&sink0);
  testcase_run_thread (&sink1);
  testcase_run_thread (&sink2);
  testcase_run_thread (&sink3);
  testcase_join (&source1);
  testcase_join (&source2);
  testcase_join (&source3);
  testcase_join (&sink0);
  testcase_join (&sink1);
  testcase_join (&sink2);
  testcase_join (&sink3);

#if ASSERT_TEST_PIPELINE_ERRORS
  g_assert (source1.pass);
  g_assert (source2.pass);
  g_assert (source3.pass);
  g_assert (sink0.pass);
  g_assert (sink1.pass);
  g_assert (sink2.pass);
  g_assert (sink3.pass);
#endif

  feed_quit = TRUE;
  g_thread_join (feed);
  g_thread_unref (feed);

  if (!opts.test_external_server) {
    close_pid (server_pid);
#if TEST_RECORDING_DATA
    {
      GFile *file = g_file_new_for_path ("test-recording.data");
      g_assert (g_file_query_exists (file, NULL));
      g_object_unref (file);
    }
#endif
  }

  g_assert (source1.desc == NULL);
  g_assert (source1.pipeline == NULL);

  g_assert (source2.desc == NULL);
  g_assert (source2.pipeline == NULL);

  g_assert (source3.desc == NULL);
  g_assert (source3.pipeline == NULL);

  g_assert (sink0.desc == NULL);
  g_assert (sink0.pipeline == NULL);

  g_assert (sink1.desc == NULL);
  g_assert (sink1.pipeline == NULL);

  g_assert (sink2.desc == NULL);
  g_assert (sink2.pipeline == NULL);

  g_assert (sink3.desc == NULL);
  g_assert (sink3.pipeline == NULL);
}

static void
test_checking_timestamps (void)
{
  const gint seconds = 60 * 5;
  GPid server_pid = 0;
  GPid ui_pid = 0;
  testcase video_source = { "test-video-source", 0 };

  g_print ("\n");

  video_source.live_seconds = seconds;
  video_source.desc = g_string_new ("videotestsrc pattern=0 ");
  g_string_append_printf (video_source.desc,
      "! video/x-raw,width=%d,height=%d ", W, H);
  g_string_append_printf (video_source.desc,
      "! timeoverlay font-desc=\"Verdana bold 50\" ! tee name=v ");
  g_string_append_printf (video_source.desc,
      "v. ! queue " "! textoverlay font-desc=\"Sans 100\" text=111 "
      "! gdppay ! tcpclientsink name=tcp_sink1 port=3000 ");
  g_string_append_printf (video_source.desc,
      "v. ! queue " "! textoverlay font-desc=\"Sans 100\" text=222 "
      "! gdppay ! tcpclientsink name=tcp_sink2 port=3000 ");
  g_string_append_printf (video_source.desc,
      "v. ! queue " "! textoverlay font-desc=\"Sans 100\" text=333 "
      "! gdppay ! tcpclientsink name=tcp_sink3 port=3000 ");
  g_string_append_printf (video_source.desc,
      "v. ! queue " "! textoverlay font-desc=\"Sans 100\" text=444 "
      "! gdppay ! tcpclientsink name=tcp_sink4 port=3000 ");

  if (!opts.test_external_server) {
    server_pid = launch_server ();
    g_assert_cmpint (server_pid, !=, 0);
    sleep (3);                  /* give a second for server to be online */
  }

  if (!opts.test_external_ui) {
    ui_pid = launch_ui ();
    g_assert_cmpint (ui_pid, !=, 0);
    sleep (2);                  /* give a second for ui to be ready */
  }

  testcase_run_thread (&video_source);
  testcase_join (&video_source);

#if ASSERT_TEST_PIPELINE_ERRORS
  g_assert (video_source.pass);
#endif

  if (!opts.test_external_ui)
    close_pid (ui_pid);
  if (!opts.test_external_server)
    close_pid (server_pid);
}

static void
test_multiple_clients (void)
{
  ERROR ("TODO: test for multiple clients");
}

static void
kill_children (void)
{
  GList *child = NULL;
  g_mutex_lock (&children_lock);
  for (child = children; child; child = g_list_next (child)) {
    GPid pid = (GPid) GPOINTER_TO_INT(child->data);
    //INFO ("kill %d", pid);
    kill (pid, SIGKILL);
    children = g_list_remove_link (children, child);
  }
  g_mutex_unlock (&children_lock);
}

int
main (int argc, char **argv)
{
  {
    GOptionContext *context;
    GOptionGroup *group;
    GError *error = NULL;
    gboolean ok;
    group = g_option_group_new ("gst-switch-test", "gst-switch test suite",
        "", NULL, NULL);
    context = g_option_context_new ("");
    g_option_context_add_main_entries (context, option_entries,
        "test-switch-server");
    g_option_context_add_group (context, group);
    ok = g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);
    if (!ok) {
      g_print ("option parsing failed: %s\n", error->message);
      return 1;
    }
  }

  atexit (&kill_children);

  srand (time (NULL));

  {
    struct rlimit r;
    r.rlim_cur = RLIM_INFINITY;
    r.rlim_max = RLIM_INFINITY;
    if (setrlimit (RLIMIT_NOFILE, &r) == 0) {
      infinity_open_file = TRUE;
    }
  }

  gst_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);
  if (opts.enable_test_controller) {
    g_test_add_func ("/gst-switch/controller", test_controller);
  }
  if (opts.enable_test_composite_mode) {
    g_test_add_func ("/gst-switch/composite-mode", test_composite_mode);
  }
  if (opts.enable_test_video) {
    g_test_add_func ("/gst-switch/video", test_video);
    g_test_add_func ("/gst-switch/video-recording-result",
        test_video_recording_result);
  }
  if (opts.enable_test_audio) {
    g_test_add_func ("/gst-switch/audio", test_audio);
    g_test_add_func ("/gst-switch/audio-recording-result",
        test_audio_recording_result);
  }
  if (opts.enable_test_ui) {
    g_test_add_func ("/gst-switch/ui", test_ui);
    g_test_add_func ("/gst-switch/recording-result", test_recording_result);
  }
  if (opts.enable_test_switching) {
    g_test_add_func ("/gst-switch/switching", test_switching);
    g_test_add_func ("/gst-switch/recording-result", test_recording_result);
  }
  if (opts.enable_test_random) {
    g_test_add_func ("/gst-switch/random", test_random);
    g_test_add_func ("/gst-switch/recording-result", test_recording_result);
  }
  if (opts.enable_test_fuzz) {
    g_test_add_func ("/gst-switch/fuzz", test_fuzz);
    g_test_add_func ("/gst-switch/recording-result", test_recording_result);
  }
  if (opts.enable_test_checking_timestamps) {
    g_test_add_func ("/gst-switch/checking-timestamps",
        test_checking_timestamps);
    g_test_add_func ("/gst-switch/recording-result", test_recording_result);
  }
  if (opts.enable_test_multiple_clients) {
    g_test_add_func ("/gst-switch/multiple-clients", test_multiple_clients);
  }
  return g_test_run ();
}
