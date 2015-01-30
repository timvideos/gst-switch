/* GstSwitch
 * Copyright (C) 2013 Duzy Chan <code@duzy.info>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*! @file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstswitchcontroller.h"

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
