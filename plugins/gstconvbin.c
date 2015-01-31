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
 * SECTION:element-gstconvbin
 *
 * The convbin element is designed to convert N inputs at once using a
 * specified converter.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v videotestsrc ! convbin converter=gdppay ! filesink location=a.gdp
 * ]|
 *
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstconvbin.h"
#include "../logutils.h"

GST_DEBUG_CATEGORY_STATIC (gst_conv_bin_debug);
#define GST_CAT_DEFAULT gst_conv_bin_debug

#define GST_CONV_BIN_LOCK(obj) (g_mutex_lock (&(obj)->lock))
#define GST_CONV_BIN_UNLOCK(obj) (g_mutex_unlock (&(obj)->lock))

static GstStaticPadTemplate gst_conv_bin_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_conv_bin_src_factory =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_AUTOSINK,
  PROP_CONVERTER,
};

G_DEFINE_TYPE (GstConvBin, gst_conv_bin, GST_TYPE_BIN);

static void
gst_conv_bin_set_property (GstConvBin * conv, guint prop_id,
    const GValue * value, GParamSpec * spec)
{
  switch (prop_id) {
    case PROP_AUTOSINK:
      g_free (conv->autosink);
      conv->autosink = g_strdup (g_value_get_string (value));
      break;
    case PROP_CONVERTER:
      g_free (conv->converter);
      conv->converter = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (conv), prop_id, spec);
      break;
  }
}

static void
gst_conv_bin_get_property (GstConvBin * conv, guint prop_id,
    GValue * value, GParamSpec * spec)
{
  switch (prop_id) {
    case PROP_AUTOSINK:
      g_value_set_string (value, conv->autosink);
      break;
    case PROP_CONVERTER:
      g_value_set_string (value, conv->converter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (conv), prop_id, spec);
      break;
  }
}

static void
gst_conv_bin_init (GstConvBin * conv)
{
  conv->converter = NULL;
  conv->autosink = NULL;

  g_mutex_init (&conv->lock);
}

static void
gst_conv_bin_finalize (GstConvBin * conv)
{
  g_free (conv->converter);
  g_free (conv->autosink);

  g_mutex_clear (&conv->lock);

  G_OBJECT_CLASS (conv)->finalize (G_OBJECT (conv));
}

static void
gst_conv_bin_autosink (GstConvBin * conv)
{
  GstElement *target = NULL, *baseconv;
  GstBin *parent;
  GstPad *basepad, *srcpad, *pp;
  GList *item;
  gchar *name;
  gint num;

  if (!conv->autosink)
    goto done;

  parent = GST_BIN (GST_ELEMENT_PARENT (conv));
  target = gst_bin_get_by_name (parent, conv->autosink);

  if (!target)
    goto done;

  for (item = GST_ELEMENT_PADS (conv), num = 0; item; item = g_list_next (item)) {
    if (GST_PAD_IS_SRC (GST_PAD (item->data))) {
      ++num;
    }
  }

  for (item = GST_BIN_CHILDREN (GST_BIN (conv));
      item; item = g_list_next (item)) {
    baseconv = GST_ELEMENT (item->data);
    basepad = gst_element_get_static_pad (baseconv, "src");

    if (basepad && (gst_pad_is_linked (basepad) ||
            GST_OBJECT_FLAG_IS_SET (basepad, GST_CONV_BIN_PAD_FLAG_GHOSTED)))
      continue;

    name = g_strdup_printf ("src_%u", num);
    srcpad = gst_ghost_pad_new (name, basepad);
    gst_object_unref (basepad);
    g_free (name);

    INFO ("requesting %s.sink_%%u", GST_ELEMENT_NAME (target));
    pp = gst_element_get_request_pad (target, "sink_%u");
    if (!pp) {
      GST_ERROR_OBJECT (target, "get request pad %s.sink_%%u",
          GST_ELEMENT_NAME (target));
      continue;
    }

    GST_DEBUG_OBJECT (conv, "Autosink %s.%s",
        GST_ELEMENT_NAME (target), GST_PAD_NAME (pp));

    INFO ("autosink %s.%s -> %s.%s",
        GST_ELEMENT_NAME (conv), GST_PAD_NAME (srcpad),
        GST_ELEMENT_NAME (GST_PAD_PARENT (pp)), GST_PAD_NAME (pp));

    if (GST_PAD_LINK_FAILED (gst_pad_link (srcpad, pp))) {
      GST_ERROR_OBJECT (srcpad, "Failed autosink %s.%s",
          GST_ELEMENT_NAME (GST_PAD_PARENT (pp)), GST_PAD_NAME (pp));
    }

    gst_pad_set_active (srcpad, TRUE);

    if (gst_element_add_pad (GST_ELEMENT (conv), srcpad)) {
      if (basepad) {
        GST_OBJECT_FLAG_SET (basepad, GST_CONV_BIN_PAD_FLAG_GHOSTED);
      }
    }
  }

done:
  return;
}

