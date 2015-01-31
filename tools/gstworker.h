/* GstSwitch							    -*- c -*-
 * Copyright (C) 2012 Duzy Chan <code@duzy.info>
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

/*! @file */

#ifndef __GST_WORKER_H__
#define __GST_WORKER_H__

#include <gst/gst.h>
#include "../logutils.h"

#define GST_TYPE_WORKER (gst_worker_get_type ())
#define GST_WORKER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_WORKER, GstWorker))
#define GST_WORKER_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_WORKER, GstWorkerClass))
#define GST_IS_WORKER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_WORKER))
#define GST_IS_WORKER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_WORKER))

typedef struct _GstWorker GstWorker;
typedef struct _GstWorkerClass GstWorkerClass;
typedef struct _GstSwitchServer GstSwitchServer;

/**
 * @enum GstWorkerNullReturn
 * 
 * Return values for worker-null signal handler.
 */
typedef enum
{
  GST_WORKER_NR_END,            /*!< The worker is end. */
  GST_WORKER_NR_REPLAY,         /*!< Try to replay the worker pipeline. */
} GstWorkerNullReturn;

/**
 *  @brief getting pipeline string function
 *  @param worker The GstWorker instance.
 *  @param data User defined data pointer.
 */
typedef GString *(*GstWorkerGetPipelineString) (GstWorker * worker,
    gpointer data);

/**
 *  @brief getting pipeline string callback function
 *  @param worker The GstWorker instance.
 */
typedef GString *(*GstWorkerGetPipelineStringFunc) (GstWorker * worker);

/**
 *  @brief worker preparation callback function
 *  @param worker The GstWorker instance.
 */
typedef gboolean (*GstWorkerPrepareFunc) (GstWorker * worker);

/**
 *  @brief worker message callback function
 *  @param worker The GstWorker instance.
 *  @param m the message
 */
typedef gboolean (*GstWorkerMessageFunc) (GstWorker * worker, GstMessage * m);

/**
 *  @brief worker null state callback function
 *  @param worker The GstWorker instance.
 */
typedef GstWorkerNullReturn (*GstWorkerNullFunc) (GstWorker * worker);

/**
 *  @brief worker alive callback function
 *  @param worker The GstWorker instance.
 */
typedef void (*GstWorkerAliveFunc) (GstWorker * worker);

/**
 *  @brief worker virtual close function
 *  @param worker The GstWorker instance.
 */

typedef void (*GstWorkerCloseFunc) (GstWorker * worker);

/**
 *  @class GstWorker
 *  @struct _GstWorker
 *  @brief A worker of pipelines.
 */
struct _GstWorker
{
  GObject base;                 /*!< The parent object. */

  gchar *name;                  /*!< The name of the worker. */

  //GstSwitchServer *server; /*!<  */

  GMutex pipeline_lock;         /*!< Mutex for %pipeline */
  GCond shutdown_cond;          /*!< Cond for shutting down pipelines cleanly */
  GstElement *pipeline;         /*!< The pipeline. */
  GstBus *bus;                  /*!< The pipeline bus. */

  GstWorkerGetPipelineString pipeline_func;     /*!< Pipeline string function. */
  gpointer pipeline_func_data;  /*!< Caller defined data for %pipeline_func. */
  GString *pipeline_string;     /*!< The pipeline string of the worker. */

  gboolean auto_replay;         /*!< The worker should replay if it's TRUE */
  gboolean paused_for_buffering;        /*!< Mark for buffering pause. */
  guint watch;                  /*!< The watch number of the pipeline bus. */

  /*!< TRUE if the recording pipeline needs clean shut-down
   * via an EOS event to finish up before stopping
   */
  gboolean send_eos_on_stop;
};

/**
 *  @class GstWorkerClass
 *  @struct _GstWorkerClass
 *  @brief The class of GstWorker.
 */
struct _GstWorkerClass
{
  GObjectClass base_class;      /*!< the parent class */

  /**
   *  @brief Signal handler when "prepare-worker" emitted.
   *  @param worker The GstWorker instance.
   */
  void (*prepare_worker) (GstWorker * worker);

