#!/usr/bin/env python

from __future__ import absolute_import, print_function, unicode_literals

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../..")))
# should be in PYTHONPATH
from gstswitch.server import Server
from gstswitch.testsource import BasePipeline
from gstswitch.helpers import *
from gstswitch.controller import Controller
import gi
import subprocess
import time
import psutil
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
GObject.threads_init()
Gst.init(None)





PATH = '/usr/bin/'
INTERVAL = 1.0




s = Server(path=PATH)
v_port = s.video_port
a_port = s.audio_port
try:
    s.run()

    cmd_vid1 = PATH
    cmd_vid2 = PATH
    cmd_vid1 += """gst-launch-1.0 videotestsrc pattern=1 ! \
                video/x-raw, width=300, height=200 !\
                gdppay !
                tcpclientsink port={0}
                """.format(v_port)
    print(cmd_vid1)

    cmd_vid2 += """gst-launch-1.0 videotestsrc ! \
                video/x-raw, width=300, height=200 !\
                gdppay !
                tcpclientsink port={0}
                """.format(v_port)
    print(cmd_vid2)

    vid1 = subprocess.Popen(
                        cmd_vid1.split(),
                        bufsize=-1,
                        shell=False)
    vid2 = subprocess.Popen(
                        cmd_vid2.split(),
                        bufsize=-1,
                        shell=False)
    time.sleep(1)
    sources = TestSources(audio_port=a_port)
    sources.new_test_audio()
    sources.new_test_audio()

    controller = Controller()
    controller.establish_connection()

    cmd = PATH
    cmd += """gst-switch-ui"""
    cmd = " ".join(cmd.split())
    with open('ui.log', 'w') as tempf:
        ui_process = subprocess.Popen(
                        cmd.split(),
                        stdout=tempf,
                        stderr=tempf,
                        bufsize=-1,
                        shell=False)

    params = [(65,3004), (65, 3003)]
    i = 0
    p = psutil.Process(s.pid)
    cpu_usage_file = open("cpu.log", "w")
    while 1:
        # print(i)
        cpu = p.get_cpu_percent(interval=INTERVAL)
        mem = p.get_memory_percent()
        print(cpu)
        print(mem)
        cpu_usage_file.write(str(cpu) + '\n' + str(mem) + '\n')
        controller.switch(params[i][0], params[i][1])
        # time.sleep(1)
        i+=1
        i%=2

    #end all
    sources.terminate_audio()
    vid1.terminate()
    vid2.terminate()
    s.terminate()
finally:
    if s.proc:
        s.kill()
