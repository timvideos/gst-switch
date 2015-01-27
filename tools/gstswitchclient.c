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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "gstswitchclient.h"
#include "gstswitchcontroller.h"

#define parent_class gst_switch_client_parent_class
G_DEFINE_TYPE (GstSwitchClient, gst_switch_client, G_TYPE_OBJECT);

#define GST_SWITCH_CLIENT_LOCK_CONTROLLER(c) (g_mutex_lock (&(c)->controller_lock))
#define GST_SWITCH_CLIENT_UNLOCK_CONTROLLER(c) (g_mutex_unlock (&(c)->controller_lock))
#define GST_SWITCH_CLIENT_LOCK_COMPOSITE_MODE(c) (g_mutex_lock (&(c)->composite_mode_lock))
#define GST_SWITCH_CLIENT_UNLOCK_COMPOSITE_MODE(c) (g_mutex_unlock (&(c)->composite_mode_lock))

extern gboolean verbose;
gint gst_switch_client_dbus_timeout = 5000;

/**
 * @brief Initialize instances of GstSwitchClient.
 * @memberof GstSwitchClient
 */
static void
gst_switch_client_init (GstSwitchClient * client)
{
  g_mutex_init (&client->controller_lock);
  g_mutex_init (&client->composite_mode_lock);
}

/**
 * @brief Destroy instances of GstSwitchClient.
 * @memberof GstSwitchClient
 */
static void
gst_switch_client_finalize (GstSwitchClient * client)
{
  g_mutex_clear (&client->controller_lock);
  g_mutex_clear (&client->composite_mode_lock);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (client));
}

/**
 * @brief Call remote method of the controller.
 * @memberof GstSwitchClient
 */
static GVariant *
gst_switch_client_call_controller (GstSwitchClient * client,
    const gchar * method_name,
    GVariant * parameters, const GVariantType * reply_type)
{
  GVariant *value = NULL;
  GError *error = NULL;

  GST_SWITCH_CLIENT_LOCK_CONTROLLER (client);
  if (!client->controller)
    goto error_no_controller_connection;

  //INFO ("calling: %s/%s", SWITCH_CONTROLLER_OBJECT_NAME, method_name);

  value = g_dbus_connection_call_sync (
    client->controller,
    /* bus_name */ NULL,
    SWITCH_CONTROLLER_OBJECT_PATH,
    SWITCH_CONTROLLER_OBJECT_NAME,
    method_name,
    parameters,
    reply_type,
    G_DBUS_CALL_FLAGS_NONE,
    gst_switch_client_dbus_timeout,
    /* cancellable */NULL,
    &error);

  if (!value)
    goto error_call_sync;

  GST_SWITCH_CLIENT_UNLOCK_CONTROLLER (client);
  return value;

  /* ERRORS */
error_no_controller_connection:
  {
    ERROR ("No controller connection (%s)", method_name);
    GST_SWITCH_CLIENT_UNLOCK_CONTROLLER (client);
    return NULL;
  }

error_call_sync:
  {
    ERROR ("%s (%s)", error->message, method_name);
    g_error_free (error);
    GST_SWITCH_CLIENT_UNLOCK_CONTROLLER (client);
    return NULL;
  }
}

/**
 * @brief The compose port number.
 * @memberof GstSwitchClient
 */
