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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstswitch.h"
#include "../logutils.h"

GST_DEBUG_CATEGORY_STATIC (gst_switch_debug);
#define GST_CAT_DEFAULT gst_switch_debug

#define GST_SWITCH_LOCK(obj) (g_mutex_lock (&(obj)->lock))
#define GST_SWITCH_UNLOCK(obj) (g_mutex_unlock (&(obj)->lock))

/**
 *  The Case Element has to be a N-to-1 fitter, and must always have a static
 *  "src" pad, e.g. funnel.
 *
 *  TODO: make it configurable, e.g. case=funnel.
 */
#define CASE_ELEMENT_NAME "identity"

static GstStaticPadTemplate gst_switch_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_switch_src_factory =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_CASES,
};

enum
{
  GST_SWITCH_PAD_FLAG_GHOSTED = (GST_PAD_FLAG_LAST << 1),
  GST_SWITCH_PAD_FLAG_LAST = (GST_PAD_FLAG_LAST << 2)
};

G_DEFINE_TYPE (GstSwitch, gst_switch, GST_TYPE_BIN);

static void
gst_switch_init (GstSwitch * swit)
{
  swit->cases_string = NULL;

  g_mutex_init (&swit->lock);
}

static void
gst_switch_set_property (GstSwitch * swit, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_CASES:
      g_free (swit->cases_string);
      swit->cases_string = g_strdup (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (swit), prop_id, pspec);
      break;
  }
}

static void
gst_switch_get_property (GstSwitch * swit, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_CASES:
      g_value_set_string (value, swit->cases_string);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (swit), prop_id, pspec);
      break;
  }
}

static void
gst_switch_finalize (GstSwitch * swit)
{
  if (swit->cases_string) {
    g_free (swit->cases_string);
    swit->cases_string = NULL;
  }

  g_mutex_clear (&swit->lock);

  G_OBJECT_CLASS (swit)->finalize (G_OBJECT (swit));
}

static GstElement *
gst_switch_request_new_case (GstSwitch * swit, GstPadTemplate * templ,
    const GstCaps * caps)
{
  GstElement *swcase = NULL;
  gchar *name;

  name = g_strdup_printf ("case_%d", GST_BIN_NUMCHILDREN (GST_BIN (swit)));
  swcase = gst_element_factory_make (CASE_ELEMENT_NAME, name);
  g_free (name);

  if (!gst_bin_add (GST_BIN (swit), swcase))
    goto error_bin_add_case;

  INFO ("new %s for %s.%s (%d cases)", GST_ELEMENT_NAME (swcase),
      GST_ELEMENT_NAME (swit), GST_PAD_TEMPLATE_NAME_TEMPLATE (templ),
      GST_BIN_NUMCHILDREN (GST_BIN (swit)));

  return swcase;

error_bin_add_case:
  {
    GST_ERROR_OBJECT (swit, "Bin add failed for %s.%s",
        GST_ELEMENT_NAME (swit), GST_PAD_TEMPLATE_NAME_TEMPLATE (templ));
    if (swcase)
      gst_object_unref (GST_OBJECT (swcase));
    return NULL;
  }
}

static GstGhostPad *
gst_switch_request_new_src_pad (GstSwitch * swit,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstGhostPad *pad = NULL;
  GstPad *basepad = NULL;
  GstElement *swcase;

  swcase = gst_switch_request_new_case (swit, templ, caps);
  if (!swcase)
    goto error_no_case;

  basepad = gst_element_get_static_pad (swcase, "src");
  if (!basepad)
    goto error_no_basepad;

  if (gst_pad_is_linked (basepad))
    goto error_basepad_already_linked;

  if (name) {
    pad = GST_GHOST_PAD (gst_ghost_pad_new (name, basepad));
  } else {
    name = g_strdup_printf ("src_%u", GST_ELEMENT (swit)->numsrcpads);
    pad = GST_GHOST_PAD (gst_ghost_pad_new (name, basepad));
    g_free ((gchar *) name);
  }

  gst_object_unref (basepad);
  return pad;

error_no_case:
  {
    GST_ERROR_OBJECT (swit, "Failed to request new case for %s.%s",
        GST_ELEMENT_NAME (swit), GST_PAD_TEMPLATE_NAME_TEMPLATE (templ));
    return NULL;
  }

error_no_basepad:
  {
    GST_ERROR_OBJECT (swit, "Failed to request new pad on %s",
        GST_ELEMENT_NAME (swcase));
    return NULL;
  }

error_basepad_already_linked:
  {
    GstPad *pp = GST_PAD_PEER (basepad);
    GstElement *ppp = GST_PAD_PARENT (pp);
    GST_ERROR_OBJECT (swit, "Pad %s.%s already linked with %s.%s",
        GST_ELEMENT_NAME (swcase), GST_PAD_NAME (basepad),
        GST_ELEMENT_NAME (ppp), GST_PAD_NAME (pp));

    gst_object_unref (basepad);
    return NULL;
  }
}

static GstPad *
gst_switch_get_case_sink_pad (GstElement * swcase, const GstCaps * caps)
{
  GstPad *basepad = gst_element_get_static_pad (swcase, "sink");
  if (!basepad) {
    gint num = GST_ELEMENT (swcase)->numsinkpads;
    gchar *name = g_strdup_printf ("sink_%u", num);
    basepad = gst_element_request_pad (swcase,
        gst_static_pad_template_get (&gst_switch_sink_factory), name, caps);
    g_free (name);
  }
  return basepad;
}

