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

#define GST_COMPOSITE_LOCK(composite) (g_mutex_lock (&(composite)->lock))
#define GST_COMPOSITE_UNLOCK(composite) (g_mutex_unlock (&(composite)->lock))
#define GST_COMPOSITE_LOCK_TRANSITION(composite) (g_mutex_lock (&(composite)->transition_lock))
#define GST_COMPOSITE_UNLOCK_TRANSITION(composite) (g_mutex_unlock (&(composite)->transition_lock))
#define GST_COMPOSITE_LOCK_ADJUSTMENT(composite) (g_mutex_lock (&(composite)->adjustment_lock))
#define GST_COMPOSITE_UNLOCK_ADJUSTMENT(composite) (g_mutex_unlock (&(composite)->adjustment_lock))

enum
{
  PROP_0,
  PROP_MODE,
  PROP_PORT,
  PROP_ENCODE_PORT,
  PROP_A_X,
  PROP_A_Y,
  PROP_A_WIDTH,
  PROP_A_HEIGHT,
  PROP_B_X,
  PROP_B_Y,
  PROP_B_WIDTH,
  PROP_B_HEIGHT,
  PROP_WIDTH,
  PROP_HEIGHT,
};

enum
{
  SIGNAL_END_TRANSITION,
  SIGNAL__LAST,                 /*!< @internal */
};

static guint gst_composite_signals[SIGNAL__LAST] = { 0 };

extern gboolean verbose;

/*!< @internal */
#define gst_composite_parent_class parent_class

/*!< @internal */
G_DEFINE_TYPE (GstComposite, gst_composite, GST_TYPE_WORKER);

static void gst_composite_set_mode (GstComposite *, GstCompositeMode);
static void gst_composite_start_transition (GstComposite *);

/**
 * Initialize the GstComposite instance.
 * 
 * @see GObject
 */
static void
gst_composite_init (GstComposite * composite)
{
  INFO ("gst_composite init %p", composite);

  composite->adjusting = FALSE;
  composite->transition = FALSE;
  composite->deprecated = FALSE;

  g_mutex_init (&composite->lock);
  g_mutex_init (&composite->transition_lock);
  g_mutex_init (&composite->adjustment_lock);

  gst_composite_set_mode (composite, DEFAULT_COMPOSE_MODE);

  /* Indicating transition from no-mode to default mode.
   */
  composite->transition = TRUE;

  //INFO ("init %p", composite);
}

/**
 * gst_composite_dispose:
 *
 * Disposing from it's parent class.
 *
 * @see GObject
 */
static void
gst_composite_dispose (GstComposite * composite)
{
  //INFO ("gst_composite dispose %p", composite);

  if (composite->scaler) {
    gst_object_unref (composite->scaler);
    composite->scaler = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (composite));
}

/**
 * gst_composite_finalize:
 *
 * Destroying the GstComposite instance.
 */
static void
gst_composite_finalize (GstComposite * composite)
{
  INFO ("gst_composite finalize %p", composite);
  g_mutex_clear (&composite->lock);
  g_mutex_clear (&composite->transition_lock);
  g_mutex_clear (&composite->adjustment_lock);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (composite));
}

/**
 * gst_composite_set_mode:
 *
 * Changing the composite mode.
 *
 * @see %GstCompositeMode
 */