  /**
   *  @brief Signal handler when "start-worker" emitted.
   *  @param worker The GstWorker instance.
   */
  void (*start_worker) (GstWorker * worker);

  /**
   *  @brief Signal handler when "end-worker" emitted.
   *  @param worker The GstWorker instance.
   */
  void (*end_worker) (GstWorker * worker);

  /**
   *  @brief Signal handler when "worker-null" emitted.
   *  @param worker The GstWorker instance.
   */
  void (*worker_null) (GstWorker * worker);

  /**
   *  @brief virtual function called when "missing plugin" discovered.
   *  @param worker The GstWorker instance.
   *  @param elements The name of the missing elements.
   */
    gboolean (*missing) (GstWorker * worker, gchar ** elements);

  /**
   *  @brief virtual function called on per message.
   *  @param worker The GstWorker instance.
   *  @param message The message to be handled.
   */
    gboolean (*message) (GstWorker * worker, GstMessage * message);

  /**
   *  @brief Callback function for getting the pipeline
   *         string for the worker.
   *  @param worker The GstWorker instance.
   */
  GString *(*get_pipeline_string) (GstWorker * worker);

  /**
   *  @brief Virtual function called when new pipeline is requested for
   *         creation.
   *  @param worker The GstWorker instance.
   */
  GstElement *(*create_pipeline) (GstWorker * worker);

  /**
   *  @brief Virtual function called when the worker is prepared.
   *  @param worker The GstWorker instance.
   */
    gboolean (*prepare) (GstWorker * worker);

  /**
   *  @brief Virtual function called when the pipeline is online.
   *  @param worker The GstWorker instance.
   */
  void (*alive) (GstWorker * worker);

  /**
   *  @brief Virtual function called when the worker is getting null.
   *  @param worker The GstWorker instance.
   */
    GstWorkerNullReturn (*null) (GstWorker * worker);

  /**
   *  @brief Reset reset the worker's pipeline.
   *  @param worker The GstWorker instance.
   */
    gboolean (*reset) (GstWorker * worker);

  /*
   * @brief Close the worker, deallocating and closing resources
   * @param worker The GstWorker instance.
   */

  void (*close) (GstWorker * worker);
};

/**
 * @internal Use GST_TYPE_WORKER instead.
 * @see GST_TYPE_WORKER
 */
GType gst_worker_get_type (void);

/**
 *  @param worker The GstWorker instance.
 *
 *  Start the worker. This will call the derived create_pipeline and the
 *  virtual "prepare" function.
 *
 *  @return TRUE if worker prepared and started.
 *  @memberof GstWorker
 */
gboolean gst_worker_start (GstWorker * worker);

/**
 *  @param worker The GstWorker instance.
 *  @param force Force stopping the pipeline if TRUE.
 *
 *  Stop the pipeline, Pass TRUE to the second argument to make it force stop.
 *
 *  @return TRUE if stop request sent.
 *  @memberof GstWorker
 */
gboolean gst_worker_stop_force (GstWorker * worker, gboolean force);

/**
 * @param worker the GstWorker instance
 *
 * Same as gst_worker_stop_force (worker, FALSE).
 *
 * @return TRUE if stop request sent.
 * @memberof GstWorker
 */
#define gst_worker_stop(worker) (gst_worker_stop_force ((worker), FALSE))

/**
 *  @brief Get element by name.
 *  @param worker The GstWorker instance.
 *  @param name The name of the element.
 *
 *  Get element by name without locking the pipeline.
 *
 *  Not MT safe.
 *
 *  @return the element of the name or NULL if not found.
 *  @memberof GstWorker
 *  @see gst_worker_get_element
 */
GstElement *gst_worker_get_element_unlocked (GstWorker * worker,
    const gchar * name);

/**
 *  @param worker The GstWorker instance.
 *  @param name The name of the element.
 *
 *  Get element by name with locking the pipeline.
 *
 *  MT safe.
 *
 *  @return the element of the name or NULL if not found.
 *  @memberof GstWorker
 *  @see gst_worker_get_element_unlocked
 */
GstElement *gst_worker_get_element (GstWorker * worker, const gchar * name);

#endif //__GST_WORKER_H__
