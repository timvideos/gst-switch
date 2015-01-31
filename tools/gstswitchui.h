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

#ifndef __GST_SWITCH_UI_H__
#define __GST_SWITCH_UI_H__

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gstswitchclient.h"
#include "gstworker.h"
#include "gstvideodisp.h"
#include "gstaudiovisual.h"

// Only allowed to use enum and to_string methods from this file.
#include "gstcomposite.h"

#define GST_TYPE_SWITCH_UI (gst_switch_ui_get_type ())
#define GST_SWITCH_UI(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_SWITCH_UI, GstSwitchUI))
#define GST_SWITCH_UI_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_SWITCH_UI, GstSwitchUIClass))
#define GST_IS_SWITCH_UI(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_SWITCH_UI))
#define GST_IS_SWITCH_UI_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_SWITCH_UI))

typedef struct _GstSwitchUI GstSwitchUI;
typedef struct _GstSwitchUIClass GstSwitchUIClass;

/**
 *  @class GstSwitchUI
 *  @struct _GstSwitchUI
 *  @brief The gst-switch-ui program.
 */
struct _GstSwitchUI
{
  GstSwitchClient base;

  GtkCssProvider *css;

  GtkWidget *window;
  GtkWidget *compose_view;
  GtkWidget *compose_overlay;
  GtkWidget *preview_box;
  GtkWidget *status;

  GMutex select_lock;
  GtkWidget *selected;

  GMutex audio_lock;
  gint audio_port;
  GstAudioVisual *audio;
  GstClockTime audio_endtime;
  gdouble audio_value;
  gint audio_stuck_count;

  GMutex compose_lock;
  GstVideoDisp *compose;

  guint32 tabtime;
  GstCompositeMode compose_mode;
  gint timer;

  GMutex faces_lock, tracking_lock;
  GVariant *faces;
  GVariant *tracking;
};

/**
 *  @class GstSwitchUIClass
 *  @struct _GstSwitchUIClass
 *  @brief The class of GstSwitchUI.
 */
struct _GstSwitchUIClass
{
  GstSwitchClientClass base_class;      /*!< The base class. */
};

GType gst_switch_ui_get_type (void);

#endif //__GST_SWITCH_UI_H__
