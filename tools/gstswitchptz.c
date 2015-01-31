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
#include <gst/video/videooverlay.h>
#include <gst/speakertrack/gstcamcontrol.h>
#include <stdlib.h>
#include "gstswitchptz.h"
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <glib/gprintf.h>

gboolean verbose;
const char *ptz_device_name = "/dev/ttyUSB0";
const char *ptz_video_name = "/dev/video0";
const char *ptz_control_protocol = "visca";

static double zoom_step = 0.0;
static gboolean do_update = FALSE;
static gboolean button_pressed = FALSE;
static struct timeval timestamp_pan_changed = { 0, 0 };
static struct timeval timestamp_tilt_changed = { 0, 0 };
static struct timeval timestamp_zoom_changed = { 0, 0 };

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

/*
static void
gst_switch_ptz_fix_coords (GstSwitchPTZ * ptz)
{
  if (ptz->x < ptz->controller->pan_min)
    ptz->x = ptz->controller->pan_min;
  if (ptz->controller->pan_max < ptz->x)
    ptz->x = ptz->controller->pan_max;

  if (ptz->y < ptz->controller->tilt_min)
    ptz->y = ptz->controller->tilt_min;
  if (ptz->controller->tilt_max < ptz->y)
    ptz->y = ptz->controller->tilt_max;

  if (ptz->z < 0.0)
    ptz->z = 0.0;
  if (1.0 < ptz->z)
    ptz->z = 1.0;
}
*/

 /*
    static gboolean
    gst_switch_ptz_update_d (GstSwitchPTZ * ptz)
    {
    const double d = 0.0001;
    if (button_pressed && d < fabs (ptz->dx))
    ptz->x += ptz->dx;
    if (button_pressed && d < fabs (ptz->dy))
    ptz->y += ptz->dy;
    if (button_pressed && d < fabs (ptz->dz))
    ptz->z += ptz->dz;
    gst_switch_ptz_fix_coords (ptz);
    return TRUE;
    }
  */

static gboolean
gst_switch_ptz_set_update_flag (void *arg)
{
  do_update = button_pressed;
  return FALSE;
}

static gboolean
gst_switch_ptz_update_xy (GstSwitchPTZ * ptz)
{
  if (do_update) {
    double x, y;
    gst_cam_controller_query (ptz->controller, &x, &y);
    gtk_adjustment_set_value (ptz->adjust_pan, x);
    gtk_adjustment_set_value (ptz->adjust_tilt, y);
    if (0.001 < fabs (zoom_step)) {
      double zoom = gtk_adjustment_get_value (ptz->adjust_zoom) + zoom_step;
      gst_cam_controller_zoom (ptz->controller,
          gtk_adjustment_get_value (ptz->adjust_zoom_speed), zoom);
      gtk_adjustment_set_value (ptz->adjust_zoom, zoom);
    }
  }
  return TRUE;
}

