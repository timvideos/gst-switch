if SPEAKERTRACK_ENABLED
  bin_PROGRAMS = gst-switch-srv gst-switch-ui gst-switch-cap gst-switch-ptz
else
  bin_PROGRAMS = gst-switch-srv gst-switch-ui gst-switch-cap
endif

if GCOV_ENABLED
# --coverage
GCOV_CFLAGS = -fprofile-arcs -ftest-coverage
GCOV_LFLAGS = -fprofile-arcs -ftest-coverage
#GCOV_CFLAGS = --coverage
#GCOV_LFLAGS = --coverage
else
GCOV_CFLAGS =
GCOV_LFLAGS =
endif

if DEBUG
AM_CFLAGS = -g3 -O0 -gdwarf-4 -fvar-tracking-assignments -fvar-tracking
else
AM_CFLAGS = -O2
endif

gst_switch_srv_SOURCES = gstworker.c gstswitchserver.c gstcase.c \
  gstcomposite.c gstswitchcontroller.c gstrecorder.c \
  gio/gsocketinputstream.c gstswitchopts.c
gst_switch_srv_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(GCOV_CFLAGS) \
  $(GST_PLUGINS_BASE_CFLAGS) $(AM_CFLAGS) -DLOG_PREFIX="\"gst-switch-srv\""
gst_switch_srv_LDFLAGS = $(GCOV_LFLAGS) $(GST_LIBS) $(GST_BASE_LIBS) \
  $(GST_PLUGINS_BASE_LIBS) $(GSTPB_BASE_LIBS)
gst_switch_srv_LDADD = $(GIO_LIBS) $(LIBM)

gst_switch_ui_SOURCES = gstworker.c gstswitchui.c gstvideodisp.c \
  gstaudiovisual.c gstswitchclient.c
gst_switch_ui_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(GCOV_CFLAGS) \
  $(GST_PLUGINS_BASE_CFLAGS) $(X_CFLAGS) $(GTK_CFLAGS) $(AM_CFLAGS) \
  -DLOG_PREFIX="\"gst-switch-ui\""
gst_switch_ui_LDFLAGS = $(GCOV_LFLAGS) $(GST_LIBS) $(GST_BASE_LIBS) \
  $(GST_PLUGINS_BASE_LIBS) $(GSTPB_BASE_LIBS) -lm
gst_switch_ui_LDADD = $(GST_LIBS) $(X_LIBS) $(LIBM) $(GTK_LIBS) $(GLIB_LIBS)

gst_switch_cap_SOURCES = gstworker.c gstswitchcapture.c gstswitchclient.c
gst_switch_cap_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(GCOV_CFLAGS) \
  $(GST_PLUGINS_BASE_CFLAGS) $(X_CFLAGS) $(GTK_CFLAGS) \
  -DLOG_PREFIX="\"gst-switch-cap\""
gst_switch_cap_LDFLAGS = $(GCOV_LFLAGS) $(GST_LIBS) $(GST_BASE_LIBS) \
  $(GST_PLUGINS_BASE_LIBS) $(GSTPB_BASE_LIBS) -lm
gst_switch_cap_LDADD = $(GST_LIBS) $(X_LIBS) $(LIBM) $(GTK_LIBS) $(GLIB_LIBS)

gst_switch_ptz_SOURCES = gstworker.c gstvideodisp.c gstswitchptz.c
gst_switch_ptz_CFLAGS = -g -ggdb $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(GCOV_CFLAGS) \
  $(GST_PLUGINS_BASE_CFLAGS) $(X_CFLAGS) $(GTK_CFLAGS) \
  -DLOG_PREFIX="\"gst-switch-ptz\""
gst_switch_ptz_LDFLAGS = $(GCOV_LFLAGS) $(GST_LIBS) $(GST_BASE_LIBS) \
  $(GST_PLUGINS_BASE_LIBS) $(GST_PLUGINS_BAD_LIBS) $(GSTPB_BASE_LIBS)
gst_switch_ptz_LDADD = $(GST_LIBS) $(X_LIBS) $(LIBM) $(GTK_LIBS) \
  $(GLIB_LIBS) -lm \
  $(prefix)/lib/libgstcamcontrol-1.0.so

#  -lgstcamcontrol

if GCOV_ENABLED
coverage:
	gcov -d gst_switch_srv-*.o
endif
