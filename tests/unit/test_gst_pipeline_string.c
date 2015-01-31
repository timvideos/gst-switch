/* GstSwitch							    -*- c -*-
 * Copyright (C) 2015 Tim 'mithro' Ansell <mithro@mithis.com>
 * Copyright (C) 2015 David Nugent <deeprave@gmail.com>
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

#include "tools/gstcase.c"
#include <stdio.h>

gboolean verbose = FALSE;

// Dummy methods needed by gstcase.c
GstCaps *gst_switch_server_getcaps (void);
const gchar *
gst_switch_server_get_audio_caps_str (void)
{
  const gchar *caps = "AUDIO_CAPS";
  return caps;
}

const gchar *
gst_switch_server_get_video_caps_str (void)
{
  const gchar *caps = "VIDEO_CAPS";
  return caps;
}

gint
gst_composite_default_width ()
{
  return 100;
}

gint
gst_composite_default_height ()
{
  return 100;
}

/*
static gchar *results[GST_CASE__LAST_TYPE + 1] = {
  // GST_CASE_UNKNOWN
  NULL,
  // GST_CASE_COMPOSITE_VIDEO_A
  NULL,
  // GST_CASE_COMPOSITE_VIDEO_B
  NULL,
  // GST_CASE_COMPOSITE_AUDIO
  NULL,
  // GST_CASE_PREVIEW
  NULL,
  // GST_CASE_INPUT_AUDIO
  NULL,
  // GST_CASE_INPUT_VIDEO
  NULL,
  // GST_CASE_BRANCH_VIDEO_A
  NULL,
  // GST_CASE_BRANCH_VIDEO_B
  NULL,
  // GST_CASE_BRANCH_AUDIO
  NULL,
  // GST_CASE_BRANCH_PREVIEW
  NULL
};

static gchar *
expected_string (int case_type, int serve_type)
{
  switch (case_type) {
    case GST_CASE_PREVIEW:     // special case
      if (serve_type == GST_SERVE_VIDEO_STREAM)
        return "";
      return "";
    case GST_CASE_COMPOSITE_VIDEO_A:
    case GST_CASE_COMPOSITE_VIDEO_B:
    case GST_CASE_COMPOSITE_AUDIO:
    case GST_CASE_INPUT_AUDIO:
    case GST_CASE_INPUT_VIDEO:
    case GST_CASE_BRANCH_VIDEO_A:
    case GST_CASE_BRANCH_VIDEO_B:
    case GST_CASE_BRANCH_AUDIO:
    case GST_CASE_BRANCH_PREVIEW:
      return results[case_type];
  }
  return NULL;
}
*/

static GstCase *
new_case (int case_type, int serve_type)
{
  switch (serve_type) {
    case GST_SERVE_AUDIO_STREAM:
      return GST_CASE (g_object_new (GST_TYPE_CASE,
              "name", "audio", "type", case_type, "serve", serve_type,
              "port", 1234, NULL));
    case GST_SERVE_VIDEO_STREAM:
      return GST_CASE (g_object_new (GST_TYPE_CASE,
              "name", "video", "type", case_type, "serve", serve_type,
              "width", 640, "height", 480, "port", 1234, NULL));
  }
  return NULL;
}

