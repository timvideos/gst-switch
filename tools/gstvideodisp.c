/* gst-switch
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

#include "gstvideodisp.h"
#include "gstswitchserver.h"
#include <gst/video/videooverlay.h>

#define parent_class gst_video_disp_parent_class

enum
{
  PROP_0,
  PROP_PORT,
  PROP_HANDLE,
};

extern gboolean verbose;

G_DEFINE_TYPE (GstVideoDisp, gst_video_disp, GST_TYPE_WORKER);

/**
 * @brief
 * @param disp The GstVideoDisp instance.
 * @memberof GstVideoDisp
 */
static void
gst_video_disp_init (GstVideoDisp * disp)
{
  //INFO ("init %p", disp);
}

/**
 * @brief
 * @param disp The GstVideoDisp instance.
 * @memberof GstVideoDisp
 */
static void
gst_video_disp_dispose (GstVideoDisp * disp)
{
  //INFO ("dispose %p", disp);
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (disp));
}

/**
 * @brief
 * @param disp The GstVideoDisp instance.
 * @memberof GstVideoDisp
 */
static void
gst_video_disp_finalize (GstVideoDisp * disp)
{
  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (disp));
}

/**
 * @brief
 * @param disp The GstVideoDisp instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstVideoDisp
 */
static void
gst_video_disp_set_property (GstVideoDisp * disp, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_PORT:
      disp->port = g_value_get_uint (value);
      break;
    case PROP_HANDLE:
      disp->handle = g_value_get_ulong (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (disp), property_id, pspec);
      break;
  }
}

/**
 * @brief
 * @param disp The GstVideoDisp instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstVideoDisp
 */
static void
gst_video_disp_get_property (GstVideoDisp * disp, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_PORT:
      g_value_set_uint (value, disp->port);
      break;
    case PROP_HANDLE:
      g_value_set_ulong (value, disp->handle);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (disp), property_id, pspec);
      break;
  }
}

/**
 * @brief
 * @param disp The GstVideoDisp instance.
 * @memberof GstVideoDisp
 */
static GString *
gst_video_disp_get_pipeline_string (GstVideoDisp * disp)
{
  GString *desc;

  INFO ("display video %d", disp->port);

  desc = g_string_new ("");

  g_string_append_printf (desc, "tcpclientsrc name=source "
      "port=%d ", disp->port);
  g_string_append_printf (desc, "! gdpdepay ");
  g_string_append_printf (desc, "! videoconvert ");
  g_string_append_printf (desc, "! cairooverlay name=overlay ");
  g_string_append_printf (desc, "! videoconvert ");
  g_string_append_printf (desc, "! xvimagesink name=sink sync=false ");

  return desc;
}

/**
 * @brief
 * @param disp The GstVideoDisp instance.
 * @memberof GstVideoDisp
 */
static gboolean
gst_video_disp_prepare (GstVideoDisp * disp)
{
  GstWorker *worker = GST_WORKER (disp);
  GstElement *sink = gst_worker_get_element_unlocked (worker, "sink");

  g_return_val_if_fail (GST_IS_ELEMENT (sink), FALSE);

  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink), disp->handle);

  gst_object_unref (sink);

  //INFO ("prepared display video on %ld", disp->handle);
  return TRUE;
}

/**
 * @brief
 * @param klass The GstVideoDispClass instance.
 * @memberof GstVideoDispClass
 */
static void
gst_video_disp_class_init (GstVideoDispClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstWorkerClass *worker_class = GST_WORKER_CLASS (klass);

  object_class->dispose = (GObjectFinalizeFunc) gst_video_disp_dispose;
  object_class->finalize = (GObjectFinalizeFunc) gst_video_disp_finalize;
  object_class->set_property =
      (GObjectSetPropertyFunc) gst_video_disp_set_property;
  object_class->get_property =
      (GObjectGetPropertyFunc) gst_video_disp_get_property;

  g_object_class_install_property (object_class, PROP_PORT,
      g_param_spec_uint ("port", "Port",
          "Sink port",
          GST_SWITCH_MIN_SINK_PORT,
          GST_SWITCH_MAX_SINK_PORT,
          GST_SWITCH_MIN_SINK_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_HANDLE,
      g_param_spec_ulong ("handle", "Handle",
          "Window Handle", 0,
          ((gulong) - 1), 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  worker_class->prepare = (GstWorkerPrepareFunc) gst_video_disp_prepare;
  worker_class->get_pipeline_string = (GstWorkerGetPipelineStringFunc)
      gst_video_disp_get_pipeline_string;
}
