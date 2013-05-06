/* GstSwitch							    -*- c -*-
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gst/video/videooverlay.h>
#include <gst/speakertrack/gstcamcontrol.h>
#include <stdlib.h>
#include "gstswitchptz.h"

gboolean verbose;

G_DEFINE_TYPE (GstSwitchPTZ, gst_switch_ptz, GST_TYPE_WORKER);

static void
gst_switch_ptz_quit (GstSwitchPTZ * ptz)
{
  gtk_main_quit ();
}

static void
gst_switch_ptz_window_closed (GtkWidget * widget, GdkEvent * event,
    gpointer data)
{
  GstSwitchPTZ *ptz = GST_SWITCH_PTZ (data);
  gst_switch_ptz_quit (ptz);
}

static void
gst_switch_ptz_button_clicked_left (GtkButton * button, GstSwitchPTZ * ptz)
{
  ptz->x -= 5;
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_pan_speed), ptz->x);
  g_print ("left: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_left_top (GtkButton * button, GstSwitchPTZ * ptz)
{
  ptz->x -= 5;
  ptz->y -= 5;
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_pan_speed), ptz->x);
  gst_cam_controller_tilt (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_tilt_speed), ptz->y);
  g_print ("left-top: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_top (GtkButton * button, GstSwitchPTZ * ptz)
{
  ptz->y -= 5;
  gst_cam_controller_tilt (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_tilt_speed), ptz->y);
  g_print ("top: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_top_right (GtkButton * button, GstSwitchPTZ * ptz)
{
  ptz->x += 5;
  ptz->y -= 5;
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_pan_speed), ptz->x);
  gst_cam_controller_tilt (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_tilt_speed), ptz->y);
  g_print ("top-right: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_right (GtkButton * button, GstSwitchPTZ * ptz)
{
  ptz->x += 5;
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_pan_speed), ptz->x);
  g_print ("right: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_right_bottom (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  ptz->x += 5;
  ptz->y += 5;
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_pan_speed), ptz->x);
  gst_cam_controller_tilt (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_tilt_speed), ptz->y);
  g_print ("right-bottom: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_bottom (GtkButton * button, GstSwitchPTZ * ptz)
{
  ptz->y += 5;
  gst_cam_controller_tilt (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_tilt_speed), ptz->y);
  g_print ("bottom: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_bottom_left (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  ptz->x -= 5;
  ptz->y += 5;
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_pan_speed), ptz->x);
  gst_cam_controller_tilt (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_tilt_speed), ptz->y);
  g_print ("bottom-left: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_button_clicked_center (GtkButton * button, GstSwitchPTZ * ptz)
{
  ptz->x = 0;
  ptz->y = 0;
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_pan_speed), ptz->x);
  gst_cam_controller_tilt (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_tilt_speed), ptz->y);
  g_print ("center: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_zoom_changed (GtkAdjustment * adjustment, GstSwitchPTZ * ptz)
{
  ptz->z = (gint) gtk_adjustment_get_value (adjustment);
  gst_cam_controller_pan (ptz->controller,
      (gint) gtk_adjustment_get_value (ptz->adjust_zoom_speed), ptz->z);
  g_print ("zoom: (%d, %d, %d)\n", ptz->x, ptz->y, ptz->z);
}

static void
gst_switch_ptz_zoom_speed_changed (GtkAdjustment * adjustment,
    GstSwitchPTZ * ptz)
{
}

static void
gst_switch_ptz_pan_speed_changed (GtkAdjustment * adjustment,
    GstSwitchPTZ * ptz)
{
}

static void
gst_switch_ptz_tilt_speed_changed (GtkAdjustment * adjustment,
    GstSwitchPTZ * ptz)
{
}

static void
gst_switch_ptz_init (GstSwitchPTZ * ptz)
{
  GtkWidget *box_main, *box_video, *box_control;
  GtkWidget *box_control_pan, *box_control_tilt, *box_control_zoom;
  GtkWidget *control_grid;
  GtkWidget *control_buttons[3][3] = { {NULL} };
  GtkWidget *scrollwin;
  GtkWidget *scale_zoom;
  GtkWidget *scale_pan_speed, *scale_tilt_speed, *scale_zoom_speed;
  GtkWidget *label_pan_speed, *label_tilt_speed, *label_zoom_speed;
  const gchar *control_labels[3][3] = {
    {"  \\  ", "  ^  ", "  /  "},
    {"  <  ", "  *  ", "  >  "},
    {"  /  ", "  v  ", "  \\  "},
  };
  int n, m;

  ptz->x = ptz->y = ptz->z = 0;
  ptz->controller = gst_cam_controller_new ("visca");

  ptz->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (ptz->window), 640, 480);
  gtk_window_set_title (GTK_WINDOW (ptz->window), "PTZ Control");
  g_signal_connect (G_OBJECT (ptz->window), "delete-event",
      G_CALLBACK (gst_switch_ptz_window_closed), ptz);

  box_main = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  box_video = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  box_control = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scrollwin, 230, -1);
  gtk_widget_set_vexpand (scrollwin, TRUE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwin),
      box_control);

  gtk_box_pack_start (GTK_BOX (box_main), box_video, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_main), scrollwin, FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (ptz->window), box_main);
  gtk_container_set_border_width (GTK_CONTAINER (ptz->window), 5);

  ptz->video_view = gtk_drawing_area_new ();
  gtk_widget_set_name (ptz->video_view, "video");
  gtk_widget_set_double_buffered (ptz->video_view, FALSE);
  gtk_widget_set_hexpand (ptz->video_view, TRUE);
  gtk_widget_set_vexpand (ptz->video_view, TRUE);
  gtk_widget_set_events (ptz->video_view, GDK_EXPOSURE_MASK
      | GDK_LEAVE_NOTIFY_MASK
      | GDK_BUTTON_PRESS_MASK
      | GDK_BUTTON_RELEASE_MASK
      | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
  gtk_box_pack_start (GTK_BOX (box_video), ptz->video_view, TRUE, TRUE, 0);

  control_grid = gtk_grid_new ();
  gtk_grid_insert_row (GTK_GRID (control_grid), 0);
  gtk_grid_insert_row (GTK_GRID (control_grid), 1);
  gtk_grid_insert_row (GTK_GRID (control_grid), 2);
  gtk_grid_insert_column (GTK_GRID (control_grid), 0);
  gtk_grid_insert_column (GTK_GRID (control_grid), 1);
  gtk_grid_insert_column (GTK_GRID (control_grid), 2);
  for (n = 0; n < 3; ++n) {
    for (m = 0; m < 3; ++m) {
      GtkWidget *btn = control_buttons[m][n] = gtk_button_new ();
      gtk_grid_attach (GTK_GRID (control_grid), btn, n, m, 1, 1);
      gtk_widget_set_size_request (scrollwin, 30, 30);
      gtk_button_set_label (GTK_BUTTON (btn), control_labels[m][n]);
    }
  }
  gtk_box_pack_start (GTK_BOX (box_control), control_grid, FALSE, TRUE, 0);

  ptz->adjust_zoom = gtk_adjustment_new (50, 0, 100, 1, 1, 5);
  scale_zoom = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, ptz->adjust_zoom);
  gtk_box_pack_start (GTK_BOX (box_control), scale_zoom, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control), gtk_label_new (""), TRUE, TRUE, 0);

  ptz->adjust_pan_speed = gtk_adjustment_new (50, 0, 100, 1, 1, 5);
  ptz->adjust_tilt_speed = gtk_adjustment_new (50, 0, 100, 1, 1, 5);
  ptz->adjust_zoom_speed = gtk_adjustment_new (50, 0, 100, 1, 1, 5);
  scale_pan_speed =
      gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, ptz->adjust_pan_speed);
  scale_tilt_speed =
      gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, ptz->adjust_tilt_speed);
  scale_zoom_speed =
      gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, ptz->adjust_zoom_speed);
  label_pan_speed = gtk_label_new ("P:");
  label_tilt_speed = gtk_label_new ("T:");
  label_zoom_speed = gtk_label_new ("Z:");
  box_control_pan = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  box_control_tilt = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  box_control_zoom = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box_control), gtk_label_new ("Speeds:"), FALSE,
      TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control_pan), label_pan_speed, FALSE, FALSE,
      0);
  gtk_box_pack_start (GTK_BOX (box_control_pan), scale_pan_speed, TRUE, TRUE,
      0);
  gtk_box_pack_start (GTK_BOX (box_control_tilt), label_tilt_speed, FALSE,
      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box_control_tilt), scale_tilt_speed, TRUE, TRUE,
      0);
  gtk_box_pack_start (GTK_BOX (box_control_zoom), label_zoom_speed, FALSE,
      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box_control_zoom), scale_zoom_speed, TRUE, TRUE,
      0);
  gtk_box_pack_start (GTK_BOX (box_control), box_control_pan, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control), box_control_tilt, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control), box_control_zoom, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control), gtk_label_new (""), TRUE, TRUE, 0);

  /*
     g_signal_connect (G_OBJECT (scale_pan_speed), "value-changed",
     G_CALLBACK (gst_switch_ptz_window_closed), ptz);
   */
  g_signal_connect (ptz->adjust_pan_speed, "value-changed",
      G_CALLBACK (gst_switch_ptz_pan_speed_changed), ptz);
  g_signal_connect (ptz->adjust_tilt_speed, "value-changed",
      G_CALLBACK (gst_switch_ptz_tilt_speed_changed), ptz);
  g_signal_connect (ptz->adjust_zoom_speed, "value-changed",
      G_CALLBACK (gst_switch_ptz_zoom_speed_changed), ptz);
  g_signal_connect (ptz->adjust_zoom, "value-changed",
      G_CALLBACK (gst_switch_ptz_zoom_changed), ptz);

  g_signal_connect (control_buttons[0][0], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_left_top), ptz);
  g_signal_connect (control_buttons[0][1], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_top), ptz);
  g_signal_connect (control_buttons[0][2], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_top_right), ptz);
  g_signal_connect (control_buttons[1][0], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_left), ptz);
  g_signal_connect (control_buttons[1][1], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_center), ptz);
  g_signal_connect (control_buttons[1][2], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_right), ptz);
  g_signal_connect (control_buttons[2][0], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_bottom_left), ptz);
  g_signal_connect (control_buttons[2][1], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_bottom), ptz);
  g_signal_connect (control_buttons[2][2], "clicked",
      G_CALLBACK (gst_switch_ptz_button_clicked_right_bottom), ptz);
}

