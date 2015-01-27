/* gst-switch							    -*- c -*-
 * Copyright (C) 2012,2013 Duzy Chan <code@duzy.info>
 *
 * This file is part of gst-switch.
 *
 * gst-switch is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*! @file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gio/gio.h>
#include <stdlib.h>
#include "gstswitchserver.h"
#include "gstrecorder.h"
#include "gstcase.h"
#include "./gio/gsocketinputstream.h"
#include "../logutils.h"

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define GST_SWITCH_SERVER_DEFAULT_HOST "0.0.0.0"
#define GST_SWITCH_SERVER_DEFAULT_VIDEO_ACCEPTOR_PORT	3000
#define GST_SWITCH_SERVER_DEFAULT_AUDIO_ACCEPTOR_PORT	4000
#define GST_SWITCH_SERVER_DEFAULT_CONTROLLER_ADDRESS	"tcp:host=0.0.0.0,port=5000"
#define GST_SWITCH_SERVER_LISTEN_BACKLOG 8      /* client connection queue */

#define GST_SWITCH_SERVER_HOST_SPEC "%q"
#define GST_SWITCH_SERVER_DEFAULT_RECORD_FILE "recording-%q-%Y%m%d-%H%M%S"
#define GST_SWITCH_SERVER_DEFAULT_RECORD_EXT ".mkv"

#define GST_SWITCH_SERVER_LOCK_MAIN_LOOP(srv) (g_mutex_lock (&(srv)->main_loop_lock))
#define GST_SWITCH_SERVER_UNLOCK_MAIN_LOOP(srv) (g_mutex_unlock (&(srv)->main_loop_lock))
#define GST_SWITCH_SERVER_LOCK_VIDEO_ACCEPTOR(srv) (g_mutex_lock (&(srv)->video_acceptor_lock))
#define GST_SWITCH_SERVER_UNLOCK_VIDEO_ACCEPTOR(srv) (g_mutex_unlock (&(srv)->video_acceptor_lock))
#define GST_SWITCH_SERVER_LOCK_AUDIO_ACCEPTOR(srv) (g_mutex_lock (&(srv)->audio_acceptor_lock))
#define GST_SWITCH_SERVER_UNLOCK_AUDIO_ACCEPTOR(srv) (g_mutex_unlock (&(srv)->audio_acceptor_lock))
#define GST_SWITCH_SERVER_LOCK_CONTROLLER(srv) (g_mutex_lock (&(srv)->controller_lock))
#define GST_SWITCH_SERVER_UNLOCK_CONTROLLER(srv) (g_mutex_unlock (&(srv)->controller_lock))
#define GST_SWITCH_SERVER_LOCK_CASES(srv) (g_mutex_lock (&(srv)->cases_lock))
#define GST_SWITCH_SERVER_UNLOCK_CASES(srv) (g_mutex_unlock (&(srv)->cases_lock))
#define GST_SWITCH_SERVER_LOCK_SERVE(srv) (g_mutex_lock (&(srv)->serve_lock))
#define GST_SWITCH_SERVER_UNLOCK_SERVE(srv) (g_mutex_unlock (&(srv)->serve_lock))
#define GST_SWITCH_SERVER_LOCK_PIP(srv) (g_mutex_lock (&(srv)->pip_lock))
#define GST_SWITCH_SERVER_UNLOCK_PIP(srv) (g_mutex_unlock (&(srv)->pip_lock))
#define GST_SWITCH_SERVER_LOCK_RECORDER(srv) (g_mutex_lock (&(srv)->recorder_lock))
#define GST_SWITCH_SERVER_UNLOCK_RECORDER(srv) (g_mutex_unlock (&(srv)->recorder_lock))
#define GST_SWITCH_SERVER_LOCK_CLOCK(srv) (g_mutex_lock (&(srv)->clock_lock))
#define GST_SWITCH_SERVER_UNLOCK_CLOCK(srv) (g_mutex_unlock (&(srv)->clock_lock))

#define gst_switch_server_parent_class parent_class
G_DEFINE_TYPE (GstSwitchServer, gst_switch_server, G_TYPE_OBJECT);

GstSwitchServerOpts opts = {
  NULL, NULL,
  GST_SWITCH_SERVER_DEFAULT_CONTROLLER_ADDRESS,
  GST_SWITCH_SERVER_DEFAULT_VIDEO_ACCEPTOR_PORT,
  GST_SWITCH_SERVER_DEFAULT_AUDIO_ACCEPTOR_PORT,
//FALSE,
  FALSE,
  NULL, NULL
};

gboolean verbose = FALSE;

static gboolean
gparse_record_filename (gchar * name, gchar * value, gpointer data,
    GError ** error)
{
  size_t maxpathlen = 256;
  gchar fnbuf[maxpathlen + 1];
  size_t fnlen = 0;

  if (value == NULL || (fnlen = strlen (value)) == 0) {
    // file not specified, use the default filename.ext
    strncpy (fnbuf, GST_SWITCH_SERVER_DEFAULT_RECORD_FILE, sizeof (fnbuf) - 5);
    strcat (fnbuf, GST_SWITCH_SERVER_DEFAULT_RECORD_EXT);
  } else if (fnlen < maxpathlen) {
    strncpy (fnbuf, value, sizeof (fnbuf));
    if ((fnlen < 5 || strchr (value + fnlen - 5, '.') == NULL)
        && fnlen + 4 < maxpathlen)
      strcat (fnbuf, GST_SWITCH_SERVER_DEFAULT_RECORD_EXT);
  } else {
    GError *err = g_error_new (G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
        "%s path/filename too long: %s\n", name, value);
    g_propagate_error (error, err);
    return FALSE;
  }

  // Do manual substitution of %q => hostname up front since this is static
  // Time and date tokens are substituted within the recorder as the recording
  // is started or cut to accurately reflect the date/time of recording
  char *h = strstr (fnbuf, GST_SWITCH_SERVER_HOST_SPEC);
  if (h != NULL) {
    int mnlen = maxpathlen - fnlen;     // limit size to what is available
    char hostname[mnlen + 1];
    if (gethostname (hostname, mnlen) != 0)
      strcpy (hostname, GST_SWITCH_SERVER_DEFAULT_HOST);
    else
      hostname[mnlen] = '\0';
    int hnlen = strlen (hostname);
    hostname[hnlen] = '\0';     // guarantee nul termination
    fnlen = strlen (fnbuf);
    do {
      size_t over = fnlen - (h - fnbuf) + 1;
      memmove (h + hnlen, h + 2, over);
      memcpy (h, hostname, hnlen);
      fnlen += (hnlen - 2);     // adjust the result length
    } while ((h = strstr (fnbuf, GST_SWITCH_SERVER_HOST_SPEC)) != NULL);
  }

  opts.record_filename = g_strdup (fnbuf);
  return TRUE;
}


extern int
parse_format (const gchar * format, GstCaps ** final_caps, GError ** err);

static gboolean
gparse_video_format (gchar * name, gchar * value, gpointer data,
    GError ** error)
{
  if (parse_format (value, &opts.video_caps, error) == -1)
    return FALSE;
  return TRUE;
}

static gboolean caps_dumped = FALSE;

/* gst_switch_server_getcaps:
 * @return current video caps
 */