static void
gst_composite_set_mode (GstComposite * composite, GstCompositeMode mode)
{
  if (composite->transition) {
    WARN ("ignore changing mode in transition");
    return;
  }

  composite->width = gst_composite_default_width ();
  composite->height = gst_composite_default_height ();

  switch ((composite->mode = mode)) {
    case COMPOSE_MODE_NONE:
      composite->a_x = 0;
      composite->a_y = 0;
      composite->a_width = composite->width;
      composite->a_height = composite->height;
      composite->b_x = 0;
      composite->b_y = 0;
      composite->b_width = 0;
      composite->b_height = 0;
      composite->width = composite->a_width;
      composite->height = composite->a_height;
      break;
    case COMPOSE_MODE_PIP:
      composite->a_x = 0;
      composite->a_y = 0;
      composite->a_width = composite->width;
      composite->a_height = composite->height;
      composite->b_x = (guint) ((double) composite->a_width * 0.08 + 0.5);
      composite->b_y = (guint) ((double) composite->a_height * 0.08 + 0.5);
      composite->b_width = (guint) ((double) composite->a_width * 0.3 + 0.5);
      composite->b_height = (guint) ((double) composite->a_height * 0.3 + 0.5);
      composite->width = composite->a_width;
      composite->height = composite->a_height;
      break;
    case COMPOSE_MODE_DUAL_PREVIEW:
      composite->a_x = 0;
      composite->a_y = 0;
      composite->a_width = (guint) ((double) composite->width * 0.7 + 0.5);
      composite->a_height = (guint) ((double) composite->height * 0.7 + 0.5);
      composite->b_x = composite->a_width + 1;
      composite->b_y = composite->a_y;
      composite->b_width =
          composite->width - composite->a_x - composite->a_width;
      composite->b_height =
          composite->height - composite->a_y - composite->a_height;
      break;
    case COMPOSE_MODE_DUAL_EQUAL:
      composite->a_width = (guint) ((double) composite->width * 0.5 + 0.5);
      composite->a_height = (guint) ((double) composite->height * 0.5 + 0.5);
      composite->a_x = 0;
      composite->a_y = (composite->height - composite->a_height) / 2;
      composite->b_x = composite->a_width + 1;
      composite->b_y = composite->a_y;
      composite->b_width =
          composite->width - composite->a_x - composite->a_width;
      composite->b_height = composite->a_height;
      break;
    default:
      break;
  }

  /*
     INFO ("new mode %d, %dx%d (%dx%d, %dx%d)", mode,
     composite->width, composite->height,
     composite->a_width, composite->a_height,
     composite->b_width, composite->b_height);
   */

  gst_composite_start_transition (composite);
}

/**
 * gst_composite_ready_for_transition:
 * @return TRUE if okay to do new transition.
 *
 * Predictor telling if it's ready for transition.
 */
static gboolean
gst_composite_ready_for_transition (GstComposite * composite)
{
  return !composite->transition;
}

/**
 * gst_composite_start_transition:
 *
 * Start the new transition request, this will set the %transition flag into
 * TRUE.
 */
static void
gst_composite_start_transition (GstComposite * composite)
{
  g_return_if_fail (GST_IS_COMPOSITE (composite));

  GST_COMPOSITE_LOCK_TRANSITION (composite);

  INFO ("starting transition");
  if (gst_composite_ready_for_transition (composite)) {
    composite->transition = gst_worker_stop (GST_WORKER (composite));
    /*
       INFO ("transtion ok=%d, %d, %dx%d", composite->transition,
       composite->mode, composite->width, composite->height);
     */
  }

  GST_COMPOSITE_UNLOCK_TRANSITION (composite);
}

/**
 * gst_composite_set_property:
 *
 * Setting the GstComposite property.
 *
 * @see GObject
 */
static void
gst_composite_set_property (GstComposite * composite, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_PORT:
      composite->sink_port = g_value_get_uint (value);
      break;
    case PROP_ENCODE_PORT:
      composite->encode_sink_port = g_value_get_uint (value);
      break;
    case PROP_A_X:
      composite->a_x = g_value_get_uint (value);
      break;
    case PROP_A_Y:
      composite->a_y = g_value_get_uint (value);
      break;
    case PROP_A_WIDTH:
      composite->a_width = g_value_get_uint (value);
      break;
    case PROP_A_HEIGHT:
      composite->a_height = g_value_get_uint (value);
      break;
    case PROP_B_X:
      composite->b_x = g_value_get_uint (value);
      break;
    case PROP_B_Y:
      composite->b_y = g_value_get_uint (value);
      break;
    case PROP_B_WIDTH:
      composite->b_width = g_value_get_uint (value);
      break;
    case PROP_B_HEIGHT:
      composite->b_height = g_value_get_uint (value);
      break;
    case PROP_MODE:
    {
      guint mode = g_value_get_uint (value);
      if (COMPOSE_MODE_NONE <= mode && mode <= COMPOSE_MODE__LAST) {
        gst_composite_set_mode (composite, (GstCompositeMode) mode);
      } else {
        WARN ("invalid composite mode %d", mode);
      }
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (composite), property_id,
          pspec);
      break;
  }
}