static void
gst_switch_ptz_fini (GstSwitchPTZ * ptz)
{
  if (G_OBJECT_CLASS (gst_switch_ptz_parent_class)->finalize)
    (*G_OBJECT_CLASS (gst_switch_ptz_parent_class)->finalize) (G_OBJECT (ptz));
}

static gboolean
gst_switch_ptz_prepare (GstSwitchPTZ * ptz)
{
  GstWorker *worker = GST_WORKER (ptz);
  GstElement *sink = gst_worker_get_element_unlocked (worker, "sink");
  gulong handle = GDK_WINDOW_XID (gtk_widget_get_window (ptz->video_view));
  g_return_val_if_fail (GST_IS_ELEMENT (sink), FALSE);
  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink), handle);
  gst_object_unref (sink);
  return TRUE;
}

static GString *
gst_switch_ptz_get_pipeline (GstSwitchPTZ * ptz)
{
  GString *desc;
  const gchar *device = "/dev/video0";

  desc = g_string_new ("");

  g_string_append_printf (desc, "v4l2src device=%s ", device);
  g_string_append_printf (desc, "! videoconvert ");
  g_string_append_printf (desc, "! xvimagesink name=sink ");
  return desc;
}

static void
gst_switch_ptz_run (GstSwitchPTZ * ptz)
{
  gtk_widget_show_all (ptz->window);
  gst_worker_start (GST_WORKER (ptz));
  gtk_main ();
}

