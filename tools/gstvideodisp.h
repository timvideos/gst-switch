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

#ifndef __GST_VIDEO_DISP_H__
#define __GST_VIDEO_DISP_H__

#include "gstworker.h"

#define GST_TYPE_VIDEO_DISP (gst_video_disp_get_type ())
#define GST_VIDEO_DISP(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_VIDEO_DISP, GstVideoDisp))
#define GST_VIDEO_DISP_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_VIDEO_DISP, GstVideoDispClass))
#define GST_IS_VIDEO_DISP(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_VIDEO_DISP))
#define GST_IS_VIDEO_DISP_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_VIDEO_DISP))

typedef struct _GstVideoDisp GstVideoDisp;
typedef struct _GstVideoDispClass GstVideoDispClass;

/**
 *  @class GstVideoDisp
 *  @struct _GstVideoDisp
 *  @brief Display video on UI.
 */
struct _GstVideoDisp
{
  GstWorker base;               /*!< The parent object. */
  gint port;                    /*!< The port number. */
  gint type;                    /*!< The video type. */
  gulong handle;                /*!< The X Window handle for displaying the video. */
};

/**
 *  @class GstVideoDispClass
 *  @struct _GstVideoDispClass
 *  @brief The class of GstVideoDisp.
 */
struct _GstVideoDispClass
{
  GstWorkerClass base_class;    /*!< The base class. */
};

GType gst_video_disp_get_type (void);

#endif //__GST_VIDEO_DISP_H__
