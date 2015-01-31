/* GstSwitchSrv
 * Copyright (C) 2012 Duzy Chan <code@duzy.info>
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
 *
 */

/*! @file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstworker.h"
#include "gstswitchserver.h"

#include <string.h>

#define GST_WORKER_LOCK_PIPELINE(srv) (g_mutex_lock (&(srv)->pipeline_lock))
#define GST_WORKER_UNLOCK_PIPELINE(srv) (g_mutex_unlock (&(srv)->pipeline_lock))

enum
{
  PROP_0,
  PROP_NAME,
};

enum
{
  SIGNAL_PREPARE_WORKER,
  SIGNAL_START_WORKER,
  SIGNAL_END_WORKER,
  SIGNAL_WORKER_NULL,
  SIGNAL__LAST,                 /*!< @internal */
};

/*!< @internal */
static guint gst_worker_signals[SIGNAL__LAST] = { 0 };

extern gboolean verbose;

#if ENABLE_ASSESSMENT
guint assess_number = 0;
#endif //ENABLE_ASSESSMENT

/*!< @internal */
#define gst_worker_parent_class parent_class

/*!< @internal */
G_DEFINE_TYPE (GstWorker, gst_worker, G_TYPE_OBJECT);

/**
 * @brief Initialize GstWorker instances.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
static void
gst_worker_init (GstWorker * worker)
{
  worker->name = NULL;
  //worker->server = NULL;
  worker->bus = NULL;
  worker->pipeline = NULL;
  worker->pipeline_func = NULL;
  worker->pipeline_func_data = NULL;
  worker->pipeline_string = NULL;
  worker->paused_for_buffering = FALSE;
  worker->watch = 0;

  g_mutex_init (&worker->pipeline_lock);
  g_cond_init (&worker->shutdown_cond);

  //INFO ("gst_worker init %p", worker);
}

/**
 * @brief Free GstWorker instances.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
static void
gst_worker_dispose (GstWorker * worker)
{
  //INFO ("gst_worker dispose %p", worker);
  if (worker->pipeline) {
    gst_element_set_state (worker->pipeline, GST_STATE_NULL);
  }
  if (worker->bus) {
    gst_bus_set_flushing (worker->bus, TRUE);
  }

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (worker));
}

/**
 * @brief Destroy GstWorker instances.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
static void
gst_worker_finalize (GstWorker * worker)
{
  if (worker->watch) {
    g_source_remove (worker->watch);
    worker->watch = 0;
  }
  if (worker->pipeline) {
    INFO ("pipeline ref %d", GST_OBJECT_REFCOUNT (worker->pipeline));
    gst_object_unref (worker->pipeline);
    worker->pipeline = NULL;
  }
  if (worker->bus) {
    INFO ("bus ref %d", GST_OBJECT_REFCOUNT (worker->bus));
    gst_object_unref (worker->bus);
    worker->bus = NULL;
  }

  /*
     if (worker->server) {
     g_object_unref (worker->server);
     worker->server = NULL;
     }
   */

  if (worker->pipeline_string) {
    g_string_free (worker->pipeline_string, TRUE);
    worker->pipeline_string = NULL;
  }


  INFO ("gst_worker finalize %p", worker);
  g_mutex_clear (&worker->pipeline_lock);
  g_cond_clear (&worker->shutdown_cond);

  g_free (worker->name);
  worker->name = NULL;

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (worker));
}

/**
 * @brief Set GstWorker properties.
 * @param worker The GstWorker instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstWorker
 */
static void
gst_worker_set_property (GstWorker * worker, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_NAME:
    {
      if (worker->name)
        g_free (worker->name);
      worker->name = g_strdup (g_value_get_string (value));
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (worker), property_id, pspec);
      break;
  }
}

/**
 * @brief Get GstWorker properties.
 * @param worker The GstWorker instance.
 * @param property_id
 * @param value
 * @param pspec
 * @memberof GstWorker
 */
static void
gst_worker_get_property (GstWorker * worker, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    case PROP_NAME:
    {
      g_value_set_string (value, worker->name);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (worker), property_id, pspec);
      break;
  }
}