gint
gst_switch_client_get_compose_port (GstSwitchClient * client)
{
  gint port = 0;
  GVariant *value =
      gst_switch_client_call_controller (client, "get_compose_port",
      NULL, G_VARIANT_TYPE ("(i)"));
  if (value) {
    g_variant_get (value, "(i)", &port);
    g_variant_unref (value);
  }
  return port;
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @return the encode port number
 *
 *  Get the encode port number.
 *  
 */
gint
gst_switch_client_get_encode_port (GstSwitchClient * client)
{
  gint port = 0;
  GVariant *value =
      gst_switch_client_call_controller (client, "get_encode_port",
      NULL, G_VARIANT_TYPE ("(i)"));
  if (value) {
    g_variant_get (value, "(i)", &port);
    g_variant_unref (value);
  }
  return port;
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @return the audio port number
 *
 *  The audio port number.
 *  
 */
gint
gst_switch_client_get_audio_port (GstSwitchClient * client)
{
  gint port = 0;
  GVariant *value = gst_switch_client_call_controller (client, "get_audio_port",
      NULL, G_VARIANT_TYPE ("(i)"));
  if (value) {
    g_variant_get (value, "(i)", &port);
    g_variant_unref (value);
  }
  return port;
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @return The preview ports of type GVariant
 *
 *  The all preview ports.
 *
 */
GVariant *
gst_switch_client_get_preview_ports (GstSwitchClient * client)
{
  return gst_switch_client_call_controller (client, "get_preview_ports",
      NULL, G_VARIANT_TYPE ("(s)"));
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @param channel The channel to be switched, 'A', 'B', 'a'
 *  @param port The target port number
 *  @return TRUE when requested.
 *
 *  Switch the channel to the target port.
 *  
 */
gboolean
gst_switch_client_switch (GstSwitchClient * client, gint channel, gint port)
{
  gboolean result = FALSE;
  GVariant *value = gst_switch_client_call_controller (client, "switch",
      g_variant_new ("(ii)",
          channel,
          port),
      G_VARIANT_TYPE ("(b)"));
  if (value) {
    g_variant_get (value, "(b)", &result);
    g_variant_unref (value);
  }
  return result;
}

/*
void
gst_switch_client_face_detected (GstSwitchClient * client,
    gint x, gint y, gint w, gint h)
{
  INFO ("face: %d, %d, %d, %d", x, y, w, h);
}
*/

static void
gst_switch_client_show_face_marker (GstSwitchClient * client, GVariant * faces)
{
  GstSwitchClientClass *klass =
      GST_SWITCH_CLIENT_CLASS (G_OBJECT_GET_CLASS (client));

  //INFO ("mark: %d, %d, %d, %d", x, y, w, h);

  if (klass->show_face_marker)
    (*klass->show_face_marker) (client, faces);
}

static void
gst_switch_client_show_track_marker (GstSwitchClient * client, GVariant * faces)
{
  GstSwitchClientClass *klass =
      GST_SWITCH_CLIENT_CLASS (G_OBJECT_GET_CLASS (client));

  //INFO ("mark: %d, %d, %d, %d", x, y, w, h);

  if (klass->show_track_marker)
    (*klass->show_track_marker) (client, faces);
}

/**
 * User click on the video.
 */
gboolean
gst_switch_client_click_video (GstSwitchClient * client,
    gint x, gint y, gint vw, gint vh)
{
  gboolean result = FALSE;
  GVariant *value = gst_switch_client_call_controller (client, "click_video",
      g_variant_new ("(iiii)", x, y, vw, vh), G_VARIANT_TYPE ("(b)"));
  if (value) {
    g_variant_get (value, "(b)", &result);
    g_variant_unref (value);
  }
  return result;
}

void
gst_switch_client_mark_face_remotely (GstSwitchClient * client,
    GVariant * faces)
{
  GVariant *v = g_variant_new_tuple (&faces, 1);
  GVariant *value = gst_switch_client_call_controller (client,
      "mark_face", v, G_VARIANT_TYPE ("()"));
  if (value) {
    g_variant_unref (value);
  }
  //g_variant_unref (faces);
}

void
gst_switch_client_mark_tracking_remotely (GstSwitchClient * client,
    GVariant * tracking)
{
  GVariant *v = g_variant_new_tuple (&tracking, 1);
  GVariant *value = gst_switch_client_call_controller (client,
      "mark_tracking", v, G_VARIANT_TYPE ("()"));
  if (value) {
    g_variant_unref (value);
  }
  //g_variant_unref (faces);
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @param mode new composite mode
 *  @return TRUE when requested.
 *
 *  Set the current composite mode.
 *  
 */
gboolean
gst_switch_client_set_composite_mode (GstSwitchClient * client,
    GstCompositeMode mode)
{
  gboolean result = FALSE;
  GVariant *value = NULL;

  //INFO ("changing: %d", client->changing_composite_mode);

  /* Only commit the change-composite-mode request once a time.
   */
  if (!client->changing_composite_mode) {
    GST_SWITCH_CLIENT_LOCK_COMPOSITE_MODE (client);
    if (!client->changing_composite_mode) {
      value = gst_switch_client_call_controller (client,
          "set_composite_mode",
          g_variant_new ("(i)", (gint) mode), G_VARIANT_TYPE ("(b)"));
      if (value) {
        g_variant_get (value, "(b)", &result);
        client->changing_composite_mode = result;
        g_variant_unref (value);
      }
    }
    GST_SWITCH_CLIENT_UNLOCK_COMPOSITE_MODE (client);
  }

  return result;
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @return TRUE when requested.
 *
 *  Start a new recording.
 *
 */
gboolean
gst_switch_client_new_record (GstSwitchClient * client)
{
  gboolean result = FALSE;
  GVariant *value = gst_switch_client_call_controller (client,
      "new_record",
      g_variant_new ("()"),
      G_VARIANT_TYPE ("(b)"));
  if (value) {
    g_variant_get (value, "(b)", &result);
    g_variant_unref (value);
  }
  return result;
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @param dx x position to be adjusted
 *  @param dy y position to be adjusted
 *  @param dw w position to be adjusted
 *  @param dh h position to be adjusted
 *  @return a unsigned integer indicating which components are changed of per
 *          bit.
 *
 *  Adjust the PIP.
 *
 */
guint
gst_switch_client_adjust_pip (GstSwitchClient * client, gint dx, gint dy,
    gint dw, gint dh)
{
  guint result = 0;
  GVariant *value = gst_switch_client_call_controller (client,
      "adjust_pip",
      g_variant_new ("(iiii)", dx, dy, dw, dh),
      G_VARIANT_TYPE ("(u)"));
  if (value) {
    g_variant_get (value, "(u)", &result);
    g_variant_unref (value);
  }
  return result;
}

/**
 * @memberof GstSwitchClient
 *
 * Invoked when the remote controller is closed.
 */
static void
gst_switch_client_on_controller_closed (GDBusConnection * connection,
    gboolean vanished, GError * error, gpointer user_data)
{
  GstSwitchClient *client = GST_SWITCH_CLIENT (user_data);
  GstSwitchClientClass *klass =
      GST_SWITCH_CLIENT_CLASS (G_OBJECT_GET_CLASS (client));
  if (klass->connection_closed)
    (*klass->connection_closed) (client, error);
}

void
gst_switch_client_on_signal_received (GDBusConnection *connection,
    const gchar *sender_name, const gchar *object_path,
    const gchar *interface_name, const gchar *signal_name,
    GVariant *parameters, gpointer user_data);

/**
 * @brief Connect to the GstSwitchServer controller.
 * @memberof GstSwitchClient
 *
 * Invoked when the remote controller is connected.
 */
static void
gst_switch_client_connect_controller (GstSwitchClient * client,
    const gchar * address)
{
  GError *error = NULL;

  GST_SWITCH_CLIENT_LOCK_CONTROLLER (client);

  GDBusConnectionFlags flags = G_DBUS_CONNECTION_FLAGS_NONE;
  flags |= G_DBUS_SERVER_FLAGS_RUN_IN_THREAD;
  flags |= G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT;

  client->controller = g_dbus_connection_new_for_address_sync (address, flags,
      /* GDBusAuthObserver */NULL, /* GCancellable */ NULL, &error);

  if (client->controller == NULL)
    goto error_new_connection;

  g_signal_connect (client->controller, "closed",
      G_CALLBACK (gst_switch_client_on_controller_closed), client);

  g_dbus_connection_signal_subscribe (client->controller,
      /*sender*/ NULL,
      SWITCH_CONTROLLER_OBJECT_NAME,
      /*member*/ NULL,
      SWITCH_CONTROLLER_OBJECT_PATH,
      /*arg0*/ NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      &gst_switch_client_on_signal_received,
      client,
      /*user_data_free_func*/ NULL);

  GST_SWITCH_CLIENT_UNLOCK_CONTROLLER (client);
  return;

error_new_connection:
  {
    ERROR ("%s", error->message);
    g_error_free (error);
    GST_SWITCH_CLIENT_UNLOCK_CONTROLLER (client);
    return;
  }
}

/**
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @return TRUE when requested.
 *
 *  Check if the client is connected to the gst-switch server.
 *  
 */
gboolean
gst_switch_client_is_connected (GstSwitchClient * client)
{
  gboolean result = FALSE;
  GST_SWITCH_CLIENT_LOCK_CONTROLLER (client);
  if (client->controller)
    result = TRUE;
  GST_SWITCH_CLIENT_UNLOCK_CONTROLLER (client);
  return result;
}

/**
 *  @brief Connect the client with the gst-switch server.
 *  @memberof GstSwitchClient
 *  @param client the GstSwitchClient instance
 *  @return TRUE when requested.
 *
 */
gboolean
gst_switch_client_connect (GstSwitchClient * client, const gchar *address)
{
  if (gst_switch_client_is_connected (client)) {
    ERROR ("already connected");
    return FALSE;
  }

  gst_switch_client_connect_controller (client, address);

  return gst_switch_client_is_connected (client);
}

/**
 * gst_switch_client_add_preview_port:
 *
 * The remote controller is adding a new preview port.
 */
static void
gst_switch_client_add_preview_port (GstSwitchClient * client, gint port,
    gint serve, gint type)
{
  GstSwitchClientClass *klass =
      GST_SWITCH_CLIENT_CLASS (G_OBJECT_GET_CLASS (client));
  if (klass->add_preview_port)
    (*klass->add_preview_port) (client, port, serve, type);
}

/**
 * The remote controller is telling about the new composite mode online.
 */
static void
gst_switch_client_new_mode_online (GstSwitchClient * client, gint mode)
{
  GstSwitchClientClass *klass =
      GST_SWITCH_CLIENT_CLASS (G_OBJECT_GET_CLASS (client));

  //INFO ("New composite mode: %d", mode);

  /* When a new composite mode changed, the server will inform us that it's
   * online, and when we receive that message, shall we release unset
   * changing_composite_mode.
   */
  if (client->changing_composite_mode) {
    GST_SWITCH_CLIENT_LOCK_COMPOSITE_MODE (client);
    if (client->changing_composite_mode) {
      client->changing_composite_mode = FALSE;
    }
    GST_SWITCH_CLIENT_UNLOCK_COMPOSITE_MODE (client);
  }

  if (klass->new_mode_online)
    (*klass->new_mode_online) (client, mode);
}

/**
 * Selecting face for tracking.
 */
static void
gst_switch_client_select_face (GstSwitchClient * client, gint x, gint y)
{
  GstSwitchClientClass *klass =
      GST_SWITCH_CLIENT_CLASS (G_OBJECT_GET_CLASS (client));
  if (klass->select_face)
    (*klass->select_face) (client, x, y);
}

void
gst_switch_client_on_signal_received (GDBusConnection *connection,
    const gchar *sender_name, const gchar *object_path,
    const gchar *interface_name, const gchar *signal_name,
    GVariant *parameters, gpointer user_data)
{
  GstSwitchClient *client = GST_SWITCH_CLIENT(user_data);
  INFO("Signal received: %s", signal_name);

  if(g_strcmp0("new_mode_online", signal_name) == 0)
  {
    gint mode = 0;
    g_variant_get (parameters, "(i)", &mode);

    gst_switch_client_new_mode_online (client, mode);
  }
  else if(g_strcmp0("preview_port_added", signal_name) == 0)
  {
    gint port = 0, serve = 0, type = 0;
    g_variant_get (parameters, "(iii)", &port, &serve, &type);

    gst_switch_client_add_preview_port (client, port, serve, type);
  }
  else if(g_strcmp0("show_face_marker", signal_name) == 0)
  {
    gst_switch_client_show_face_marker (client, parameters);
  }
  else if(g_strcmp0("show_track_marker", signal_name) == 0)
  {
    gst_switch_client_show_track_marker (client, parameters);
  }
  else if(g_strcmp0("select_face", signal_name) == 0)
  {
    gint x = 0, y = 0;
    g_variant_get (parameters, "(ii)", &x, &y);

    gst_switch_client_select_face (client, x, y);
  }
  else
  {
    INFO("unhandled signal on bus: %s", signal_name);
  }
}

/**
 * @brief Initialize the GstSwitchClientClass.
 * @param klass
 * @memberof GstSwitchClientClass
 */
static void
gst_switch_client_class_init (GstSwitchClientClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = (GObjectFinalizeFunc) gst_switch_client_finalize;

}