static void
test_get_pipeline_string_input_audio (void)
{
  GstCase *cas = new_case (GST_CASE_INPUT_AUDIO, GST_SERVE_AUDIO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_INPUT_AUDIO, GST_SERVE_AUDIO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_INPUT_AUDIO: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_input_video (void)
{
  GstCase *cas = new_case (GST_CASE_INPUT_VIDEO, GST_SERVE_VIDEO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_INPUT_VIDEO, GST_SERVE_VIDEO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_INPUT_VIDEO: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_preview_video (void)
{
  GstCase *cas = new_case (GST_CASE_PREVIEW, GST_SERVE_VIDEO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_PREVIEW, GST_SERVE_VIDEO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_PREVIEW/V: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_preview_audio (void)
{
  GstCase *cas = new_case (GST_CASE_PREVIEW, GST_SERVE_AUDIO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_PREVIEW, GST_SERVE_AUDIO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_PREVIEW/A: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_composite_audio (void)
{
  GstCase *cas = new_case (GST_CASE_COMPOSITE_AUDIO, GST_SERVE_AUDIO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_COMPOSITE_AUDIO, GST_SERVE_AUDIO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_COMPOSITE_AUDIO: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_composite_video_a (void)
{
  GstCase *cas = new_case (GST_CASE_COMPOSITE_VIDEO_A, GST_SERVE_VIDEO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_COMPOSITE_VIDEO_A, GST_SERVE_VIDEO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_COMPOSITE_VIDEO_A: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_composite_video_b (void)
{
  GstCase *cas = new_case (GST_CASE_COMPOSITE_VIDEO_B, GST_SERVE_VIDEO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_COMPOSITE_VIDEO_B, GST_SERVE_VIDEO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_COMPOSITE_VIDEO_B: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_branch_video_a (void)
{
  GstCase *cas = new_case (GST_CASE_BRANCH_VIDEO_A, GST_SERVE_VIDEO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_BRANCH_VIDEO_A, GST_SERVE_VIDEO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_BRANCH_VIDEO_A: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_branch_video_b (void)
{
  GstCase *cas = new_case (GST_CASE_BRANCH_VIDEO_B, GST_SERVE_VIDEO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_BRANCH_VIDEO_B, GST_SERVE_VIDEO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_BRANCH_VIDEO_B: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_branch_preview (void)
{
  GstCase *cas = new_case (GST_CASE_BRANCH_PREVIEW, GST_SERVE_VIDEO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_BRANCH_PREVIEW, GST_SERVE_VIDEO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_BRANCH_PREVIEW: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

static void
test_get_pipeline_string_branch_audio (void)
{
  GstCase *cas = new_case (GST_CASE_BRANCH_AUDIO, GST_SERVE_AUDIO_STREAM);
  GString *desc = gst_case_get_pipeline_string (cas);
  g_assert (desc != NULL && strlen (desc->str) > 0);
//g_assert_cmpstr(expected_string(GST_CASE_BRANCH_AUDIO, GST_SERVE_AUDIO_STREAM), ==, desc->str);
  printf ("\nGST_CASE_BRANCH_AUDIO: %s\n", desc->str);
  g_string_free (desc, TRUE);
  g_object_unref (cas);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  gst_init (&argc, &argv);
  g_test_set_nonfatal_assertions ();
  g_test_add_func ("/gstswitch/server/gstcase/get_pipeline_string/INPUT/AUDIO",
      test_get_pipeline_string_input_audio);
  g_test_add_func ("/gstswitch/server/gstcase/get_pipeline_string/INPUT/VIDEO",
      test_get_pipeline_string_input_video);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/BRANCH/PREVIEW/AUDIO",
      test_get_pipeline_string_preview_audio);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/BRANCH/PREVIEW/VIDEO",
      test_get_pipeline_string_preview_video);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/COMPOSITE/AUDIO",
      test_get_pipeline_string_composite_audio);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/COMPOSITE/VIDEO_A",
      test_get_pipeline_string_composite_video_a);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/COMPOSITE/VIDEO_B",
      test_get_pipeline_string_composite_video_b);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/BRANCH/VIDEO_A",
      test_get_pipeline_string_branch_video_a);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/BRANCH/VIDEO_B",
      test_get_pipeline_string_branch_video_b);
  g_test_add_func ("/gstswitch/server/gstcase/get_pipeline_string/BRANCH/AUDIO",
      test_get_pipeline_string_branch_audio);
  g_test_add_func
      ("/gstswitch/server/gstcase/get_pipeline_string/BRANCH/PREVIEW",
      test_get_pipeline_string_branch_preview);
  return g_test_run ();
}
