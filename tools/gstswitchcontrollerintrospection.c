/* gst-switch							    -*- c -*-
 * Copyright (C) 2012,2013 Duzy Chan <code@duzy.info>
 * Copyright (C) 2015 Tim 'mithro' Ansell <mithro@mithis.com>
 * Copyright (C) 2015 MaZderMind <peter@mazdermind.de>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstswitchcontroller.h"

/* *INDENT-OFF* */
const gchar gstswitchcontroller_introspection_xml[] =
    "<node>"
    "  <interface name='" SWITCH_CONTROLLER_OBJECT_NAME "'>"
    "    <method name='get_compose_port'>"
    "      <arg type='i' name='port' direction='out'/>"
    "    </method>"
    "    <method name='get_encode_port'>"
    "      <arg type='i' name='port' direction='out'/>"
    "    </method>"
    "    <method name='get_audio_port'>"
    "      <arg type='i' name='port' direction='out'/>"
    "    </method>"
    "    <method name='get_preview_ports'>"
    "      <arg type='s' name='ports' direction='out'/>"
    "    </method>"
    "    <method name='set_composite_mode'>"
    "      <arg type='i' name='channel' direction='in'/>"
    "      <arg type='b' name='result' direction='out'/>"
    "    </method>"
    "    <method name='get_composite_mode'>"
    "      <arg type='i' name='result' direction='out'/>"
    "    </method>"
    "    <method name='set_encode_mode'>"
    "      <arg type='i' name='channel' direction='in'/>"
    "      <arg type='b' name='result' direction='out'/>"
    "    </method>"
    "    <method name='new_record'>"
    "      <arg type='b' name='result' direction='out'/>"
    "    </method>"
    "    <method name='adjust_pip'>"
    "      <arg type='i' name='dx' direction='in'/>"
    "      <arg type='i' name='dy' direction='in'/>"
    "      <arg type='i' name='dw' direction='in'/>"
    "      <arg type='i' name='dh' direction='in'/>"
    "      <arg type='u' name='result' direction='out'/>"
    "    </method>"
    "    <method name='switch'>"
    "      <arg type='i' name='channel' direction='in'/>"
    "      <arg type='i' name='port' direction='in'/>"
    "      <arg type='b' name='result' direction='out'/>"
    "    </method>"
    "    <method name='click_video'>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "      <arg type='i' name='fw' direction='in'/>"
    "      <arg type='i' name='fh' direction='in'/>"
    "      <arg type='b' name='result' direction='out'/>"
    "    </method>"
    "    <method name='mark_face'>"
    "      <arg type='a(iiii)' name='faces' direction='in'/>"
    "    </method>"
    "    <method name='mark_tracking'>"
    "      <arg type='a(iiii)' name='faces' direction='in'/>"
    "    </method>"
    "    "
    "    <signal name='preview_port_added'>"
    "      <arg type='i' name='port'/>"
    "      <arg type='i' name='serve'/>"
    "      <arg type='i' name='type'/>"
    "    </signal>"
    "    <signal name='preview_port_removed'>"
    "      <arg type='i' name='port'/>"
    "      <arg type='i' name='serve'/>"
    "      <arg type='i' name='type'/>"
    "    </signal>"
    "    <signal name='new_mode_online'>"
    "      <arg type='i' name='mode'/>"
    "    </signal>"
    "    <signal name='show_face_marker'>"
    "      <arg type='a(iiii)' name='mode'/>"
    "    </signal>"
    "    <signal name='show_track_marker'>"
    "      <arg type='a(iiii)' name='mode'/>"
    "    </signal>"
    "    <signal name='select_face'>"
    "      <arg type='i' name='x'/>"
    "      <arg type='i' name='y'/>"
    "    </signal>"
    "  </interface>"
    "</node>";
/* *INDENT-ON* */
