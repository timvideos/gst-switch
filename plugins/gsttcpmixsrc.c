/* GstSwitch							    -*- c -*-
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

/**
 * SECTION:element-gsttcpmixsrc
 *
 * The tcpmixsrc element is very similar to tcpserversrc, except that
 * tcpmixsrc will accept more than one TCP client connections, and it will
 * dynamically create multiple source pads for new connections.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v tcpmixsrc port=3000 ! fdsink fd=2
 * ]|
 *
 * And from the client side:
 * |[
 * gst-launch -v fdsrc fd=1 !tcpclientsink port=3000
 * ]|
 *
 * You can run as many clients as you want.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "gsttcpmixsrc.h"
#include "../logutils.h"

GST_DEBUG_CATEGORY_STATIC (tcpmixsrc_debug);
#define GST_CAT_DEFAULT tcpmixsrc_debug

#define TCP_BACKLOG             1       /* client connection queue */
#define TCP_HIGHEST_PORT        65535
#define TCP_DEFAULT_PORT        4953
#define TCP_DEFAULT_HOST        "0.0.0.0"
#define TCP_DEFAULT_LISTEN_HOST NULL    /* listen on all interfaces */

#define MAX_READ_SIZE           4 * 1024

enum
{
  PROP_0,
  PROP_HOST,
  PROP_PORT,
  PROP_BOUND_PORT,
  PROP_MODE,
  PROP_FILL,
  PROP_AUTOSINK,
};

enum
{
  MODE_DEFAULT,                 /* stop on EOS */
  MODE_LOOP,                    /* wait for other connections on EOS */
};

enum
{
  FILL_NONE,                    /* just block until next client */
  FILL_ZERO,                    /* keep alive, fill the stream with zeros  */
  FILL_RAND,                    /* keep alive, fill the stream with random data */
};

enum
{
  SIGNAL_NEW_CLIENT,
  LAST_SIGNAL
};

static guint gst_tcpmixsrc_signals[LAST_SIGNAL] = { 0 };

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

#define GST_TYPE_TCP_MIX_SRC_PAD \
  (gst_tcp_mix_src_pad_get_type())
#define GST_TCP_MIX_SRC_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TCP_MIX_SRC_PAD,GstTCPMixSrcPad))
#define GST_TCP_MIX_SRC_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TCP_MIX_SRC_PAD,GstTCPMixSrcPadClass))
#define GST_IS_TCP_MIX_SRC_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TCP_MIX_SRC_PAD))
#define GST_IS_TCP_MIX_SRC_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TCP_MIX_SRC_PAD))

#define GST_TCP_MIX_SRC_PAD_CLIENT_LOCK(obj) \
  (g_mutex_lock (&(obj)->client_lock))
#define GST_TCP_MIX_SRC_PAD_CLIENT_UNLOCK(obj) \
  (g_mutex_unlock (&(obj)->client_lock))
#define GST_TCP_MIX_SRC_PAD_CLIENT_NOTIFY(obj) \
  (g_cond_signal (&(obj)->has_client))

typedef struct _GstTCPMixSrcPadClass GstTCPMixSrcPadClass;
typedef struct _GstTCPMixSrcPad GstTCPMixSrcPad;

/**
 * @deprecated Not used!
 */
struct _GstTCPMixSrcPadClass
{
  GstPadClass base_class;       /*!< The base class. */
};

/**
 * @deprecated Not used!
 */
struct _GstTCPMixSrcPad
{
  GstPad base;                  /*!< The base object. */

  GCancellable *cancellable;    /*!< @deprecated */

  GMutex client_lock;           /*!< @deprecated */
  GCond has_client;             /*!< @deprecated */
  GSocket *client;              /*!< @deprecated */

  gboolean running;             /*!< @deprecated */

  GstElement *fillsrc;          /*!< @deprecated */
};

/*!< @deprecated */
GType gst_tcp_mix_src_pad_get_type (void);

G_DEFINE_TYPE (GstTCPMixSrcPad, gst_tcp_mix_src_pad, GST_TYPE_PAD);

static void gst_tcp_mix_src_loop (GstTCPMixSrcPad * pad);

static void
gst_tcp_mix_src_pad_reset (GstTCPMixSrcPad * pad)
{
  if (pad->cancellable) {
    g_cancellable_reset (pad->cancellable);
  }

  if (pad->client) {
    GST_TCP_MIX_SRC_PAD_CLIENT_LOCK (pad);
    if (pad->client) {
      g_object_unref (pad->client);
      pad->client = NULL;
    }
    GST_TCP_MIX_SRC_PAD_CLIENT_UNLOCK (pad);
  }
}

static void
gst_tcp_mix_src_pad_wait_for_client (GstTCPMixSrcPad * pad)
{
  GST_TCP_MIX_SRC_PAD_CLIENT_LOCK (pad);
  while (!pad->client) {
    g_cond_wait (&pad->has_client, &pad->client_lock);
  }
  GST_TCP_MIX_SRC_PAD_CLIENT_UNLOCK (pad);
}

