/* gst-switch							    -*- c -*-
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

#ifndef __GST_RECORDER_H__
#define __GST_RECORDER_H__

#include "gstworker.h"
#include "gstcomposite.h"
#include <gio/gio.h>

#define GST_TYPE_RECORDER (gst_recorder_get_type ())
#define GST_RECORDER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_RECORDER, GstRecorder))
#define GST_RECORDER_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_RECORDER, GstRecorderClass))
#define GST_IS_RECORDER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_RECORDER))
#define GST_IS_RECORDER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_RECORDER))

typedef struct _GstRecorderClass GstRecorderClass;

/**
 *  @class GstRecorder
 *  @struct _GstRecorder
 *  @brief Recorder for recording composite result.
 */
struct _GstRecorder
{
  GstWorker base;               /*!< the parent object */

  gint sink_port;               /*!< the encode sink port */
  guint width;                  /*!< the video width */
  guint height;                 /*!< the video height */

  GstCompositeMode mode;        /*!< the composite mode which is the same as in GstComposite */
};

/**
 *  @class GstRecorderClass
 *  @struct _GstRecorderClass
 *  @brief The class of GstRecorder.
 */
struct _GstRecorderClass
{
  GstWorkerClass base_class;    /*!< The base class. */
};

/**
 *  @internal Use GST_TYPE_RECORDER instead.
 *  @see GST_TYPE_RECORDER
 */
GType gst_recorder_get_type (void);

#endif //__GST_RECORDER_H__