static gboolean
gst_switch_ptz_update (GstSwitchPTZ * ptz)
{
  static double pan, tilt, zoom;        //, pan_speed, tilt_speed, zoom_speed;
  static struct timeval timestamp = { 0, 0 };
  static struct timeval timestamp_z = { 0, 0 };
  struct timeval now = { 0 };
  suseconds_t millis, millis_z;
  suseconds_t millis_pan, millis_tilt, millis_zoom;
  const double d = 0.001;
  gettimeofday (&now, NULL);

  if (timestamp.tv_usec == 0 || do_update) {
    timestamp = timestamp_z = now;
    pan = gtk_adjustment_get_value (ptz->adjust_pan);
    tilt = gtk_adjustment_get_value (ptz->adjust_tilt);
    zoom = gtk_adjustment_get_value (ptz->adjust_zoom);
    return TRUE;
  }

  millis =
      (now.tv_sec - timestamp.tv_sec) * 1000 + (now.tv_usec -
      timestamp.tv_usec) / 1000;
  millis_z =
      (now.tv_sec - timestamp_z.tv_sec) * 1000 + (now.tv_usec -
      timestamp_z.tv_usec) / 1000;

  millis_pan = (now.tv_sec - timestamp_pan_changed.tv_sec) * 1000 +
      (now.tv_usec - timestamp_pan_changed.tv_usec) / 1000;
  millis_tilt = (now.tv_sec - timestamp_tilt_changed.tv_sec) * 1000 +
      (now.tv_usec - timestamp_tilt_changed.tv_usec) / 1000;
  millis_zoom = (now.tv_sec - timestamp_zoom_changed.tv_sec) * 1000 +
      (now.tv_usec - timestamp_zoom_changed.tv_usec) / 1000;

  if (millis_pan < millis && millis_tilt < millis && millis_zoom < millis_z) {
    return TRUE;
  }

  if (!(ptz->controller)) {
    return TRUE;
  }
  //g_print ("%ld, %ld\n", millis_pan, millis_tilt);
  //g_print ("%lld, %lld; %f, %f, %f; %f, %f, %f\n", millis, millis_z, pan, tilt, zoom, ptz->x, ptz->y, ptz->z);

  const double limit = 1000;
  const double g = button_pressed ? 600.0 : 700.0;
  double x = gtk_adjustment_get_value (ptz->adjust_pan);
  double y = gtk_adjustment_get_value (ptz->adjust_tilt);
  double z = gtk_adjustment_get_value (ptz->adjust_zoom);
  double rx, ry;

  gst_cam_controller_query (ptz->controller, &rx, &ry);

  if (limit <= millis_pan && limit <= millis_tilt) {
    if (d < fabs (x - pan) || d < fabs (y - tilt) ||
        d < fabs (rx - pan) || d < fabs (ry - tilt)) {
      if (g < millis) {
        gst_cam_controller_move (ptz->controller,
            gtk_adjustment_get_value (ptz->adjust_pan_speed), (pan = x),
            gtk_adjustment_get_value (ptz->adjust_tilt_speed), (tilt = y));
        timestamp = now;
      }
    }
  }

  if (limit <= millis_zoom) {
    if (d < fabs (z - zoom)) {
      if (g < millis_z) {
        gst_cam_controller_zoom (ptz->controller,
            gtk_adjustment_get_value (ptz->adjust_zoom_speed), (zoom = z));
        timestamp_z = now;
      }
    }
  }
  return TRUE;
}

static void
gst_switch_ptz_button_pressed (GtkButton * button, gboolean value)
{
  button_pressed = value;
  if (value) {
    gst_switch_ptz_set_update_flag ((void *) TRUE);
  } else {
    g_timeout_add (600, (GSourceFunc) gst_switch_ptz_set_update_flag,
        (void *) FALSE);
  }
}

static void
gst_switch_ptz_button_pressed_left (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed), CAM_RUN_LEFT, TRUE);
}

static void
gst_switch_ptz_button_released_left (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed), CAM_RUN_LEFT, FALSE);
}

static void
gst_switch_ptz_button_pressed_left_top (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_LEFT_TOP, TRUE);
}

static void
gst_switch_ptz_button_released_left_top (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_LEFT_TOP, FALSE);
}

static void
gst_switch_ptz_button_pressed_top (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed), CAM_RUN_TOP, TRUE);
}

static void
gst_switch_ptz_button_released_top (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed), CAM_RUN_TOP, FALSE);
}

static void
gst_switch_ptz_button_pressed_top_right (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_RIGHT_TOP, TRUE);
}

static void
gst_switch_ptz_button_released_top_right (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_RIGHT_TOP, FALSE);
}

static void
gst_switch_ptz_button_pressed_right (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_RIGHT, button_pressed);
}

static void
gst_switch_ptz_button_released_right (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_RIGHT, button_pressed);
}

static void
gst_switch_ptz_button_pressed_right_bottom (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_RIGHT_BOTTOM, button_pressed);
}

static void
gst_switch_ptz_button_released_right_bottom (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_RIGHT_BOTTOM, button_pressed);
}

static void
gst_switch_ptz_button_pressed_bottom (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_BOTTOM, button_pressed);
}

static void
gst_switch_ptz_button_released_bottom (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_BOTTOM, button_pressed);
}