/**
 * @brief Get GstWorker pipeline string.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
static GString *
gst_worker_get_pipeline_string (GstWorker * worker)
{
  GString *desc = NULL;
  if (worker->pipeline_func)
    desc = worker->pipeline_func (worker, worker->pipeline_func_data);
  if (!desc && worker->pipeline_string)
    desc = g_string_new (worker->pipeline_string->str);
  if (!desc)
    desc = g_string_new ("");
  return desc;
}

/**
 * @brief Create GstWorker pipeline.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
static GstElement *
gst_worker_create_pipeline (GstWorker * worker)
{
  GstWorkerClass *workerclass = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker));

  GString *desc = NULL;
  GstElement *pipeline = NULL;
  GError *error = NULL;
  GstParseContext *context = NULL;
  gint parse_flags = GST_PARSE_FLAG_NONE;
  parse_flags |= GST_PARSE_FLAG_FATAL_ERRORS;

create_pipeline:
  desc = workerclass->get_pipeline_string (worker);
  context = gst_parse_context_new ();

  if (verbose) {
    g_print ("%s: %s\n", worker->name, desc->str);
  }

  pipeline = (GstElement *) gst_parse_launch_full (desc->str, context,
      parse_flags, &error);
  g_string_free (desc, TRUE);

  if (error == NULL) {
    goto end;
  }

  if (pipeline) {
    gchar *name = g_strdup_printf ("%s-pipeline", worker->name);
    //g_object_set (G_OBJECT (pipeline), "name", name, NULL);
    gst_element_set_name (pipeline, name);
    g_free (name);
  }

  if (g_error_matches (error, GST_PARSE_ERROR, GST_PARSE_ERROR_NO_SUCH_ELEMENT)) {
    gchar **name = NULL;
    gchar **names = gst_parse_context_get_missing_elements (context);
    gboolean retry = workerclass->missing &&
        (*workerclass->missing) (worker, names);
    for (name = names; *name; ++name)
      ERROR ("missing: %s", *name);
    g_strfreev (names);

    if (retry) {
      gst_parse_context_free (context);
      context = NULL;

      g_assert (GST_OBJECT_REFCOUNT (pipeline) == 1);
      gst_object_unref (pipeline);
      pipeline = NULL;

      goto create_pipeline;
    }
  } else {
    ERROR ("%s: pipeline parsing error: %s", worker->name, error->message);
  }

  if (pipeline) {
    g_assert (GST_OBJECT_REFCOUNT (pipeline) == 1);
    gst_object_unref (pipeline);
    pipeline = NULL;
  }

end:
  gst_parse_context_free (context);
  return pipeline;
}

/**
 * @brief Handler of the pipeline null message.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
static GstWorkerNullReturn
gst_worker_null (GstWorker * worker)
{
  return (GST_IS_WORKER (worker) && worker->auto_replay) ?
      GST_WORKER_NR_END : GST_WORKER_NR_END;
}

static gboolean gst_worker_prepare (GstWorker *);

/**
 * @brief Start the worker pipeline.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
gboolean
gst_worker_start (GstWorker * worker)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  g_return_val_if_fail (GST_IS_WORKER (worker), FALSE);

  if (gst_worker_prepare (worker)) {
    GST_WORKER_LOCK_PIPELINE (worker);
    ret = gst_element_set_state (worker->pipeline, GST_STATE_READY);
    GST_WORKER_UNLOCK_PIPELINE (worker);
  }

  return ret == GST_STATE_CHANGE_SUCCESS ? TRUE : FALSE;
}

/**
 * @brief Restart the worker pipeline.
 * @param worker The GstWorker instance.
 * @memberof GstWorker
 */
static gboolean
gst_worker_replay (GstWorker * worker)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  g_return_val_if_fail (GST_IS_WORKER (worker), FALSE);

  GST_WORKER_LOCK_PIPELINE (worker);

  if (worker->pipeline) {
    GstState state;

    ret = gst_element_get_state (worker->pipeline, &state, NULL,
        GST_CLOCK_TIME_NONE);

    if (state != GST_STATE_PLAYING) {
      ret = gst_element_set_state (worker->pipeline, GST_STATE_READY);
    }
  }

  GST_WORKER_UNLOCK_PIPELINE (worker);

  return ret == GST_STATE_CHANGE_SUCCESS ? TRUE : FALSE;
}

static void gst_worker_state_ready_to_null (GstWorker *);
static gboolean
gst_worker_state_ready_to_null_proxy (GstWorker * worker)
{
  gst_worker_state_ready_to_null (worker);
  return FALSE;
}

