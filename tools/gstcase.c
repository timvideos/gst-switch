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

#include <stdlib.h>
#include <string.h>
#include "gstswitchserver.h"
#include "gstcase.h"

enum
{
  PROP_0,
  PROP_TYPE,
  PROP_SERVE,
  PROP_STREAM,
  PROP_INPUT,
  PROP_BRANCH,
  PROP_PORT,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_A_WIDTH,
  PROP_A_HEIGHT,
  PROP_B_WIDTH,
  PROP_B_HEIGHT,
};

enum
{
  SIGNAL__LAST,
};

//static guint gst_case_signals[SIGNAL__LAST] = { 0 };
extern gboolean verbose;

#define gst_case_parent_class parent_class
G_DEFINE_TYPE (GstCase, gst_case, GST_TYPE_WORKER);

/**
 * @param cas The GstCase instance.
 * @memberof GstCase
 *
 * Initialize the GstCase instance.
 *
 * @see GObject
 */
static void
gst_case_init (GstCase * cas)
{
  cas->type = GST_CASE_UNKNOWN;
  cas->stream = NULL;
  cas->input = NULL;
  cas->branch = NULL;
  cas->serve_type = GST_SERVE_NOTHING;
  cas->sink_port = 0;
  cas->width = 0;
  cas->height = 0;
  cas->a_width = 0;
  cas->a_height = 0;
  cas->b_width = 0;
  cas->b_height = 0;

  //INFO ("init %p", cas);
}

/**
 * @param cas the GstCase instance.
 * @memberof GstCase
 *
 * Closes/releases resources used by the GstCase
 */
static void
gst_case_close (GstCase * cas)
{
  if (cas->stream) {
    GError *error = NULL;
    g_input_stream_close (cas->stream, NULL, &error);
    if (error) {
      ERROR ("%s", error->message);
    }
    g_object_unref (cas->stream);
    cas->stream = NULL;
  }

  if (cas->input) {
    g_object_unref (cas->input);
    cas->input = NULL;
  }

  if (cas->branch) {
    g_object_unref (cas->branch);
    cas->branch = NULL;
  }
}

/**
 * @param cas The GstR (Case instance.
 * @memberof GstCase
 *
 * Disposing from it's parent object.
 *
 * @see GObject
 */
static void
gst_case_dispose (GstCase * cas)
{
  gst_case_close (cas);
  //INFO ("dispose %p", cas);
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (cas));
}

/**
 * @param cas The GstCase instance.
 * @memberof GstCase
 *
 * Destroying the GstCase instance.
 *
 * @see GObject
 */
static void
gst_case_finalize (GstCase * cas)
{
  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (cas));
}

/**
 * @param cas The GstCase instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstCase
 *
 * Getting GstCase property.
 *
 * @see GObject
 */
static void
gst_case_get_property (GstCase * cas, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_TYPE:
      g_value_set_uint (value, cas->type);
      break;
    case PROP_SERVE:
      g_value_set_uint (value, cas->serve_type);
      break;
    case PROP_STREAM:
      g_value_set_object (value, cas->stream);
      break;
    case PROP_INPUT:
      g_value_set_object (value, cas->input);
      break;
    case PROP_BRANCH:
      g_value_set_object (value, cas->branch);
      break;
    case PROP_PORT:
      g_value_set_uint (value, cas->sink_port);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, cas->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, cas->height);
      break;
    case PROP_A_WIDTH:
      g_value_set_uint (value, cas->a_width);
      break;
    case PROP_A_HEIGHT:
      g_value_set_uint (value, cas->a_height);
      break;
    case PROP_B_WIDTH:
      g_value_set_uint (value, cas->b_width);
      break;
    case PROP_B_HEIGHT:
      g_value_set_uint (value, cas->b_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (cas, property_id, pspec);
      break;
  }
}

/**
 * @param cas The GstCase instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstCase
 *
 * Setting GstCase property.
 *
 * @see GObject
 */
static void
gst_case_set_property (GstCase * cas, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_TYPE:
      cas->type = (GstCaseType) g_value_get_uint (value);
      break;
    case PROP_SERVE:
      cas->serve_type = (GstSwitchServeStreamType) g_value_get_uint (value);
      break;
    case PROP_STREAM:
    {
      GObject *stream = g_value_dup_object (value);
      if (cas->stream)
        g_object_unref (cas->stream);
      cas->stream = G_INPUT_STREAM (stream);
    }
      break;
    case PROP_INPUT:
    {
      GObject *input = g_value_dup_object (value);
      if (cas->input)
        g_object_unref (cas->input);
      cas->input = GST_CASE (input);
    }
      break;
    case PROP_BRANCH:
    {
      GObject *branch = g_value_dup_object (value);
      if (cas->branch)
        g_object_unref (cas->branch);
      cas->branch = GST_CASE (branch);
    }
      break;
    case PROP_PORT:
      cas->sink_port = g_value_get_uint (value);
      break;
    case PROP_WIDTH:
      cas->width = g_value_get_uint (value);
      break;
    case PROP_HEIGHT:
      cas->height = g_value_get_uint (value);
      break;
    case PROP_A_WIDTH:
      cas->a_width = g_value_get_uint (value);
      break;
    case PROP_A_HEIGHT:
      cas->a_height = g_value_get_uint (value);
      break;
    case PROP_B_WIDTH:
      cas->b_width = g_value_get_uint (value);
      break;
    case PROP_B_HEIGHT:
      cas->b_height = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (cas), property_id, pspec);
      break;
  }
}

