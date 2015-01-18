#!/usr/bin/env python

# WARNING:
# THIS FILE IS ONLY FOR MY PERSONAL TESTING PURPOSES.
# IT IS NOT MADE TO AND SHOULD NOT BE TESTING THE ENTIRE API

from __future__ import absolute_import, print_function, unicode_literals

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))
import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst

GObject.threads_init()
Gst.init(None)
from gstswitch.server import Server
from gstswitch.helpers import *
from gstswitch.controller import Controller
import time

import gi
gi.require_version('Gst', '1.0')

# all executables (gst-launch-1.0, gst-switch-srv, gst-switch-ui, gst-switch-cap) at this path
path = '/usr/bin/'
# path = '/usr/bin/'
s = Server(path)
try:
    s.run()  # launches the server default parameters
    video_port = s.video_port
    audio_port = s.audio_port
    # connects a gstreamer module to view the output of the gst-switch-srv
    output = PreviewSinks()
    output.run()
    # adding two test video sources
    sources = TestSources(video_port=video_port, audio_port=audio_port)

    sources.new_test_audio()
    sources.new_test_audio()

    sources.new_test_video()
    sources.new_test_video()
    sources.new_test_video(timeoverlay=True)

    controller = Controller()
    controller.establish_connection()
    res = []
    tests_get = [controller.get_compose_port,
                 controller.get_encode_port,
                 controller.get_audio_port,
                 controller.get_preview_ports,
                 ]

    test_set_composite_mode = controller.set_composite_mode

    # testing random 10 modes
    for x in tests_get:
        print(x.__name__, x())
    time.sleep(1)
    modes = [0, 3, 2, 1, 0, 1, 2, 3, 0]
    for mode in modes:
        print('composite mode=', mode)
        test_set_composite_mode(mode)
        time.sleep(0.6)
    output.terminate()
    sources.terminate_video()
    sources.terminate_audio()
    s.terminate()

finally:
    if s.proc:
        s.kill()