/**
 * @brief Stop the worker pipeline.
 * @param worker The GstWorker instance.
 * @param force Force stop if TRUE.
 * @memberof GstWorker
 */
gboolean
gst_worker_stop_force (GstWorker * worker, gboolean force)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;
  gboolean no_eos;

  g_return_val_if_fail (GST_IS_WORKER (worker), FALSE);

  GST_WORKER_LOCK_PIPELINE (worker);

  if (worker->pipeline) {
    GstState state;

    ret = gst_element_get_state (worker->pipeline, &state, NULL,
        GST_CLOCK_TIME_NONE);
    no_eos = (state == GST_STATE_PLAYING) && !worker->send_eos_on_stop;

    if (force || no_eos) {
      ret = gst_element_set_state (worker->pipeline, GST_STATE_NULL);

      gst_bus_set_flushing (worker->bus, TRUE);

      g_timeout_add (5,
          (GSourceFunc) gst_worker_state_ready_to_null_proxy, worker);
    } else if (state == GST_STATE_PLAYING) {
      /* Send an EOS to cleanly shutdown */
      gst_element_send_event (worker->pipeline, gst_event_new_eos ());

      /* Go to sleep until the EOS handler calls stop_force (worker, TRUE); */
      g_cond_wait (&worker->shutdown_cond, &worker->pipeline_lock);
    }
  }

  GST_WORKER_UNLOCK_PIPELINE (worker);

  return ret == GST_STATE_CHANGE_SUCCESS ? TRUE : FALSE;
}

/**
 * @memberof GstWorker
 */
GstElement *
gst_worker_get_element_unlocked (GstWorker * worker, const gchar * name)
{
  g_return_val_if_fail (GST_IS_WORKER (worker), NULL);

  return gst_bin_get_by_name (GST_BIN (worker->pipeline), name);
}

/**
 */
GstElement *
gst_worker_get_element (GstWorker * worker, const gchar * name)
{
  GstElement *element = NULL;

  GST_WORKER_LOCK_PIPELINE (worker);
  element = gst_worker_get_element_unlocked (worker, name);
  GST_WORKER_UNLOCK_PIPELINE (worker);
  return element;
}

/*
static void
gst_worker_missing_plugin (GstWorker *worker, GstStructure *structure)
{
  GstWorkerClass *workerclass = GST_WORKER_CLASS (
      G_OBJECT_GET_CLASS (worker));

  ERROR ("missing plugin");

  if (workerclass->missing_plugin)
    (*workerclass->missing_plugin) (worker, structure);
}
*/

static void
gst_worker_handle_eos (GstWorker * worker)
{
  gst_worker_stop_force (worker, TRUE);
  GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker))->close (worker);
}

static void
gst_worker_handle_error (GstWorker * worker, GError * error, const char *debug)
{
  //ERROR ("%s: %s", worker->name, error->message);

  if (error->domain == GST_CORE_ERROR) {
    ERROR ("%s: (CORE: %d) %s", worker->name, error->code, error->message);
    switch (error->code) {
      case GST_CORE_ERROR_MISSING_PLUGIN:
        ERROR ("missing plugin..");
        break;
      case GST_CORE_ERROR_NEGOTIATION:
        ERROR ("%s: negotiation: %s", worker->name, error->message);
        break;
    }
  }

  if (error->domain == GST_LIBRARY_ERROR) {
    ERROR ("%s: (LIBRARY: %d) %s", worker->name, error->code, error->message);
  }

  if (error->domain == GST_RESOURCE_ERROR) {
    ERROR ("%s: (RESOURCE: %d) %s", worker->name, error->code, error->message);
  }

  if (error->domain == GST_STREAM_ERROR) {
    ERROR ("%s: (STREAM: %d) %s", worker->name, error->code, error->message);
  }
  ERROR ("DEBUG INFO:\n%s\n", debug);

  gst_worker_stop (worker);
  GstWorkerClass *worker_class = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker));
  worker_class->close (worker);
}

static void
gst_worker_handle_warning (GstWorker * worker, GError * error,
    const char *debug)
{
  // kludge: some gstreamer "warnings" are apparently non-recoverable errors
  if (strstr (error->message, "error:") != NULL)
    gst_worker_handle_error (worker, error, debug);
  else
    WARN ("%s: %s (%s)", worker->name, error->message, debug);
}