static void
gst_switch_ptz_button_pressed_bottom_left (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_LEFT_BOTTOM, button_pressed);
}

static void
gst_switch_ptz_button_released_bottom_left (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_LEFT_BOTTOM, button_pressed);
}

static void
gst_switch_ptz_button_pressed_center (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_run (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed),
      gtk_adjustment_get_value (ptz->adjust_tilt_speed),
      CAM_RUN_NONE, button_pressed);
  gst_cam_controller_move (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_pan_speed), 0,
      gtk_adjustment_get_value (ptz->adjust_tilt_speed), 0);

  gtk_adjustment_set_value (ptz->adjust_pan, 0);
  gtk_adjustment_set_value (ptz->adjust_tilt, 0);
}

static void
gst_switch_ptz_button_pressed_zoom_minus (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  zoom_step = -0.01;
}

static void
gst_switch_ptz_button_released_zoom_minus (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  zoom_step = 0.0;
}

static void
gst_switch_ptz_button_pressed_zoom_reset (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  double zoom = 0.5;
  gst_switch_ptz_button_pressed (button, FALSE);
  gst_cam_controller_zoom (ptz->controller,
      gtk_adjustment_get_value (ptz->adjust_zoom_speed), zoom);
  gtk_adjustment_set_value (ptz->adjust_zoom, zoom);
  zoom_step = 0.0;
}

static void
gst_switch_ptz_button_pressed_zoom_plus (GtkButton * button, GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, TRUE);
  zoom_step = 0.01;
}

