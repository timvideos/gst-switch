/* GstSwitch							    -*- c -*-
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
