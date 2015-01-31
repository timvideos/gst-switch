/* GstSwitch							    -*- c -*-
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

/*! @file */

#ifndef __GST_COMPOSITE_H__
#define __GST_COMPOSITE_H__

#include "gstworker.h"

#define GST_TYPE_COMPOSITE (gst_composite_get_type ())
#define GST_COMPOSITE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_COMPOSITE, GstComposite))
#define GST_COMPOSITE_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_COMPOSITE, GstCompositeClass))
#define GST_IS_COMPOSITE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_COMPOSITE))
#define GST_IS_COMPOSITE_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_COMPOSITE))

//#if ENABLE_LOW_RESOLUTION
//#define GST_SWITCH_COMPOSITE_DEFAULT_WIDTH    LOW_RES_W       /* 640 */
//#define GST_SWITCH_COMPOSITE_DEFAULT_HEIGHT   LOW_RES_H       /* 480 */
//#define GST_SWITCH_COMPOSITE_MIN_PIP_W                13
//#define GST_SWITCH_COMPOSITE_MIN_PIP_H                7
//#else
//#define GST_SWITCH_COMPOSITE_DEFAULT_WIDTH    1280
//#define GST_SWITCH_COMPOSITE_DEFAULT_HEIGHT   720
//#define GST_SWITCH_COMPOSITE_MIN_PIP_W                320
//#define GST_SWITCH_COMPOSITE_MIN_PIP_H                240
//#endif

#define GST_SWITCH_FACEDETECT_FRAME_WIDTH	150
#define GST_SWITCH_FACEDETECT_FRAME_HEIGHT	100

#define DEFAULT_COMPOSE_MODE COMPOSE_MODE_DUAL_EQUAL

/**
 *  @enum GstCompositeMode:
 */
typedef enum
{
  COMPOSE_MODE_NONE,            /*!< none */
  COMPOSE_MODE_PIP,             /*!< picture-in-picture */
  COMPOSE_MODE_DUAL_PREVIEW,    /*!< side-by-side (preview) */
  COMPOSE_MODE_DUAL_EQUAL,      /*!< side-by-side (equal) */
  COMPOSE_MODE__LAST = COMPOSE_MODE_DUAL_EQUAL
} GstCompositeMode;

inline static const char *
gst_composite_mode_to_string (GstCompositeMode mode)
{
  switch (mode) {
    case COMPOSE_MODE_NONE:
      return "COMPOSE_MODE_NONE";
    case COMPOSE_MODE_PIP:
      return "COMPOSE_MODE_PIP";
    case COMPOSE_MODE_DUAL_PREVIEW:
      return "COMPOSE_MODE_DUAL_PREVIEW";
    case COMPOSE_MODE_DUAL_EQUAL:
      return "COMPOSE_MODE_DUAL_EQUAL";
  }
  //ASSERT(false);
  return "COMPOSE_INVALID_VALUE";
}

typedef struct _GstComposite GstComposite;
typedef struct _GstCompositeClass GstCompositeClass;

/**
 *  @brief The GstComposite class.
 *  @param base the parent object
 *  @param mode the composite mode, @see GstCompositeMode
 *  @param lock lock for composite object
 *  @param transition_lock lock for transition of modes 
 *  @param adjustment_lock lock for PIP adjustment
 *  @param sink_port sink port number
 *  @param encode_sink_port encode port number
 *  @param a_x X position of A video
 *  @param a_y Y position of A video
 *  @param a_width width of A video
 *  @param a_height height of A video
 *  @param b_x X position of B video
 *  @param b_y Y position of B video
 *  @param b_width width of B video
 *  @param b_height height of B video
 *  @param width output width
 *  @param height output height
 *  @param adjusting the status of adjusting PIP
 *  @param transition the status of transiting modes
 *  @param deprecated (deprecated)
 *  @param scaler the scaler for A/B videos
 */
struct _GstComposite
{
  GstWorker base;

  GstCompositeMode mode;

  GMutex lock;
  GMutex transition_lock;
  GMutex adjustment_lock;

  gint sink_port;
  gint encode_sink_port;

  guint a_x;
  guint a_y;
  guint a_width;
  guint a_height;
  guint b_x;
  guint b_y;
  guint b_width;
  guint b_height;

  guint width;
  guint height;

  gboolean adjusting;
  gboolean transition;
  gboolean deprecated;

  GstWorker *scaler;
};

/**
 *  GstCompositeClass:
 *  @param base_class the parent class
 *  @param end_transition signal handler of "end-transition"
 */
struct _GstCompositeClass
{
  GstWorkerClass base_class;

  void (*end_transition) (GstComposite * composite);
};

GType gst_composite_get_type (void);
gboolean gst_composite_adjust_pip (GstComposite * composite,
    gint x, gint y, gint w, gint h);
gint gst_composite_default_width ();
gint gst_composite_default_height ();
gint gst_check_composite_min_pip_width (gint pip_w);
gint gst_check_composite_min_pip_height (gint pip_h);


#endif //__GST_COMPOSITE_H__
