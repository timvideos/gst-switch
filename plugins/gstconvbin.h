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

#ifndef __GST_CONV_BIN_H__
#define __GST_CONV_BIN_H__

#include <gst/gst.h>
#include <gst/gstbin.h>

G_BEGIN_DECLS
#define GST_TYPE_CONV_BIN			\
  (gst_conv_bin_get_type ())
#define GST_CONV_BIN(obj)						\
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),GST_TYPE_CONV_BIN,GstConvBin))
#define GST_CONV_BIN_CLASS(klass)					\
  (G_TYPE_CHECK_CLASS_CAST ((klass),GST_TYPE_CONV_BIN,GstConvBinClass))
#define GST_IS_CONV_BIN(obj)					\
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),GST_TYPE_CONV_BIN))
#define GST_IS_CONV_BIN_CLASS(klass)			\
  (G_TYPE_CHECK_CLASS_TYPE ((klass),GST_TYPE_CONV_BIN))
typedef struct _GstConvBin GstConvBin;
typedef struct _GstConvBinClass GstConvBinClass;

typedef enum
{
  GST_CONV_BIN_PAD_FLAG_GHOSTED = (GST_BIN_FLAG_LAST << 1),
  GST_CONV_BIN_PAD_FLAG_LAST = (GST_BIN_FLAG_LAST << 2),
} GstConvBinFlags;

/**
 * @deprecated
 */
struct _GstConvBin
{
  GstBin base;

  GMutex lock;
  gchar *converter;
  gchar *autosink;
};

/**
 * @deprecated
 */
struct _GstConvBinClass
{
  GstBinClass base_class;
};

GType gst_conv_bin_get_type (void);

G_END_DECLS
#endif //__GST_CONV_BIN_H__
