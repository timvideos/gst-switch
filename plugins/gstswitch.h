/* GStreamer							      -*- c -*-
 * Copyright (C) 2012 Duzy Chan <code@duzy.info>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_SWITCH_H__
#define __GST_SWITCH_H__

#include <gst/gst.h>
#include <gst/gstbin.h>

G_BEGIN_DECLS
#define GST_TYPE_SWITCH \
  (gst_switch_get_type ())
#define GST_SWITCH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),GST_TYPE_SWITCH,GstSwitch))
#define GST_SWITCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),GST_TYPE_SWITCH,GstSwitchClass))
#define GST_IS_SWITCH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SWITCH))
#define GST_IS_SWITCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SWITCH))
typedef struct _GstSwitch GstSwitch;
typedef struct _GstSwitchClass GstSwitchClass;

typedef enum
{
  GST_SWITCH_FLAG_LAST = (GST_BIN_FLAG_LAST << 2)
} GstSwitchFlags;

/**
 * @deprecated
 */
struct _GstSwitch
{
  GstBin base;
  GMutex lock;
  gchar *cases_string;
};

/**
 * @deprecated
 */
struct _GstSwitchClass
{
  GstBinClass base_class;
};

GType gst_switch_get_type (void);

G_END_DECLS
#endif //__GST_SWITCH_H__
