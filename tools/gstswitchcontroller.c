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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstswitchcontroller.h"
#include "gstswitchserver.h"
#include "gstswitchclient.h"

#define GST_SWITCH_CONTROLLER_LOCK_CLIENTS(c) (g_mutex_lock (&(c)->clients_lock))
#define GST_SWITCH_CONTROLLER_UNLOCK_CLIENTS(c) (g_mutex_unlock (&(c)->clients_lock))

G_DEFINE_TYPE (GstSwitchController, gst_switch_controller, G_TYPE_OBJECT);

static GDBusNodeInfo *introspection_data = NULL;
gint gst_switch_controller_dbus_timeout = 5000;

/**
 * @brief Helper function for matching remoting method names.
 * @memberof GstSwitchController
 */
static gboolean
gst_switch_controller_method_match (const gchar * key,
    MethodTableEntry * entry, const gchar * match)
{
  if (g_strcmp0 (key, match) == 0)
    return TRUE;
  return FALSE;
}

/**
 * @brief Performing a remoting method call from a gst-switch client.
 * @memberof GstSwitchController
 */
static void
gst_switch_controller_do_method_call (GDBusConnection * connection,
    const gchar * sender,
    const gchar * object_path,
    const gchar * interface_name,
    const gchar * method_name,
    GVariant * parameters,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  GstSwitchController *controller = GST_SWITCH_CONTROLLER (user_data);
  GstSwitchControllerClass *klass =
      GST_SWITCH_CONTROLLER_CLASS (G_OBJECT_GET_CLASS (controller));
  MethodFunc entry = (MethodFunc) g_hash_table_find (klass->methods,
      (GHRFunc)
      gst_switch_controller_method_match,
      (gpointer) method_name);
  GVariant *results;

  if (!entry)
    goto error_no_method;

  /*
     INFO ("calling: %s/%s", interface_name, method_name);
   */

  results = (*entry) (G_OBJECT (controller), connection, parameters);
  g_dbus_method_invocation_return_value (invocation, results);
  return;

error_no_method:
  {
    ERROR ("unsupported method: %s", method_name);
    g_dbus_method_invocation_return_error (invocation, 0, -1,
        "Unsupported call %s", method_name);
  }
}

/**
 * @brief Fetching the controller property remotely (it's useless currently).
 * @memberof GstSwitchController
 */
static GVariant *
gst_switch_controller_do_get_property (GDBusConnection * connection,
    const gchar * sender,
    const gchar * object_path,
    const gchar * interface_name,
    const gchar * property_name, GError ** error, gpointer user_data)
{
  GstSwitchController *controller = GST_SWITCH_CONTROLLER (user_data);
  GVariant *ret = NULL;

  (void) controller;

  INFO ("get: %s", property_name);

  if (g_strcmp0 (property_name, "num") == 0) {
  }

  return ret;
}

/**
 * @brief Setting the property of controller remotely (it's useless currently).
 * @memberof GstSwitchController
 */
static gboolean
gst_switch_controller_do_set_property (GDBusConnection * connection,
    const gchar * sender,
    const gchar * object_path,
    const gchar * interface_name,
    const gchar * property_name,
    GVariant * value, GError ** error, gpointer user_data)
{
  INFO ("set: %s", property_name);
  return FALSE;
}

/**
 * @brief The remoting virtual table.
 * @see GDBusInterfaceVTable.
 */
static const GDBusInterfaceVTable gst_switch_controller_interface_vtable = {
  gst_switch_controller_do_method_call,
  gst_switch_controller_do_get_property,
  gst_switch_controller_do_set_property
};

#if 0
static void
gst_switch_controller_send_property_change (GstSwitchController * controller,
    GParamSpec * pspec, GDBusConnection * connection)
{
  INFO ();
}

static void
gst_switch_controller_bus_acquired (GDBusConnection * connection,
    const gchar * name, gpointer data)
{
  GstSwitchController *controller = GST_SWITCH_CONTROLLER (data);
  guint register_id;

  INFO ("bus acquired: %s", name);

  g_signal_connect (controller, "notify",
      G_CALLBACK (gst_switch_controller_send_property_change), connection);

  register_id = g_dbus_connection_register_object (connection, SWITCH_CONTROLLER_OBJECT_PATH, introspection_data->interfaces[0], &gst_switch_controller_interface_vtable, controller,   /* user_data */
      NULL,                     /* user_data_free_func */
      NULL /* GError** */ );

  g_assert (0 < register_id);
}

