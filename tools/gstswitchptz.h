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

#ifndef __GST_SWITCH_PTZ_H__
#define __GST_SWITCH_PTZ_H__

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gstworker.h"

#define GST_TYPE_SWITCH_PTZ (gst_switch_ptz_get_type ())
#define GST_SWITCH_PTZ(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_SWITCH_PTZ, GstSwitchPTZ))
#define GST_SWITCH_PTZ_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_SWITCH_PTZ, GstSwitchPTZClass))
#define GST_IS_SWITCH_PTZ(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_SWITCH_PTZ))
#define GST_IS_SWITCH_PTZ_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_SWITCH_PTZ))

typedef struct _GstSwitchPTZ
{
  GstWorker base;

  GtkWidget *window;
  GtkWidget *video_view;
  GtkAdjustment *adjust_pan_speed;
  GtkAdjustment *adjust_pan;
  GtkAdjustment *adjust_tilt_speed;
  GtkAdjustment *adjust_tilt;
  GtkAdjustment *adjust_zoom_speed;
  GtkAdjustment *adjust_zoom;

  GstCamController *controller;
} GstSwitchPTZ;

typedef struct _GstSwitchPTZClass
{
  GstWorkerClass base_class;
} GstSwitchPTZClass;

GType gst_switch_ptz_get_type (void);

#endif //__GST_SWITCH_PTZ_H__
