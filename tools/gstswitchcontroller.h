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

#ifndef __GST_SWITCH_CONTROLLER_H__
#define __GST_SWITCH_CONTROLLER_H__

#include <gio/gio.h>
#include "../logutils.h"
#include "gstworker.h"

#define GST_TYPE_SWITCH_CONTROLLER (gst_switch_controller_get_type ())
#define GST_SWITCH_CONTROLLER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_SWITCH_CONTROLLER, GstSwitchController))
#define GST_SWITCH_CONTROLLER_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_SWITCH_CONTROLLER, GstSwitchControllerClass))
#define GST_IS_SWITCH_CONTROLLER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_SWITCH_CONTROLLER))
#define GST_IS_SWITCH_CONTROLLER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_SWITCH_CONTROLLER))

#define SWITCH_CONTROLLER_OBJECT_NAME "info.duzy.gst.switch.SwitchControllerInterface"
#define SWITCH_CONTROLLER_OBJECT_PATH "/info/duzy/gst/switch/SwitchController"

typedef struct _GstSwitchController GstSwitchController;
typedef struct _GstSwitchControllerClass GstSwitchControllerClass;

typedef GVariant *(*MethodFunc) (GObject *, GDBusConnection *, GVariant *);
typedef struct _MethodTableEntry MethodTableEntry;

/**
 *  @brief Remote method table entry.
 */
struct _MethodTableEntry
{
  const gchar *name;            /*!< the remote method name */
  MethodFunc func;              /*!< the bound function */
};

/**
 *  @class GstSwitchController
 *  @struct _GstSwitchController
 *  @brief GstSwitch controller.
 */
typedef struct _GstSwitchController
{
  GObject base;                 /*!< the parent object */
  GstSwitchServer *server;      /*!< the GstSwitchServer instance */
  GDBusServer *bus_server;      /*!< the dbus server instance */
  GMutex clients_lock;          /*!< the lock for %clients */
  GList *clients;               /*!< the client list */
} GstSwitchController;

/**
 *  @class GstSwitchControllerClass
 *  @struct _GstSwitchControllerClass
 *  @brief The class of GstSwitchController.
 */
typedef struct _GstSwitchControllerClass
{
  GObjectClass base_class;      /*!< the parent class */
  GHashTable *methods;          /*!< the remote method table */
} GstSwitchControllerClass;

GType gst_switch_controller_get_type (void);

gboolean gst_switch_controller_is_valid (GstSwitchController *);
void gst_switch_controller_tell_preview_port_added (GstSwitchController *,
    gint port, gint serve, gint type);
void gst_switch_controller_tell_new_mode_onlne (GstSwitchController *,
    gint mode);
gboolean gst_switch_controller_select_face (GstSwitchController * controller,
    gint x, gint y);
void gst_switch_controller_show_face_marker (GstSwitchController * controller,
    GVariant * faces);
void gst_switch_controller_show_track_marker (GstSwitchController * controller,
    GVariant * faces);

extern gint gst_switch_controller_dbus_timeout;

#endif //__GST_SWITCH_CONTROLLER_H__
