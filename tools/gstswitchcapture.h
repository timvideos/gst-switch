/* GstSwitch							    -*- c -*-
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

#ifndef __GST_SWITCH_CAPTURE_H__
#define __GST_SWITCH_CAPTURE_H__

#include "gstworker.h"
#include "gstswitchclient.h"

#define GST_TYPE_SWITCH_CAPTURE (gst_switch_capture_get_type ())
#define GST_SWITCH_CAPTURE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_SWITCH_CAPTURE, GstSwitchCapture))
#define GST_SWITCH_CAPTURE_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_SWITCH_CAPTURE, GstSwitchCaptureClass))
#define GST_IS_SWITCH_CAPTURE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_SWITCH_CAPTURE))
#define GST_IS_SWITCH_CAPTURE_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_SWITCH_CAPTURE))

typedef struct _GstSwitchCaptureWorker GstSwitchCaptureWorker;

/**
 *  @class GstSwitchCapture
 *  @struct _GstSwitchCapture
 *  @brief The gst-switch-cap program.
 */
typedef struct _GstSwitchCapture
{
  GstSwitchClient base;         /*!< The base object */

  GMainLoop *mainloop;

  GstSwitchCaptureWorker *worker;
  GThread *pulse;
} GstSwitchCapture;

/**
 *  @class GstSwitchCaptureClass
 *  @struct _GstSwitchCaptureClass
 *  @brief The gst-switch-cap program.
 */
typedef struct _GstSwitchCaptureClass
{
  GstSwitchClientClass base_class;      /*!< The base class. */
} GstSwitchCaptureClass;

GType gst_switch_capture_get_type (void);

#endif //__GST_SWITCH_CAPTURE_H__