static GstPad *
gst_conv_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * unused, const GstCaps * caps)
{
  GstConvBin *conv = GST_CONV_BIN (element);
  GstPadDirection dir = GST_PAD_TEMPLATE_DIRECTION (templ);
  GstPad *pad = NULL, *basepad = NULL;
  GstElement *baseconv = NULL;
  gchar *name = NULL;
  GList *item = NULL;
  gint num = 0;

  if (conv->converter == NULL)
    goto error_no_converter_name;

  INFO ("requesting: %s.%s",
      GST_ELEMENT_NAME (conv), GST_PAD_TEMPLATE_NAME_TEMPLATE (templ));

  GST_CONV_BIN_LOCK (conv);

  for (item = GST_BIN_CHILDREN (GST_BIN (conv)), num = 0;
      item; item = g_list_next (item)) {
    ++num;

    if (!basepad) {
      GstPad *p = NULL;
      gchar *s = NULL;
      switch (dir) {
        case GST_PAD_SINK:
          s = "sink";
          break;
        case GST_PAD_SRC:
          s = "src";
          break;
        default:
          break;
      }
      p = gst_element_get_static_pad (GST_ELEMENT (item->data), s);
      if (!GST_PAD_IS_LINKED (p))
        basepad = p;
    }
  }

  if (!baseconv) {
    name = g_strdup_printf ("conv_%u", num);
    baseconv = gst_element_factory_make (conv->converter, name);
    g_free (name);
    name = NULL;

    if (!baseconv)
      goto error_no_converter;

    if (!gst_bin_add (GST_BIN (conv), baseconv))
      goto error_bin_add;
  }
  if (!baseconv)
    goto error_no_converter;

  if (!basepad) {
    switch (dir) {
      case GST_PAD_SINK:
        basepad = gst_element_get_static_pad (baseconv, "sink");
        break;
      case GST_PAD_SRC:
        basepad = gst_element_get_static_pad (baseconv, "src");
        break;
      default:
        basepad = NULL;
        break;
    }
  }
  if (!basepad)
    goto error_no_basepad;

  for (item = GST_ELEMENT_PADS (conv), num = 0; item; item = g_list_next (item)) {
    if (GST_PAD_DIRECTION (GST_PAD (item->data)) == dir)
      ++num;
  }

  if (dir == GST_PAD_SINK) {
    name = g_strdup_printf ("sink_%u", num);
  } else {
    name = g_strdup_printf ("src_%u", num);
  }
  pad = gst_ghost_pad_new (name, basepad);
  gst_object_unref (basepad);
  g_free (name);

  gst_pad_set_active (pad, TRUE);
  if (gst_element_add_pad (GST_ELEMENT (conv), pad)) {
    GST_OBJECT_FLAG_SET (basepad, GST_CONV_BIN_PAD_FLAG_GHOSTED);

    GST_DEBUG_OBJECT (pad, "Requested %s.%s",
        GST_ELEMENT_NAME (baseconv), GST_PAD_NAME (basepad));

    /*
       INFO ("requested %s.%s on %s.%s",
       GST_ELEMENT_NAME (conv), GST_PAD_NAME (pad),
       GST_ELEMENT_NAME (baseconv), GST_PAD_NAME (basepad));
     */
  } else {
    gst_object_unref (pad);
  }

  gst_conv_bin_autosink (conv);
  GST_CONV_BIN_UNLOCK (conv);
  return pad;

  /* Handing Errors */
error_no_converter_name:
  {
    GST_ERROR_OBJECT (conv, "No underlayer converter name");
    return NULL;
  }

error_no_converter:
  {
    GST_CONV_BIN_UNLOCK (conv);
    GST_ERROR_OBJECT (conv, "No converter '%s'", conv->converter);
    return NULL;
  }

error_bin_add:
  {
    gst_object_unref (baseconv);
    GST_CONV_BIN_UNLOCK (conv);
    GST_ERROR_OBJECT (conv, "failed to add converter");
    return NULL;
  }

error_no_basepad:
  {
    gst_object_unref (baseconv);
    GST_CONV_BIN_UNLOCK (conv);
    GST_ERROR_OBJECT (conv, "No basepad for %s",
        GST_PAD_TEMPLATE_NAME_TEMPLATE (templ));
    return NULL;
  }
}

static void
gst_conv_bin_release_pad (GstElement * element, GstPad * pad)
{
  GstConvBin *conv = GST_CONV_BIN (element);
  GST_CONV_BIN_LOCK (conv);
  gst_pad_set_active (pad, FALSE);
  gst_element_remove_pad (element, pad);
  GST_CONV_BIN_UNLOCK (conv);
}

/*
static void
gst_conv_bin_state_changed (GstElement *element, GstState oldstate,
    GstState newstate, GstState pending)
{
  GstConvBin * conv = GST_CONV_BIN (element);
  GST_DEBUG_OBJECT (conv, "State changed: %d -> %d (%d)",
      oldstate, newstate, pending);
}
*/

static void
gst_conv_bin_class_init (GstConvBinClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  object_class->set_property =
      (GObjectSetPropertyFunc) gst_conv_bin_set_property;
  object_class->get_property =
      (GObjectGetPropertyFunc) gst_conv_bin_get_property;
  object_class->finalize = (GObjectFinalizeFunc) gst_conv_bin_finalize;

  g_object_class_install_property (object_class, PROP_AUTOSINK,
      g_param_spec_string ("autosink", "Autosink",
          "Specify the element name for autosink", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_CONVERTER,
      g_param_spec_string ("converter", "Base Converter",
          "Convert N inputs using a specified base converter.", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class,
      "Convert Bin", "Bin",
      "Convert N inputs base on a specified converter",
      "Duzy Chan <code@duzy.info>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_conv_bin_sink_factory));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_conv_bin_src_factory));

  element_class->request_new_pad = gst_conv_bin_request_new_pad;
  element_class->release_pad = gst_conv_bin_release_pad;
  //element_class->state_changed = gst_conv_bin_state_changed;

  GST_DEBUG_CATEGORY_INIT (gst_conv_bin_debug, "convbin", 0, "ConvertBin");
}