static void
gst_switch_controller_name_acquired (GDBusConnection * connection,
    const gchar * name, gpointer data)
{
  INFO ("name acquired: %s", name);
}

static void
gst_switch_controller_name_lost (GDBusConnection * connection,
    const gchar * name, gpointer data)
{
  INFO ("name lost: %s", name);
}

static void
gst_switch_controller_export (GstSwitchController * controller)
{
  controller->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
      SWITCH_CONTROLLER_OBJECT_NAME,
      G_BUS_NAME_OWNER_FLAGS_NONE,
      gst_switch_controller_bus_acquired,
      gst_switch_controller_name_acquired,
      gst_switch_controller_name_lost, controller, NULL);

  INFO ("TODO: export");
}
#endif

/**
 * @brief Perform sending remote signals to connected clients.
 * @memberof GstSwitchController
 */
static void
gst_switch_controller_emit_signal (GstSwitchController * controller,
    const gchar * signame, GVariant * parameters)
{
  GError *error;
  gboolean res;
  GList *client;
  gint num;

  GST_SWITCH_CONTROLLER_LOCK_CLIENTS (controller);
  g_assert (parameters);
  g_variant_ref_sink (parameters);

  for (client = controller->clients, num = 0; client;
      client = g_list_next (client)) {
    error = NULL;
    res = g_dbus_connection_emit_signal (G_DBUS_CONNECTION (client->data),
        /*destination_bus_name */ NULL,
        SWITCH_CONTROLLER_OBJECT_PATH,
        SWITCH_CONTROLLER_OBJECT_NAME, signame, parameters, &error);

    if (!res) {
      g_assert (error != NULL);
      ERROR ("emit: (%d) %s", num, error->message);
    } else {
      ++num;
    }
  }
  /*
     INFO ("emit: %s (%d/%d)", signame, num, g_list_length (controller->uis));
   */
  g_variant_unref (parameters);

  GST_SWITCH_CONTROLLER_UNLOCK_CLIENTS (controller);
}

/**
 * @brief Invoked to cleanup when a connected client is closed.
 * @memberof GstSwitchController
 */
static void
gst_switch_controller_on_connection_closed (GDBusConnection * connection,
    gboolean vanished, GError * error, gpointer user_data)
{
  GstSwitchController *controller = GST_SWITCH_CONTROLLER (user_data);

  (void) controller;

  if (error) {
    WARN ("close: %s", error->message);
  }

  GST_SWITCH_CONTROLLER_LOCK_CLIENTS (controller);
  controller->clients = g_list_remove (controller->clients, connection);
  GST_SWITCH_CONTROLLER_UNLOCK_CLIENTS (controller);

  INFO ("closed: %p, %d (%d clients remaining)", connection, vanished,
      g_list_length (controller->clients));

  g_object_unref (connection);
}

/**
 * @brief Invoked when a new incomming client connection is established.
 * @memberof GstSwitchController
 */
static gboolean
gst_switch_controller_on_new_connection (GDBusServer * server,
    GDBusConnection * connection, gpointer user_data)
{
  GstSwitchController *controller = GST_SWITCH_CONTROLLER (user_data);
  guint register_id = 0;
  GError *error = NULL;

  register_id = g_dbus_connection_register_object (connection, SWITCH_CONTROLLER_OBJECT_PATH, introspection_data->interfaces[0], &gst_switch_controller_interface_vtable, controller,   /* user_data */
      NULL,                     /* user_data_free_func */
      &error);

  if (error != NULL) {
    ERROR ("failed to register controller: %s", error->message);
    g_error_free (error);
    return FALSE;
  } else if (register_id <= 0) {
    ERROR ("register_id invalid (<= 0): %d", register_id);
    return FALSE;
  }

  g_signal_connect (connection, "closed",
      G_CALLBACK (gst_switch_controller_on_connection_closed), controller);

  GST_SWITCH_CONTROLLER_LOCK_CLIENTS (controller);
  controller->clients = g_list_append (controller->clients, connection);
  GST_SWITCH_CONTROLLER_UNLOCK_CLIENTS (controller);

  INFO ("registered: %d, %s, %s", register_id,
    SWITCH_CONTROLLER_OBJECT_PATH, introspection_data->interfaces[0]->name);


  g_object_ref (connection);

  return TRUE;
}

