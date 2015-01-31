/* GstSwitch							    -*- c -*-
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

#include "tools/gstrecorder.c"
#include <fcntl.h>

#define BASEDIR  "/tmp/unittests/"

// fake options struct, not used in these tests
GstSwitchServerOpts opts;
gboolean verbose = FALSE;



static char *
filepath (char *buf, size_t len, char const *fn)
{
  snprintf (buf, len, BASEDIR "%d/%s", getpid (), fn);
  return buf;
}


static gboolean
exists (const char *filepath, gboolean dir)
{
  struct stat s;

  if (stat (filepath, &s) == 0) {
    if (!dir || S_ISDIR (s.st_mode))
      return TRUE;
  }
  return FALSE;
}

static gboolean
touch (const char *filepath)
{
  int fd =
      open (filepath, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd != -1) {
    close (fd);
    return TRUE;
  }
  return FALSE;
}


static int
create_new_directory (const char *name)
{
  char buf[256];

  gst_recorder_mkdirs (filepath (buf, sizeof buf, name));
  return exists (buf, TRUE);
}


static gboolean
create_capture_file (const char *name)
{
  char buf[256];
  char const *capfilename =
      gst_recorder_new_filename (filepath (buf, sizeof buf, name));
  return touch (capfilename);
}

static void
test_mkdirs ()
{
  g_assert_cmpint (create_new_directory ("testdir1"), !=, 0);
  g_assert_cmpint (create_new_directory ("testdir2"), !=, 0);
  g_assert_cmpint (create_new_directory ("testdir1/testdir3"), !=, 0);
  g_assert_cmpint (create_new_directory ("testdir3/testdir1"), !=, 0);
}

static void
test_new_filename ()
{
  g_assert_true (create_capture_file ("%Y%m%d_%T"));
  g_assert_true (create_capture_file ("%Y%m/%d_%T"));
  g_assert_true (create_capture_file ("%Y/%m/%d/%T"));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gst_init (&argc, &argv);
  g_test_set_nonfatal_assertions ();
  g_test_add_func ("/gstswitch/options/mkdirs", test_mkdirs);
  g_test_add_func ("/gstswitch/options/filename", test_new_filename);
  return g_test_run ();
}