GstCaps *
gst_switch_server_getcaps (void)
{
  static const gchar *audio_caps_str =
      "audio/x-raw, rate=48000, channels=2, format=S16LE, layout=interleaved";

  if (opts.video_caps == NULL) {
    // use a sane default we know will parse correctly
    parse_format (opts.low_res ? "VGA@60" : "720p60", &opts.video_caps, NULL);
  }
  if (!caps_dumped) {
    opts.video_caps_str = gst_caps_to_string (opts.video_caps);
    g_print ("video-caps: %s\n", opts.video_caps_str);

    opts.audio_caps_str = g_strdup (audio_caps_str);
    g_print ("audio-caps: %s\n", opts.audio_caps_str);

    caps_dumped = TRUE;
  }
  return opts.video_caps;
}

/* gst_switch_server_get_video_caps_str
 * return video caps as a string
 */
const gchar *
gst_switch_server_get_video_caps_str (void)
{
  (void) gst_switch_server_getcaps ();
  return opts.video_caps_str;
}

/* gst_switch_server_get_audio_caps_str
 * return audio caps as a string
 */
const gchar *
gst_switch_server_get_audio_caps_str (void)
{
  (void) gst_switch_server_getcaps ();
  return opts.audio_caps_str;
}

/* gst_switch_server_get_record_filename:
 * @return record file template name, if one was set
 */
const gchar *
gst_switch_server_get_record_filename (void)
{
  return opts.record_filename;
}

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
      "Prompt more messages", NULL},
  {"test-switch", 't', 0, G_OPTION_ARG_STRING, &opts.test_switch,
      "Perform switch test", "OUTPUT"},
  {"dbus-timeout", 'd', 0, G_OPTION_ARG_INT,
        &gst_switch_controller_dbus_timeout,
      "DBus timeout in msec (default 5000)"},
  {"record", 'r', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
        (gpointer) gparse_record_filename,
      "Enable recorder and record into the specified FILENAME"},
  {"low-resolution", 'l', 0, G_OPTION_ARG_NONE, &opts.low_res,
      "Enable low resolution mode (-f overrides)"},
  {"video-format", 'f', 0, G_OPTION_ARG_CALLBACK,
        (gpointer) gparse_video_format,
      "Specify the video format to use (shortcuts supported)"},
  {"video-input-port", 'p', 0, G_OPTION_ARG_INT, &opts.video_input_port,
      "Specify the video input listen port.", "NUM"},
  {"audio-input-port", 'a', 0, G_OPTION_ARG_INT, &opts.audio_input_port,
      "Specify the audio input listen port.", "NUM"},
  {"controller-address", 'c', 0, G_OPTION_ARG_STRING, &opts.controller_address,
      "Specify DBus-Address for remote control, defaults to "
        GST_SWITCH_SERVER_DEFAULT_CONTROLLER_ADDRESS ".", "ADDRESS"},
  {NULL}
};

/**
 * gst_switch_server_parse_args:
 *
 * Parsing command line parameters.
 */
static void
gst_switch_server_parse_args (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;

  gst_init (NULL, NULL);
  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, "gst-switch");
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    ERROR ("option parsing failed: %s", error->message);
    exit (1);
  } else if (argc > 1) {
    ERROR ("unknown option: %s", argv[1]);
    exit (1);
  }

  g_option_context_free (context);
}

/**
 * gst_switch_server_init:
 *
 * Initialize the GstSwitchServer instance.
 */
static void
gst_switch_server_init (GstSwitchServer * srv)
{
  INFO ("gst_switch_server init %p", srv);
  srv->host = g_strdup (GST_SWITCH_SERVER_DEFAULT_HOST);

  srv->cancellable = g_cancellable_new ();
  srv->video_acceptor_port = opts.video_input_port;
  srv->video_acceptor_socket = NULL;
  srv->video_acceptor = NULL;
  srv->audio_acceptor_port = opts.audio_input_port;
  srv->audio_acceptor_socket = NULL;
  srv->audio_acceptor = NULL;
  srv->controller = NULL;
  srv->main_loop = NULL;
  srv->cases = NULL;
  srv->composite = NULL;
  srv->alloc_port_count = 0;

  srv->pip_x = 0;
  srv->pip_y = 0;
  srv->pip_w = 0;
  srv->pip_h = 0;

  srv->clock = gst_system_clock_obtain ();

  g_mutex_init (&srv->main_loop_lock);
  g_mutex_init (&srv->video_acceptor_lock);
  g_mutex_init (&srv->audio_acceptor_lock);
  g_mutex_init (&srv->controller_lock);
  g_mutex_init (&srv->cases_lock);
  g_mutex_init (&srv->alloc_port_lock);
  g_mutex_init (&srv->pip_lock);
  g_mutex_init (&srv->recorder_lock);
  g_mutex_init (&srv->clock_lock);
}

/**
 * gst_switch_server_finalize:
 *
 * Destroying the GstSwitchServer instance.
 * 
 */
static void
gst_switch_server_finalize (GstSwitchServer * srv)
{
  INFO ("gst_switch_server finalize %p", srv);

  g_free (srv->host);
  srv->host = NULL;

  if (srv->cancellable) {
    g_object_unref (srv->cancellable);
    srv->cancellable = NULL;
  }

  if (srv->video_acceptor_socket) {
    g_object_unref (srv->video_acceptor_socket);
    srv->video_acceptor_socket = NULL;
  }
/*
  if (srv->video_acceptor) {
    DEBUG("Waiting for video_acceptor thread to die.");
    g_thread_join (srv->video_acceptor);
  }
*/
  if (srv->audio_acceptor_socket) {
    g_object_unref (srv->audio_acceptor_socket);
    srv->audio_acceptor_socket = NULL;
  }
/*
  if (srv->audio_acceptor) {
    DEBUG("Waiting for audio_acceptor thread to die.");
    g_thread_join (srv->audio_acceptor);
  }
*/
  if (srv->controller) {
    g_object_unref (srv->controller);
    srv->controller = NULL;
  }

  if (srv->cases) {
    g_list_free_full (srv->cases, (GDestroyNotify) g_object_unref);
    srv->cases = NULL;
  }

  if (srv->composite) {
    g_object_unref (srv->composite);
    srv->composite = NULL;
  }

  gst_object_unref (srv->clock);

  g_mutex_clear (&srv->main_loop_lock);
  g_mutex_clear (&srv->video_acceptor_lock);
  g_mutex_clear (&srv->audio_acceptor_lock);
  g_mutex_clear (&srv->controller_lock);
  g_mutex_clear (&srv->cases_lock);
  g_mutex_clear (&srv->alloc_port_lock);
  g_mutex_clear (&srv->pip_lock);
  g_mutex_clear (&srv->recorder_lock);
  g_mutex_clear (&srv->clock_lock);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (srv));
}

/**
 * gst_switch_server_class_init:
 *
 * Initializet the GstSwitchServerClass.
 */
static void
gst_switch_server_class_init (GstSwitchServerClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = (GObjectFinalizeFunc) gst_switch_server_finalize;
}

/**
 * gst_switch_server_quit:
 *
 * Force terminating the server. 
 */
static void
gst_switch_server_quit (GstSwitchServer * srv, gint exit_code)
{
  GST_SWITCH_SERVER_LOCK_MAIN_LOOP (srv);
  g_main_loop_quit (srv->main_loop);
  srv->exit_code = exit_code;
  GST_SWITCH_SERVER_UNLOCK_MAIN_LOOP (srv);
}

/**
 * gst_switch_server_alloc_port:
 *
 * Allocate a new port number.
 */
static gint
gst_switch_server_alloc_port (GstSwitchServer * srv)
{
  gint port;
  g_mutex_lock (&srv->alloc_port_lock);
  srv->alloc_port_count += 1;
  port = srv->video_acceptor_port + srv->alloc_port_count;

  // TODO: new policy for port allocation

  g_mutex_unlock (&srv->alloc_port_lock);
  return port;
}