/**
 * @brief Initializing the GstSwitchController.
 * @memberof GstSwitchController
 * @see GObject
 */
static void
gst_switch_controller_init (GstSwitchController * controller)
{
  gchar *guid = g_dbus_generate_guid ();
  GDBusServerFlags flags = G_DBUS_SERVER_FLAGS_NONE;
  GError *error = NULL;

  g_mutex_init (&controller->clients_lock);
  controller->clients = NULL;

  flags |= G_DBUS_SERVER_FLAGS_RUN_IN_THREAD;
  flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;

  controller->bus_server =
      g_dbus_server_new_sync (opts.controller_address, flags, guid,
      /* GDBusAuthObserver */ NULL, /* GCancellable */ NULL, &error);

  if (error != NULL) {
    g_error ("failed to register controller: %s", error->message);
  }
  g_assert (controller->bus_server != NULL);

  g_free (guid);

  INFO ("Controller is listening at: %s",
      g_dbus_server_get_client_address (controller->bus_server));

  g_signal_connect (controller->bus_server, "new-connection",
      G_CALLBACK (gst_switch_controller_on_new_connection), controller);

  g_dbus_server_start (controller->bus_server);

  return;
}

/**
 * @brief Destroying the GstSwitchController.
 * @memberof GstSwitchController
 * @see GObject
 */
static void
gst_switch_controller_finalize (GstSwitchController * controller)
{
  INFO ("gst_switch_controller finalized");
  if (controller->bus_server) {
    g_dbus_server_stop (controller->bus_server);
    g_assert (!g_dbus_server_is_active (controller->bus_server));
    g_object_unref (controller->bus_server);
    controller->bus_server = NULL;
  }

  g_mutex_clear (&controller->clients_lock);

  if (G_OBJECT_CLASS (gst_switch_controller_parent_class)->finalize)
    (*G_OBJECT_CLASS (gst_switch_controller_parent_class)->finalize)
        (G_OBJECT (controller));
}

/**
 * @brief Check if the controller is valid.
 * @param controller the GstSwitchController instance
 * @return TRUE is the controller is valid connected to the dbus server
 * @memberof GstSwitchController
 */
gboolean
gst_switch_controller_is_valid (GstSwitchController * controller)
{
  gboolean valid = FALSE;
  if (controller->bus_server) {
    valid = TRUE;
  }
  return valid;
}

/**
 * @memberof GstSwitchController
 *
 * Fetching a GstSwitchController object's property.
 *
 * @see GObject
 */
static void
gst_switch_controller_get_property (GstSwitchController * controller,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (controller), prop_id, pspec);
      break;
  }
}

/**
 * @memberof GstSwitchController
 *
 * Setting a GstSwitchController object's property.
 *
 * @see GObject.
 */
static void
gst_switch_controller_set_property (GstSwitchController * controller,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (controller), prop_id, pspec);
      break;
  }
}

/**
 *  @memberof GstSwitchController
 *  @param controller the GstSwitchController instance
 *  @param port the port number
 *  @param serve value of GstSwitchServeStreamType
 *  @param type value of GstCaseType
 *
 *  Tell the preview port to the clients.
 */
void
gst_switch_controller_tell_preview_port_added (GstSwitchController * controller,
    gint port, gint serve, gint type)
{
  gst_switch_controller_emit_signal (controller, "preview_port_added",
      g_variant_new ("(iii)", port, serve, type));
}

void
gst_switch_controller_tell_preview_port_removed (GstSwitchController *
    controller, gint port, gint serve, gint type)
{
  gst_switch_controller_emit_signal (controller, "preview_port_removed",
      g_variant_new ("(iii)", port, serve, type));
}

