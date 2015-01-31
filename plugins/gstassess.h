/* gst-switch							      -*- c -*-
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

#ifndef __GST_ASSESS_H__
#define __GST_ASSESS_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS
#define GST_TYPE_ASSESS \
  (gst_assess_get_type ())
#define GST_ASSESS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),GST_TYPE_ASSESS,GstAssess))
#define GST_ASSESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),GST_TYPE_ASSESS,GstAssessClass))
#define GST_IS_ASSESS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ASSESS))
#define GST_IS_ASSESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ASSESS))
typedef struct _GstAssess GstAssess;
typedef struct _GstAssessClass GstAssessClass;

/**
 * @brief Helper class for assessment.
 */
struct _GstAssess
{
  GstBaseTransform base;
  GMutex lock;

  guint number;

  GstPad *sinkpad;
  GstPad *srcpad;
};

/**
 * @brief GstAssessClass
 */
struct _GstAssessClass
{
  GstBaseTransformClass base_class;
};

GType gst_assess_get_type (void);

G_END_DECLS
#endif //__GST_ASSESS_H__