/**
 * @param cas The GstCase instance.
 * @memberof GstCase
 * @return A GString instance representing the pipeline string.
 *
 * Retreiving the GstCase pipeline string, it's invoked by GstWorker.
 */
static GString *
gst_case_get_pipeline_string (GstCase * cas)
{
  gboolean is_audiostream = cas->serve_type == GST_SERVE_AUDIO_STREAM;
  GString *desc = g_string_new ("");
  const gchar *caps =
      is_audiostream ?
      gst_switch_server_get_audio_caps_str () :
      gst_switch_server_get_video_caps_str ();

  switch (cas->type) {
    case GST_CASE_INPUT_AUDIO:
      g_string_append_printf (desc,
          "giostreamsrc name=source ! gdpdepay ! %s ! interaudiosink name=sink channel=input_%d",
          caps, cas->sink_port);
      break;

    case GST_CASE_INPUT_VIDEO:
      g_string_append_printf (desc,
          "giostreamsrc name=source ! gdpdepay ! %s ! intervideosink name=sink channel=input_%d",
          caps, cas->sink_port);
      break;

    case GST_CASE_PREVIEW:
      if (is_audiostream) {
        g_string_append_printf (desc,
            "interaudiosrc name=source channel=input_%d ! %s ! audioparse raw-format=s16le rate=48000 ! interaudiosink name=sink channel=branch_%d",
            cas->sink_port, caps, cas->sink_port);
      } else {
        g_string_append_printf (desc,
            "intervideosrc name=source channel=input_%d ! %s ! intervideosink name=sink channel=branch_%d",
            cas->sink_port, caps, cas->sink_port);
      }
      break;

    case GST_CASE_COMPOSITE_AUDIO:
      g_string_append_printf (desc,
          "interaudiosrc name=source channel=input_%d ! %s ! audioparse raw-format=s16le rate=48000 ! tee name=s "
          "s. ! queue ! interaudiosink name=sink1 channel=branch_%d "
          "s. ! queue ! interaudiosink name=sink2 channel=composite_audio",
          cas->sink_port, caps, cas->sink_port);
      break;

    case GST_CASE_COMPOSITE_VIDEO_A:
    case GST_CASE_COMPOSITE_VIDEO_B:
    {
      gchar *channel = cas->type == GST_CASE_COMPOSITE_VIDEO_A ? "a" : "b";
      g_string_append_printf (desc,
          "intervideosrc name=source channel=input_%d ! %s ! tee name=s "
          "s. ! queue ! intervideosink name=sink1 channel=branch_%d "
          "s. ! queue ! intervideosink name=sink2 channel=composite_%s",
          cas->sink_port, caps, cas->sink_port, channel);
      break;
    }

    case GST_CASE_BRANCH_AUDIO:
      g_string_append_printf (desc,
          "interaudiosrc name=source channel=branch_%d ! %s ! audioparse raw-format=s16le rate=48000 ! gdppay ! tcpserversink name=sink port=%d",
          cas->sink_port, caps, cas->sink_port);
      break;

    case GST_CASE_BRANCH_VIDEO_A:
    case GST_CASE_BRANCH_VIDEO_B:
      g_string_append_printf (desc,
          "intervideosrc name=source channel=branch_%d ! %s ! gdppay ! tcpserversink name=sink port=%d",
          cas->sink_port, caps, cas->sink_port);
      break;

    case GST_CASE_BRANCH_PREVIEW:
      g_string_append_printf (desc,
          "intervideosrc name=source channel=branch_%d ! %s ! gdppay ! tcpserversink name=sink port=%d",
          cas->sink_port, caps, cas->sink_port);
      break;

    default:
      ERROR ("unknown case (%d)", cas->type);
      break;
  }

  INFO ("pipeline(%p): %s\n", cas, desc->str);
  return desc;
}

/**
 * @param element
 * @param socket
 * @param cas The GstCase instance.
 * @memberof GstCase
 *
 * Invoked when a client socket is added.
 */
static void
gst_case_client_socket_added (GstElement * element,
    GSocket * socket, GstCase * cas)
{
  g_return_if_fail (G_IS_SOCKET (socket));

  //INFO ("client-socket-added: %d", g_socket_get_fd (socket));
}