static void
gst_worker_handle_info (GstWorker * worker, GError * error, const char *debug)
{
  INFO ("%s: %s (%s)", worker->name, error->message, debug);
}

static void
gst_worker_state_null_to_ready (GstWorker * worker)
{
  g_return_if_fail (GST_IS_WORKER (worker));

  gst_element_set_state (worker->pipeline, GST_STATE_PAUSED);
}

static void
gst_worker_state_ready_to_paused (GstWorker * worker)
{
  g_return_if_fail (GST_IS_WORKER (worker));

  if (!worker->paused_for_buffering) {
    gst_element_set_state (worker->pipeline, GST_STATE_PLAYING);
  }
}

static void
gst_worker_state_paused_to_playing (GstWorker * worker)
{
  GstWorkerClass *workerclass;

  g_return_if_fail (GST_IS_WORKER (worker));

  workerclass = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker));
  if (workerclass->alive) {
    (*workerclass->alive) (worker);
  }

  g_signal_emit (worker, gst_worker_signals[SIGNAL_START_WORKER], 0);
}

static void
gst_worker_state_playing_to_paused (GstWorker * worker)
{
}

static void
gst_worker_state_paused_to_ready (GstWorker * worker)
{
}

static void
gst_worker_state_ready_to_null (GstWorker * worker)
{
  GstWorkerClass *workerclass;
  GstWorkerNullReturn ret = GST_WORKER_NR_END;

  //INFO ("%s", __FUNCTION__);

  g_return_if_fail (GST_IS_WORKER (worker));

  workerclass = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker));
  if (workerclass->null) {
    switch ((ret = (*workerclass->null) (worker))) {
      case GST_WORKER_NR_REPLAY:
        gst_worker_replay (worker);
        break;
      case GST_WORKER_NR_END:
        break;
    }
  }

  g_signal_emit (worker, gst_worker_signals[SIGNAL_WORKER_NULL], 0);

  if (ret == GST_WORKER_NR_END)
    g_signal_emit (worker, gst_worker_signals[SIGNAL_END_WORKER], 0);
}

static gboolean
gst_worker_pipeline_state_changed (GstWorker * worker,
    GstStateChange statechange)
{
  switch (statechange) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      gst_worker_state_null_to_ready (worker);
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_worker_state_ready_to_paused (worker);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      gst_worker_state_paused_to_playing (worker);
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      gst_worker_state_playing_to_paused (worker);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_worker_state_paused_to_ready (worker);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
#if 0
      gst_worker_state_ready_to_null (worker);
#endif
      break;
    default:
      return FALSE;
  }
  return TRUE;
}

static GstBusSyncReply
gst_worker_message_sync (GstBus * bus, GstMessage * message, GstWorker * worker)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      /* When we see EOS, wake up and gst_worker_stop that might be waiting */
      g_cond_signal (&worker->shutdown_cond);
      break;
    default:
      break;
  }

  return GST_BUS_PASS;
}

