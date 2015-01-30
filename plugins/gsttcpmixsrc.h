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

#ifndef __GST_TCP_MIX_SRC_H__
#define __GST_TCP_MIX_SRC_H__

#include <gst/gst.h>
#include <gst/gstelement.h>
#include <gio/gio.h>

G_BEGIN_DECLS
#define GST_TYPE_TCP_MIX_SRC \
  (gst_tcp_mix_src_get_type())
#define GST_TCP_MIX_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TCP_MIX_SRC,GstTCPMixSrc))
#define GST_TCP_MIX_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TCP_MIX_SRC,GstTCPMixSrcClass))
#define GST_IS_TCP_MIX_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TCP_MIX_SRC))
#define GST_IS_TCP_MIX_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TCP_MIX_SRC))
typedef struct _GstTCPMixSrc GstTCPMixSrc;
typedef struct _GstTCPMixSrcClass GstTCPMixSrcClass;

typedef enum
{
  GST_TCP_MIX_SRC_OPEN = (GST_ELEMENT_FLAG_LAST << 0),
  GST_TCP_MIX_SRC_FLAG_LAST = (GST_ELEMENT_FLAG_LAST << 2)
} GstTCPMixSrcFlags;

/**
 * @deprecated
 */
struct _GstTCPMixSrc
{
  GstElement base;

  gchar *host;
  int server_port;
  int bound_port;               /* currently bound-to port, or 0 *//* ATOMIC */
  int mode;                     /* stream working mode for disconnection */
  int fill;                     /* fill type for disconnected stream */

  GCancellable *cancellable;
  GSocket *server_socket;

  GMutex acceptor_mutex;

  GThread *acceptor;
  gchar *autosink;
};

/**
 * @deprecated
 */
struct _GstTCPMixSrcClass
{
  GstElementClass base_class;

  void (*new_client) (GstElement * element, GstPad * pad);
};

/**
 * @deprecated
 */
GType gst_tcp_mix_src_get_type (void);

G_END_DECLS
#endif //__GST_TCP_MIX_SRC_H__
