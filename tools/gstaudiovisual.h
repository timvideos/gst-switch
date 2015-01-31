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

#ifndef __GST_AUDIO_VISUAL_H__
#define __GST_AUDIO_VISUAL_H__

#include "gstworker.h"

#define GST_TYPE_AUDIO_VISUAL (gst_audio_visual_get_type ())
#define GST_AUDIO_VISUAL(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_AUDIO_VISUAL, GstAudioVisual))
#define GST_AUDIO_VISUAL_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_AUDIO_VISUAL, GstAudioVisualClass))
#define GST_IS_AUDIO_VISUAL(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_AUDIO_VISUAL))
#define GST_IS_AUDIO_VISUAL_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_AUDIO_VISUAL))

typedef struct _GstAudioVisual GstAudioVisual;
typedef struct _GstAudioVisualClass GstAudioVisualClass;

/**
 *  @class GstAudioVisual
 *  @struct _GstAudioVisual
 *  @brief Visualize audio stream.
 */
struct _GstAudioVisual
{
  GstWorker base;               /*!< the parent object */

  gint port;                    /*!< the port number */
  gulong handle;                /*!< the X window handle for displaying the audio visualization */
  gboolean active;              /*!< TRUE if the audio is active. A active audio will be sinked to
                                 *   the real hardware speaker, e.g. ALSA
                                 **/

  gboolean renewing;            /*!< Used by GstSwitchUI. */

  GMutex endtime_lock;          /*!< the lock for %endtime */
  GstClockTime endtime;         /*!< the endtime of the last audio sample */
  GMutex value_lock;            /*!< the lock for %value */
  gdouble value;                /*!< the value of the last audio sample */
};

/**
 *  @class GstAudioVisualClass
 *  @struct _GstAudioVisualClass
 *  @brief The class of GstAudioVisualClass.
 */
struct _GstAudioVisualClass
{
  GstWorkerClass base_class;    /*!< the parent class */
};

GType gst_audio_visual_get_type (void);

GstClockTime gst_audio_visual_get_endtime (GstAudioVisual * visual);
gdouble gst_audio_visual_get_value (GstAudioVisual * visual);

#endif //__GST_AUDIO_VISUAL_H__