/**
 * gst_switch_server_revoke_port:
 *
 * This is intended to revoke a allocated port number.
 */
static void
gst_switch_server_revoke_port (GstSwitchServer * srv, int port)
{
  g_mutex_lock (&srv->alloc_port_lock);
  //srv->alloc_port_count -= 1;

  // TODO: new policy for port allocation

  g_mutex_unlock (&srv->alloc_port_lock);
}

/**
 * gst_switch_server_end_case:
 *
 * Invoked when a %GstCase is ended.
 */
static void
gst_switch_server_end_case (GstCase * cas, GstSwitchServer * srv)
{
  gint caseport = 0;
  GList *item;

  GST_SWITCH_SERVER_LOCK_CASES (srv);

  switch (cas->type) {
    default:
      srv->cases = g_list_remove (srv->cases, cas);
      INFO ("Removed %s (%p, %d) (%d cases left)", GST_WORKER (cas)->name,
          cas, G_OBJECT (cas)->ref_count, g_list_length (srv->cases));
      caseport = cas->sink_port;
      g_object_unref (cas);
      break;
    case GST_CASE_INPUT_AUDIO:
    case GST_CASE_INPUT_VIDEO:
      srv->cases = g_list_remove (srv->cases, cas);
      INFO ("Removed %s %p (%d cases left)", GST_WORKER (cas)->name, cas,
          g_list_length (srv->cases));
      caseport = cas->sink_port;
      g_object_unref (cas);
      for (item = srv->cases; item;) {
        GstCase *c = GST_CASE (item->data);
        if (c->sink_port == caseport) {
          gst_worker_stop (GST_WORKER (c));
          item = g_list_next (item);
          /*
             srv->cases = g_list_remove (srv->cases, c);
             g_object_unref (G_OBJECT (c));
           */
        } else {
          item = g_list_next (item);
        }
      }
      break;
  }

  GST_SWITCH_SERVER_UNLOCK_CASES (srv);

  if (caseport)
    gst_switch_server_revoke_port (srv, caseport);

  switch (cas->type) {
    case GST_CASE_BRANCH_VIDEO_A:
    case GST_CASE_BRANCH_VIDEO_B:
    case GST_CASE_BRANCH_AUDIO:
    case GST_CASE_BRANCH_PREVIEW:
      GST_SWITCH_SERVER_LOCK_CONTROLLER (srv);
      if (srv->controller) {
        gst_switch_controller_tell_preview_port_removed (srv->controller,
            cas->sink_port, cas->serve_type, cas->type);
      }
      GST_SWITCH_SERVER_UNLOCK_CONTROLLER (srv);

    default:
      break;
  }
}

/**
 * gst_switch_server_start_case:
 *
 * Start a new %GstCase.
 */
static void
gst_switch_server_start_case (GstCase * cas, GstSwitchServer * srv)
{
  switch (cas->type) {
    case GST_CASE_BRANCH_VIDEO_A:
    case GST_CASE_BRANCH_VIDEO_B:
    case GST_CASE_BRANCH_AUDIO:
    case GST_CASE_BRANCH_PREVIEW:
      GST_SWITCH_SERVER_LOCK_CONTROLLER (srv);
      if (srv->controller) {
        gst_switch_controller_tell_preview_port_added (srv->controller,
            cas->sink_port, cas->serve_type, cas->type);
      }
      GST_SWITCH_SERVER_UNLOCK_CONTROLLER (srv);

    default:
      break;
  }
}

/**
 * gst_switch_server_suggest_case_type:
 *
 * Get a proper GstCase type according to the stream type.
 */
static GstCaseType
gst_switch_server_suggest_case_type (GstSwitchServer * srv,
    GstSwitchServeStreamType serve_type)
{
  GstCaseType type = GST_CASE_UNKNOWN;
  gboolean has_composite_video_A = FALSE;
  gboolean has_composite_video_B = FALSE;
  gboolean has_composite_audio = FALSE;
  GList *item = srv->cases;

  for (; item; item = g_list_next (item)) {
    GstCase *cas = GST_CASE (item->data);
#if 0
    switch (cas->serve_type) {
      case GST_SERVE_VIDEO_STREAM:
      {
        switch (cas->type) {
          case GST_CASE_COMPOSITE_VIDEO_A:
            has_composite_video_A = TRUE;
            break;
          case GST_CASE_COMPOSITE_VIDEO_B:
            has_composite_video_B = TRUE;
            break;
          default:
            break;
        }
      }
        break;
      case GST_SERVE_AUDIO_STREAM:
      {
        if (cas->type == GST_CASE_COMPOSITE_AUDIO)
          has_composite_audio = TRUE;
      }
        break;
      case GST_SERVE_NOTHING:
        break;
    }
#else
    switch (cas->type) {
      case GST_CASE_COMPOSITE_VIDEO_A:
        has_composite_video_A = TRUE;
        break;
      case GST_CASE_COMPOSITE_VIDEO_B:
        has_composite_video_B = TRUE;
        break;
      case GST_CASE_COMPOSITE_AUDIO:
        has_composite_audio = TRUE;
        break;
      default:
        break;
    }
#endif
    //INFO ("case: %d, %d, %d", cas->sink_port, cas->type, cas->serve_type);
  }

  switch (serve_type) {
    case GST_SERVE_VIDEO_STREAM:
      if (!has_composite_video_A)
        type = GST_CASE_COMPOSITE_VIDEO_A;
      else if (!has_composite_video_B)
        type = GST_CASE_COMPOSITE_VIDEO_B;
      else
        type = GST_CASE_PREVIEW;
      break;
    case GST_SERVE_AUDIO_STREAM:
      if (!has_composite_audio)
        type = GST_CASE_COMPOSITE_AUDIO;
      else
        type = GST_CASE_PREVIEW;
      break;
    case GST_SERVE_NOTHING:
      break;
  }

  // TODO: better switching policy?

  return type;
}

/**
 * gst_switch_server_serve:
 *
 * The gst-switch-srv serving thread.
 */