static int
gst_switch_ptz_main (int argc, char **argv)
{
  GstSwitchPTZ *ptz = GST_SWITCH_PTZ (g_object_new (GST_TYPE_SWITCH_PTZ, NULL));

  gst_switch_ptz_run (ptz);

  g_object_unref (G_OBJECT (ptz));
  return 0;
}

static void
gst_switch_ptz_class_init (GstSwitchPTZClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstWorkerClass *worker_class = GST_WORKER_CLASS (klass);
  object_class->finalize = (GObjectFinalizeFunc) gst_switch_ptz_fini;
  worker_class->prepare = (GstWorkerPrepareFunc) gst_switch_ptz_prepare;
  worker_class->get_pipeline_string = (GstWorkerGetPipelineStringFunc)
      gst_switch_ptz_get_pipeline;
}

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL},
  {NULL}
};

static void
gst_switch_ptz_args (int *argc, char **argv[])
{
  GOptionContext *context;
  GError *error = NULL;
  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, "gst-switch-ptz");
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, argc, argv, &error)) {
    g_print ("option parsing failed: %s\n", error->message);
    exit (1);
  }
  g_option_context_free (context);
}

int
main (int argc, char **argv)
{
  gst_switch_ptz_args (&argc, &argv);
  gtk_init (&argc, &argv);
  return gst_switch_ptz_main (argc, argv);
}