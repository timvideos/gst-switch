include $(top_srcdir)/build/glib-tap.mk

LDADD = $(GTK_LIBS) $(GLIB_LIBS) $(GST_LIBS)

test_gstswitchopts_SOURCES = test_gstswitchopts.c
test_gstswitchopts_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) \
  $(GCOV_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) -DLOG_PREFIX="\"./tests\""
test_gstrecorder_filename_LDFLAGS = $(GCOV_LFLAGS)
test_gstcomposite_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) \
  $(GCOV_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) -DLOG_PREFIX="\"./tests\""
test_gstcomposite_LDFLAGS = $(GCOV_LFLAGS)

test_gst_pipeline_string_SOURCES = test_gst_pipeline_string.c ../../tools/gstworker.c
test_gst_pipeline_string_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) \
  $(GCOV_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) -DLOG_PREFIX="\"./tests\""
test_gst_pipeline_string_LDFLAGS = $(GCOV_LFLAGS)


dist_test_data = \
  $(NULL)

test_programs = \
  test_gstswitchopts \
  test_gstcomposite \
  test_gst_pipeline_string \
  $(NULL)

if GCOV_ENABLED
coverage:
	gcov *.o
endif

include $(top_srcdir)/build/Makefile.am.gitignore

# Disabled gstrecorder test
#  test_gstrecorder_filename \
#test_gstrecorder_filename_SOURCES = test_gstrecorder_filename.c ../../tools/gstrecorder.c
#test_gstrecorder_filename_CFLAGS = -fprofile-arcs $(GST_CFLAGS) $(GST_BASE_CFLAGS) \
#  $(GCOV_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) -DLOG_PREFIX="\"./tests\""