/**
 * gst_composite_get_property:
 *
 * Fetching the GstComposite property.
 *
 * @see GObject
 */
static void
gst_composite_get_property (GstComposite * composite, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_MODE:
      g_value_set_uint (value, composite->mode);
      break;
    case PROP_PORT:
      g_value_set_uint (value, composite->sink_port);
      break;
    case PROP_ENCODE_PORT:
      g_value_set_uint (value, composite->encode_sink_port);
      break;
    case PROP_A_X:
      g_value_set_uint (value, composite->a_x);
      break;
    case PROP_A_Y:
      g_value_set_uint (value, composite->a_y);
      break;
    case PROP_A_WIDTH:
      g_value_set_uint (value, composite->a_width);
      break;
    case PROP_A_HEIGHT:
      g_value_set_uint (value, composite->a_height);
      break;
    case PROP_B_X:
      g_value_set_uint (value, composite->b_x);
      break;
    case PROP_B_Y:
      g_value_set_uint (value, composite->b_y);
      break;
    case PROP_B_WIDTH:
      g_value_set_uint (value, composite->b_width);
      break;
    case PROP_B_HEIGHT:
      g_value_set_uint (value, composite->b_height);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, composite->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, composite->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (composite), property_id,
          pspec);
      break;
  }
}

/**
 * gst_composite_apply_parameters:
 *
 * Applying new composite parameters such as PIP position. This is actually
 * resetting the composite pipeline with the new parameters.
 */
static void
gst_composite_apply_parameters (GstComposite * composite)
{
  GstWorkerClass *worker_class;

  g_return_if_fail (GST_IS_COMPOSITE (composite));

  worker_class = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (composite));

  if (!worker_class->reset (GST_WORKER (composite))) {
    ERROR ("failed to reset composite");
  }
  /*
     if (!worker_class->reset (GST_WORKER (composite->output))) {
     ERROR ("failed to reset composite output");
     }
   */

  /*
     g_object_set (composite->recorder,
     "port", composite->encode_sink_port,
     "mode", composite->mode, "width", composite->width,
     "height", composite->height, NULL);

     if (!worker_class->reset (GST_WORKER (composite->recorder))) {
     ERROR ("failed to reset composite recorder");
     }
   */
}

/**
 * gst_composite_get_pipeline_string:
 *
 * Fetching the composite pipeline string, it's invoked by %GstWorker when
 * preparing the worker.
 */
static GString *
gst_composite_get_pipeline_string (GstComposite * composite)
{
  GString *desc;

  desc = g_string_new ("");

  g_string_append_printf (desc,
      "intervideosrc name=source_a channel=composite_a_scaled ");
  if (composite->mode == COMPOSE_MODE_NONE) {
    g_string_append_printf (desc,
        "source_a. ! video/x-raw,width=%d,height=%d ",
        composite->a_width, composite->a_height);
    /*
       ASSESS ("assess-compose-a-source");
     */
    g_string_append_printf (desc, "! queue ");
    g_string_append_printf (desc, "! identity name=mix ");
  } else {
    g_string_append_printf (desc,
        "intervideosrc name=source_b channel=composite_b_scaled ");
    g_string_append_printf (desc,
        "videomixer name=mix "
        "sink_0::xpos=%d "
        "sink_0::ypos=%d "
        "sink_0::zorder=0 "
        "sink_1::xpos=%d "
        "sink_1::ypos=%d "
        "sink_1::zorder=1 ",
        composite->a_x, composite->a_y, composite->b_x, composite->b_y);

    // ===== B =====
    g_string_append_printf (desc,
        "source_b. ! video/x-raw,width=%d,height=%d ",
        composite->b_width, composite->b_height);
    ASSESS ("assess-compose-b-source");
    g_string_append_printf (desc, "! queue ");
#if 0
    if (composite->width != composite->b_width ||
        composite->height != composite->b_height) {
      g_string_append_printf (desc,
          "! videoscale ! video/x-raw,width=%d,height=%d ",
          composite->b_width, composite->b_height);
      /*
         ASSESS ("assess-compose-b-scaled");
       */
    }
#endif
    g_string_append_printf (desc, "! mix.sink_1 ");

    // ===== A =====
    g_string_append_printf (desc,
        "source_a. ! video/x-raw,width=%d,height=%d ",
        composite->a_width, composite->a_height);
    ASSESS ("assess-compose-a-source");
    g_string_append_printf (desc, "! queue ");
#if 0
    if (composite->width != composite->a_width ||
        composite->height != composite->a_height) {
      g_string_append_printf (desc,
          "! videoscale ! video/x-raw,width=%d,height=%d ",
          composite->a_width, composite->a_height);
      /*
         ASSESS ("assess-compose-b-scaled");
       */
    }
#endif
    g_string_append_printf (desc, "! mix.sink_0 ");
  }

  g_string_append_printf (desc, "mix. ! video/x-raw,width=%d,height=%d ",
      composite->width, composite->height);
  ASSESS ("assess-compose-result");
  g_string_append_printf (desc, "! tee name=result ");

  g_string_append_printf (desc, "result. ! queue ");
  /*
     ASSESS ("assess-compose-to-output");
   */
  g_string_append_printf (desc, "! out. ");
  g_string_append_printf (desc,
      "intervideosink name=out channel=composite_out ");

  if (opts.record_filename) {
    g_string_append_printf (desc, "result. ! queue ");
    /*
       ASSESS ("assess-compose-to-record");
     */
    g_string_append_printf (desc, "! record. ");
    g_string_append_printf (desc, "intervideosink name=record "
        "channel=composite_video ");
  }

  return desc;
}