static void
gst_tcp_mix_src_pad_finalize (GstTCPMixSrcPad * pad)
{
  gst_tcp_mix_src_pad_reset (pad);

  if (pad->cancellable) {
    g_object_unref (pad->cancellable);
    pad->cancellable = NULL;
  }

  g_mutex_clear (&pad->client_lock);
  g_cond_clear (&pad->has_client);

  G_OBJECT_CLASS (pad)->finalize (G_OBJECT (pad));
}

static void
gst_tcp_mix_src_pad_init (GstTCPMixSrcPad * pad)
{
  pad->running = FALSE;
  pad->client = NULL;
  pad->cancellable = g_cancellable_new ();
  pad->fillsrc = NULL;

  g_mutex_init (&pad->client_lock);
  g_cond_init (&pad->has_client);
}

static gboolean
gst_tcp_mix_src_pad_start (GstTCPMixSrcPad * pad)
{
  gboolean res = FALSE;

  if (GST_PAD_TASK (pad) == NULL) {
    GstTaskFunction func = (GstTaskFunction) gst_tcp_mix_src_loop;
    res = gst_pad_start_task (GST_PAD (pad), func, GST_PAD (pad), NULL);
  }

  return res;
}

#if 0
static gboolean
gst_tcp_mix_src_pad_pause (GstTCPMixSrcPad * pad, GstTaskFunction func)
{
  gboolean res;

  if (GST_PAD_TASK (pad) != NULL) {     // FIXME: pad->status == started
    res = gst_pad_pause_task (GST_PAD (pad));
  }

  return res;
}

static gboolean
gst_tcp_mix_src_pad_stop (GstTCPMixSrcPad * pad, GstTaskFunction func)
{
  gboolean res;

  if (GST_PAD_TASK (pad) != NULL) {     // FIXME: pad->status == started
    res = gst_pad_stop_task (GST_PAD (pad));
  }

  return res;
}
#endif

static GstFlowReturn
gst_tcp_mix_src_pad_read (GstTCPMixSrcPad * pad, GstBuffer ** outbuf)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (GST_PAD_PARENT (pad));
  gssize avail, receivedBytes;
  GstMapInfo map;
  GError *err = NULL;

  /* if we have a client, wait for read */
  GST_LOG_OBJECT (pad, "asked for a buffer");

  if (!pad->client) {
    if (src->mode == MODE_LOOP)
      goto loop_read;
    else
      goto no_client;
  }

  /* read the buffer header */

read_available_bytes:
  avail = g_socket_get_available_bytes (pad->client);
  if (avail < 0) {
    goto socket_get_available_bytes_error;
  } else if (avail == 0) {
    GIOCondition condition;

    if (!g_socket_condition_wait (pad->client,
            G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP, pad->cancellable, &err))
      goto socket_condition_wait_error;

    condition = g_socket_condition_check (pad->client,
        G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP);

    if ((condition & G_IO_ERR))
      goto socket_condition_error;
    else if ((condition & G_IO_HUP))
      goto socket_condition_hup;

    avail = g_socket_get_available_bytes (pad->client);
    if (avail < 0)
      goto socket_get_available_bytes_error;
  }

  if (0 < avail) {
    gsize readBytes = MIN (avail, MAX_READ_SIZE);
    *outbuf = gst_buffer_new_and_alloc (readBytes);
    gst_buffer_map (*outbuf, &map, GST_MAP_READWRITE);
    receivedBytes = g_socket_receive (pad->client, (gchar *) map.data,
        readBytes, pad->cancellable, &err);
  } else {
    /* Connection closed */
    receivedBytes = 0;
    *outbuf = NULL;
  }

  if (receivedBytes == 0)
    goto socket_connection_closed;
  else if (receivedBytes < 0)
    goto socket_receive_error;

  gst_buffer_unmap (*outbuf, &map);
  gst_buffer_resize (*outbuf, 0, receivedBytes);

#if 0
  GST_LOG_OBJECT (pad,
      "Returning buffer from _get of size %" G_GSIZE_FORMAT
      ", ts %" GST_TIME_FORMAT ", dur %" GST_TIME_FORMAT
      ", offset %" G_GINT64_FORMAT ", offset_end %" G_GINT64_FORMAT,
      gst_buffer_get_size (*outbuf),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (*outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (*outbuf)),
      GST_BUFFER_OFFSET (*outbuf), GST_BUFFER_OFFSET_END (*outbuf));
#endif

  g_clear_error (&err);

  return GST_FLOW_OK;

  /* Handling Errors */
no_client:
  {
    GST_ELEMENT_ERROR (pad, RESOURCE, READ, (NULL),
        ("No client socket (%s)", GST_PAD_NAME (pad)));

    if (src->mode == MODE_LOOP)
      goto loop_read;
    return GST_FLOW_ERROR;
  }

socket_get_available_bytes_error:
  {
    GST_ELEMENT_ERROR (pad, RESOURCE, READ, (NULL),
        ("Failed to get available bytes from socket"));

    gst_tcp_mix_src_pad_reset (pad);

    if (src->mode == MODE_LOOP)
      goto loop_read;
    return GST_FLOW_ERROR;
  }

