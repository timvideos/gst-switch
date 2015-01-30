/* GstSwitch							    -*- c -*-
 * Copyright (C) 2013 Duzy Chan <code@duzy.info>
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
