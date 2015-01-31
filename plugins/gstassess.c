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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstassess.h"
#include "../logutils.h"

GST_DEBUG_CATEGORY_STATIC (gst_assess_debug);
#define GST_CAT_DEFAULT gst_assess_debug
#define ASSESS_NAME_LENGTH (8 * 5)

#define GST_ASSESS_LOCK(obj) (g_mutex_lock (&(obj)->lock))
#define GST_ASSESS_UNLOCK(obj) (g_mutex_unlock (&(obj)->lock))
#define ASSESS_DB_LOCK() (g_mutex_lock (&assess_db_lock))
#define ASSESS_DB_UNLOCK() (g_mutex_unlock (&assess_db_lock))
#define ASSESS_POINT_LOCK(ap) (g_mutex_lock (&(ap)->lock))
#define ASSESS_POINT_UNLOCK(ap) (g_mutex_unlock (&(ap)->lock))

/**
 * @class GstAssessPoint 
 * @struct _GstAssessPoint 
 * @brief Helper class for assessment.
 */
typedef struct _GstAssessPoint
{
  GMutex lock;
  guint number;
  guint sequence;
  gchar *name;
  GstClockTime ats;
  GstClockTime pts;
  GstClockTime dts;
  GstClockTime duration;
  guint64 running_time;         /* measured in milliseconds */
  guint64 offset, offset_end;
  guint64 buffer_count;
} GstAssessPoint;

/**
 * @deprecated Not Used!
 */
typedef struct _GstAssessDB
{
  GstClock *clock;
  guint timer;
  GHashTable *hash;
} GstAssessDB;

/**
 * @deprecated Not Used!
 */
typedef struct _GstAssessMeta
{
  GstMeta base;
  guint sequence;
} GstAssessMeta;

static GMutex assess_db_lock = { 0 };
static GstAssessDB assess_db = { 0 };

//static GType assess_meta_api;

static GstStaticPadTemplate gst_assess_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_assess_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_N,
};

#define gst_assess_parent_class parent_class
G_DEFINE_TYPE (GstAssess, gst_assess, GST_TYPE_BASE_TRANSFORM);

#define GST_ASSESS_META_IMPL "GstSwitchAssessMetaImpl"

/**
 * @deprecated
 */
gboolean
gst_assess_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GstAssessMeta *assess_meta = (GstAssessMeta *) meta;
  assess_meta->sequence = 0;
  INFO ("%s", __FUNCTION__);
  return TRUE;
}

/**
 * @deprecated
 */
void
gst_assess_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  //g_free (meta);
  INFO ("%s", __FUNCTION__);
}

/**
 * @deprecated
 */
gboolean
gst_assess_meta_transform (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data)
{
  //GstAssessMeta *assess_meta = (GstAssessMeta *) meta;
  INFO ("%s", __FUNCTION__);
  return TRUE;
}

/**
 * @deprecated
 */
static gint
assess_compare_name (gconstpointer a, gconstpointer b, gpointer user_data)
{
  GHashTable *assess_point_hash = user_data;
  GstAssessPoint *ap1 = g_hash_table_lookup (assess_point_hash, a);
  GstAssessPoint *ap2 = g_hash_table_lookup (assess_point_hash, b);
  if (ap1->number < ap2->number)
    return -1;
  if (ap1->number > ap2->number)
    return 1;
  if (ap1->pts < ap2->pts)
    return -1;
  if (ap1->pts > ap2->pts)
    return 1;
  return 0;
}

/**
 * @deprecated
 */
