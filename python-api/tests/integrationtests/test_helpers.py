"""
Integartion tests for TestSources, PreviewSinks in helpers.py
"""

from __future__ import absolute_import, print_function, unicode_literals

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))

from gstswitch.server import Server
from gstswitch.helpers import TestSources, PreviewSinks
import time


# PATH = os.getenv("HOME") + '/gst/stage/bin/'
PATH = '../tools/'


class TestTestSourcesPreviews(object):

    """Test for TestSources and PreviewSinks"""
    NUM = 1

    def add_video_sources(self, num, video_port):
        """Add a video source"""
        sources = TestSources(video_port=video_port)
        for _ in range(num):
            sources.new_test_video(pattern=6)
        # print("done adding" + str(num))
        time.sleep(2)
        sources.terminate_video()
        time.sleep(2)

    def test_video_sources(self):
        """Test video sources"""
        video_port = 3000
        serv = Server(PATH, video_port=video_port)
        try:
            serv.run()
            preview = PreviewSinks()
            preview.run()
            for i in range(self.NUM):
                self.add_video_sources(i * 10 + 1, video_port)
            preview.terminate()
            serv.terminate()
        finally:
            if serv.proc:
                serv.kill()

    def add_audio_sources(self, num, audio_port):
        """Add audio sources"""
        sources = TestSources(audio_port=audio_port)
        for _ in range(num):
            sources.new_test_audio(wave=10)
        # print("done adding" + str(num))
        # print(sources.get_test_audio())
        time.sleep(2)
        sources.terminate_audio()
        time.sleep(2)

    def test_audio_sources(self):
        """Test audio sources"""
        audio_port = 4000
        serv = Server(PATH, audio_port=audio_port)
        try:
            serv.run()
            preview = PreviewSinks()
            preview.run()
            for i in range(self.NUM):
                self.add_audio_sources(i * 10 + 1, audio_port)

            preview.terminate()
            serv.terminate()
        finally:
            if serv.proc:
                serv.kill()