static void
gst_switch_server_serve (GstSwitchServer * srv, GSocket * client,
    GstSwitchServeStreamType serve_type)
{
  GSocketInputStreamX *stream =
      G_SOCKET_INPUT_STREAM (g_object_new
      (G_TYPE_SOCKET_INPUT_STREAM, "socket", client,
          NULL));
  GstCaseType type = GST_CASE_UNKNOWN;
  GstCaseType inputtype = GST_CASE_UNKNOWN;
  GstCaseType branchtype = GST_CASE_UNKNOWN;
  gint num_cases = g_list_length (srv->cases);
  GstCase *input = NULL, *branch = NULL, *workcase = NULL;
  gchar *name;
  gint port = 0;
  GCallback start_callback = G_CALLBACK (gst_switch_server_start_case);
  GCallback end_callback = G_CALLBACK (gst_switch_server_end_case);

  GST_SWITCH_SERVER_LOCK_SERVE (srv);
  GST_SWITCH_SERVER_LOCK_CASES (srv);
  switch (serve_type) {
    case GST_SERVE_AUDIO_STREAM:
      inputtype = GST_CASE_INPUT_AUDIO;
      break;
    case GST_SERVE_VIDEO_STREAM:
      inputtype = GST_CASE_INPUT_VIDEO;
      break;
    default:
      goto error_unknown_serve_type;
  }

  type = gst_switch_server_suggest_case_type (srv, serve_type);
  switch (type) {
    case GST_CASE_COMPOSITE_VIDEO_A:
      branchtype = GST_CASE_BRANCH_VIDEO_A;
      break;
    case GST_CASE_COMPOSITE_VIDEO_B:
      branchtype = GST_CASE_BRANCH_VIDEO_B;
      break;
    case GST_CASE_COMPOSITE_AUDIO:
      branchtype = GST_CASE_BRANCH_AUDIO;
      break;
    case GST_CASE_PREVIEW:
      branchtype = GST_CASE_BRANCH_PREVIEW;
      break;
    default:
      goto error_unknown_case_type;
  }

  port = gst_switch_server_alloc_port (srv);

  //INFO ("case-type: %d, %d, %d", type, branchtype, port);

  name = g_strdup_printf ("input_%d", port);
  input = GST_CASE (g_object_new (GST_TYPE_CASE, "name", name,
          "type", inputtype, "port", port, "serve",
          serve_type, "stream", stream, NULL));
  g_object_unref (stream);
  g_object_unref (client);
  g_free (name);

  name = g_strdup_printf ("branch_%d", port);
  branch = GST_CASE (g_object_new (GST_TYPE_CASE, "name", name,
          "type", branchtype, "port", port, "serve", serve_type, NULL));
  g_free (name);

  name = g_strdup_printf ("case-%d", num_cases);
  workcase = GST_CASE (g_object_new (GST_TYPE_CASE, "name", name,
          "type", type, "port", port, "serve",
          serve_type, "input", input, "branch", branch, NULL));
  g_free (name);

  srv->cases = g_list_append (srv->cases, input);
  srv->cases = g_list_append (srv->cases, branch);
  srv->cases = g_list_append (srv->cases, workcase);
  GST_SWITCH_SERVER_UNLOCK_CASES (srv);

  if (serve_type == GST_SERVE_VIDEO_STREAM) {
    g_object_set (input,
        "width", srv->composite->width,
        "height", srv->composite->height,
        "awidth", srv->composite->a_width,
        "aheight", srv->composite->a_height,
        "bwidth", srv->composite->b_width,
        "bheight", srv->composite->b_height, NULL);
    g_object_set (branch,
        "width", srv->composite->width,
        "height", srv->composite->height,
        "awidth", srv->composite->a_width,
        "aheight", srv->composite->a_height,
        "bwidth", srv->composite->b_width,
        "bheight", srv->composite->b_height, NULL);
    g_object_set (workcase,
        "width", srv->composite->width,
        "height", srv->composite->height,
        "awidth", srv->composite->a_width,
        "aheight", srv->composite->a_height,
        "bwidth", srv->composite->b_width,
        "bheight", srv->composite->b_height, NULL);
  }

  g_signal_connect (branch, "start-worker", start_callback, srv);
  g_signal_connect (input, "end-worker", end_callback, srv);
  g_signal_connect (branch, "end-worker", end_callback, srv);
  g_signal_connect (workcase, "end-worker", end_callback, srv);

  if (!gst_worker_start (GST_WORKER (input)))
    goto error_start_branch;
  if (!gst_worker_start (GST_WORKER (branch)))
    goto error_start_branch;
  if (!gst_worker_start (GST_WORKER (workcase)))
    goto error_start_workcase;

  GST_SWITCH_SERVER_UNLOCK_SERVE (srv);
  return;

  /* Errors Handling */
error_unknown_serve_type:
  {
    ERROR ("unknown serve type %d", serve_type);
    g_object_unref (stream);
    g_object_unref (client);
    GST_SWITCH_SERVER_UNLOCK_CASES (srv);
    GST_SWITCH_SERVER_UNLOCK_SERVE (srv);
    return;
  }

error_unknown_case_type:
  {
    ERROR ("unknown case type (serve type %d)", serve_type);
    g_object_unref (stream);
    g_object_unref (client);
    GST_SWITCH_SERVER_UNLOCK_CASES (srv);
    GST_SWITCH_SERVER_UNLOCK_SERVE (srv);
    return;
  }

error_start_branch:
error_start_workcase:
  {
    ERROR ("failed serving new client");
    GST_SWITCH_SERVER_LOCK_CASES (srv);
    srv->cases = g_list_remove (srv->cases, branch);
    srv->cases = g_list_remove (srv->cases, workcase);
    GST_SWITCH_SERVER_UNLOCK_CASES (srv);
    g_object_unref (stream);
    g_object_unref (branch);
    g_object_unref (workcase);
    gst_switch_server_revoke_port (srv, port);
    GST_SWITCH_SERVER_UNLOCK_SERVE (srv);
    return;
  }
}

/**
 * gst_switch_server_listen:
 * @port: The port number to listen on.
 * @bound_port: The local bound port number of the socket.
 * @return: The socket instance.
 *
 * Create a new socket on the specific port and listen on the created socket.
 */
static GSocket *
gst_switch_server_listen (GstSwitchServer * srv, gint port, gint * bound_port)
{
  GError *err = NULL;
  GInetAddress *addr;
  GSocket *socket = NULL;
  GSocketAddress *saddr;
  GResolver *resolver;
  gchar *ip;

  *bound_port = 0;

  /* look up name if we need to */
  addr = g_inet_address_new_from_string (srv->host);
  if (!addr) {
    GList *results;

    resolver = g_resolver_get_default ();
    results = g_resolver_lookup_by_name (resolver, srv->host,
        srv->cancellable, &err);
    if (!results)
      goto resolve_no_name;

    addr = G_INET_ADDRESS (g_object_ref (results->data));

    g_resolver_free_addresses (results);
    g_object_unref (resolver);
  }

  ip = g_inet_address_to_string (addr);
  saddr = g_inet_socket_address_new (addr, port);
  g_object_unref (addr);

  /* create the server listener socket */
  socket = g_socket_new (g_socket_address_get_family (saddr),
      G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &err);
  if (!socket)
    goto socket_new_failed;

  /* bind it */
  if (!g_socket_bind (socket, saddr, TRUE, &err))
    goto socket_bind_failed;

  g_object_unref (saddr);

  /* listen on the socket */
  g_socket_set_listen_backlog (socket, GST_SWITCH_SERVER_LISTEN_BACKLOG);
  if (!g_socket_listen (socket, &err))
    goto socket_listen_failed;

  if (port == 0) {
    saddr = g_socket_get_local_address (socket, NULL);
    *bound_port = g_inet_socket_address_get_port ((GInetSocketAddress *) saddr);
    g_object_unref (saddr);
  } else {
    *bound_port = port;
  }

  INFO ("Listening on %s (%s:%d)", srv->host, ip, *bound_port);

  g_free (ip);

  //g_atomic_int_set (&srv->bound_port, bound_port);
  //g_object_notify (G_OBJECT (src), "bound-port");
  return socket;

  /* Errors Handling */

resolve_no_name:
  {
    ERROR ("resolve: %s", err->message);
    g_object_unref (resolver);
    g_object_unref (addr);
    return NULL;
  }

socket_new_failed:
  {
    ERROR ("new socket: %s", err->message);
    g_clear_error (&err);
    g_object_unref (saddr);
    g_free (ip);
    return NULL;
  }

socket_bind_failed:
  {
    ERROR ("bind socket: %s", err->message);
    g_clear_error (&err);
    g_object_unref (saddr);
    g_free (ip);
    return NULL;
  }

socket_listen_failed:
  {
    ERROR ("listen socket: %s", err->message);
    g_clear_error (&err);
    g_object_unref (saddr);
    g_free (ip);
    return NULL;
  }
}

/**
 * gst_switch_server_video_acceptor:
 *
 * Thread for accepting video inputs.
 */