/**
 * gst_composite_get_scaler_string:
 *
 * Getting the scaler pipeline string.
 *
 * <b>The Scaler Pipeline</b>
 *     The scaler pipeline is tending to scale the A/B inputs into the proper
 *     video size for composite.
 */
static GString *
gst_composite_get_scaler_string (GstWorker * worker, GstComposite * composite)
{
  GString *desc;

  desc = g_string_new ("");

  g_string_append_printf (desc,
      "intervideosrc name=source_a channel=composite_a ");
  g_string_append_printf (desc,
      "intervideosink name=sink_a sync=false channel=composite_a_scaled ");

  g_string_append_printf (desc,
      "source_a. ! video/x-raw,width=%d,height=%d ",
      composite->width, composite->height);
  g_string_append_printf (desc, "! queue ");
  /*
     g_string_append_printf (desc,
     "! videoconvert ! facedetect2 ! speakertrack ! videoconvert ");
   */
  g_string_append_printf (desc,
      "! videoscale ! video/x-raw,width=%d,height=%d ! sink_a. ",
      composite->a_width, composite->a_height);

  if (composite->mode == COMPOSE_MODE_NONE) {
  } else {
    g_string_append_printf (desc,
        "intervideosrc name=source_b channel=composite_b ");
    g_string_append_printf (desc,
        "intervideosink name=sink_b sync=false channel=composite_b_scaled ");

    g_string_append_printf (desc,
        "source_b. ! video/x-raw,width=%d,height=%d ",
        composite->width, composite->height);
    g_string_append_printf (desc, "! queue ");
    /*
       g_string_append_printf (desc,
       "! videoconvert ! facedetect2 ! speakertrack ! videoconvert ");
     */
    g_string_append_printf (desc,
        "! videoscale ! video/x-raw,width=%d,height=%d ! sink_b. ",
        composite->b_width, composite->b_height);
  }
  return desc;
}

/**
 * gst_composite_prepare:
 * @return TRUE if the composite pipeline is well prepared.
 *
 * Prepare the composite pipeline.
 */
static gboolean
gst_composite_prepare (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->scaler == NULL) {
    composite->scaler = GST_WORKER (g_object_new (GST_TYPE_WORKER,
            "name", "scale", NULL));
    composite->scaler->pipeline_func_data = composite;
    composite->scaler->pipeline_func = (GstWorkerGetPipelineString)
        gst_composite_get_scaler_string;
  } else {
    GstWorkerClass *worker_class;
    worker_class = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (composite->scaler));
    if (!worker_class->reset (GST_WORKER (composite->scaler))) {
      ERROR ("failed to reset scaler");
    }
  }
  return TRUE;
}