socket_condition_wait_error:
  {
    GST_ELEMENT_ERROR (pad, RESOURCE, READ, (NULL),
        ("Select failed: %s", err->message));
    g_clear_error (&err);

    gst_tcp_mix_src_pad_reset (pad);

    if (src->mode == MODE_LOOP)
      goto loop_read;
    return GST_FLOW_ERROR;
  }

socket_condition_error:
  {
    GST_ELEMENT_ERROR (pad, RESOURCE, READ, (NULL), ("Socket in error state"));
    *outbuf = NULL;

    gst_tcp_mix_src_pad_reset (pad);

    if (src->mode == MODE_LOOP)
      goto loop_read;
    return GST_FLOW_ERROR;
  }

socket_condition_hup:
  {
    GST_DEBUG_OBJECT (pad, "Connection closed");
    *outbuf = NULL;

    gst_tcp_mix_src_pad_reset (pad);

    if (src->mode == MODE_LOOP)
      goto loop_read;
    return GST_FLOW_EOS;
  }

socket_connection_closed:
  {
    GST_DEBUG_OBJECT (pad, "Connection closed");
    if (*outbuf) {
      gst_buffer_unmap (*outbuf, &map);
      gst_buffer_unref (*outbuf);
    }
    *outbuf = NULL;

    gst_tcp_mix_src_pad_reset (pad);

    if (src->mode == MODE_LOOP)
      goto loop_read;
    return GST_FLOW_EOS;
  }

socket_receive_error:
  {
    gst_buffer_unmap (*outbuf, &map);
    gst_buffer_unref (*outbuf);
    *outbuf = NULL;

    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      GST_DEBUG_OBJECT (pad, "Cancelled reading from socket");

      if (src->mode == MODE_LOOP)
        goto loop_read;
      return GST_FLOW_FLUSHING;
    } else {
      GST_ELEMENT_ERROR (pad, RESOURCE, READ, (NULL),
          ("Failed to read from socket: %s", err->message));

      if (src->mode == MODE_LOOP)
        goto loop_read;
      return GST_FLOW_ERROR;
    }
  }

loop_read:
  {
#if 0
    GstEvent *event = gst_event_new_flush_start ();

    if (!gst_pad_push_event (pad, event)) {
      GST_ERROR_OBJECT (src, "Failed to flush data on %s.%s",
          GST_ELEMENT_NAME (src), GST_PAD_NAME (pad));
    }
#endif

#if 0
    GST_DEBUG_OBJECT (pad, "Looping");
#endif

    if (src->fill == FILL_NONE) {
      gst_tcp_mix_src_pad_wait_for_client (pad);
      goto read_available_bytes;
    }

    enum
    { buffer_size = 1024 };
    *outbuf = gst_buffer_new_and_alloc (buffer_size);

    switch (src->fill) {
      case FILL_ZERO:
        break;
      case FILL_RAND:
      {
        guchar *p;
        gst_buffer_map (*outbuf, &map, GST_MAP_READWRITE);
        for (p = map.data; p < map.data + buffer_size; p += 4) {
          *((int *) p) = rand ();
        }
      } break;
    }
    return GST_FLOW_OK;
  }
}

static void
gst_tcp_mix_src_pad_class_init (GstTCPMixSrcPadClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = (GObjectFinalizeFunc) gst_tcp_mix_src_pad_finalize;
}

G_DEFINE_TYPE (GstTCPMixSrc, gst_tcp_mix_src, GST_TYPE_ELEMENT);

static void
gst_tcp_mix_src_stop_acceptor (GstTCPMixSrc * src)
{
  if (src->acceptor) {
    /* wait for end of accept_thread */
    g_thread_join (src->acceptor);
    src->acceptor = NULL;       // Duplicated in gst_tcp_mix_src_acceptor_thread
  }
}

static void
gst_tcp_mix_src_finalize (GObject * gobject)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (gobject);

  if (src->cancellable) {
    g_cancellable_reset (src->cancellable);
    g_object_unref (src->cancellable);
    src->cancellable = NULL;
  }

  if (src->server_socket) {
    g_object_unref (src->server_socket);
    src->server_socket = NULL;
  }

  gst_tcp_mix_src_stop_acceptor (src);

  g_mutex_clear (&src->acceptor_mutex);

  g_free (src->host);
  src->host = NULL;

  G_OBJECT_CLASS (gst_tcp_mix_src_parent_class)->finalize (gobject);
}