static void
gst_switch_ptz_button_released_zoom_plus (GtkButton * button,
    GstSwitchPTZ * ptz)
{
  gst_switch_ptz_button_pressed (button, FALSE);
  zoom_step = 0.0;
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
gst_switch_ptz_pan_changed (GtkAdjustment * adjustment, GstSwitchPTZ * ptz)
{
  suseconds_t millis;
  gettimeofday (&timestamp_pan_changed, NULL);
  millis = timestamp_pan_changed.tv_sec * 1000 +
      timestamp_pan_changed.tv_usec / 1000;
  (void) millis;
  //g_print ("pan: %ld\n", millis);
}

static void
gst_switch_ptz_tilt_changed (GtkAdjustment * adjustment, GstSwitchPTZ * ptz)
{
  suseconds_t millis;
  gettimeofday (&timestamp_tilt_changed, NULL);
  millis = timestamp_tilt_changed.tv_sec * 1000 +
      timestamp_tilt_changed.tv_usec / 1000;
  (void) millis;
  //g_print ("tilt: %ld\n", millis);
}

static void
gst_switch_ptz_zoom_changed (GtkAdjustment * adjustment, GstSwitchPTZ * ptz)
{
  suseconds_t millis;
  gettimeofday (&timestamp_zoom_changed, NULL);
  millis = timestamp_zoom_changed.tv_sec * 1000 +
      timestamp_zoom_changed.tv_usec / 1000;
  (void) millis;
  //g_print ("tilt: %ld\n", millis);
  //ptz->z = gtk_adjustment_get_value (adjustment);
}

static void
gst_switch_ptz_init (GstSwitchPTZ * ptz)
{
  GtkWidget *box_main, *box_video, *box_control;
  GtkWidget *box_control_pan, *box_control_tilt, *box_control_zoom;
  GtkWidget *scale_pan, *scale_tilt, *control_grid, *box_buttons_tilt;
  GtkWidget *control_buttons[3][3] = { {NULL} };
  GtkWidget *box_zoom, *zoom_minus, *zoom_reset, *zoom_plus;
  GtkWidget *scrollwin;
  GtkWidget *scale_zoom;
  GtkWidget *scale_pan_speed, *scale_tilt_speed, *scale_zoom_speed;
  GtkWidget *label_pan_speed, *label_tilt_speed, *label_zoom_speed;
  GtkWidget *label;
  const gchar *control_labels[3][3] = {
    /*
       {"  \\  ", "  ^  ", "  /  "},
       {"  <  ", "  *  ", "  >  "},
       {"  /  ", "  v  ", "  \\  "},
     */
    /*
       { "  \\  ", GTK_STOCK_GO_UP, "  /  " },
       { GTK_STOCK_GO_BACK, GTK_STOCK_HOME, GTK_STOCK_GO_FORWARD },
       { "  /  ", GTK_STOCK_GO_DOWN, "  \\  " },
     */
    {"icons/up_left.png", "icons/up.png", "icons/up_right.png"},
    {"icons/left.png", "icons/center.png", "icons/right.png"},
    {"icons/down_left.png", "icons/down.png", "icons/down_right.png"},
  };
  int n, m;
  const char *title = NULL;

  ptz->controller = gst_cam_controller_new (ptz_control_protocol);
  if (ptz->controller == NULL) {
    return;
  }

  gst_cam_controller_open (ptz->controller, ptz_device_name);
  title = ptz->controller->device_info;
  if (title == NULL)
    title = "??? - PTZ Controller";

  ptz->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (ptz->window), 640, 480);
  gtk_window_set_title (GTK_WINDOW (ptz->window), title);
  g_signal_connect (G_OBJECT (ptz->window), "delete-event",
      G_CALLBACK (gst_switch_ptz_window_closed), ptz);

  box_main = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  box_video = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  box_control = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scrollwin, 200, -1);
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

  scale_pan =
      gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      ptz->controller->pan_min, ptz->controller->pan_max, 1.0);
  scale_tilt =
      gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL,
      ptz->controller->tilt_min, ptz->controller->tilt_max, 1.0);
  //gtk_range_set_slider_size_fixed (GTK_RANGE (scale_pan), TRUE);
  gtk_widget_set_size_request (scale_pan, 300, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale_pan), GTK_POS_RIGHT);
  gtk_scale_set_value_pos (GTK_SCALE (scale_tilt), GTK_POS_BOTTOM);
  gtk_range_set_inverted (GTK_RANGE (scale_tilt), TRUE);
  ptz->adjust_pan = gtk_range_get_adjustment (GTK_RANGE (scale_pan));
  ptz->adjust_tilt = gtk_range_get_adjustment (GTK_RANGE (scale_tilt));
  g_signal_connect (gtk_range_get_adjustment (GTK_RANGE (scale_pan)),
      "value-changed", G_CALLBACK (gst_switch_ptz_pan_changed), ptz);
  g_signal_connect (gtk_range_get_adjustment (GTK_RANGE (scale_tilt)),
      "value-changed", G_CALLBACK (gst_switch_ptz_tilt_changed), ptz);

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
      gtk_widget_set_size_request (btn, 70, 70);
      gtk_button_set_image (GTK_BUTTON (btn),
          gtk_image_new_from_file (control_labels[m][n]));
    }
  }

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Pan/Tilt:</b>");
  box_buttons_tilt = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box_control), label, TRUE, TRUE, 1);
  gtk_box_pack_start (GTK_BOX (box_control), scale_pan, FALSE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (box_buttons_tilt), scale_tilt, FALSE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (box_buttons_tilt), control_grid, FALSE, TRUE,
      10);
  gtk_box_pack_start (GTK_BOX (box_control), box_buttons_tilt, FALSE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (box_control), gtk_label_new (""), TRUE, TRUE,
      10);

  box_zoom = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  zoom_minus = gtk_button_new ();
  zoom_reset = gtk_button_new ();
  zoom_plus = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (zoom_minus),
      gtk_image_new_from_file ("icons/zoom_out.png"));
  gtk_button_set_image (GTK_BUTTON (zoom_reset),
      gtk_image_new_from_file ("icons/zoom.png"));
  gtk_button_set_image (GTK_BUTTON (zoom_plus),
      gtk_image_new_from_file ("icons/zoom_in.png"));
  gtk_widget_set_size_request (zoom_minus, 70, 70);
  gtk_widget_set_size_request (zoom_reset, 70, 70);
  gtk_widget_set_size_request (zoom_plus, 70, 70);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Zoom:</b>");
  gtk_box_pack_start (GTK_BOX (box_control), label, TRUE, TRUE, 1);
  gtk_box_pack_start (GTK_BOX (box_zoom), gtk_label_new (""), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_zoom), zoom_minus, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (box_zoom), zoom_reset, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (box_zoom), zoom_plus, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (box_zoom), gtk_label_new (""), TRUE, TRUE, 10);

  scale_zoom =
      gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
  gtk_range_set_value (GTK_RANGE (scale_zoom), 0.5);
  ptz->adjust_zoom = gtk_range_get_adjustment (GTK_RANGE (scale_zoom));
  label = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (label), scale_zoom, TRUE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (box_control), label, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box_control), box_zoom, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box_control), gtk_label_new (""), TRUE, TRUE,
      10);

  g_signal_connect (zoom_minus, "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_zoom_minus), ptz);
  g_signal_connect (zoom_reset, "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_zoom_reset), ptz);
  g_signal_connect (zoom_plus, "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_zoom_plus), ptz);

  g_signal_connect (zoom_minus, "released",
      G_CALLBACK (gst_switch_ptz_button_released_zoom_minus), ptz);
  g_signal_connect (zoom_plus, "released",
      G_CALLBACK (gst_switch_ptz_button_released_zoom_plus), ptz);


  scale_pan_speed =
      gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      ptz->controller->pan_speed_min, ptz->controller->pan_speed_max, 1.0);
  scale_tilt_speed =
      gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      ptz->controller->tilt_speed_min, ptz->controller->tilt_speed_max, 1.0);
  scale_zoom_speed =
      gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
  gtk_range_set_value (GTK_RANGE (scale_pan_speed),
      ptz->controller->pan_speed_max);
  gtk_range_set_value (GTK_RANGE (scale_tilt_speed),
      ptz->controller->tilt_speed_max);
  gtk_range_set_value (GTK_RANGE (scale_zoom_speed), 1.0);

  ptz->adjust_pan_speed =
      gtk_range_get_adjustment (GTK_RANGE (scale_pan_speed));
  ptz->adjust_tilt_speed =
      gtk_range_get_adjustment (GTK_RANGE (scale_tilt_speed));
  ptz->adjust_zoom_speed =
      gtk_range_get_adjustment (GTK_RANGE (scale_zoom_speed));
  label_pan_speed = gtk_label_new ("");
  label_tilt_speed = gtk_label_new ("");
  label_zoom_speed = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label_pan_speed), "<b>P:</b>");
  gtk_label_set_markup (GTK_LABEL (label_tilt_speed), "<b>T:</b>");
  gtk_label_set_markup (GTK_LABEL (label_zoom_speed), "<b>Z:</b>");
  box_control_pan = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  box_control_tilt = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  box_control_zoom = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Speeds:</b>");
  gtk_box_pack_start (GTK_BOX (box_control), label, FALSE, TRUE, 1);
  gtk_box_pack_start (GTK_BOX (box_control_pan), label_pan_speed, FALSE, FALSE,
      0);
  gtk_box_pack_start (GTK_BOX (box_control_pan), scale_pan_speed, TRUE, TRUE,
      10);
  gtk_box_pack_start (GTK_BOX (box_control_tilt), label_tilt_speed, FALSE,
      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box_control_tilt), scale_tilt_speed, TRUE, TRUE,
      10);
  gtk_box_pack_start (GTK_BOX (box_control_zoom), label_zoom_speed, FALSE,
      FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box_control_zoom), scale_zoom_speed, TRUE, TRUE,
      10);

  gtk_box_pack_start (GTK_BOX (box_control), box_control_pan, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control), box_control_tilt, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control), box_control_zoom, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box_control), gtk_label_new (""), TRUE, TRUE,
      10);

  {
    double v, tick = 50;
    gchar *s;
    gchar buf[64] = { 0 };
    int n;
    const gchar *fmt = "<small><sub>%d</sub></small>";
    const gchar *fmtb = "<small><sub><b>%d</b></sub></small>";
    g_sprintf ((s = buf), fmtb, 0);
    gtk_scale_add_mark (GTK_SCALE (scale_pan), 0, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (int) ptz->controller->pan_min);
    gtk_scale_add_mark (GTK_SCALE (scale_pan), ptz->controller->pan_min,
        GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (int) ptz->controller->pan_max);
    gtk_scale_add_mark (GTK_SCALE (scale_pan), ptz->controller->pan_max,
        GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, 0);
    gtk_scale_add_mark (GTK_SCALE (scale_tilt), 0, GTK_POS_RIGHT, s);
    g_sprintf ((s = buf), fmtb, (int) ptz->controller->tilt_min);
    gtk_scale_add_mark (GTK_SCALE (scale_tilt), ptz->controller->tilt_min,
        GTK_POS_RIGHT, s);
    g_sprintf ((s = buf), fmtb, (int) ptz->controller->tilt_max);
    gtk_scale_add_mark (GTK_SCALE (scale_tilt), ptz->controller->tilt_max,
        GTK_POS_RIGHT, s);
    for (v = -tick; ptz->controller->pan_min <= v; v -= tick) {
      if (v - ptz->controller->pan_min < 1)
        continue;
      n = (int) v;
      s = NULL;
      if (abs (n) % 50 == 0)
        g_sprintf ((s = buf), fmt, n);
      gtk_scale_add_mark (GTK_SCALE (scale_pan), v, GTK_POS_BOTTOM, s);
    }
    for (v = tick; v <= ptz->controller->pan_max; v += tick) {
      if (ptz->controller->pan_max - v < 1)
        continue;
      n = (int) v;
      s = NULL;
      if (abs (n) % 50 == 0)
        g_sprintf ((s = buf), fmt, n);
      gtk_scale_add_mark (GTK_SCALE (scale_pan), v, GTK_POS_BOTTOM, s);
    }
    for (v = -tick; ptz->controller->tilt_min <= v; v -= tick) {
      if (v - ptz->controller->tilt_min < 1)
        continue;
      n = (int) v;
      s = NULL;
      if (abs (n) % 50 == 0)
        g_sprintf ((s = buf), fmt, n);
      gtk_scale_add_mark (GTK_SCALE (scale_tilt), v, GTK_POS_RIGHT, s);
    }
    for (v = tick; v <= ptz->controller->tilt_max; v += tick) {
      if (ptz->controller->tilt_max - v < 1)
        continue;
      n = (int) v;
      s = NULL;
      if (abs (n) % 50 == 0)
        g_sprintf ((s = buf), fmt, n);
      gtk_scale_add_mark (GTK_SCALE (scale_tilt), v, GTK_POS_RIGHT, s);
    }

    g_sprintf ((s = buf), fmtb, (int) (v =
            ptz->controller->pan_speed_min + 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_pan_speed), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (int) (v =
            ptz->controller->pan_speed_max + 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_pan_speed), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (int) (v =
            (ptz->controller->pan_speed_max -
                ptz->controller->pan_speed_min) / 2 + 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_pan_speed), v, GTK_POS_BOTTOM, s);

    g_sprintf ((s = buf), fmtb, (int) (v =
            ptz->controller->tilt_speed_min + 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_tilt_speed), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (int) (v =
            ptz->controller->tilt_speed_max + 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_tilt_speed), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (int) (v =
            (ptz->controller->tilt_speed_max -
                ptz->controller->tilt_speed_min) / 2 + 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_tilt_speed), v, GTK_POS_BOTTOM, s);

    fmtb = "<small><sub><b>%.1f</b></sub></small>";
    g_sprintf ((s = buf), fmtb, (v = 0.0));
    gtk_scale_add_mark (GTK_SCALE (scale_zoom_speed), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (v = 1.0));
    gtk_scale_add_mark (GTK_SCALE (scale_zoom_speed), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (v = 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_zoom_speed), v, GTK_POS_BOTTOM, s);

    g_sprintf ((s = buf), fmtb, (v = 0.0));
    gtk_scale_add_mark (GTK_SCALE (scale_zoom), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (v = 1.0));
    gtk_scale_add_mark (GTK_SCALE (scale_zoom), v, GTK_POS_BOTTOM, s);
    g_sprintf ((s = buf), fmtb, (v = 0.5));
    gtk_scale_add_mark (GTK_SCALE (scale_zoom), v, GTK_POS_BOTTOM, s);
  }

  if (strcmp (ptz_control_protocol, "visca-sony") == 0) {
    gtk_widget_set_sensitive (box_control_pan, FALSE);
    gtk_widget_set_sensitive (box_control_tilt, FALSE);
    gtk_widget_set_sensitive (box_control_zoom, FALSE);
  }

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

  g_signal_connect (control_buttons[0][0], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_left_top), ptz);
  g_signal_connect (control_buttons[0][1], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_top), ptz);
  g_signal_connect (control_buttons[0][2], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_top_right), ptz);
  g_signal_connect (control_buttons[1][0], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_left), ptz);
  g_signal_connect (control_buttons[1][1], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_center), ptz);
  g_signal_connect (control_buttons[1][2], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_right), ptz);
  g_signal_connect (control_buttons[2][0], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_bottom_left), ptz);
  g_signal_connect (control_buttons[2][1], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_bottom), ptz);
  g_signal_connect (control_buttons[2][2], "pressed",
      G_CALLBACK (gst_switch_ptz_button_pressed_right_bottom), ptz);

  g_signal_connect (control_buttons[0][0], "released",
      G_CALLBACK (gst_switch_ptz_button_released_left_top), ptz);
  g_signal_connect (control_buttons[0][1], "released",
      G_CALLBACK (gst_switch_ptz_button_released_top), ptz);
  g_signal_connect (control_buttons[0][2], "released",
      G_CALLBACK (gst_switch_ptz_button_released_top_right), ptz);
  g_signal_connect (control_buttons[1][0], "released",
      G_CALLBACK (gst_switch_ptz_button_released_left), ptz);
  g_signal_connect (control_buttons[1][2], "released",
      G_CALLBACK (gst_switch_ptz_button_released_right), ptz);
  g_signal_connect (control_buttons[2][0], "released",
      G_CALLBACK (gst_switch_ptz_button_released_bottom_left), ptz);
  g_signal_connect (control_buttons[2][1], "released",
      G_CALLBACK (gst_switch_ptz_button_released_bottom), ptz);
  g_signal_connect (control_buttons[2][2], "released",
      G_CALLBACK (gst_switch_ptz_button_released_right_bottom), ptz);

  do_update = TRUE;
  gst_switch_ptz_update_xy (ptz);
  do_update = FALSE;

  g_timeout_add (300, (GSourceFunc) gst_switch_ptz_update, ptz);
  g_timeout_add (100, (GSourceFunc) gst_switch_ptz_update_xy, ptz);
  //g_timeout_add (50, (GSourceFunc) gst_switch_ptz_update_d, ptz);
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

  desc = g_string_new ("");

  g_string_append_printf (desc, "v4l2src device=%s ", ptz_video_name);
  //g_string_append_printf (desc, "! video/x-raw,width=320,height=240 ");
  g_string_append_printf (desc, "! videoconvert ! videoscale ");
  g_string_append_printf (desc, "! ximagesink name=sink sync=false ");
  return desc;
}

static void
gst_switch_ptz_run (GstSwitchPTZ * ptz)
{
  if (ptz->controller == NULL) {
#if 0
    GtkWidget *d = gtk_message_dialog_new (NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "You don't specify the correct protocol (check the -p option)!");
    gtk_dialog_run (GTK_DIALOG (d));
    gtk_widget_destroy (d);
#endif
    g_print ("You don't specify the correct protocol (check the -p option)!\n");
    return;
  }

  if (ptz->window) {
    gtk_widget_show_all (ptz->window);
    gst_worker_start (GST_WORKER (ptz));
    gtk_main ();
  }
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
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", "VERBOSE"},
  {"device", 'd', 0, G_OPTION_ARG_STRING, &ptz_device_name,
      "PTZ camera control device", "DEVICE"},
  {"protocol", 'p', 0, G_OPTION_ARG_STRING, &ptz_control_protocol,
      "PTZ camera control protocol", "PROTOCOL"},
  {"video", 'i', 0, G_OPTION_ARG_STRING, &ptz_video_name,
      "Camera video name (default /dev/video0)", "NAME"},
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
