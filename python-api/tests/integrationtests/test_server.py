"""
Integration tests for Server in server.py
"""

from __future__ import absolute_import, print_function, unicode_literals

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))

from gstswitch.server import Server

# PATH = os.getenv("HOME") + '/gst/stage/bin/'
PATH = '../tools/'


class TestServerStartStop(object):

    """Test Starting and Stopping the Server
    Run Server and Stop multiple times
    """
    NUM = 5

    def startstop(self):
        """Start and Stop the Server"""
        serv = Server(path=PATH)
        try:
            serv.run()
            pid = serv.pid
            assert pid > 0
            serv.terminate(1)
            assert serv.proc is None
        finally:
            if serv.proc:
                poll = serv.proc.poll()
                print(self.__class__)
                if poll == -11:
                    print("SEGMENTATION FAULT OCCURRED")
                print("ERROR CODE - {0}".format(abs(poll)))
                serv.terminate(1)
                log = open('server.log')
                print(log.read())

    def test_start_stop(self):
        """Test Start and Stop the Server"""
        for _ in range(self.NUM):
            self.startstop()