static gpointer
gst_switch_server_video_acceptor (GstSwitchServer * srv)
{
  GSocket *socket;
  GError *error;
  gint bound_port;

  srv->video_acceptor_socket = gst_switch_server_listen (srv,
      srv->video_acceptor_port, &bound_port);
  if (!srv->video_acceptor_socket) {
    gst_switch_server_quit (srv, -__LINE__);
    return NULL;
  }

  while (srv->video_acceptor && srv->video_acceptor_socket && srv->cancellable) {
    socket =
        g_socket_accept (srv->video_acceptor_socket, srv->cancellable, &error);
    if (!socket) {
      ERROR ("accept: %s", error->message);
      continue;
    }

    gst_switch_server_serve (srv, socket, GST_SERVE_VIDEO_STREAM);
  }

  GST_SWITCH_SERVER_LOCK_VIDEO_ACCEPTOR (srv);
  g_thread_unref (srv->video_acceptor);
  srv->video_acceptor = NULL;
  GST_SWITCH_SERVER_UNLOCK_VIDEO_ACCEPTOR (srv);
  return NULL;
}

/**
 * gst_switch_server_audio_acceptor:
 *
 * Thread for accepting audio inputs.
 */
static gpointer
gst_switch_server_audio_acceptor (GstSwitchServer * srv)
{
  GSocket *socket;
  GError *error;
  gint bound_port;

  srv->audio_acceptor_socket = gst_switch_server_listen (srv,
      srv->audio_acceptor_port, &bound_port);
  if (!srv->audio_acceptor_socket) {
    gst_switch_server_quit (srv, -__LINE__);
    return NULL;
  }

  while (srv->audio_acceptor && srv->audio_acceptor_socket && srv->cancellable) {
    socket =
        g_socket_accept (srv->audio_acceptor_socket, srv->cancellable, &error);
    if (!socket) {
      ERROR ("accept: %s", error->message);
      continue;
    }

    gst_switch_server_serve (srv, socket, GST_SERVE_AUDIO_STREAM);
  }

  GST_SWITCH_SERVER_LOCK_AUDIO_ACCEPTOR (srv);
  g_thread_unref (srv->audio_acceptor);
  srv->audio_acceptor = NULL;
  GST_SWITCH_SERVER_UNLOCK_AUDIO_ACCEPTOR (srv);
  return NULL;
}

/**
 * gst_switch_server_prepare_bus_controller:
 *
 * Preparing the dbus controller.
 */
static void
gst_switch_server_prepare_bus_controller (GstSwitchServer * srv)
{
  if (srv->controller == NULL) {
    GST_SWITCH_SERVER_LOCK_CONTROLLER (srv);
    if (srv->controller == NULL) {
      srv->controller =
          GST_SWITCH_CONTROLLER (g_object_new
          (GST_TYPE_SWITCH_CONTROLLER, NULL));
      if (!gst_switch_controller_is_valid (srv->controller)) {
        gst_switch_server_quit (srv, -__LINE__);
      }
      srv->controller->server = srv;
    }
    GST_SWITCH_SERVER_UNLOCK_CONTROLLER (srv);
  }
}

/**
 * gst_switch_server_get_composite_sink_port:
 *  @srv: the GstSwitchServer instance
 *  @return: the composite sink port
 *
 *  Get the composite port.
 *  
 */
gint
gst_switch_server_get_composite_sink_port (GstSwitchServer * srv)
{
  gint port = 0;
  if (srv->composite) {
    //gst_composite_lock (srv->composite);
    port = srv->composite->sink_port;
    //gst_composite_unlock (srv->composite);
  }
  return port;
}

/**
 * gst_switch_server_get_encode_sink_port:
 *  @srv: the GstSwitchServer instance
 *  @return: the encode sink port number
 *
 *  Get the encode port.
 *
 */
gint
gst_switch_server_get_encode_sink_port (GstSwitchServer * srv)
{
  gint port = 0;
  if (srv->composite) {
    //gst_composite_lock (srv->composite);
    port = srv->composite->encode_sink_port;
    //gst_composite_unlock (srv->composite);
  }
  return port;
}

/**
 * gst_switch_server_get_audio_sink_port:
 *  @param srv the GstSwitchServer instance
 *
 *  Get the audio port.
 *
 *  @return: the audio sink port number.
 */
gint
gst_switch_server_get_audio_sink_port (GstSwitchServer * srv)
{
  gint port = 0;
  GList *item;
  GST_SWITCH_SERVER_LOCK_CASES (srv);
  for (item = srv->cases; item; item = g_list_next (item)) {
    if (GST_CASE (item->data)->type == GST_CASE_COMPOSITE_AUDIO) {
      port = GST_CASE (item->data)->sink_port;
      break;
    }
  }
  GST_SWITCH_SERVER_UNLOCK_CASES (srv);
  return port;
}

/**
 *  @brief Get the preview ports.
 *  @param srv
 *  @param s (output) the preview serve types.
 *  @param t (output) the preview types.
 *  @return: The array of preview ports.
 */
GArray *
gst_switch_server_get_preview_sink_ports (GstSwitchServer * srv,
    GArray ** s, GArray ** t)
{
  GArray *a = g_array_new (FALSE, TRUE, sizeof (gint));
  GList *item;

  if (s)
    *s = g_array_new (FALSE, TRUE, sizeof (gint));
  if (t)
    *t = g_array_new (FALSE, TRUE, sizeof (gint));

  GST_SWITCH_SERVER_LOCK_CASES (srv);
  for (item = srv->cases; item; item = g_list_next (item)) {
    switch (GST_CASE (item->data)->type) {
      case GST_CASE_BRANCH_VIDEO_A:
      case GST_CASE_BRANCH_VIDEO_B:
      case GST_CASE_BRANCH_AUDIO:
      case GST_CASE_PREVIEW:
        a = g_array_append_val (a, GST_CASE (item->data)->sink_port);
        if (s)
          *s = g_array_append_val (*s, GST_CASE (item->data)->serve_type);
        if (t)
          *t = g_array_append_val (*t, GST_CASE (item->data)->type);
      default:
        break;
    }
  }
  GST_SWITCH_SERVER_UNLOCK_CASES (srv);
  return a;
}

/**
 * gst_switch_server_set_composite_mode:
 *  @return: TRUE if succeeded.
 *
 *  Change a composite mode.
 *
 */
gboolean
gst_switch_server_set_composite_mode (GstSwitchServer * srv, gint mode)
{
  gboolean result = FALSE;

  GST_SWITCH_SERVER_LOCK_PIP (srv);

  if (mode == srv->composite->mode) {
    WARN ("ignore the same composite mode %d", mode);
    goto end;
  }

  g_object_set (srv->composite, "mode", mode, NULL);

  result = (mode == srv->composite->mode);

  if (result) {
    srv->pip_x = srv->composite->b_x;
    srv->pip_y = srv->composite->b_y;
    srv->pip_w = srv->composite->b_width;
    srv->pip_h = srv->composite->b_height;
  }

end:
  GST_SWITCH_SERVER_UNLOCK_PIP (srv);
  return result;
}

gint
gst_switch_server_get_composite_mode (GstSwitchServer * srv)
{
  return srv->composite->mode;
}

static void
gst_switch_server_start_audio (GstCase * cas, GstSwitchServer * srv)
{
  INFO ("audio %d started", cas->sink_port);
}

/**
 * gst_switch_server_new_record:
 *  @return: TRUE if succeeded.
 *
 *  Start a new recording.
 *  
 */
