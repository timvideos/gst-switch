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

#include "gstaudiovisual.h"
#include "gstswitchserver.h"
#include <gst/video/videooverlay.h>
#include <math.h>

/*!< @internal */
#define parent_class gst_audio_visual_parent_class

enum
{
  PROP_0,
  PROP_PORT,
  PROP_HANDLE,
  PROP_ACTIVE,
};

/**
 *  @internal
 */
extern gboolean verbose;

/**
 *  @internal
 */
G_DEFINE_TYPE (GstAudioVisual, gst_audio_visual, GST_TYPE_WORKER);

/**
 * @brief Initialize GstAudioVisual instance.
 * @param visual The GstAudioVisual instance.
 * @memberof GstAudioVisual
 *
 * @see GObject
 */
static void
gst_audio_visual_init (GstAudioVisual * visual)
{
  visual->port = 0;
  visual->handle = 0;
  visual->active = FALSE;
  visual->endtime = 0;

  g_mutex_init (&visual->endtime_lock);
  g_mutex_init (&visual->value_lock);

  INFO ("init %p", visual);
}

/**
 * @brief Free GstAudioVisual while detaching from it's parent.
 * @param visual The GstAudioVisual instance.
 * @memberof GstAudioVisual
 *
 * @see GObject
 */
static void
gst_audio_visual_dispose (GstAudioVisual * visual)
{
  INFO ("dispose %p", visual);
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (visual));
}

/**
 * @brief Destroy GstAudioVisual instance.
 * @param visual The GstAudioVisual instance.
 * @memberof GstAudioVisual
 *
 * @see GObject
 */
static void
gst_audio_visual_finalize (GstAudioVisual * visual)
{
  g_mutex_clear (&visual->endtime_lock);
  g_mutex_clear (&visual->value_lock);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (visual));

  INFO ("Audio visual finalized");
}

/**
 * @brief Setting GstAudioVisual properties.
 * @param visual The GstAudioVisual instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstAudioVisual
 *
 * @see GObject
 */
static void
gst_audio_visual_set_property (GstAudioVisual * visual, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_PORT:
      visual->port = g_value_get_uint (value);
      break;
    case PROP_HANDLE:
      visual->handle = g_value_get_ulong (value);
      break;
    case PROP_ACTIVE:
      visual->active = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (visual), property_id, pspec);
      break;
  }
}

/**
 * @brief Retrieves properties of GstAudioVisual.
 * @param visual The GstAudioVisual instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstAudioVisual
 *
 * @see GObject.
 */
static void
gst_audio_visual_get_property (GstAudioVisual * visual, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_PORT:
      g_value_set_uint (value, visual->port);
      break;
    case PROP_HANDLE:
      g_value_set_ulong (value, visual->handle);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, visual->active);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (visual), property_id, pspec);
      break;
  }
}

/**
 * @brief Get the endtime of the last audio sample value.
 * @param visual The GstAudioVisual instance.
 * @memberof GstAudioVisual
 *
 * @return the endtime of the last audio sample
 */
GstClockTime
gst_audio_visual_get_endtime (GstAudioVisual * visual)
{
  GstClockTime endtime;
  g_mutex_lock (&visual->endtime_lock);
  endtime = visual->endtime;
  g_mutex_unlock (&visual->endtime_lock);
  return endtime;
}

/**
 *  @brief Get the current audio sample value.
 *  @param visual The GstAudioVisual instance.
 *  @memberof GstAudioVisual
 *  @return the value of the last audio sample
 */
gdouble
gst_audio_visual_get_value (GstAudioVisual * visual)
{
  gdouble value;
  g_mutex_lock (&visual->value_lock);
  value = visual->value;
  g_mutex_unlock (&visual->value_lock);
  return value;
}

/**
 * @brief Handling elements missing situations.
 * @param worker the GstWorker instance
 * @param elements names of missing elements
 * @memberof GstAudioVisual
 * @return TRUE if suggesting the parent to retry with something else.
 */
static gboolean
gst_audio_visual_missing (GstWorker * worker, gchar ** elements)
{
  GstAudioVisual *visual = NULL;
  gboolean retry = FALSE;
  gchar **name = elements;
  visual = GST_AUDIO_VISUAL (worker);
  (void) visual;
  for (name = elements; name; ++name) {
    ERROR ("missing element: %s", *name);
  }
  return retry;
}

/**
 * @brief Getting the pipeline strings invoked by the parent class.
 * @param visual The GstAudioVisual instance.
 * @memberof GstAudioVisual
 * @return a GString instance of the pipeline string, need to free after used
 */
static GString *
gst_audio_visual_get_pipeline_string (GstAudioVisual * visual)
{
  GString *desc;

  INFO ("display audio %d", visual->port);

  desc = g_string_new ("");

  g_string_append_printf (desc, "tcpclientsrc name=source port=%d ",
      visual->port);
  g_string_append_printf (desc, "! gdpdepay ! tee name=a\n");

  if (visual->active) {
    g_string_append_printf (desc,
        "a. ! queue ! audioconvert ! level name=level message=true !"
        " autoaudiosink name=play sync=false\n");
  }

  g_string_append_printf (desc, "a. ! queue ! audioconvert ! monoscope ");
  if (visual->active) {
    g_string_append_printf (desc, "! textoverlay text=\"active\" "
        "font-desc=\"Sans 50\" " "shaded-background=true " "auto-resize=true ");
  }
  g_string_append_printf (desc, "! videoconvert ");
  g_string_append_printf (desc, "! xvimagesink name=visual sync=false ");

  INFO ("Audio preview: %s", desc->str);

  return desc;
}

