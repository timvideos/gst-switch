/* GstSwitch
 * Copyright (C) 2013 Duzy Chan <code@duzy.info>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "gstswitchserver.h"
#include "gstcomposite.h"
#include "gstrecorder.h"

enum
{
  PROP_0,
  PROP_MODE,
  PROP_PORT,
  PROP_WIDTH,
  PROP_HEIGHT,
};

enum
{
  SIGNAL__LAST,
};

//static guint gst_recorder_signals[SIGNAL__LAST] = { 0 };
extern gboolean verbose;

#define parent_class gst_recorder_parent_class

G_DEFINE_TYPE (GstRecorder, gst_recorder, GST_TYPE_WORKER);

/**
 * @brief Initialize the GstRecorder instance.
 * @param rec The GstRecorder instance.
 * @memberof GstRecorder
 */
static void
gst_recorder_init (GstRecorder * rec)
{
  rec->sink_port = 0;
  rec->mode = 0;
  rec->width = 0;
  rec->height = 0;

  // Recording pipeline needs clean shut-down
  // via EOS to close out each recording
  GST_WORKER (rec)->send_eos_on_stop = TRUE;
  //INFO ("init %p", rec);
}

/**
 * @brief Invoked to unref objects.
 * @param rec The GstRecorder instance.
 * @memberof GstRecorder
 * @see GObject
 */
static void
gst_recorder_dispose (GstRecorder * rec)
{
  INFO ("dispose %p", rec);
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (rec));
}

/**
 * @param rec The GstRecorder instance.
 * @memberof GstRecorder
 *
 * Destroying the GstRecorder instance.
 *
 * @see GObject
 */
static void
gst_recorder_finalize (GstRecorder * rec)
{
  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (rec));
}

/**
 * @param rec The GstRecorder instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstRecorder
 *
 * Fetching the GstRecorder property.
 *
 * @see GObject
 */
static void
gst_recorder_get_property (GstRecorder * rec, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_MODE:
      g_value_set_uint (value, rec->mode);
      break;
    case PROP_PORT:
      g_value_set_uint (value, rec->sink_port);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, rec->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, rec->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (rec, property_id, pspec);
      break;
  }
}

/**
 * @param rec The GstRecorder instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstRecorder
 *
 * Changing the GstRecorder properties.
 *
 * @see GObject
 */
static void
gst_recorder_set_property (GstRecorder * rec, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_MODE:
      rec->mode = (GstCompositeMode) (g_value_get_uint (value));
      break;
    case PROP_PORT:
      rec->sink_port = g_value_get_uint (value);
      break;
    case PROP_WIDTH:
      rec->width = g_value_get_uint (value);
      break;
    case PROP_HEIGHT:
      rec->height = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (rec), property_id, pspec);
      break;
  }
}

/*
 * @param dir - directory to create
 * @return nothing
 * Create a directory and all intermediary directories
 * if necessary. Note that errors are ignored here, if
 * the resulting path is in fact unusable having early
 * warning here is not necessary
 */
static void
gst_recorder_mkdirs (const char *dir)
{
  char tmp[256];
  strncpy (tmp, dir, sizeof (tmp));
  size_t len = strlen (tmp);
  if (len > 0) {
    if (tmp[len - 1] == '/')
      tmp[len--] = 0;
    if (len > 0) {
      size_t at = 1;            // skip leading slash
      while (at < len) {
        char *p = strchr (tmp + at, '/');
        if (p != NULL && *p == '/') {
          *p = '\0';
          mkdir (tmp, S_IRWXU);
          *p = '/';
          at = p - tmp + 1;
        } else
          at = len;
      }
      mkdir (tmp, S_IRWXU);
    }
  }
}

/**
 * @param filepath - file file/path of a unix file
 * @return length of the path potion of the file, excluding the separator
 */
static size_t
gst_recorder_pathlen (const char *filepath)
{
  if (filepath != NULL && strlen (filepath) > 0) {
    char const *sep = strrchr (filepath + 1, '/');
    if (sep != NULL)
      return sep - filepath;
  }
  return 0;
}


/**
 * @param filename Template name of the file to save
 * @return the file name string, need to be freed after used
 *
 * This is used to generate a new recording file name for the recorder.
 */
static const gchar *
gst_recorder_new_filename (const gchar * filename)
{
  if (!filename)
    return NULL;

  gchar fnbuf[256];
  time_t t = time (NULL);
  struct tm *tm = localtime (&t);
  // Note: reserve some space for collision suffix
  strftime (fnbuf, sizeof (fnbuf) - 5, filename, tm);
  // We now have a fully built name in our buffer
  // If there is at least one directory present, make sure they exist
  size_t pathlen = gst_recorder_pathlen (fnbuf);
  if (pathlen > 0) {
    fnbuf[pathlen] = '\0';
    gst_recorder_mkdirs (fnbuf);
    fnbuf[pathlen] = '/';
  }
  pathlen = strlen (fnbuf);     // reuse for length of file/path

  // handle name collisions by adding a suffix/extension
  size_t suffix = 0;
  while (1) {
    struct stat s;
    if (-1 == stat (fnbuf, &s)) {
      if (ENOENT == errno)
        break;
      else {
        perror (fnbuf);
        return NULL;            // can't record
      }
    }
    snprintf (fnbuf + pathlen, 256 - pathlen, ".%03d", (int) suffix++);
    // can't record if we've used up our additions
    if (suffix > 999)
      return NULL;
  }

  return g_strdup (fnbuf);
}