gboolean
gst_switch_server_new_record (GstSwitchServer * srv)
{
  GstWorkerClass *worker_class;
  gboolean result = FALSE;

  g_return_val_if_fail (GST_IS_RECORDER (srv->recorder), FALSE);

  if (srv->recorder) {
    GST_SWITCH_SERVER_LOCK_RECORDER (srv);
    if (srv->recorder) {
      gst_worker_stop (GST_WORKER (srv->recorder));
      g_object_set (G_OBJECT (srv->recorder),
          "mode", srv->composite->mode,
          "port", srv->composite->encode_sink_port,
          "width", srv->composite->width,
          "height", srv->composite->height, NULL);
      worker_class = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (srv->recorder));
      if (worker_class->reset (GST_WORKER (srv->recorder))) {
        result = gst_worker_start (GST_WORKER (srv->recorder));
      } else {
        ERROR ("failed to reset composite recorder");
      }
    }
    GST_SWITCH_SERVER_UNLOCK_RECORDER (srv);
  }
  return result;
}

/**
 * gst_switch_server_adjust_pip:
 *  @return: a unsigned number of indicating which component (x,y,w,h) has
 *           been changed
 *
 *  Adjust the PIP position and size.
 *
 */
guint
gst_switch_server_adjust_pip (GstSwitchServer * srv,
    gint dx, gint dy, gint dw, gint dh)
{
  guint result = 0;

  g_return_val_if_fail (GST_IS_COMPOSITE (srv->composite), 0);

  GST_SWITCH_SERVER_LOCK_PIP (srv);

  srv->pip_x += dx, srv->pip_y += dy;
  srv->pip_w += dw, srv->pip_h += dh;
  if (srv->pip_x < 0)
    srv->pip_x = 0;
  if (srv->pip_y < 0)
    srv->pip_y = 0;
  srv->pip_w = gst_check_composite_min_pip_width (srv->pip_w);
  srv->pip_h = gst_check_composite_min_pip_height (srv->pip_h);

  result = gst_composite_adjust_pip (srv->composite,
      srv->pip_x, srv->pip_y, srv->pip_w, srv->pip_h);

  if (dx != 0)
    result |= (1 << 0);
  if (dy != 0)
    result |= (1 << 1);
  if (dw != 0)
    result |= (1 << 2);
  if (dh != 0)
    result |= (1 << 3);

  GST_SWITCH_SERVER_UNLOCK_PIP (srv);
  return result;
}

static void gst_switch_server_worker_start (GstWorker *, GstSwitchServer *);
static void gst_switch_server_worker_null (GstWorker *, GstSwitchServer *);

/**
 * gst_switch_server_switch:
 *  @return: TRUE if succeeded.
 *
 *  Switch the channel to the specific port.
 *
 */
gboolean
gst_switch_server_switch (GstSwitchServer * srv, gint channel, gint port)
{
  GList *item;
  gboolean result = FALSE;
  GstCase *compose_case, *candidate_case;
  GstCase *work1, *work2;
  GCallback callback = G_CALLBACK (gst_switch_server_end_case);
  gchar *name;

  compose_case = NULL;
  candidate_case = NULL;

  GST_SWITCH_SERVER_LOCK_CASES (srv);

  for (item = srv->cases; item; item = g_list_next (item)) {
    GstCase *cas = GST_CASE (item->data);
    switch (channel) {
      case 'A':
        if (cas->type == GST_CASE_COMPOSITE_VIDEO_A)
          goto get_target_stream;
        break;
      case 'B':
        if (cas->type == GST_CASE_COMPOSITE_VIDEO_B)
          goto get_target_stream;
        break;
      case 'a':
        if (cas->type == GST_CASE_COMPOSITE_AUDIO)
          goto get_target_stream;
        break;
      default:
        WARN ("unknown channel %c", (gchar) channel);
        break;
      get_target_stream:
        if (compose_case == NULL) {
          compose_case = cas;
        }
    }
    switch (cas->type) {
      case GST_CASE_COMPOSITE_VIDEO_A:
      case GST_CASE_COMPOSITE_VIDEO_B:
      case GST_CASE_COMPOSITE_AUDIO:
      case GST_CASE_PREVIEW:
        if (cas->sink_port == port) {
          candidate_case = cas;
        }
      default:
        break;
    }
  }

  if (!candidate_case) {
    ERROR ("no stream for port %d (candidate)", port);
    goto end;
  }

  if (!compose_case) {
    ERROR ("no stream for port %d (compose)", port);
    goto end;
  }

  if (candidate_case == compose_case) {
    ERROR ("stream on %d already at %c", port, (gchar) channel);
    goto end;
  }

  INFO ("switching: %s (%d), %s (%d)",
      GST_WORKER (compose_case)->name, compose_case->type,
      GST_WORKER (candidate_case)->name, candidate_case->type);

  if (candidate_case->serve_type != compose_case->serve_type) {
    ERROR ("stream type not matched");
    goto end;
  }

  name = g_strdup (GST_WORKER (compose_case)->name);
  work1 = GST_CASE (g_object_new (GST_TYPE_CASE, "name", name,
          "type", compose_case->type,
          "serve", compose_case->serve_type,
          "port", candidate_case->sink_port,
          "input", candidate_case->input,
          "branch", candidate_case->branch, NULL));
  g_free (name);

  name = g_strdup (GST_WORKER (candidate_case)->name);
  work2 = GST_CASE (g_object_new (GST_TYPE_CASE, "name", name,
          "type", candidate_case->type,
          "serve", candidate_case->serve_type,
          "port", compose_case->sink_port,
          "input", compose_case->input, "branch", compose_case->branch, NULL));
  g_free (name);

  if (compose_case->serve_type == GST_SERVE_VIDEO_STREAM) {
    g_object_set (work1,
        "width", compose_case->width,
        "height", compose_case->height,
        "awidth", compose_case->a_width,
        "aheight", compose_case->a_height,
        "bwidth", compose_case->b_width,
        "bheight", compose_case->b_height, NULL);
    g_object_set (work2,
        "width", candidate_case->width,
        "height", candidate_case->height,
        "awidth", candidate_case->a_width,
        "aheight", candidate_case->a_height,
        "bwidth", candidate_case->b_width,
        "bheight", candidate_case->b_height, NULL);
  } else {
    g_signal_connect (G_OBJECT (work1), "start-worker",
        G_CALLBACK (gst_switch_server_start_audio), srv);
  }

  compose_case->switching = TRUE;
  candidate_case->switching = TRUE;

  g_signal_connect (compose_case, "worker-null",
      G_CALLBACK (gst_switch_server_worker_null), srv);
  g_signal_connect (candidate_case, "worker-null",
      G_CALLBACK (gst_switch_server_worker_null), srv);

  gst_worker_stop (GST_WORKER (compose_case));
  gst_worker_stop (GST_WORKER (candidate_case));

  g_signal_connect (work1, "start-worker",
      G_CALLBACK (gst_switch_server_worker_start), srv);
  g_signal_connect (work2, "start-worker",
      G_CALLBACK (gst_switch_server_worker_start), srv);

  g_signal_connect (work1, "end-worker", callback, srv);
  g_signal_connect (work2, "end-worker", callback, srv);

  if (!gst_worker_start (GST_WORKER (work1)))
    goto error_start_work;
  if (!gst_worker_start (GST_WORKER (work2)))
    goto error_start_work;

  srv->cases = g_list_append (srv->cases, work1);
  srv->cases = g_list_append (srv->cases, work2);

  result = TRUE;

  INFO ("switched: %s <-> %s",
      GST_WORKER (work1)->name, GST_WORKER (work2)->name);

end:
  GST_SWITCH_SERVER_UNLOCK_CASES (srv);
  return result;

error_start_work:
  {
    ERROR ("failed to start works");
    g_object_unref (work1);
    g_object_unref (work2);
    GST_SWITCH_SERVER_UNLOCK_CASES (srv);
    return result;
  }
}

