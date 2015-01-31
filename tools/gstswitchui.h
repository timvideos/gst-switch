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