static void
gst_tcp_mix_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (object);

  switch (prop_id) {
    case PROP_AUTOSINK:
      g_free (src->autosink);
      src->autosink = g_strdup (g_value_get_string (value));
      break;
    case PROP_HOST:
      if (!g_value_get_string (value)) {
        g_warning ("host property cannot be NULL");
        break;
      }
      g_free (src->host);
      src->host = g_strdup (g_value_get_string (value));
      break;
    case PROP_PORT:
      src->server_port = g_value_get_int (value);
      break;
    case PROP_MODE:
      if (g_ascii_strcasecmp (g_value_get_string (value), "default") == 0) {
        src->mode = MODE_DEFAULT;
      } else if (g_ascii_strcasecmp (g_value_get_string (value), "loop") == 0) {
        src->mode = MODE_LOOP;
      }
      break;
    case PROP_FILL:
      if (g_ascii_strcasecmp (g_value_get_string (value), "none") == 0) {
        src->fill = FILL_NONE;
      } else if (g_ascii_strcasecmp (g_value_get_string (value), "zero") == 0) {
        src->fill = FILL_ZERO;
      } else if (g_ascii_strcasecmp (g_value_get_string (value), "rand") == 0) {
        src->fill = FILL_RAND;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_tcp_mix_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (object);

  switch (prop_id) {
    case PROP_AUTOSINK:
      g_value_set_string (value, src->autosink);
      break;
    case PROP_HOST:
      g_value_set_string (value, src->host);
      break;
    case PROP_PORT:
      g_value_set_int (value, src->server_port);
      break;
    case PROP_BOUND_PORT:
      g_value_set_int (value, g_atomic_int_get (&src->bound_port));
      break;
    case PROP_MODE:
      switch (src->mode) {
        case MODE_DEFAULT:
          g_value_set_string (value, "default");
          break;
        case MODE_LOOP:
          g_value_set_string (value, "loop");
          break;
      }
      break;
    case PROP_FILL:
      switch (src->fill) {
        case FILL_NONE:
          g_value_set_string (value, "none");
          break;
        case FILL_ZERO:
          g_value_set_string (value, "zero");
          break;
        case FILL_RAND:
          g_value_set_string (value, "rand");
          break;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_tcp_mix_src_stop (GstTCPMixSrc * src, GstTCPMixSrcPad * pad)
{
  GError *err = NULL;
  GList *item;

  GST_OBJECT_LOCK (src);
  GST_DEBUG_OBJECT (src, "Closing client sockets");
  for (item = GST_ELEMENT_PADS (src); item; item = g_list_next (item)) {
    GstPad *p = GST_PAD (item->data);
    if (GST_PAD_IS_SRC (p)) {
      gst_tcp_mix_src_pad_reset (GST_TCP_MIX_SRC_PAD (p));
    }
  }
  GST_OBJECT_UNLOCK (src);

  if (src->server_socket) {
    GST_DEBUG_OBJECT (src, "Closing server socket");

    if (!g_socket_close (src->server_socket, &err)) {
      GST_ERROR_OBJECT (src, "Failed to close socket: %s", err->message);
      g_clear_error (&err);
    }

    g_object_unref (src->server_socket);
    src->server_socket = NULL;

    gst_tcp_mix_src_stop_acceptor (src);

    g_atomic_int_set (&src->bound_port, 0);
    g_object_notify (G_OBJECT (src), "bound-port");
  }

  GST_OBJECT_FLAG_UNSET (src, GST_TCP_MIX_SRC_OPEN);

  return TRUE;
}

static void
gst_tcp_mix_src_request_link_pad (GstTCPMixSrc * src, GstTCPMixSrcPad * pad)
{
  GstTCPMixSrcPad *p;
  GstPad *pp;
  GList *item;
  gboolean linked = FALSE;
  GstBin *parent;
  GstElement *target;
  GstPadLinkReturn linkRet;

  if (gst_pad_is_linked (GST_PAD (pad))) {
#if 1
    pp = GST_PAD_PEER (pad);
    GST_WARNING_OBJECT (src, "Pad %s.%s already linked to %s.%s",
        GST_ELEMENT_NAME (src), GST_PAD_NAME (pad),
        GST_ELEMENT_NAME (GST_PAD_PARENT (pp)), GST_PAD_NAME (pp));
#endif
    return;
  }

  GST_LOG_OBJECT (src, "Linking pad '%s.%s'",
      GST_ELEMENT_NAME (src), GST_PAD_NAME (pad));

  INFO ("link");

  /**
   *  Don't do GST_OBJECT_LOCK() here, it causes DEADLOCK.
   */
  /* GST_OBJECT_LOCK (src); */

  if (!src->autosink)
    goto find_sink;

  parent = GST_BIN (GST_ELEMENT_PARENT (src));
  target = gst_bin_get_by_name (parent, src->autosink);

  INFO ("link");

  if (!target)
    goto find_sink;

  pp = gst_element_get_request_pad (target, "sink_%u");

  GST_DEBUG_OBJECT (src, "Link %s.%s-%s.%s",
      GST_ELEMENT_NAME (src), GST_PAD_NAME (pad),
      GST_ELEMENT_NAME (GST_PAD_PARENT (pp)), GST_PAD_NAME (pp));

  INFO ("link");

  linkRet = gst_pad_link (GST_PAD (pad), GST_PAD (pp));
  if (GST_PAD_LINK_FAILED (linkRet)) {
    GST_ERROR_OBJECT (src, "can't link");
  }

  return;

find_sink:
#if 1
  for (item = GST_ELEMENT_PADS (src); item; item = g_list_next (item)) {
    p = GST_TCP_MIX_SRC_PAD (item->data);
    if (GST_PAD_IS_SRC (p)) {
      GST_OBJECT_LOCK (p);
      if ((pp = GST_PAD_PEER (p))) {
        GstElement *ele = GST_ELEMENT (GST_PAD_PARENT (pp));

        // FIXME: pad name calculation
        pp = gst_element_get_request_pad (ele, "sink_%u");

        GST_DEBUG_OBJECT (src, "Link %s.%s-%s.%s",
            GST_ELEMENT_NAME (src), GST_PAD_NAME (pad),
            GST_ELEMENT_NAME (GST_PAD_PARENT (pp)), GST_PAD_NAME (pp));

        linkRet = gst_pad_link (GST_PAD (pad), GST_PAD (pp));
        if (GST_PAD_LINK_FAILED (linkRet)) {
          GST_ERROR_OBJECT (src, "can't link");
        } else {
          linked = TRUE;
        }
      }
      GST_OBJECT_UNLOCK (p);

      if (linked)
        break;
    }
  }
#endif

  /* GST_OBJECT_UNLOCK (src); */
  return;
}

static void
gst_tcp_mix_src_add_client (GstTCPMixSrc * src, GSocket * socket)
{
  GstTCPMixSrcPad *pad, *p;
  GList *item;
  GError *err;

  pad = NULL;

  GST_OBJECT_LOCK (src);
  for (item = GST_ELEMENT_PADS (src); item; item = g_list_next (item)) {
    p = pad = GST_TCP_MIX_SRC_PAD (item->data);
    if (GST_PAD_IS_SRC (p)) {
      GST_OBJECT_LOCK (p);
      if (pad->client) {
        pad = NULL;
      } else {
        GST_TCP_MIX_SRC_PAD_CLIENT_LOCK (pad);
        pad->client = socket;
        GST_TCP_MIX_SRC_PAD_CLIENT_NOTIFY (pad);
        GST_TCP_MIX_SRC_PAD_CLIENT_UNLOCK (pad);
      }
      GST_OBJECT_UNLOCK (p);
      if (pad)
        break;
    }
  }
  GST_OBJECT_UNLOCK (src);

  if (!pad) {
    pad =
        GST_TCP_MIX_SRC_PAD (gst_element_get_request_pad (GST_ELEMENT (src),
            srctemplate.name_template));
    GST_OBJECT_LOCK (pad);
    GST_TCP_MIX_SRC_PAD_CLIENT_LOCK (pad);
    pad->client = socket;
    GST_TCP_MIX_SRC_PAD_CLIENT_NOTIFY (pad);
    GST_TCP_MIX_SRC_PAD_CLIENT_UNLOCK (pad);
    GST_OBJECT_UNLOCK (pad);
  }

  if (pad) {
    GST_DEBUG_OBJECT (pad, "New client on %s.%s (%d srcpads)",
        GST_ELEMENT_NAME (src), GST_PAD_NAME (pad),
        GST_ELEMENT (src)->numsrcpads);

    gst_tcp_mix_src_request_link_pad (src, pad);

    if (!gst_pad_is_linked (GST_PAD (pad))) {
      GST_ERROR_OBJECT (src, "Pad %s.%s is not linked",
          GST_ELEMENT_NAME (src), GST_PAD_NAME (pad));
    }

    if (!gst_pad_is_active (GST_PAD (pad)))
      gst_pad_set_active (GST_PAD (pad), TRUE);

    g_signal_emit (src, gst_tcpmixsrc_signals[SIGNAL_NEW_CLIENT], 0, pad);
  } else {
    GST_WARNING_OBJECT (src, "No pad for new client, closing..");

    if (!g_socket_close (socket, &err)) {
      GST_ERROR_OBJECT (src, "Failed to close socket: %s", err->message);
      g_clear_error (&err);
    }

    g_object_unref (socket);
  }
}

static gpointer
gst_tcp_mix_src_acceptor_thread (GstTCPMixSrc * src)
{
  GSocket *socket;
  GError *err;

  while (src->server_socket && src->cancellable) {
    socket = g_socket_accept (src->server_socket, src->cancellable, &err);
    if (!socket) {
      GST_WARNING_OBJECT (src, "Failed to accept: %s", err->message);
      continue;
    }

    gst_tcp_mix_src_add_client (src, socket);
  }

  g_mutex_lock (&src->acceptor_mutex);
  g_thread_unref (src->acceptor);
  src->acceptor = NULL;
  g_mutex_unlock (&src->acceptor_mutex);
  return NULL;
}

static void
gst_tcp_mix_src_loop (GstTCPMixSrcPad * pad)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (GST_PAD_PARENT (pad));
  GstBuffer *buffer = NULL;
  GstFlowReturn ret;

  /*
     INFO ("%p, %s", g_thread_self (), GST_PAD_NAME (pad));
   */

  if (!pad->running) {
    gst_tcp_mix_src_pad_wait_for_client (pad);
    pad->running = TRUE;
  }

  ret = gst_tcp_mix_src_pad_read (pad, &buffer);
  if (ret != GST_FLOW_OK)
    goto error_read_pad;

  ret = gst_pad_push (GST_PAD (pad), buffer);
  if (ret != GST_FLOW_OK)
    goto error_push_buffer;

  return;

  /* Handling Errors */
error_read_pad:
  {
    GST_ERROR_OBJECT (src, "Can't read from %s:%s",
        GST_ELEMENT_NAME (src), GST_PAD_NAME (pad));
    goto pause;
  }

error_push_buffer:
  {
    GST_ERROR_OBJECT (src, "Can't push buffer to %s:%s",
        GST_ELEMENT_NAME (src), GST_PAD_NAME (pad));
    goto pause;
  }

pause:
  {
    if (src->mode == MODE_LOOP)
      goto loop;

    GstEvent *event;

    if (ret == GST_FLOW_EOS) {
      event = gst_event_new_eos ();
      gst_pad_push_event (GST_PAD (pad), event);
    } else if (ret == GST_FLOW_NOT_LINKED || ret <= GST_FLOW_EOS) {
      event = gst_event_new_eos ();

      GST_ELEMENT_ERROR (src, STREAM, FAILED,
          ("Internal data flow error."),
          ("streaming task paused (%s (%d))", gst_flow_get_name (ret), ret));

      gst_pad_push_event (GST_PAD (pad), event);
    }

    /*
       GST_DEBUG_OBJECT (pad, "Paused %s.%s (%s)",
       GST_ELEMENT_NAME (src), GST_PAD_NAME (pad),
       gst_flow_get_name (ret));
     */

    gst_pad_pause_task (GST_PAD (pad));
    return;
  }

loop:
  {
    if (ret == GST_FLOW_NOT_LINKED || ret <= GST_FLOW_EOS) {
      GST_ELEMENT_ERROR (src, STREAM, FAILED,
          ("Internal data flow error."),
          ("streaming task paused (%s (%d))", gst_flow_get_name (ret), ret));
    }
    return;
  }
}

static gboolean
gst_tcp_mix_src_listen (GstTCPMixSrc * src, GstTCPMixSrcPad * pad)
{
  GError *err = NULL;
  GInetAddress *addr;
  GSocketAddress *saddr;
  GResolver *resolver;
  gint bound_port = 0;
  gchar *ip;

  /* look up name if we need to */
  addr = g_inet_address_new_from_string (src->host);
  if (!addr) {
    GList *results;

    resolver = g_resolver_get_default ();
    results = g_resolver_lookup_by_name (resolver, src->host,
        src->cancellable, &err);
    if (!results)
      goto resolve_no_name;

    addr = G_INET_ADDRESS (g_object_ref (results->data));

    g_resolver_free_addresses (results);
    g_object_unref (resolver);
  }

  /* get IP address */
  ip = g_inet_address_to_string (addr);

  saddr = g_inet_socket_address_new (addr, src->server_port);
  g_object_unref (addr);

  /* create the server listener socket */
  src->server_socket = g_socket_new (g_socket_address_get_family (saddr),
      G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &err);
  if (!src->server_socket)
    goto socket_new_failed;

  /* bind it */
  if (!g_socket_bind (src->server_socket, saddr, TRUE, &err))
    goto socket_bind_failed;

  g_object_unref (saddr);

  g_socket_set_listen_backlog (src->server_socket, TCP_BACKLOG);
  if (!g_socket_listen (src->server_socket, &err))
    goto socket_listen_failed;

  GST_OBJECT_FLAG_SET (src, GST_TCP_MIX_SRC_OPEN);

  if (src->server_port == 0) {
    saddr = g_socket_get_local_address (src->server_socket, NULL);
    bound_port = g_inet_socket_address_get_port ((GInetSocketAddress *) saddr);
    g_object_unref (saddr);
  } else {
    bound_port = src->server_port;
  }

  GST_DEBUG_OBJECT (src, "Listening on %s (%s:%d)", src->host, ip, bound_port);

  g_free (ip);

  g_atomic_int_set (&src->bound_port, bound_port);
  g_object_notify (G_OBJECT (src), "bound-port");

  return TRUE;

  /* Handling Errors */
resolve_no_name:
  {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      GST_DEBUG_OBJECT (src, "Cancelled name resolval");
    } else {
      GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
          ("Failed to resolve host '%s': %s", src->host, err->message));
    }
    g_clear_error (&err);
    g_object_unref (resolver);
    return FALSE;
  }

socket_new_failed:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("Failed to create socket: %s", err->message));
    g_clear_error (&err);
    g_object_unref (saddr);
    return FALSE;
  }

