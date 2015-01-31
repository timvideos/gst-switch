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

/*! @file */

#ifndef __GST_SWITCH_CLIENT_H__
#define __GST_SWITCH_CLIENT_H__

#include <gst/gst.h>
#include <gio/gio.h>

#include "gstcomposite.h"

#define GST_TYPE_SWITCH_CLIENT (gst_switch_client_get_type ())
#define GST_SWITCH_CLIENT(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_SWITCH_CLIENT, GstSwitchClient))
#define GST_SWITCH_CLIENT_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_SWITCH_CLIENT, GstSwitchClientClass))
#define GST_IS_SWITCH_CLIENT(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_SWITCH_CLIENT))
#define GST_IS_SWITCH_CLIENT_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_SWITCH_CLIENT))

typedef struct _GstSwitchClient GstSwitchClient;
typedef struct _GstSwitchClientClass GstSwitchClientClass;

typedef void (*GstSwitchClientConnectionClosedFunc) (GstSwitchClient * client,
    GError * error);
typedef void (*GstSwitchClientSetComposePortFunc) (GstSwitchClient * client,
    gint port);
typedef void (*GstSwitchClientSetEncodePortFunc) (GstSwitchClient * client,
    gint port);
typedef void (*GstSwitchClientSetAudioPortFunc) (GstSwitchClient * client,
    gint port);
typedef void (*GstSwitchClientAddPreviewPortFunc) (GstSwitchClient * client,
    gint port, gint serve, gint type);
typedef void (*GstSwitchClientNewModeOnlineFunc) (GstSwitchClient * client,
    gint port);
typedef void (*GstSwitchClientSelectFaceFunc) (GstSwitchClient * client,
    gint x, gint y);
typedef void (*GstSwitchClientShowFaceMarkerFunc) (GstSwitchClient * client,
    GVariant * faces);

/**
 * @enum GstSwitchClientRole
 * @brief The role of a client.
 */
typedef enum
{
  CLIENT_ROLE_NONE,             /*!< the client is trivial */
  CLIENT_ROLE_UI,               /*!< the client is acting as a UI */
  CLIENT_ROLE_CAPTURE,          /*!< the client is acting as a capture */
} GstSwitchClientRole;

/**
 *  @class GstSwitchClient
 *  @struct _GstSwitchClient
 *  @brief The GstSwitch client.
 */
struct _GstSwitchClient
{
  GObject base;

  GstSwitchClientRole role;

  GMutex controller_lock;
  GDBusConnection *controller;

  GMutex composite_mode_lock;
  gboolean changing_composite_mode;
};

/**
 *  @class GstSwitchClientClass
 *  @struct _GstSwitchClientClass
 *  @brief The class of GstSwitchClient
 */
struct _GstSwitchClientClass
{
  GObjectClass base_class;

  GHashTable *methods;

  void (*connection_closed) (GstSwitchClient * client, GError * error);
  void (*set_audio_port) (GstSwitchClient * client, gint port);
  void (*set_compose_port) (GstSwitchClient * client, gint port);
  void (*set_encode_port) (GstSwitchClient * client, gint port);
  void (*add_preview_port) (GstSwitchClient * client, gint port, gint serve,
      gint type);
  void (*new_mode_online) (GstSwitchClient * client, gint mode);
  void (*select_face) (GstSwitchClient * client, gint x, gint y);
  void (*show_face_marker) (GstSwitchClient * client, GVariant * faces);
  void (*show_track_marker) (GstSwitchClient * client, GVariant * faces);
};

GType gst_switch_client_get_type (void);

gboolean gst_switch_client_is_connected (GstSwitchClient * client);
gboolean gst_switch_client_connect (GstSwitchClient * client,
    GstSwitchClientRole role, const gchar * address);
gint gst_switch_client_get_compose_port (GstSwitchClient * client);
gint gst_switch_client_get_encode_port (GstSwitchClient * client);
gint gst_switch_client_get_audio_port (GstSwitchClient * client);
GVariant *gst_switch_client_get_preview_ports (GstSwitchClient * client);
gboolean gst_switch_client_switch (GstSwitchClient * client, gint channel,
    gint port);
gboolean gst_switch_client_set_composite_mode (GstSwitchClient * client,
    GstCompositeMode mode);
gboolean gst_switch_client_click_video (GstSwitchClient * client,
    gint x, gint y, gint fw, gint fh);
void gst_switch_client_mark_face_remotely (GstSwitchClient * client,
    GVariant * faces);
void gst_switch_client_mark_tracking_remotely (GstSwitchClient * client,
    GVariant * tracking);
gboolean gst_switch_client_new_record (GstSwitchClient * client);
guint gst_switch_client_adjust_pip (GstSwitchClient * client, gint dx,
    gint dy, gint dw, gint dh);

extern gint gst_switch_client_dbus_timeout;

#endif //__GST_SWITCH_CLIENT_H__