/**
 * gst_composite_start:
 *
 * This is invoked when the composite pipeline started.
 */
static void
gst_composite_start (GstComposite * composite)
{
  g_return_if_fail (GST_IS_COMPOSITE (composite));

  gst_worker_start (composite->scaler);
}

/**
 * gst_composite_end:
 *
 * This is invoked when the composite pipeline is ended.
 */
static void
gst_composite_end (GstComposite * composite)
{
  g_return_if_fail (GST_IS_COMPOSITE (composite));

  gst_worker_stop (composite->scaler);
}

/**
 * gst_composite_end_transition:
 * @return Always return FALSE to allow glib to free the event source.
 *
 * Invoked when the transition is finished.
 *
 * @see %gst_composite_commit_transition
 */
static gboolean
gst_composite_end_transition (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->transition) {
    GST_COMPOSITE_LOCK_TRANSITION (composite);
    if (composite->transition) {
      /*
         INFO ("new mode %d, %dx%d transited", composite->mode,
         composite->width, composite->height);
       */
      INFO ("ending transition");
      composite->transition = FALSE;
      g_signal_emit (composite,
          gst_composite_signals[SIGNAL_END_TRANSITION],
          0 /*, composite->mode */ );
    }
    GST_COMPOSITE_UNLOCK_TRANSITION (composite);
  }
  return FALSE;
}

/**
 * gst_composite_commit_transition:
 * @return Always return FALSE to tell glib to cleanup the event source.
 *
 * Commit a transition request.
 *
 * @see %gst_composite_end_transition
 */
static gboolean
gst_composite_commit_transition (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->transition) {
    GST_COMPOSITE_LOCK_TRANSITION (composite);
    if (composite->transition) {
      /*
         INFO ("new mode %d, %dx%d applying...",
         composite->mode, composite->width, composite->height);
       */
      gst_composite_apply_parameters (composite);
    }
    GST_COMPOSITE_UNLOCK_TRANSITION (composite);
  }
  return FALSE;
}

/**
 * gst_composite_close_transition:
 *
 * Invoked when the composite pipeline is coming alive. This will emit
 * %gst_composite_end_transition.
 *
 * @see %gst_composite_end_transition
 */
static gboolean
gst_composite_close_transition (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->transition) {
    GST_COMPOSITE_LOCK_TRANSITION (composite);
    if (composite->transition) {
      //gst_worker_start (GST_WORKER (composite->output));
      /* It's ok to discard the source ID here, the timeout is one-shot. */
      g_timeout_add (200, (GSourceFunc) gst_composite_end_transition,
          composite);
    }
    GST_COMPOSITE_UNLOCK_TRANSITION (composite);
  }
  return FALSE;
}

/**
 * gst_composite_commit_adjustment:
 * @return Always FALSE to tell glib to cleanup the timeout source.
 *
 * Commit a PIP adjustment request.
 */
static gboolean
gst_composite_commit_adjustment (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->adjusting) {
    GST_COMPOSITE_LOCK_ADJUSTMENT (composite);
    if (composite->adjusting) {
      GstWorkerClass *worker_class;
      worker_class = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (composite));
      if (!worker_class->reset (GST_WORKER (composite))) {
        ERROR ("failed to reset composite");
      }
    }
    GST_COMPOSITE_UNLOCK_ADJUSTMENT (composite);
  }
  return FALSE;
}

/**
 * gst_composite_close_adjustment:
 * @return Always return false to let glib to cleanup timeout source
 *
 * Invoked when composite pipeline is coming alive and it's currently
 * adjusting the PIP.
 */
static gboolean
gst_composite_close_adjustment (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->adjusting) {
    GST_COMPOSITE_LOCK_ADJUSTMENT (composite);
    composite->adjusting = FALSE;
    GST_COMPOSITE_UNLOCK_ADJUSTMENT (composite);
  }
  return FALSE;
}

/**
 * gst_composite_alive:
 *
 * Invoked when the composite pipeline is online.
 */