/**
 * @param element
 * @param socket
 * @param cas The GstCase instance.
 * @memberof GstCase
 *
 * Invoked when a client socket is removed.
 */
static void
gst_case_client_socket_removed (GstElement * element,
    GSocket * socket, GstCase * cas)
{
  g_return_if_fail (G_IS_SOCKET (socket));

  //INFO ("client-socket-removed: %d", g_socket_get_fd (socket));

  g_socket_close (socket, NULL);
}

/**
 * @param cas The GstCase instance.
 * @memberof GstCase
 *
 * Invoked by GstWorker when preparing the pipeline.
 */
static gboolean
gst_case_prepare (GstCase * cas)
{
  GstWorker *worker = GST_WORKER (cas);
  GstElement *source = NULL;
  switch (cas->type) {
    case GST_CASE_INPUT_AUDIO:
    case GST_CASE_INPUT_VIDEO:
      if (!cas->stream) {
        ERROR ("no stream for new case");
        return FALSE;
      }
      source = gst_worker_get_element_unlocked (worker, "source");
      if (!source) {
        ERROR ("no source");
        return FALSE;
      }
      g_object_set (source, "stream", cas->stream, NULL);
      gst_object_unref (source);
      break;

    case GST_CASE_BRANCH_VIDEO_A:
    case GST_CASE_BRANCH_VIDEO_B:
    case GST_CASE_BRANCH_AUDIO:
    case GST_CASE_BRANCH_PREVIEW:
    {
      GstElement *sink = gst_worker_get_element_unlocked (worker, "sink");

      g_return_val_if_fail (GST_IS_ELEMENT (sink), FALSE);

      g_signal_connect (sink, "client-added",
          G_CALLBACK (gst_case_client_socket_added), cas);

      g_signal_connect (sink, "client-socket-removed",
          G_CALLBACK (gst_case_client_socket_removed), cas);
    }
      break;

    default:
      break;
  }

  return TRUE;
}

/**
 * @brief Initialize GstCaseClass.
 * @param klass The GstCaseClass instance.
 * @memberof GstCaseClass
 */
static void
gst_case_class_init (GstCaseClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstWorkerClass *worker_class = GST_WORKER_CLASS (klass);

  object_class->dispose = (GObjectFinalizeFunc) gst_case_dispose;
  object_class->finalize = (GObjectFinalizeFunc) gst_case_finalize;
  object_class->set_property = (GObjectSetPropertyFunc) gst_case_set_property;
  object_class->get_property = (GObjectGetPropertyFunc) gst_case_get_property;

  g_object_class_install_property (object_class, PROP_TYPE,
      g_param_spec_uint ("type", "Type",
          "Case type",
          GST_CASE_UNKNOWN,
          GST_CASE__LAST_TYPE,
          GST_CASE_UNKNOWN, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_SERVE,
      g_param_spec_uint ("serve", "Serve",
          "Serve type",
          GST_SERVE_NOTHING,
          GST_SERVE_AUDIO_STREAM,
          GST_SERVE_NOTHING, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_STREAM,
      g_param_spec_object ("stream", "Stream",
          "Stream to read from",
          G_TYPE_INPUT_STREAM, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_INPUT,
      g_param_spec_object ("input", "Input",
          "The input of the case",
          GST_TYPE_CASE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_BRANCH,
      g_param_spec_object ("branch", "Branch",
          "The branch of the case",
          GST_TYPE_CASE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PORT,
      g_param_spec_uint ("port", "Port",
          "Sink port",
          GST_SWITCH_MIN_SINK_PORT,
          GST_SWITCH_MAX_SINK_PORT,
          GST_SWITCH_MIN_SINK_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_WIDTH,
      g_param_spec_uint ("width", "Width",
          "Output width", 1,
          G_MAXINT,
          gst_composite_default_width (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_HEIGHT,
      g_param_spec_uint ("height", "Height",
          "Output height", 1,
          G_MAXINT,
          gst_composite_default_height (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_A_WIDTH,
      g_param_spec_uint ("awidth", "A Width",
          "Channel A width", 1,
          G_MAXINT,
          gst_composite_default_width (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_A_HEIGHT,
      g_param_spec_uint ("aheight", "A Height",
          "Channel A height", 1,
          G_MAXINT,
          gst_composite_default_height (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_B_WIDTH,
      g_param_spec_uint ("bwidth", "B Width",
          "Channel B width", 1,
          G_MAXINT,
          gst_composite_default_width (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_B_HEIGHT,
      g_param_spec_uint ("bheight", "B Height",
          "Channel B height", 1,
          G_MAXINT,
          gst_composite_default_height (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  worker_class->prepare = (GstWorkerPrepareFunc) gst_case_prepare;
  worker_class->get_pipeline_string = (GstWorkerGetPipelineStringFunc)
      gst_case_get_pipeline_string;
  worker_class->close = (GstWorkerCloseFunc) gst_case_close;
}
