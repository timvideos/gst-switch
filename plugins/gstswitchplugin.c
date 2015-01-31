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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include "gsttcpmixsrc.h"
#include "gstswitch.h"
#include "gstconvbin.h"
#include "../logutils.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "tcpmixsrc", GST_RANK_NONE,
          GST_TYPE_TCP_MIX_SRC)) {
    return FALSE;
  }

  if (!gst_element_register (plugin, "convbin", GST_RANK_NONE,
          GST_TYPE_CONV_BIN)) {
    return FALSE;
  }

  if (!gst_element_register (plugin, "switch", GST_RANK_NONE, GST_TYPE_SWITCH)) {
    return FALSE;
  }

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR,
    tcpmixsrc, "GStreamer TCP Mix Source Plugin",
    plugin_init, VERSION, "LGPL",
    "GStreamer", "https://github.com/duzy/gst-switch")