static void
gst_composite_alive (GstComposite * composite)
{
  g_return_if_fail (GST_IS_COMPOSITE (composite));

  if (composite->transition) {
#if 0
    g_timeout_add (10, (GSourceFunc) gst_composite_close_transition, composite);
#else
    gst_composite_close_transition (composite);
#endif
  } else if (composite->adjusting) {
    g_timeout_add (10, (GSourceFunc) gst_composite_close_adjustment, composite);
  }
}

/**
 * gst_composite_null:
 *
 * Invoked when the composite pipeline is going NULL.
 */
static GstWorkerNullReturn
gst_composite_null (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), GST_WORKER_NR_END);

  if (composite->transition) {
#if 0
    g_timeout_add (10, (GSourceFunc) gst_composite_commit_transition,
        composite);
#else
    gst_composite_commit_transition (composite);
#endif
  } else if (composite->adjusting) {
    g_timeout_add (10, (GSourceFunc) gst_composite_commit_adjustment,
        composite);
  }

  return composite->deprecated ? GST_WORKER_NR_END : GST_WORKER_NR_REPLAY;
}


/* gst_caps_get_value:
 * - the missing API
 * iterate the gst_structure in the given caps to return a specified field vaue
 */
static gboolean
gst_caps_get_value (GstCaps * caps, const gchar * field, GValue * value)
{
  guint i;
  GstStructure *structure;
  for (i = 0; (structure = gst_caps_get_structure (caps, i)) != NULL; i++) {
    const GValue *svalue = gst_structure_get_value (structure, field);
    if (svalue != NULL) {
      g_value_copy (svalue, value);
      return TRUE;
    }
  }
  return FALSE;
}

static gint
gst_composite_get_caps_int_value (const gchar * fieldname)
{
  GstCaps *caps = gst_switch_server_getcaps ();
  g_assert (caps != NULL && !gst_caps_is_empty (caps));
  GValue value = G_VALUE_INIT;
  g_value_init (&value, G_TYPE_INT);
  gst_caps_get_value (caps, fieldname, &value);
  return g_value_get_int (&value);
}

/* cache these so we don't need to do the above every time */
static gint cached_default_width = -1;
static gint cached_default_height = -1;

/* gst_composite_defaut_width:
 *
 *  @return default width based on current video caps 
 */
gint
gst_composite_default_width ()
{
  if (cached_default_width == -1)
    cached_default_width = gst_composite_get_caps_int_value ("width");
  return cached_default_width;
}

/* gst_composite_defaut_height:
 *
 *  @return default height based on current video caps 
 */
gint
gst_composite_default_height ()
{
  if (cached_default_height == -1)
    cached_default_height = gst_composite_get_caps_int_value ("height");
  return cached_default_height;
}

/* gst_composite_min_pip_width:
 *
 *  @param pip_w desired width of PIP
 *  @return enforce minimum width of PIP 
 */
gint
gst_check_composite_min_pip_width (gint pip_w)
{
  gint min_width = gst_composite_default_width () / 4;
  return pip_w < min_width ? min_width : pip_w;
}

/* gst_composite_min_pip_height:
 *
 *  @param pip_w desired height of PIP
 *  @return enforce minimum height of PIP 
 */
gint
gst_check_composite_min_pip_height (gint pip_h)
{
  gint min_height = gst_composite_default_height () / 3;
  return pip_h < min_height ? min_height : pip_h;
}

/**
 * gst_composite_adjust_pip:
 *  @param composite The GstComposite instance
 *  @param x the X position of the PIP
 *  @param y the Y position of the PIP
 *  @param w the width of the PIP
 *  @param h the height of the PIP
 *  @return PIP has been changed successfully 
 *
 *  Change the PIP position and size.
 */