/**
 *  @memberof GstSwitchController
 *  @param controller the GstSwitchController instance
 *  @param mode the new mode changed
 *
 *  Tell the clients that new composite mode is online.
 */
void
gst_switch_controller_tell_new_mode_onlne (GstSwitchController * controller,
    gint mode)
{
  gst_switch_controller_emit_signal (controller, "new_mode_online",
      g_variant_new ("(i)", mode));
}

gboolean
gst_switch_controller_select_face (GstSwitchController * controller,
    gint x, gint y)
{
  gst_switch_controller_emit_signal (controller, "select_face",
      g_variant_new ("(ii)", x, y));

  return TRUE;
}

void
gst_switch_controller_show_face_marker (GstSwitchController * controller,
    GVariant * faces)
{
  gst_switch_controller_emit_signal (controller, "show_face_marker",
      g_variant_new_tuple (&faces, 1));
}

void
gst_switch_controller_show_track_marker (GstSwitchController * controller,
    GVariant * faces)
{
  gst_switch_controller_emit_signal (controller, "show_track_marker",
      g_variant_new_tuple (&faces, 1));
}

/**
 * @memberof GstSwitchController
 *  
 * Remoting method stub of "get_compose_port".
 */
static GVariant *
gst_switch_controller__get_compose_port (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  gint port = 0;
  if (controller->server) {
    port = gst_switch_server_get_composite_sink_port (controller->server);
  }
  return g_variant_new ("(i)", port);
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "get_encode_port".
 */
static GVariant *
gst_switch_controller__get_encode_port (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  gint port = 0;
  if (controller->server) {
    port = gst_switch_server_get_encode_sink_port (controller->server);
  }
  return g_variant_new ("(i)", port);
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "get_audio_port".
 */
static GVariant *
gst_switch_controller__get_audio_port (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  gint port = 0;
  if (controller->server) {
    port = gst_switch_server_get_audio_sink_port (controller->server);
  }
  return g_variant_new ("(i)", port);
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "get_preview_ports".
 */
static GVariant *
gst_switch_controller__get_preview_ports (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  if (controller->server) {
    GArray *serves = NULL, *types = NULL;
    GArray *ports =
        gst_switch_server_get_preview_sink_ports (controller->server, &serves,
        &types);
    int n;
    GVariantBuilder *builder;
    GVariant *value;
    gchar *res;

    builder = g_variant_builder_new (G_VARIANT_TYPE ("a(iii)"));
    for (n = 0; n < ports->len; ++n) {
      g_variant_builder_add (builder, "(iii)",
          g_array_index (ports, gint, n),
          g_array_index (serves, gint, n), g_array_index (types, gint, n));
    }
    value = g_variant_builder_end (builder);
    //result = g_variant_new_tuple (&value, 1);
    res = g_variant_print (value, FALSE);
    result = g_variant_new ("(s)", res);

    /*
       INFO ("value: %s (%d)", g_variant_get_type_string (value),
       g_variant_n_children (value));
       INFO ("result: %s, %s", g_variant_get_type_string (result), res);
     */

    g_free (res);
    g_array_free (ports, TRUE);
    g_array_free (types, TRUE);
  }
  return result;
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "set_composite_mode".
 */
static GVariant *
gst_switch_controller__set_composite_mode (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  gboolean ok = FALSE;
  gint mode;
  g_variant_get (parameters, "(i)", &mode);
  if (controller->server) {
    ok = gst_switch_server_set_composite_mode (controller->server, mode);
    result = g_variant_new ("(b)", ok);
  }
  return result;
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "get_composite_mode".
 */
static GVariant *
gst_switch_controller__get_composite_mode (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  gint mode;
  if (controller->server) {
    mode = gst_switch_server_get_composite_mode (controller->server);
    result = g_variant_new ("(i)", mode);
  }
  return result;
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "new_record".
 */
static GVariant *
gst_switch_controller__new_record (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  gboolean ok = FALSE;
  if (controller->server) {
    ok = gst_switch_server_new_record (controller->server);
    result = g_variant_new ("(b)", ok);
  }
  return result;
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "adjust_pip".
 */
static GVariant *
gst_switch_controller__adjust_pip (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  gint dx, dy, dw, dh;
  guint res = 0;
  g_variant_get (parameters, "(iiii)", &dx, &dy, &dw, &dh);
  if (controller->server) {
    res = gst_switch_server_adjust_pip (controller->server, dx, dy, dw, dh);
    result = g_variant_new ("(u)", res);
  }
  return result;
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "switch".
 */
static GVariant *
gst_switch_controller__switch (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  gint channel, port;
  gboolean ok = FALSE;
  g_variant_get (parameters, "(ii)", &channel, &port);
  if (controller->server) {
    ok = gst_switch_server_switch (controller->server, channel, port);
    result = g_variant_new ("(b)", ok);
  }
  return result;
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "click_video".
 */
static GVariant *
gst_switch_controller__click_video (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  gboolean ok = FALSE;
  gint x, y, vw, vh;
  g_variant_get (parameters, "(iiii)", &x, &y, &vw, &vh);
  if (controller->server) {
    ok = gst_switch_server_click_video (controller->server, x, y, vw, vh);
    result = g_variant_new ("(b)", ok);
  }
  return result;
}

/**
 * @memberof GstSwitchController
 *
 * Remoting method stub of "click_video".
 */
static GVariant *
gst_switch_controller__mark_face (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  //gint size = g_variant_n_children (parameters);
  if (controller->server) {
    gst_switch_server_mark_face (controller->server,
        g_variant_get_child_value (parameters, 0), FALSE);
  }
  return result;
}

static GVariant *
gst_switch_controller__mark_tracking (GstSwitchController * controller,
    GDBusConnection * connection, GVariant * parameters)
{
  GVariant *result = NULL;
  //gint size = g_variant_n_children (parameters);
  if (controller->server) {
    gst_switch_server_mark_face (controller->server,
        g_variant_get_child_value (parameters, 0), TRUE);
  }
  return result;
}

/**
 *
 * Remoting method table of the gst-switch controller.
 */
static MethodTableEntry gst_switch_controller_method_table[] = {
  {"get_compose_port", (MethodFunc) gst_switch_controller__get_compose_port},
  {"get_encode_port", (MethodFunc) gst_switch_controller__get_encode_port},
  {"get_audio_port", (MethodFunc) gst_switch_controller__get_audio_port},
  {"get_preview_ports",
      (MethodFunc) gst_switch_controller__get_preview_ports},
  {"set_composite_mode",
      (MethodFunc) gst_switch_controller__set_composite_mode},
  {"get_composite_mode",
      (MethodFunc) gst_switch_controller__get_composite_mode},
  {"new_record", (MethodFunc) gst_switch_controller__new_record},
  {"adjust_pip", (MethodFunc) gst_switch_controller__adjust_pip},
  {"click_video", (MethodFunc) gst_switch_controller__click_video},
  {"mark_face", (MethodFunc) gst_switch_controller__mark_face},
  {"mark_tracking", (MethodFunc) gst_switch_controller__mark_tracking},
  {"switch", (MethodFunc) gst_switch_controller__switch},
  {NULL, NULL}
};

/**
 * @brief Initialize the GstSwitchControllerClass.
 * @param klass
 * @memberof GstSwitchControllerClass
 */
static void
gst_switch_controller_class_init (GstSwitchControllerClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = (GObjectFinalizeFunc) gst_switch_controller_finalize;
  object_class->get_property =
      (GObjectGetPropertyFunc) gst_switch_controller_get_property;
  object_class->set_property =
      (GObjectSetPropertyFunc) gst_switch_controller_set_property;

  klass->methods = g_hash_table_new (g_str_hash, g_str_equal);

  MethodTableEntry *entry = &gst_switch_controller_method_table[0];
  for (; entry->name && entry->func; ++entry) {
    g_hash_table_insert (klass->methods, (gpointer) entry->name,
        (gpointer) entry->func);
  }

  introspection_data =
      g_dbus_node_info_new_for_xml (gstswitchcontroller_introspection_xml,
      NULL);
  g_assert (introspection_data != NULL);
}
