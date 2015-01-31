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

#ifndef __GST_SWITCH_SERVER_H__
#define __GST_SWITCH_SERVER_H__

#include <gio/gio.h>
#include "gstcomposite.h"
#include "gstswitchcontroller.h"
#include "../logutils.h"

#define GST_TYPE_SWITCH_SERVER (gst_switch_server_get_type())
#define GST_SWITCH_SERVER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_SWITCH_SERVER, GstSwitchServer))
#define GST_SWITCH_SERVER_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_SWITCH_SERVER, GstSwitchServerClass))
#define GST_IS_SWITCH_SERVER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_SWITCH_SERVER))
#define GST_IS_SWITCH_SERVER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_SWITCH_SERVER))

#define GST_SWITCH_MIN_SINK_PORT 1
#define GST_SWITCH_MAX_SINK_PORT 65535

typedef struct _GstRecorder GstRecorder;
typedef struct _GstSwitchServerClass GstSwitchServerClass;
typedef struct _GstSwitchServerOpts GstSwitchServerOpts;

/**
 *  GstSwitchServerOpts:
 *  @param record_filename the recording filename
 *  @param controller_address the dbus address for the controller
 *  @param video_input_port the video input TCP port
 *  @param audio_input_port the audio input TCP port
 */
struct _GstSwitchServerOpts
{
  gchar *test_switch;
  gchar *record_filename;
  gchar *controller_address;
  gint video_input_port;
  gint audio_input_port;
//should really be in here
//gboolean verbose;
  gboolean low_res;
  GstCaps *video_caps;
  gchar *video_caps_str;
  gchar *audio_caps_str;
};

/**
 *  GstSwitchServer:
 *  @param base the parent object
 *  @param host the server host name, e.g. localhost
 *  @param main_loop the main loop
 *  @param main_loop_lock the lock for the %main_loop
 *  @param exit_code the exit code in cases of force quit.
 *  @param cancellable 
 *  @param video_acceptor_lock the lock for the video acceptor
 *  @param video_acceptor the video acceptor thread
 *  @param video_acceptor_socket the video acceptor socket
 *  @param video_acceptor_port the video acceptor port number
 *  @param audio_acceptor_lock the lock for the audio acceptor
 *  @param audio_acceptor the audio acceptor thread
 *  @param audio_acceptor_socket the audio acceptor socket
 *  @param audio_acceptor_port the audio acceptor port
 *  @param controller_lock the lock for controller
 *  @param controller_thread the controller thread (deprecated)
 *  @param controller_socket the controller socket (deprecated)
 *  @param controller_port the controller port number (deprecated)
 *  @param controller the controller instance
 *  @param alloc_port_lock the lock for %alloc_port_count
 *  @param alloc_port_count port allocation counter
 *  @param serve_lock the lock for serving new inputs
 *  @param cases_lock the lock for the %cases
 *  @param cases the case list
 *  @param composite the composite instance
 *  @param new_composite_mode the new composite mode to be applied
 *  @param output the output instance
 *  @param recorder_lock the lock for the %recorder
 *  @param recorder the recorder instance
 *  @param pip_lock the lock for PIP
 *  @param pip_x the PIP X position
 *  @param pip_y the PIP Y position
 *  @param pip_w the PIP width
 *  @param pip_h the PIP height
 *  @param clock_lock the lock for %clock
 *  @param clock a system clock
 */
struct _GstSwitchServer
{
  GObject base;

  gchar *host;

  GMainLoop *main_loop;
  GMutex main_loop_lock;
  gint exit_code;

  GCancellable *cancellable;
  GMutex video_acceptor_lock;
  GThread *video_acceptor;
  GSocket *video_acceptor_socket;
  gint video_acceptor_port;

  GMutex audio_acceptor_lock;
  GThread *audio_acceptor;
  GSocket *audio_acceptor_socket;
  gint audio_acceptor_port;

  GMutex controller_lock;
  GstSwitchController *controller;

  GMutex alloc_port_lock;
  gint alloc_port_count;

  GMutex serve_lock;
  GMutex cases_lock;
  GList *cases;

  GstComposite *composite;
  GstCompositeMode new_composite_mode;

  GstWorker *output;

  GMutex recorder_lock;
  GstRecorder *recorder;

  GMutex pip_lock;
  gint pip_x, pip_y, pip_w, pip_h;

  GMutex clock_lock;
  GstClock *clock;
};

/**
 *  GstSwitchServerClass:
 *  @param base_class the parent class
 */
struct _GstSwitchServerClass
{
  GObjectClass base_class;
};

GType gst_switch_server_get_type (void);
gint gst_switch_server_get_composite_sink_port (GstSwitchServer * srv);
gint gst_switch_server_get_encode_sink_port (GstSwitchServer * srv);
gint gst_switch_server_get_audio_sink_port (GstSwitchServer * srv);
GArray *gst_switch_server_get_preview_sink_ports (GstSwitchServer * srv,
    GArray ** serves, GArray ** types);
gboolean gst_switch_server_set_composite_mode (GstSwitchServer * srv,
    gint mode);
gint gst_switch_server_get_composite_mode (GstSwitchServer * srv);
gboolean gst_switch_server_switch (GstSwitchServer * srv, gint channel,
    gint port);
gboolean gst_switch_server_click_video (GstSwitchServer * srv,
    gint x, gint y, gint fw, gint fh);
void gst_switch_server_mark_face (GstSwitchServer * srv,
    GVariant * faces, gboolean tracking);
guint gst_switch_server_adjust_pip (GstSwitchServer * srv, gint dx, gint dy,
    gint dw, gint dh);
gboolean gst_switch_server_new_record (GstSwitchServer * srv);

GstCaps *gst_switch_server_getcaps (void);
const gchar *gst_switch_server_get_audio_caps_str (void);
const gchar *gst_switch_server_get_video_caps_str (void);
const gchar *gst_switch_server_get_record_filename (void);

extern GstSwitchServerOpts opts;

#endif //__GST_SWITCH_SERVER_H__
