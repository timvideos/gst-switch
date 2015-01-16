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