gboolean
gst_composite_adjust_pip (GstComposite * composite, gint x, gint y,
    gint w, gint h)
{
  gboolean result = FALSE;
  GstIterator *iter = NULL;
  GValue value = { 0 };
  GstElement *element = NULL;
  gboolean done = FALSE;

  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  GST_COMPOSITE_LOCK (composite);
  if (composite->adjusting) {
    WARN ("last PIP adjustment request is progressing");
    goto end;
  }

  composite->b_x = x;
  composite->b_y = y;

  if (composite->b_width != w || composite->b_height != h) {
    composite->b_width = w;
    composite->b_height = h;
    composite->adjusting = TRUE;
    gst_worker_stop (GST_WORKER (composite));
    result = TRUE;
    goto end;
  }

  element = gst_worker_get_element (GST_WORKER (composite), "mix");
  iter = gst_element_iterate_sink_pads (element);
  while (iter && !done) {
    switch (gst_iterator_next (iter, &value)) {
      case GST_ITERATOR_OK:
      {
        GstPad *pad = g_value_get_object (&value);
        if (g_strcmp0 (gst_pad_get_name (pad), "sink_1") == 0) {
          g_object_set (pad, "xpos", composite->b_x,
              "ypos", composite->b_y, NULL);
          done = TRUE;
          result = TRUE;
        }
        g_value_reset (&value);
      }
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iter);
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
      default:
        /* iterator returned _ERROR or premature end with _OK,
         * mark an error and exit */
        done = TRUE;
        result = FALSE;
        break;
    }
  }

  if (G_IS_VALUE (&value))
    g_value_unset (&value);
  if (iter)
    gst_iterator_free (iter);

  composite->adjusting = FALSE;

  /*
     if (!result) {
     WARN ("failed to adjust PIP: %d, %d, %d, %d", x, y, w, h);
     }
   */

end:
  GST_COMPOSITE_UNLOCK (composite);
  return result;
}

/**
 * gst_composite_retry_transition:
 * @return Always FALSE to allow glib to cleanup the timeout source
 *
 * This is invoked when the pipeline's getting errors to retry the transition
 * request.
 */
static gboolean
gst_composite_retry_transition (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->transition) {
    GST_COMPOSITE_LOCK_TRANSITION (composite);
    if (composite->transition) {
      WARN ("new mode %d, %dx%d (error transition)",
          composite->mode, composite->width, composite->height);
      gst_composite_apply_parameters (composite);
      gst_worker_start (GST_WORKER (composite));
    }
    GST_COMPOSITE_UNLOCK_TRANSITION (composite);
  }

  return FALSE;
}

/**
 * gst_composite_retry_adjustment:
 * @return Always FALSE to allow glib to cleanup the timeout source
 *
 * This is invoked when the pipeline is reporting errors and requiring PIP
 * adjustment.
 */
static gboolean
gst_composite_retry_adjustment (GstComposite * composite)
{
  g_return_val_if_fail (GST_IS_COMPOSITE (composite), FALSE);

  if (composite->adjusting) {
    GST_COMPOSITE_LOCK_ADJUSTMENT (composite);
    if (composite->adjusting) {
      GstWorkerClass *worker_class;
      WARN ("adjusting PIP error, retry..");
      worker_class = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (composite));
      if (!worker_class->reset (GST_WORKER (composite))) {
        ERROR ("failed to reset composite");
      }
      gst_worker_start (GST_WORKER (composite));
    }
    GST_COMPOSITE_UNLOCK_ADJUSTMENT (composite);
  }

  return FALSE;
}

/**
 * gst_composite_error:
 *
 * Handling the composite pipeline errors.
 */
static void
gst_composite_error (GstComposite * composite)
{
  g_return_if_fail (GST_IS_COMPOSITE (composite));

  if (composite->transition) {
    g_timeout_add (10, (GSourceFunc) gst_composite_retry_transition, composite);
  } else if (composite->adjusting) {
    g_timeout_add (10, (GSourceFunc) gst_composite_retry_adjustment, composite);
  }
}

/**
 * @brief Pipeline message handling.
 *
 * Handle the composite pipeline messages. It's current only taking care
 * of GST_MESSAGE_ERROR.
 *
 * @see GstMessage
 */
static gboolean
gst_composite_message (GstComposite * composite, GstMessage * message)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      gst_composite_error (composite);
      break;
    default:
      break;
  }
  return TRUE;
}

/**
 * gst_composite_class_init:
 *
 * Initialize the GstCompositeClass.
 */