socket_bind_failed:
  {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      GST_DEBUG_OBJECT (src, "Cancelled binding");
    } else {
      GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
          ("Failed to bind on host '%s:%d': %s", src->host, src->server_port,
              err->message));
    }
    g_clear_error (&err);
    g_object_unref (saddr);
    gst_tcp_mix_src_stop (src, pad);
    return FALSE;
  }

socket_listen_failed:
  {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      GST_DEBUG_OBJECT (src, "Cancelled listening");
    } else {
      GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
          ("Failed to listen on host '%s:%d': %s", src->host,
              src->server_port, err->message));
    }
    g_clear_error (&err);
    gst_tcp_mix_src_stop (src, pad);
    return FALSE;
  }
}

static gboolean
gst_tcp_mix_src_start_acceptor (GstTCPMixSrc * src, GstTCPMixSrcPad * pad)
{
  gboolean res = TRUE;
  if (!src->acceptor) {
    g_mutex_lock (&src->acceptor_mutex);
    if (!src->acceptor) {
      if (!gst_tcp_mix_src_listen (src, pad)) {
        res = FALSE;
      } else {
        src->acceptor = g_thread_new ("tcpmixsrc.acceptor",
            (GThreadFunc) gst_tcp_mix_src_acceptor_thread, src);
      }
    }
    g_mutex_unlock (&src->acceptor_mutex);
  }
  return res;
}