static gboolean
gst_worker_message (GstBus * bus, GstMessage * message, GstWorker * worker)
{
  GstWorkerClass *workerclass;

  /* The event source should be removed if not worker ! */
  g_return_val_if_fail (GST_IS_WORKER (worker), FALSE);

  workerclass = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker));

  //INFO ("%s: %s", __FUNCTION__, GST_MESSAGE_TYPE_NAME (message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      gst_worker_handle_eos (worker);
      break;
    case GST_MESSAGE_ERROR:
    {
      GError *error = NULL;
      gchar *debug;
      gst_message_parse_error (message, &error, &debug);
      gst_worker_handle_error (worker, error, debug);
    }
      break;
    case GST_MESSAGE_WARNING:
    {
      GError *error = NULL;
      gchar *debug;
      gst_message_parse_warning (message, &error, &debug);
      gst_worker_handle_warning (worker, error, debug);
    }
      break;
    case GST_MESSAGE_INFO:
    {
      GError *error = NULL;
      gchar *debug;
      gst_message_parse_info (message, &error, &debug);
      gst_worker_handle_info (worker, error, debug);
    }
      break;
    case GST_MESSAGE_TAG:
    {
      /*
         GstTagList *tag_list;

         gst_message_parse_tag (message, &tag_list);

         //if (verbose)
         //  g_print ("tag\n");

         gst_tag_list_unref (tag_list);
       */
    }
      break;
    case GST_MESSAGE_STATE_CHANGED:
    {
      gboolean ret;
      GstState oldstate, newstate, pending;
      gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
      if (GST_ELEMENT (message->src) == worker->pipeline) {
        /*
           if (verbose) {
           g_print ("%s: state change from %s to %s\n", worker->name,
           gst_element_state_get_name (oldstate),
           gst_element_state_get_name (newstate));
           }
         */
        /*
           INFO ("%s: %s to %s", worker->name,
           gst_element_state_get_name (oldstate),
           gst_element_state_get_name (newstate));
         */

        ret = gst_worker_pipeline_state_changed (worker,
            GST_STATE_TRANSITION (oldstate, newstate));

        if (!ret /*&& verbose */ ) {
          WARN ("%s: UNKNOWN state change from %s to %s\n",
              worker->name, gst_element_state_get_name (oldstate),
              gst_element_state_get_name (newstate));
        }
      }
    }
      break;
    case GST_MESSAGE_BUFFERING:
    {
      int percent;
      gst_message_parse_buffering (message, &percent);
      //g_print("buffering %d\n", percent);
      if (!worker->paused_for_buffering && percent < 100) {
        g_print ("pausing for buffing\n");
        worker->paused_for_buffering = TRUE;
        gst_element_set_state (worker->pipeline, GST_STATE_PAUSED);
      } else if (worker->paused_for_buffering && percent == 100) {
        g_print ("unpausing for buffing\n");
        worker->paused_for_buffering = FALSE;
        gst_element_set_state (worker->pipeline, GST_STATE_PLAYING);
      }
    }
      break;
    case GST_MESSAGE_ELEMENT:
    case GST_MESSAGE_STATE_DIRTY:
    case GST_MESSAGE_CLOCK_PROVIDE:
    case GST_MESSAGE_CLOCK_LOST:
    case GST_MESSAGE_NEW_CLOCK:
    case GST_MESSAGE_STRUCTURE_CHANGE:
    case GST_MESSAGE_STREAM_STATUS:
      break;
    case GST_MESSAGE_STEP_DONE:
    case GST_MESSAGE_APPLICATION:
    case GST_MESSAGE_SEGMENT_START:
    case GST_MESSAGE_SEGMENT_DONE:
    case GST_MESSAGE_LATENCY:
    case GST_MESSAGE_ASYNC_START:
    case GST_MESSAGE_ASYNC_DONE:
    case GST_MESSAGE_REQUEST_STATE:
    case GST_MESSAGE_STEP_START:
    case GST_MESSAGE_QOS:
    default:
      if (verbose) {
        //g_print ("message: %s\n", GST_MESSAGE_TYPE_NAME (message));
      }
      break;
  }

  return workerclass->message ? workerclass->message (worker, message) : TRUE;
}

static gboolean
gst_worker_prepare_unsafe (GstWorker * worker)
{
  GstWorkerClass *workerclass;

  g_return_val_if_fail (worker, FALSE);

  workerclass = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker));
  if (!workerclass->create_pipeline)
    goto error_create_pipeline_not_installed;

  //GST_WORKER_LOCK_PIPELINE (worker);
  if (worker->pipeline)
    goto end;

  worker->pipeline = workerclass->create_pipeline (worker);
  if (!worker->pipeline)
    goto error_create_pipeline;

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (worker->pipeline), FALSE);

  worker->bus = gst_pipeline_get_bus (GST_PIPELINE (worker->pipeline));
  if (!worker->bus)
    goto error_get_bus;

  worker->watch = gst_bus_add_watch (worker->bus,
      (GstBusFunc) gst_worker_message, worker);

  if (!worker->watch)
    goto error_add_watch;

  gst_bus_set_sync_handler (worker->bus,
      (GstBusSyncHandler) (gst_worker_message_sync), worker, NULL);

  if (workerclass->prepare && !workerclass->prepare (worker))
    goto error_prepare;

  g_signal_emit (worker, gst_worker_signals[SIGNAL_PREPARE_WORKER], 0);

end:
  //GST_WORKER_UNLOCK_PIPELINE (worker);
  return TRUE;

  /* Errors Handling */