static gboolean
assess_db_timeout (gpointer data)
{
  gboolean ret = TRUE;
  GHashTable *assess_point_hash = NULL;
  GList *keys, *key, *names, *name;
  ASSESS_DB_LOCK ();
  keys = g_hash_table_get_keys (assess_db.hash);
  g_print ("========== %d pipelines ==========\n", g_list_length (keys));
  for (key = keys; key; key = g_list_next (key)) {
    const gchar *s = (gchar *) key->data, *ss = NULL;
    assess_point_hash = g_hash_table_lookup (assess_db.hash, s);
    names = g_hash_table_get_keys (assess_point_hash);
    names =
        g_list_sort_with_data (names, assess_compare_name, assess_point_hash);
    /*
       g_print ("%s\n", s);
     */
    for (name = names; name; name = g_list_next (name)) {
      GstAssessPoint *assess_point =
          g_hash_table_lookup (assess_point_hash, name->data);
      const int padlen = ASSESS_NAME_LENGTH - strlen (assess_point->name);
      if (0 < padlen)
        ss = g_strnfill (padlen, ' ');
      else
        ss = g_strdup ("");
      g_print ("\t%d\t%s%s\tats=%lld, " //"\t%d\t%s%s\tsequence=%d,ats=%lld, "
          "pts=%lld, dts=%lld, duration=%lld, "
          "buffers=%lld, time=%lldms, offset=%lld"
          "\n", assess_point->number, assess_point->name, ss,
          //assess_point->sequence,
          (long long int) (assess_point->ats /*/ GST_MSECOND */ ),
          (assess_point->pts == GST_CLOCK_TIME_NONE ? -1 :
              (long long int) (assess_point->pts /*/ GST_MSECOND */ )),
          (assess_point->dts == GST_CLOCK_TIME_NONE ? -1 :
              (long long int) (assess_point->dts /*/ GST_MSECOND */ )),
          (assess_point->duration == GST_CLOCK_TIME_NONE ? -1 :
              (long long int) (assess_point->duration /*/ GST_MSECOND */ )),
          (long long int) assess_point->buffer_count,
          (long long int) assess_point->running_time,
          (long long int) assess_point->offset);
      g_free ((gpointer) ss);
    }
    g_list_free (names);
  }
  g_list_free (keys);
  ASSESS_DB_UNLOCK ();
  return ret;
}

/**
 * @deprecated
 */
static void
gst_assess_init (GstAssess * assess)
{
  g_mutex_init (&assess->lock);
}

/**
 * @deprecated
 */
static void
gst_assess_dispose (GstAssess * assess)
{
  GstElement *pipeline = NULL;
  GHashTable *assess_point_hash = NULL;

  pipeline = GST_ELEMENT (gst_element_get_parent (GST_ELEMENT (assess)));

  if (!pipeline) {
    //ERROR ("fix me");
    goto end;
  }

  ASSESS_DB_LOCK ();

  if (assess_db.hash) {
    assess_point_hash = g_hash_table_lookup (assess_db.hash,
        GST_ELEMENT_NAME (pipeline));
    if (assess_point_hash) {
      g_hash_table_remove (assess_point_hash, GST_ELEMENT_NAME (assess));
    }
  }

  ASSESS_DB_UNLOCK ();

end:
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (assess));
}

/**
 * @deprecated
 */
static void
gst_assess_finalize (GstAssess * assess)
{
  g_mutex_clear (&assess->lock);

  G_OBJECT_CLASS (parent_class)->finalize (G_OBJECT (assess));
}

/**
 * @deprecated
 */
static void
gst_assess_set_property (GstAssess * assess, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_N:
      assess->number = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (assess), prop_id, pspec);
      break;
  }
}

/**
 * @deprecated
 */
static void
gst_assess_get_property (GstAssess * assess, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_N:
      g_value_set_uint (value, assess->number);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (G_OBJECT (assess), prop_id, pspec);
      break;
  }
}

/**
 * @deprecated
 */
static gboolean
gst_assess_start (GstBaseTransform * trans)
{
  GstAssess *this = GST_ASSESS (trans);
  (void) this;
  return TRUE;
}

/**
 * @deprecated
 */
static gboolean
gst_assess_stop (GstBaseTransform * trans)
{
  GstAssess *this = GST_ASSESS (trans);
  (void) this;
  return TRUE;
}

/**
 * @deprecated
 */