/* set up server */
static gboolean
gst_tcp_mix_src_start (GstTCPMixSrc * src, GstTCPMixSrcPad * pad)
{
  if (!src->acceptor) {
    gst_tcp_mix_src_start_acceptor (src, pad);
  }

  gst_tcp_mix_src_pad_start (pad);

  return TRUE;
}

#if 0
/* will be called only between calls to start() and stop() */
static gboolean
gst_tcp_mix_src_unlock (GstBaseSrc * bsrc)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (bsrc);

  g_cancellable_cancel (src->cancellable);

  return TRUE;
}

static gboolean
gst_tcp_mix_src_unlock_stop (GstBaseSrc * bsrc)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (bsrc);

  g_cancellable_reset (src->cancellable);

  return TRUE;
}
#endif

static gboolean
gst_tcp_mix_src_activate_pull (GstTCPMixSrcPad * pad, GstObject * parent,
    gboolean active)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (parent);

  g_print ("%s:%d:info: activate-pull: %s\n", __FILE__, __LINE__,
      GST_PAD_NAME (pad));

  if (active) {
    GST_DEBUG_OBJECT (src, "Activating in pull mode");

    if (G_UNLIKELY (!gst_tcp_mix_src_start (src, pad)))
      goto error_start;
  } else {
    GST_DEBUG_OBJECT (src, "Deactivating in pull mode");

    if (G_UNLIKELY (!gst_tcp_mix_src_stop (src, pad)))
      goto error_stop;
  }

  return TRUE;

  /* Handling Errors */