gboolean
gst_switch_server_click_video (GstSwitchServer * srv,
    gint avx, gint avy, gint avw, gint avh)
{
  const double w = (double) srv->composite->width;
  const double h = (double) srv->composite->height;
  const double ax = (double) srv->composite->a_x;
  const double ay = (double) srv->composite->a_y;
  const double aw = (double) srv->composite->a_width;
  const double ah = (double) srv->composite->a_height;
  const double bx = (double) srv->composite->b_x;
  const double by = (double) srv->composite->b_y;
  const double bw = (double) srv->composite->b_width;
  const double bh = (double) srv->composite->b_height;
  //const double sw = (double) GST_SWITCH_FACEDETECT_FRAME_WIDTH;
  //const double sh = (double) GST_SWITCH_FACEDETECT_FRAME_HEIGHT;
  const double r = w / h;
  double vx = (double) avx;
  double vy = (double) avy;
  double vw = (double) avw;
  double vh = (double) avh;
  double vr = vw / vh;
  double rx = 1.0, ry = 1.0, x1 = .0, y1 = .0;
  char chan = '?';

  // convert to video space
  if (vr < r) {
    rx = ry = w / vw;
    vy -= (vh - (vw * h) / w) / 2;
  } else {
    rx = ry = h / vh;
    vx -= (vw - (vh * w) / h) / 2;
  }

  x1 = rx * vx;
  y1 = ry * vy;

  if (bx <= x1 && x1 <= bx + bw && by <= y1 && y1 <= by + bh) {
    chan = 'B', x1 -= bx, y1 -= by;
  } else if (ax <= x1 && x1 <= ax + aw && ay <= y1 && y1 <= ay + ah) {
    chan = 'A', x1 -= ax, y1 -= ay;
  }

  g_print ("select-face: %c, (%d, %d)\n", chan, (gint) x1, (gint) y1);

  // conert to detect space
  //rx = sw / w, ry = sh / h; FIXME: This final (x,y) is not accurate.
  rx = 1.0, ry = 1.0;
  avx = (gint) (rx * x1 + 0.5);
  avy = (gint) (ry * y1 + 0.5);

  if (chan == 'A') {
    return gst_switch_controller_select_face (srv->controller, avx, avy);
  } else if (chan == 'B') {
    //return gst_switch_controller_select_face (srv->controller, avx, avy);
  }
  return 0;
}

void
gst_switch_server_mark_face (GstSwitchServer * srv, GVariant * faces,
    gboolean tracking)
{
  const int size = g_variant_n_children (faces);
  const double cw = srv->composite->a_width;
  const double ch = srv->composite->a_height;
  double rx = 1.0, ry = 1.0, dx, dy,
      sw = GST_SWITCH_FACEDETECT_FRAME_WIDTH,
      sh = GST_SWITCH_FACEDETECT_FRAME_HEIGHT;
  GVariantBuilder *vb = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
  int x, y, w, h, n;

  rx = sw / cw;
  ry = sh / ch;
  dx = rx * ((double) srv->composite->a_x);
  dy = ry * ((double) srv->composite->a_y);
  for (n = 0; n < size; ++n) {
    g_variant_get_child (faces, n, "(iiii)", &x, &y, &w, &h);
    x = rx * ((double) x) + 0.5 + dx;
    y = ry * ((double) y) + 0.5 + dy;
    w = rx * ((double) w) + 0.5;
    h = ry * ((double) h) + 0.5;
    g_variant_builder_add (vb, "(iiii)", x, y, w, h);
  }

  if (tracking) {
    gst_switch_controller_show_track_marker (srv->controller,
        g_variant_builder_end (vb));
  } else {
    gst_switch_controller_show_face_marker (srv->controller,
        g_variant_builder_end (vb));
  }
  g_variant_builder_unref (vb);
}

/**
 * gst_switch_server_worker_start:
 *
 * Invoked when a worker is started.
 */
static void
gst_switch_server_worker_start (GstWorker * worker, GstSwitchServer * srv)
{
  GstClockTime t = GST_CLOCK_TIME_NONE;

  g_return_if_fail (GST_IS_WORKER (worker));

  GST_SWITCH_SERVER_LOCK_CLOCK (srv);
  t = gst_clock_get_time (srv->clock);
  GST_SWITCH_SERVER_UNLOCK_CLOCK (srv);

  g_print ("online: %s @%lld\n", worker->name, (long long int) t);
}

/**
 * gst_switch_server_worker_null:
 *
 * Invoked when a worker is running into NULL. 
 */
static void
gst_switch_server_worker_null (GstWorker * worker, GstSwitchServer * srv)
{
  GstClockTime t = GST_CLOCK_TIME_NONE;

  g_return_if_fail (GST_IS_WORKER (worker));

  GST_SWITCH_SERVER_LOCK_CLOCK (srv);
  t = gst_clock_get_time (srv->clock);
  GST_SWITCH_SERVER_UNLOCK_CLOCK (srv);

  g_print ("offline: %s @%lld\n", worker->name, (long long int) t);
}

/**
 * gst_switch_server_start_output:
 *
 * Start the composite output worker.
 */
static void
gst_switch_server_start_output (GstWorker * worker, GstSwitchServer * srv)
{
  INFO ("composite-output started");
}

/**
 * gst_switch_server_start_recorder:
 *
 * Start the recorder worker.
 */
static void
gst_switch_server_start_recorder (GstWorker * worker, GstSwitchServer * srv)
{
  INFO ("encoding-output started");
}

/**
 * gst_switch_server_end_transition:
 *
 * The composite worker has finished a transition of modes.
 */
static void
gst_switch_server_end_transition (GstWorker * worker, GstSwitchServer * srv)
{
  g_return_if_fail (GST_IS_WORKER (worker));

  GST_SWITCH_SERVER_LOCK_CONTROLLER (srv);
  if (srv->controller) {
    gint mode = srv->composite->mode;
    gst_switch_controller_tell_new_mode_onlne (srv->controller, mode);
  }
  GST_SWITCH_SERVER_UNLOCK_CONTROLLER (srv);
}

/**
 * gst_switch_server_output_client_socket_added:
 *
 * Invoked when a client socket is added.
 */
static void
gst_switch_server_output_client_socket_added (GstElement * element,
    GSocket * socket, GstSwitchServer * srv)
{
  g_return_if_fail (G_IS_SOCKET (socket));

  //INFO ("client-socket-added: %d", g_socket_get_fd (socket));
}

/**
 * gst_switch_server_output_client_socket_removed:
 *
 * Invoked when a client socket is removed on the output port. The socket
 * is required to be freed manually here.
 */
static void
gst_switch_server_output_client_socket_removed (GstElement * element,
    GSocket * socket, GstSwitchServer * srv)
{
  g_return_if_fail (G_IS_SOCKET (socket));

  //INFO ("client-socket-removed: %d", g_socket_get_fd (socket));

  g_socket_close (socket, NULL);
}

/**
 * gst_switch_server_prepare_composite:
 * @return TRUE if the composite worker is prepared.
 *
 * Preparing the composite worker.
 */