/**
 * @param rec The GstRecorder instance.
 * @memberof GstRecorder
 * @return The recorder pipeline string, needs freeing when used
 *
 * Fetching the recorder pipeline invoked by the GstWorker.
 */
static GString *
gst_recorder_get_pipeline_string (GstRecorder * rec)
{
  const gchar *filename =
      gst_recorder_new_filename (gst_switch_server_get_record_filename ());
  GString *desc;

  //INFO ("Recording to %s and port %d", filename, rec->sink_port);

  desc = g_string_new ("");

  // Encode the video with lossless jpeg
  g_string_append_printf (desc,
      "intervideosrc name=source_video channel=composite_video "
      "! video/x-raw,width=%d,height=%d "
      "! queue ! jpegenc quality=100 ! mux. \n", rec->width, rec->height);

  // Don't encode the audio
  g_string_append_printf (desc,
      "interaudiosrc name=source_audio channel=composite_audio ! queue ! mux. \n");

  // Output in streamable mkv format
  g_string_append_printf (desc,
      "matroskamux name=mux streamable=true "
      " writing-app='gst-switch' min-index-interval=1000000 ");

  g_string_append_printf (desc, "! tee name=result ");
  g_string_append_printf (desc, "\n");

  if (filename) {
    g_string_append_printf (desc, "result. ! queue max-size-buffers=1 "
        "! filesink name=disk_sink sync=false location=\"%s\" ", filename);
    g_free ((gpointer) filename);
  }

  g_string_append_printf (desc, "result. ! queue max-size-buffers=1 ! gdppay "
      "! tcpserversink name=tcp_sink sync=false port=%d ", rec->sink_port);

  INFO ("Recording pipeline\n----\n%s\n---", desc->str);

  return desc;
}

/**
 * @param rec The GstRecorder instance.
 * @param element
 * @param socket
 * @memberof GstRecorder
 *
 * Invoked when client socket added on the encoding out port.
 */
static void
gst_recorder_client_socket_added (GstElement * element,
    GSocket * socket, GstRecorder * rec)
{
  g_return_if_fail (G_IS_SOCKET (socket));

  INFO ("client-socket-added: %d", g_socket_get_fd (socket));
}

/**
 * @param rec The GstRecorder instance.
 * @param element
 * @param socket
 * @memberof GstRecorder
 *
 * Invoked when the client socket on the encoding out port is closed. We need
 * to manually close the socket to avoid FD leaks.
 */
static void
gst_recorder_client_socket_removed (GstElement * element,
    GSocket * socket, GstRecorder * rec)
{
  g_return_if_fail (G_IS_SOCKET (socket));

  INFO ("client-socket-removed: %d", g_socket_get_fd (socket));

  g_socket_close (socket, NULL);
}

/**
 * @param rec The GstRecorder instance.
 * @memberof GstRecorder
 * @return TRUE indicating the recorder is prepared, FALSE otherwise.
 *
 * Invoked when the GstWorker is preparing the pipeline.
 */
static gboolean
gst_recorder_prepare (GstRecorder * rec)
{
  GstElement *tcp_sink = NULL;

  g_return_val_if_fail (GST_IS_RECORDER (rec), FALSE);

  tcp_sink = gst_worker_get_element_unlocked (GST_WORKER (rec), "tcp_sink");

  g_return_val_if_fail (GST_IS_ELEMENT (tcp_sink), FALSE);

  g_signal_connect (tcp_sink, "client-added",
      G_CALLBACK (gst_recorder_client_socket_added), rec);

  g_signal_connect (tcp_sink, "client-socket-removed",
      G_CALLBACK (gst_recorder_client_socket_removed), rec);

  gst_object_unref (tcp_sink);
  return TRUE;
}

/**
 * @brief Initialize the GstRecorderClass.
 * @param klass The GstRecorderClass instance.
 * @memberof GstRecorderClass
 */
static void
gst_recorder_class_init (GstRecorderClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstWorkerClass *worker_class = GST_WORKER_CLASS (klass);

  object_class->dispose = (GObjectFinalizeFunc) gst_recorder_dispose;
  object_class->finalize = (GObjectFinalizeFunc) gst_recorder_finalize;
  object_class->set_property =
      (GObjectSetPropertyFunc) gst_recorder_set_property;
  object_class->get_property =
      (GObjectGetPropertyFunc) gst_recorder_get_property;

  g_object_class_install_property (object_class, PROP_MODE,
      g_param_spec_uint ("mode", "Mode",
          "Composite Mode",
          COMPOSE_MODE_NONE,
          COMPOSE_MODE__LAST,
          COMPOSE_MODE_NONE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PORT,
      g_param_spec_uint ("port", "Port",
          "Sink port",
          GST_SWITCH_MIN_SINK_PORT,
          GST_SWITCH_MAX_SINK_PORT,
          GST_SWITCH_MIN_SINK_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_WIDTH,
      g_param_spec_uint ("width", "Input Width",
          "Input video frame width",
          1, G_MAXINT,
          gst_composite_default_width (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_HEIGHT,
      g_param_spec_uint ("height",
          "Input Height",
          "Input video frame height",
          1, G_MAXINT,
          gst_composite_default_height (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  worker_class->prepare = (GstWorkerPrepareFunc) gst_recorder_prepare;
  worker_class->get_pipeline_string = (GstWorkerGetPipelineStringFunc)
      gst_recorder_get_pipeline_string;
}