error_start:
  {
    GST_ERROR_OBJECT (src, "Failed to start in pull mode");
    return FALSE;
  }

error_stop:
  {
    GST_ERROR_OBJECT (src, "Failed to stop in pull mode");
    return FALSE;
  }
}

static gboolean
gst_tcp_mix_src_activate_push (GstTCPMixSrcPad * pad, GstObject * parent,
    gboolean active)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (parent);

  if (!src)
    return TRUE;

  if (active) {
    GST_DEBUG_OBJECT (src, "Activating %s in push mode",
        GST_ELEMENT_NAME (src));

    if (G_UNLIKELY (!gst_tcp_mix_src_start (src, pad)))
      goto error_start;
  } else {
    GST_DEBUG_OBJECT (src, "Deactivating %s in push mode",
        GST_ELEMENT_NAME (src));

    if (G_UNLIKELY (!gst_tcp_mix_src_stop (src, pad)))
      goto error_stop;
  }

  return TRUE;

  /* Handling Errors */
error_start:
  {
    GST_ERROR_OBJECT (src, "Failed to start in push mode");
    return FALSE;
  }

error_stop:
  {
    GST_ERROR_OBJECT (src, "Failed to stop in push mode");
    return FALSE;
  }
}

static gboolean
gst_tcp_mix_src_activate_mode (GstTCPMixSrcPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  gboolean res = FALSE;

  switch (mode) {
    case GST_PAD_MODE_PULL:
      res = gst_tcp_mix_src_activate_pull (pad, parent, active);
      break;
    case GST_PAD_MODE_PUSH:
      res = gst_tcp_mix_src_activate_push (pad, parent, active);
      break;
    default:
      break;
  }

  return res;
}

static gboolean
gst_tcp_mix_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  //g_print ("%s:%d:info: query: %s\n", __FILE__, __LINE__, GST_QUERY_TYPE_NAME (query));

  return gst_pad_query_default (pad, parent, query);
}

static gboolean
gst_tcp_mix_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
#if 0
  g_print (LOG_PREFIX "%s:%d:info: event: %s\n", __FILE__, __LINE__,
      GST_EVENT_TYPE_NAME (event));
#endif

  return gst_pad_event_default (pad, parent, event);
}

static GstFlowReturn
gst_tcp_mix_src_get_range (GstTCPMixSrc * src, GstPad * pad, guint64 offset,
    guint length, GstBuffer ** buf)
{
  g_print ("%s:%d:info: getrange: %s, %ld, %d\n", __FILE__, __LINE__,
      GST_PAD_NAME (pad), offset, length);



  return GST_FLOW_OK;
}