static gboolean
gst_switch_server_prepare_composite (GstSwitchServer * srv,
    GstCompositeMode mode)
{
  gint port, encode;

  if (srv->composite) {
    return TRUE;
  }

  port = gst_switch_server_alloc_port (srv);
  encode = gst_switch_server_alloc_port (srv);

  INFO ("Compose sink to %d, %d", port, encode);

  g_assert (srv->composite == NULL);
  srv->composite = GST_COMPOSITE (g_object_new (GST_TYPE_COMPOSITE,
          "name", "composite", "port",
          port, "encode", encode, "mode", mode, NULL));

  g_signal_connect (srv->composite, "start-worker",
      G_CALLBACK (gst_switch_server_worker_start), srv);
  g_signal_connect (srv->composite, "worker-null",
      G_CALLBACK (gst_switch_server_worker_null), srv);

  /*
     g_signal_connect (srv->composite, "start-output",
     G_CALLBACK (gst_switch_server_start_output), srv);
     g_signal_connect (srv->composite, "start-recorder",
     G_CALLBACK (gst_switch_server_start_recorder), srv);
   */
  g_signal_connect (srv->composite, "end-transition",
      G_CALLBACK (gst_switch_server_end_transition), srv);

  GST_SWITCH_SERVER_LOCK_PIP (srv);
  srv->pip_x = srv->composite->b_x;
  srv->pip_y = srv->composite->b_y;
  srv->pip_w = srv->composite->b_width;
  srv->pip_h = srv->composite->b_height;
  GST_SWITCH_SERVER_UNLOCK_PIP (srv);

  if (!gst_worker_start (GST_WORKER (srv->composite)))
    goto error_start_composite;

  return TRUE;

error_start_composite:
  {
    g_object_unref (srv->composite);
    srv->composite = NULL;
    return FALSE;
  }
}

/**
 * gst_switch_server_get_output_string:
 * @return The composite output pipeline string, needs freeing after used
 *
 * Fetching the composite output pipeline.
 */
static GString *
gst_switch_server_get_output_string (GstWorker * worker, GstSwitchServer * srv)
{
  GString *desc;

  desc = g_string_new ("");

  g_string_append_printf (desc, "intervideosrc name=source "
      "channel=composite_out ");
  g_string_append_printf (desc, "tcpserversink name=sink "
      "port=%d ", srv->composite->sink_port);
  g_string_append_printf (desc, "source. ! video/x-raw,width=%d,height=%d ",
      srv->composite->width, srv->composite->height);
  ASSESS ("assess-output");
  g_string_append_printf (desc, "! gdppay ");
  /*
     ASSESS ("assess-output-payed");
   */
  g_string_append_printf (desc, "! sink. ");

  return desc;
}

/**
 * gst_switch_server_prepare_output:
 *
 * Preparing the composite output worker.
 */
static void
gst_switch_server_prepare_output (GstWorker * worker, GstSwitchServer * srv)
{
  GstElement *sink = NULL;
  sink = gst_worker_get_element_unlocked (worker, "sink");

  g_return_if_fail (GST_IS_ELEMENT (sink));

  g_signal_connect (sink, "client-added",
      G_CALLBACK (gst_switch_server_output_client_socket_added), srv);

  g_signal_connect (sink, "client-socket-removed",
      G_CALLBACK (gst_switch_server_output_client_socket_removed), srv);

  gst_object_unref (sink);
}

/**
 * gst_switch_server_create_output:
 * @return TRUE if the composite output worker is created.
 *
 * Create the composite output worker.
 */
static gboolean
gst_switch_server_create_output (GstSwitchServer * srv)
{
  if (srv->output) {
    return TRUE;
  }

  srv->output = GST_WORKER (g_object_new (GST_TYPE_WORKER,
          "name", "output", NULL));
  srv->output->pipeline_func_data = srv;
  srv->output->pipeline_func = (GstWorkerGetPipelineString)
      gst_switch_server_get_output_string;

  g_signal_connect (srv->output, "prepare-worker",
      G_CALLBACK (gst_switch_server_prepare_output), srv);
  g_signal_connect (srv->output, "start-worker",
      G_CALLBACK (gst_switch_server_start_output), srv);

  gst_worker_start (srv->output);
  return TRUE;
}

/**
 * gst_switch_server_create_recorder:
 * @return TRUE if the recorder is created successfully.
 *
 * Creating the recorder.
 */
static gboolean
gst_switch_server_create_recorder (GstSwitchServer * srv)
{
  if (srv->recorder) {
    return TRUE;
  }

  GST_SWITCH_SERVER_LOCK_RECORDER (srv);
  srv->recorder = GST_RECORDER (g_object_new (GST_TYPE_RECORDER,
          "name", "recorder", "port",
          srv->composite->encode_sink_port, "mode",
          srv->composite->mode, "width",
          srv->composite->width, "height", srv->composite->height, NULL));

  g_signal_connect (srv->recorder, "start-worker",
      G_CALLBACK (gst_switch_server_start_recorder), srv);

  gst_worker_start (GST_WORKER (srv->recorder));
  GST_SWITCH_SERVER_UNLOCK_RECORDER (srv);
  return TRUE;
}

/*
gboolean timeout(gpointer user_data) {
  INFO ("Exiting!");
  GstSwitchServer *srv = (GstSwitchServer*)user_data;
  g_main_loop_quit (srv->main_loop);
  return FALSE;
} */

/**
 * gst_switch_server_run:
 *
 * Running the GstSwitchServer instance.
 */
static void
gst_switch_server_run (GstSwitchServer * srv)
{
  GST_SWITCH_SERVER_LOCK_MAIN_LOOP (srv);
  srv->main_loop = g_main_loop_new (NULL, TRUE);
  GST_SWITCH_SERVER_UNLOCK_MAIN_LOOP (srv);

  //g_timeout_add_seconds (15, &timeout, srv);

  if (!gst_switch_server_prepare_composite (srv, DEFAULT_COMPOSE_MODE))
    goto error_prepare_composite;

  if (!gst_switch_server_create_output (srv))
    goto error_prepare_output;

  if (!gst_switch_server_create_recorder (srv))
    goto error_prepare_recorder;

  srv->video_acceptor = g_thread_new ("switch-server-video-acceptor",
      (GThreadFunc)
      gst_switch_server_video_acceptor, srv);

  srv->audio_acceptor = g_thread_new ("switch-server-audio-acceptor",
      (GThreadFunc)
      gst_switch_server_audio_acceptor, srv);

  // TODO: quit the server if controller is not ready
  gst_switch_server_prepare_bus_controller (srv);

  g_main_loop_run (srv->main_loop);

  GST_SWITCH_SERVER_LOCK_MAIN_LOOP (srv);
  srv->main_loop = NULL;
  GST_SWITCH_SERVER_UNLOCK_MAIN_LOOP (srv);

/*
  g_thread_join (srv->video_acceptor);
  g_thread_join (srv->audio_acceptor);
*/
  return;

  /* Errors Handling */
error_prepare_composite:
  {
    ERROR ("error preparing server");
    return;
  }
error_prepare_output:
  {
    ERROR ("error preparing server");
    return;
  }
error_prepare_recorder:
  {
    ERROR ("error preparing server");
    return;
  }
}

static unsigned long long i = 0;

void
my_handler (int signum)
{
  extern void __gcov_flush ();
  printf ("received signal\n");
  printf ("%llu\n", i);
  __gcov_flush ();              /* dump coverage data on receiving SIGUSR1 */
}


int
main (int argc, char *argv[])
{

  struct sigaction new_action, old_action;

  /* setup signal hander */
  new_action.sa_handler = my_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGUSR1, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGUSR1, &new_action, NULL);

  gint exit_code = 0;
  GstSwitchServer *srv;
  gst_switch_server_parse_args (argc, argv);

  srv = GST_SWITCH_SERVER (g_object_new (GST_TYPE_SWITCH_SERVER, NULL));

  gst_switch_server_run (srv);

  exit_code = srv->exit_code;
  g_object_unref (srv);

  gst_deinit ();
  return exit_code;
}