static GstElement *
gst_switch_select (GstSwitch * swit,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GList *item = GST_BIN_CHILDREN (GST_BIN (swit));
  GstElement *swcase = NULL;

  for (; item; item = g_list_next (item)) {
    GList *paditem = GST_ELEMENT_PADS (GST_ELEMENT (item->data));
    GstPad *basepad = NULL, *pad = NULL;
    for (; paditem; paditem = g_list_next (paditem)) {
      pad = GST_PAD (paditem->data);
      if (GST_PAD_IS_SINK (pad) && !gst_pad_is_linked (pad) &&
          !GST_OBJECT_FLAG_IS_SET (GST_OBJECT (pad),
              GST_SWITCH_PAD_FLAG_GHOSTED)) {
        basepad = pad;
        break;
      }
    }

    if (basepad) {
      swcase = GST_ELEMENT (item->data);
      break;
    }
  }

  if (!swcase) {
    swcase = gst_switch_request_new_case (swit, templ, caps);
  }

  return swcase;
}

static GstGhostPad *
gst_switch_request_new_sink_pad (GstSwitch * swit,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstElement *swcase = gst_switch_select (swit, templ, name, caps);
  GstGhostPad *pad = NULL;
  GstPad *basepad = NULL;

  if (!swcase)
    goto error_no_case;

  basepad = gst_switch_get_case_sink_pad (swcase, caps);

  if (!basepad)
    goto error_no_basepad;

  if (gst_pad_is_linked (basepad))
    goto error_basepad_already_linked;

  if (name) {
    pad = GST_GHOST_PAD (gst_ghost_pad_new (name, basepad));
  } else {
    name = g_strdup_printf ("sink_%u", GST_ELEMENT (swit)->numsinkpads);
    pad = GST_GHOST_PAD (gst_ghost_pad_new (name, basepad));
    g_free ((gchar *) name);
  }

  gst_object_unref (basepad);
  return pad;

error_no_case:
  {
    GST_ERROR_OBJECT (swit, "Failed to request new case for %s.%s",
        GST_ELEMENT_NAME (swit), GST_PAD_TEMPLATE_NAME_TEMPLATE (templ));
    return NULL;
  }

error_no_basepad:
  {
    GST_ERROR_OBJECT (swit, "Failed to request new pad on %s",
        GST_ELEMENT_NAME (swcase));
    return NULL;
  }

error_basepad_already_linked:
  {
    GstPad *pp = GST_PAD_PEER (basepad);
    GstElement *ppp = GST_PAD_PARENT (pp);
    GST_ERROR_OBJECT (swit, "Pad %s.%s already linked with %s.%s",
        GST_ELEMENT_NAME (swcase), GST_PAD_NAME (basepad),
        GST_ELEMENT_NAME (ppp), GST_PAD_NAME (pp));

    gst_object_unref (basepad);
    return NULL;
  }
}

static GstPad *
gst_switch_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstSwitch *swit = GST_SWITCH (element);
  GstGhostPad *pad;

  INFO ("requesting: %s.%s (%d=%d+%d)", GST_ELEMENT_NAME (swit),
      name ? name : GST_PAD_TEMPLATE_NAME_TEMPLATE (templ),
      element->numpads, element->numsrcpads, element->numsinkpads);

  GST_SWITCH_LOCK (swit);

  switch (GST_PAD_TEMPLATE_DIRECTION (templ)) {
    case GST_PAD_SRC:
      pad = gst_switch_request_new_src_pad (swit, templ, name, caps);
      break;
    case GST_PAD_SINK:
      pad = gst_switch_request_new_sink_pad (swit, templ, name, caps);
      break;
    default:
      pad = NULL;
      break;
  }

  if (pad) {
    gst_pad_set_active (GST_PAD (pad), TRUE);

    if (gst_element_add_pad (GST_ELEMENT (swit), GST_PAD (pad))) {
      GstPad *basepad = gst_ghost_pad_get_target (pad);
      GstElement *swcase = GST_ELEMENT (GST_PAD_PARENT (basepad));

      GST_OBJECT_FLAG_SET (basepad, GST_SWITCH_PAD_FLAG_GHOSTED);

      GST_DEBUG_OBJECT (swit, "New %s:%s on %s:%s",
          GST_ELEMENT_NAME (swit), GST_PAD_NAME (pad),
          GST_ELEMENT_NAME (swcase), GST_PAD_NAME (basepad));

      INFO ("requested %s.%s on %s.%s",
          GST_ELEMENT_NAME (swit), GST_PAD_NAME (pad),
          GST_ELEMENT_NAME (swcase), GST_PAD_NAME (basepad));
    }
  }

  GST_SWITCH_UNLOCK (swit);

  return GST_PAD (pad);
}

static void
gst_switch_release_pad (GstElement * element, GstPad * pad)
{
  GstSwitch *swit = GST_SWITCH (element);
  GST_SWITCH_LOCK (swit);
  gst_pad_set_active (pad, FALSE);
  gst_element_remove_pad (element, pad);
  GST_SWITCH_UNLOCK (swit);
}

static void
gst_switch_class_init (GstSwitchClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  object_class->set_property = (GObjectSetPropertyFunc) gst_switch_set_property;
  object_class->get_property = (GObjectGetPropertyFunc) gst_switch_get_property;
  object_class->finalize = (GObjectFinalizeFunc) gst_switch_finalize;

  g_object_class_install_property (object_class, PROP_CASES,
      g_param_spec_string ("cases", "Switch Cases",
          "The cases for switching", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class,
      "Stream Switch", "Element",
      "Switch within streams", "Duzy Chan <code@duzy.info>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_switch_sink_factory));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_switch_src_factory));

  element_class->request_new_pad = gst_switch_request_new_pad;
  element_class->release_pad = gst_switch_release_pad;

  GST_DEBUG_CATEGORY_INIT (gst_switch_debug, "switch", 0, "Switch");
}