static GstFlowReturn
gst_tcp_mix_src_getrange (GstPad * pad, GstObject * parent, guint64 offset,
    guint length, GstBuffer ** buf)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (parent);
  GstFlowReturn res;

  //GST_LIVE_LOCK (src);

  res = gst_tcp_mix_src_get_range (src, pad, offset, length, buf);

  //GST_LIVE_UNLOCK (src);
  return res;
}

static GstPad *
gst_tcp_mix_src_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * unused, const GstCaps * caps)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (element);
  GstPad *srcpad;
  gchar *name;
  gboolean res;
  int num;

  //g_print ("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);

  GST_INFO_OBJECT (src, "Requesting new pad %s.%s (caps: %s)",
      GST_ELEMENT_NAME (src), GST_PAD_TEMPLATE_NAME_TEMPLATE (templ),
      gst_caps_to_string (caps));

  GST_OBJECT_LOCK (src);
  num = g_list_length (GST_ELEMENT_PADS (src));
  name = g_strdup_printf ("src_%u", num);
  srcpad = GST_PAD_CAST (g_object_new (GST_TYPE_TCP_MIX_SRC_PAD,
          "name", name, "direction", templ->direction, "template", templ,
          NULL));
  g_free (name);
  GST_OBJECT_UNLOCK (src);


  // see: gst_tcp_mix_src_activate_push
  gst_pad_set_active (srcpad, TRUE);
  gst_pad_activate_mode (srcpad, GST_PAD_MODE_PUSH, TRUE);

  gst_pad_set_activatemode_function (srcpad,
      (GstPadActivateModeFunction) gst_tcp_mix_src_activate_mode);
  gst_pad_set_query_function (srcpad, gst_tcp_mix_src_query);
  gst_pad_set_event_function (srcpad, gst_tcp_mix_src_event);
  gst_pad_set_getrange_function (srcpad, gst_tcp_mix_src_getrange);

  //GST_OBJECT_FLAG_SET (srcpad, GST_PAD_FLAG_PROXY_CAPS);

  //INFO ("add-pad: %s.%s", GST_ELEMENT_NAME (src), GST_PAD_NAME (srcpad));
  res = gst_element_add_pad (GST_ELEMENT_CAST (src), srcpad);

  gst_tcp_mix_src_start (src, GST_TCP_MIX_SRC_PAD (srcpad));

  if (G_UNLIKELY (!res)) {
    GST_ERROR_OBJECT (src, "Failed to add new pad");
  }

  return srcpad;
}

static void
gst_tcp_mix_src_release_pad (GstElement * element, GstPad * pad)
{
  GstTCPMixSrc *src = GST_TCP_MIX_SRC (element);

  (void) src;

  gst_object_unref (pad);
}

static void
gst_tcp_mix_src_class_init (GstTCPMixSrcClass * klass)
{
  GObjectClass *object_class;
  GstElementClass *element_class;

  object_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;

  object_class->set_property = gst_tcp_mix_src_set_property;
  object_class->get_property = gst_tcp_mix_src_get_property;
  object_class->finalize = gst_tcp_mix_src_finalize;

  gst_tcpmixsrc_signals[SIGNAL_NEW_CLIENT] =
      g_signal_new ("new-client", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstTCPMixSrcClass, new_client),
      NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 1, GST_TYPE_PAD);

  g_object_class_install_property (object_class, PROP_HOST,
      g_param_spec_string ("host", "Host", "The hostname to listen as",
          TCP_DEFAULT_LISTEN_HOST, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PORT,
      g_param_spec_int ("port", "Port", "The port to listen to (0=random)",
          0, TCP_HIGHEST_PORT, TCP_DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_BOUND_PORT,
      g_param_spec_int ("bound-port", "BoundPort",
          "The port number the socket is currently bound to", 0,
          TCP_HIGHEST_PORT, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_MODE,
      g_param_spec_string ("mode", "Mode", "The working mode",
          "default", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_FILL,
      g_param_spec_string ("fill", "Fill Mode",
          "The fill mode for disconnected stream",
          "none", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_AUTOSINK,
      g_param_spec_string ("autosink", "Autosink",
          "The fill mode for disconnected stream",
          "none", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));

  gst_element_class_set_static_metadata (element_class,
      "TCP mix server source", "Source/Network",
      "Receive data as a server over the network via TCP from "
      "multiple clients", "Duzy Chan <code@duzy.info>");

  element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_tcp_mix_src_request_new_pad);
  element_class->release_pad = GST_DEBUG_FUNCPTR (gst_tcp_mix_src_release_pad);

  /* debug category for fltering log messages */
  GST_DEBUG_CATEGORY_INIT (tcpmixsrc_debug, "tcpmixsrc", 0,
      "Performs face detection on videos and images, providing "
      "detected positions via bus messages");
}

static void
gst_tcp_mix_src_init (GstTCPMixSrc * src)
{
  src->server_port = TCP_DEFAULT_PORT;
  src->host = g_strdup (TCP_DEFAULT_HOST);
  src->server_socket = NULL;
  src->cancellable = g_cancellable_new ();

  g_mutex_init (&src->acceptor_mutex);

  GST_OBJECT_FLAG_UNSET (src, GST_TCP_MIX_SRC_OPEN);
}