static GstFlowReturn
gst_assess_transform (GstBaseTransform * trans, GstBuffer * buffer)
{
  GstAssess *this = NULL;
  GstElement *pipeline = NULL;
  GstClock *pipeline_clock = NULL;
  GstClockTime ats;
  GstFlowReturn ret;
  GHashTable *assess_point_hash = NULL;
  GstAssessPoint *assess_point = NULL;
  //GstMeta *assess_meta = NULL;

  this = GST_ASSESS (trans);
  pipeline = GST_ELEMENT (gst_element_get_parent (GST_ELEMENT (trans)));
  pipeline_clock = gst_pipeline_get_clock (GST_PIPELINE (pipeline));

  ASSESS_DB_LOCK ();

  assess_point_hash = g_hash_table_lookup (assess_db.hash,
      GST_ELEMENT_NAME (pipeline));
  if (assess_point_hash == NULL) {
    assess_point_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
        g_free, g_free);
    g_hash_table_insert (assess_db.hash,
        g_strdup (GST_ELEMENT_NAME (pipeline)), assess_point_hash);
  }

  assess_point = g_hash_table_lookup (assess_point_hash,
      GST_ELEMENT_NAME (this));
  if (assess_point == NULL) {
    assess_point = g_new0 (GstAssessPoint, 1);
    assess_point->number = this->number;
    assess_point->name = g_strdup (GST_ELEMENT_NAME (this));
    g_hash_table_insert (assess_point_hash, assess_point->name, assess_point);
    g_mutex_init (&assess_point->lock);
  }

  ats = gst_clock_get_time (assess_db.clock);

  ASSESS_DB_UNLOCK ();

  if (!assess_point) {
    goto end;
  }

  ASSESS_POINT_LOCK (assess_point);

  //assess_meta = gst_buffer_get_meta (buffer, GST_TYPE_ASSESS);

  assess_point->ats = ats;
  assess_point->pts = GST_BUFFER_PTS (buffer);
  assess_point->buffer_count += 1;
  assess_point->offset = GST_BUFFER_OFFSET (buffer);
  assess_point->offset_end += GST_BUFFER_OFFSET_END (buffer);
  assess_point->duration = GST_BUFFER_DURATION (buffer);

  if (GST_CLOCK_TIME_NONE != GST_BUFFER_DURATION (buffer)) {
    assess_point->running_time += GST_BUFFER_DURATION (buffer) / GST_MSECOND;
  }

  ASSESS_POINT_UNLOCK (assess_point);

end:
  gst_object_unref (pipeline_clock);
  ret = GST_FLOW_OK;
  return ret;
}

/**
 * @deprecated
 */
static gboolean
gst_assess_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstAssess *this = GST_ASSESS (trans);
  gboolean ret = TRUE;
  (void) this;
  ret = GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);
  return ret;
}

/**
 * @deprecated
 */
static GstStateChangeReturn
gst_assess_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstAssess *this = GST_ASSESS (element);

  (void) this;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      //gst_assess_reset (this);
      break;
    default:
      break;
  }

  return ret;
}

/**
 * @deprecated
 */
static void
gst_assess_class_init (GstAssessClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *basetrans_class = GST_BASE_TRANSFORM_CLASS (klass);

  object_class->set_property = (GObjectSetPropertyFunc) gst_assess_set_property;
  object_class->get_property = (GObjectGetPropertyFunc) gst_assess_get_property;
  object_class->finalize = (GObjectFinalizeFunc) gst_assess_finalize;
  object_class->dispose = (GObjectFinalizeFunc) gst_assess_dispose;

  g_object_class_install_property (object_class, PROP_N,
      g_param_spec_uint ("n", "N", "Number",
          0, (guint) - 1, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class,
      "Stream Assessment", "Element",
      "Assess streams for frame drop and latency",
      "Duzy Chan <code@duzy.info>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_assess_sink_factory));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_assess_src_factory));

  //element_class->request_new_pad = gst_assess_request_new_pad;
  //element_class->release_pad = gst_assess_release_pad;
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_assess_change_state);

  basetrans_class->sink_event = GST_DEBUG_FUNCPTR (gst_assess_sink_event);
  basetrans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_assess_transform);
  basetrans_class->start = GST_DEBUG_FUNCPTR (gst_assess_start);
  basetrans_class->stop = GST_DEBUG_FUNCPTR (gst_assess_stop);

  if (assess_db.hash == NULL) {
    ASSESS_DB_LOCK ();
    /*
       const gchar *tags[] = { "assess", NULL };
       assess_meta_api = gst_meta_api_type_register ("GstSwitchAssessMeta",
       &tags[0]);
       gst_meta_register (assess_meta_api, GST_ASSESS_META_IMPL,
       sizeof (GstAssessMeta), gst_assess_meta_init,
       gst_assess_meta_free, gst_assess_meta_transform);
     */

    if (assess_db.hash == NULL) {
      assess_db.clock = gst_system_clock_obtain ();
      assess_db.hash = g_hash_table_new_full (g_str_hash, g_str_equal,
          g_free, (GDestroyNotify) g_hash_table_destroy);
      assess_db.timer = g_timeout_add (5000,
          (GSourceFunc) assess_db_timeout, NULL);
    }
    ASSESS_DB_UNLOCK ();
  }

  GST_DEBUG_CATEGORY_INIT (gst_assess_debug, "assess", 0, "Assess");
}