error_create_pipeline_not_installed:
  {
    ERROR ("%s: create_pipeline was not installed", worker->name);
    return FALSE;
  }

error_create_pipeline:
  {
    ERROR ("%s: failed to create new pipeline", worker->name);
    //GST_WORKER_UNLOCK_PIPELINE (worker);
    return FALSE;
  }

error_prepare:
  {
    g_source_remove (worker->watch);
    worker->watch = 0;
  error_add_watch:
    g_assert (GST_OBJECT_REFCOUNT (worker->bus) == 1);
    gst_object_unref (worker->bus);
    worker->bus = NULL;
  error_get_bus:
    g_assert (GST_OBJECT_REFCOUNT (worker->pipeline) == 1);
    gst_object_unref (worker->pipeline);
    worker->pipeline = NULL;

    ERROR ("%s: failed to prepare", worker->name);
    //GST_WORKER_UNLOCK_PIPELINE (worker);
    return FALSE;
  }
}

static gboolean
gst_worker_prepare (GstWorker * worker)
{
  gboolean ok = FALSE;
  if (worker->pipeline == NULL) {
    GST_WORKER_LOCK_PIPELINE (worker);
    ok = gst_worker_prepare_unsafe (worker);
    GST_WORKER_UNLOCK_PIPELINE (worker);
  } else {
    ok = TRUE;
  }
  return ok;
}

static gboolean
gst_worker_reset (GstWorker * worker)
{
  gboolean ok = FALSE;

  g_return_val_if_fail (GST_IS_WORKER (worker), FALSE);

  //INFO ("%s", __FUNCTION__);

#if 1
  if (worker) {
    GST_WORKER_LOCK_PIPELINE (worker);
    if (worker->pipeline) {
      gst_element_set_state (worker->pipeline, GST_STATE_NULL);
    }
    if (worker->bus) {
      gst_bus_set_flushing (worker->bus, TRUE);
    }

    if (worker->watch) {
      g_source_remove (worker->watch);
      worker->watch = 0;
    }
    if (worker->pipeline) {
      if (1 < GST_OBJECT_REFCOUNT (worker->pipeline)) {
        WARN ("possible pipeline leaks: %d",
            GST_OBJECT_REFCOUNT (worker->pipeline));
      }
      gst_object_unref (worker->pipeline);
      worker->pipeline = NULL;
    }
    if (worker->bus) {
      if (1 < GST_OBJECT_REFCOUNT (worker->bus)) {
        WARN ("possible bus leaks: %d", GST_OBJECT_REFCOUNT (worker->bus));
      }
      gst_object_unref (worker->bus);
      worker->bus = NULL;
    }
    ok = gst_worker_prepare_unsafe (worker);
    GST_WORKER_UNLOCK_PIPELINE (worker);
  }
#else
  ok = TRUE;
#endif

  return ok;
}

static void
gst_worker_close (GstWorker * worker)
{

}

/**
 * @brief Initialize GstWorkerClass.
 * @param klass The instance of GstWorkerClass.
 * @memberof GstWorkerClass
 */
static void
gst_worker_class_init (GstWorkerClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = (GObjectFinalizeFunc) gst_worker_dispose;
  object_class->finalize = (GObjectFinalizeFunc) gst_worker_finalize;
  object_class->set_property = (GObjectSetPropertyFunc) gst_worker_set_property;
  object_class->get_property = (GObjectGetPropertyFunc) gst_worker_get_property;

  gst_worker_signals[SIGNAL_PREPARE_WORKER] =
      g_signal_new ("prepare-worker", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstWorkerClass,
          prepare_worker), NULL,
      NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);

  gst_worker_signals[SIGNAL_START_WORKER] =
      g_signal_new ("start-worker", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstWorkerClass,
          start_worker), NULL,
      NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);

  gst_worker_signals[SIGNAL_END_WORKER] =
      g_signal_new ("end-worker", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstWorkerClass,
          end_worker), NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);

  gst_worker_signals[SIGNAL_WORKER_NULL] =
      g_signal_new ("worker-null", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstWorkerClass,
          worker_null), NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", "Name",
          "Name of the case", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  klass->get_pipeline_string = gst_worker_get_pipeline_string;
  klass->create_pipeline = gst_worker_create_pipeline;
  klass->null = gst_worker_null;
  klass->reset = gst_worker_reset;
  klass->close = gst_worker_close;
}