/**
 * @brief Preparing the GstAudioVisual instance.
 * @param visual The GstAudioVisual instance.
 * @memberof GstAudioVisual
 *
 * This is invoked by the parent class.
 *  
 * @return TRUE if everything prepared.
 */
static gboolean
gst_audio_visual_prepare (GstAudioVisual * visual)
{
  GstWorker *worker = GST_WORKER (visual);
  GstElement *e = gst_bin_get_by_name (GST_BIN (worker->pipeline), "visual");
  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (e), visual->handle);

  //INFO ("prepared audio visual display on %ld", visual->handle);
  return TRUE;
}

/**
 * @brief Handling pipeline messages.
 * @param visual The GstAudioVisual instance.
 * @param message The message.
 * @memberof GstAudioVisual
 * @return %FALSE if no further messages are going to be received, %TRUE to
 *	receive all further messages.
 *
 */
static gboolean
gst_audio_visual_message (GstAudioVisual * visual, GstMessage * message)
{
  //INFO ("message: %s", GST_MESSAGE_TYPE_NAME (message));
  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);
    if (g_strcmp0 (name, "level") == 0) {
      gint channels;
      GstClockTime endtime;
      gdouble rms_dB, peak_dB, decay_dB;
      gdouble rms;
      const GValue *list_rms, *list_peak, *list_decay;
      const GValue *value;

      gint i;

      if (!gst_structure_get_clock_time (s, "endtime", &endtime))
        g_warning ("Could not parse endtime");
      else {
        g_mutex_lock (&visual->endtime_lock);
        visual->endtime = endtime;
        g_mutex_unlock (&visual->endtime_lock);
      }

      /* we can get the number of channels as the length of any of the value
       * lists */
      list_rms = gst_structure_get_value (s, "rms");
      list_peak = gst_structure_get_value (s, "peak");
      list_decay = gst_structure_get_value (s, "decay");

      if (GST_VALUE_HOLDS_LIST (list_rms)) {
        channels = gst_value_list_get_size (list_rms);

        INFO ("endtime: %" GST_TIME_FORMAT ", channels: %d",
            GST_TIME_ARGS (endtime), channels);

        for (i = 0; i < channels; ++i) {
          value = gst_value_list_get_value (list_rms, i);
          rms_dB = g_value_get_double (value);

          value = gst_value_list_get_value (list_peak, i);
          peak_dB = g_value_get_double (value);

          value = gst_value_list_get_value (list_decay, i);
          decay_dB = g_value_get_double (value);

          /* converting from dB to normal gives us a value between 0.0 and 1.0
           */
          rms = pow (10, rms_dB / 20);

          INFO (" channel: %d, RMS=%f dB, peak=%f dB, decay=%f dB, "
              "normalized-RMS=%f", i, rms_dB, peak_dB, decay_dB, rms);
        }
      } else if (G_VALUE_HOLDS (list_rms, G_TYPE_ARRAY)) {
        GArray *va = (GArray *) g_value_get_boxed (list_rms);
        gdouble v = 0.0;
        channels = va->len;
        for (i = 0; i < channels; ++i) {
          value = g_array_index (va, GValue *, i);
          rms_dB = g_value_get_double (value);

          /* converting from dB to normal gives us a value between 0.0 and 1.0
           */
          rms = pow (10, rms_dB / 20);

          v += rms;
        }

        g_mutex_lock (&visual->value_lock);
        visual->value = v / (gdouble) channels;
        g_mutex_unlock (&visual->value_lock);

        /*
           INFO ("endtime: %" GST_TIME_FORMAT ", channels: %d",
           GST_TIME_ARGS (endtime), channels);
         */
      } else if (list_rms) {
        INFO ("endtime: %" GST_TIME_FORMAT ", (%s) ",
            GST_TIME_ARGS (endtime), G_VALUE_TYPE_NAME (list_rms));
      } else {
        INFO ("endtime: %" GST_TIME_FORMAT ", ->NULL<- ",
            GST_TIME_ARGS (endtime));
      }
    }
  }
  return TRUE;
}

/**
 * @brief Initialize GstAudioVisualClass.
 * @param klass The GstAudioVisualClass instance.
 * @memberof GstAudioVisualClass
 */
static void
gst_audio_visual_class_init (GstAudioVisualClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstWorkerClass *worker_class = GST_WORKER_CLASS (klass);

  object_class->dispose = (GObjectFinalizeFunc) gst_audio_visual_dispose;
  object_class->finalize = (GObjectFinalizeFunc) gst_audio_visual_finalize;
  object_class->set_property =
      (GObjectSetPropertyFunc) gst_audio_visual_set_property;
  object_class->get_property =
      (GObjectGetPropertyFunc) gst_audio_visual_get_property;

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

  g_object_class_install_property (object_class, PROP_ACTIVE,
      g_param_spec_boolean ("active", "Active",
          "Activated audio",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  worker_class->missing = gst_audio_visual_missing;
  worker_class->prepare = (GstWorkerPrepareFunc) gst_audio_visual_prepare;
  worker_class->message = (GstWorkerMessageFunc) gst_audio_visual_message;
  worker_class->get_pipeline_string = (GstWorkerGetPipelineStringFunc)
      gst_audio_visual_get_pipeline_string;
}
