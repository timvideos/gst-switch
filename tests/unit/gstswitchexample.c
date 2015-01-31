/* gst-switch							    -*- c -*-
 * Copyright (C) 2015 Tim 'mithro' Ansell <mithro@mithis.com>
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
#include <glib.h>

static void
success (void)
{
  g_assert_cmpint (1234, ==, 1234);
}

static void
fail (void)
{
  g_assert_cmpint (1234, ==, 1);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_set_nonfatal_assertions ();
  g_test_add_func ("/gstswitch/server/options/success", success);
  // g_test_add_func ("/gstswitch/server/options/fail", fail);
  return g_test_run ();
}