static void
gst_composite_class_init (GstCompositeClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstWorkerClass *worker_class = GST_WORKER_CLASS (klass);

  object_class->dispose = (GObjectFinalizeFunc) gst_composite_dispose;
  object_class->finalize = (GObjectFinalizeFunc) gst_composite_finalize;
  object_class->set_property =
      (GObjectSetPropertyFunc) gst_composite_set_property;
  object_class->get_property =
      (GObjectGetPropertyFunc) gst_composite_get_property;

  /*
     gst_composite_signals[SIGNAL_START_OUTPUT] = 
     g_signal_new ("start-output", G_TYPE_FROM_CLASS (klass),
     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstCompositeClass, start_output),
     NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);

     gst_composite_signals[SIGNAL_START_RECORDER] = 
     g_signal_new ("start-recorder", G_TYPE_FROM_CLASS (klass),
     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstCompositeClass, start_recorder),
     NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);

     gst_composite_signals[SIGNAL_END_OUTPUT] = 
     g_signal_new ("end-output", G_TYPE_FROM_CLASS (klass),
     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstCompositeClass, end_output),
     NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);

     gst_composite_signals[SIGNAL_END_RECORDER] = 
     g_signal_new ("end-recorder", G_TYPE_FROM_CLASS (klass),
     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstCompositeClass, end_recorder),
     NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);
   */

  gst_composite_signals[SIGNAL_END_TRANSITION] =
      g_signal_new ("end-transition", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstCompositeClass,
          end_transition), NULL,
      NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0 /*1, G_TYPE_INT */ );

  g_object_class_install_property (object_class, PROP_MODE,
      g_param_spec_uint ("mode", "Mode",
          "Composite Mode",
          COMPOSE_MODE_NONE,
          COMPOSE_MODE__LAST,
          DEFAULT_COMPOSE_MODE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PORT,
      g_param_spec_uint ("port", "Port",
          "Sink port",
          GST_SWITCH_MIN_SINK_PORT,
          GST_SWITCH_MAX_SINK_PORT,
          GST_SWITCH_MIN_SINK_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_ENCODE_PORT,
      g_param_spec_uint ("encode", "EncodePort",
          "Encoding Sink port",
          GST_SWITCH_MIN_SINK_PORT,
          GST_SWITCH_MAX_SINK_PORT,
          GST_SWITCH_MIN_SINK_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_A_X,
      g_param_spec_uint ("ax", "A xpos",
          "Channel A frame xpos",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_A_Y,
      g_param_spec_uint ("ay", "A ypos",
          "Channel A frame ypos",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_A_WIDTH,
      g_param_spec_uint ("awidth", "A Width",
          "Channel A frame width",
          1, G_MAXINT,
          gst_composite_default_width
          (), G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_A_HEIGHT,
      g_param_spec_uint ("aheight", "A Height",
          "Channel A frame height",
          1, G_MAXINT,
          gst_composite_default_height
          (), G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_B_X,
      g_param_spec_uint ("bx", "B xpos",
          "Channel B frame xpos",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_B_Y,
      g_param_spec_uint ("by", "B ypos",
          "Channel B frame ypos",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_B_WIDTH,
      g_param_spec_uint ("bwidth", "B Width",
          "Channel B frame width",
          1, G_MAXINT,
          gst_composite_default_width
          (), G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_B_HEIGHT,
      g_param_spec_uint ("bheight", "B Height",
          "Channel B frame height",
          1, G_MAXINT,
          gst_composite_default_height
          (), G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_WIDTH,
      g_param_spec_uint ("width",
          "Composite Width",
          "Output frame width", 1,
          G_MAXINT,
          gst_composite_default_width
          (), G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_HEIGHT,
      g_param_spec_uint ("height",
          "Composite Height",
          "Output frame height",
          1, G_MAXINT,
          gst_composite_default_height
          (), G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  worker_class->alive = (GstWorkerAliveFunc) gst_composite_alive;
  worker_class->null = (GstWorkerNullFunc) gst_composite_null;
  worker_class->prepare = (GstWorkerPrepareFunc) gst_composite_prepare;
  worker_class->start_worker = (GstWorkerAliveFunc) gst_composite_start;
  worker_class->end_worker = (GstWorkerAliveFunc) gst_composite_end;
  worker_class->message = (GstWorkerMessageFunc) gst_composite_message;
  worker_class->get_pipeline_string = (GstWorkerGetPipelineStringFunc)
      gst_composite_get_pipeline_string;
}
