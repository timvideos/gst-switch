"""
Contains the Base class for integration tests.
"""

import sys
import os
import logging

sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))
from gstswitch.server import Server
from gstswitch.helpers import TestSources  # PreviewSinks
from gstswitch.controller import Controller
from gi.repository import GLib

# PATH = os.getenv("HOME") + '/gst/stage/bin/'
PATH = '../tools/'


class IntegrationTestbase(object):
    """Base class for integration tests."""

    # Tests are not allowed to have an __init__ method
    def setup_method(self, _):
        """Set up called automatically before every test_XXXX method."""
        self.log = logging.getLogger()
        self.log.setLevel(logging.DEBUG)
        logging.basicConfig(
            format="%(filename)s:%(lineno)d (%(funcName)s): %(message)s")

        self.serv = None
        self._sources = None
        self.controller = None

    def setup_server(self, record_file=False):
        """Set up a gst-switch server for testing."""
        assert self.serv is None

        self.log.info("setting up Server")
        self.serv = Server(path=PATH, video_format="debug",
                           record_file=record_file)

        self.log.info("running Server")
        self.serv.run()
        assert self.serv.is_alive()

        self.log.info("waiting for Server to open Controller-Port")
        self.serv.wait_for_output('tcp:host=0.0.0.0,port=5000')

        self.log.info("setting up TestSources")
        self._sources = TestSources(
            video_port=self.serv.video_port,
            audio_port=self.serv.audio_port)

    def setup_controller(self):
        """Create Controller object and call setup_controller."""
        self.log.info("setting up Controller")
        self.controller = Controller()

        self.log.info("connecting Controller to Server")
        self.controller.establish_connection()
        self.serv.wait_for_output('registered: ')

        assert self.controller.connection is not None

    def new_test_video(self, pattern=None):
        """Start a new Video-Testsource and wait until it's ready"""
        self.serv.wait_for_output('0.0.0.0:3000')
        self._sources.new_test_video(pattern=pattern)

    def new_test_audio(self):
        """Start a new Audio-Testsource and wait until it's ready"""
        self.serv.wait_for_output('0.0.0.0:4000')
        self._sources.new_test_audio()

    def teardown_method(self, _):
        """Tear down called automatically after every test_XXXX method."""
        self.controller = None

        # Kill all the sources
        if self._sources is not None:
            self.log.info("terminating Video-TestSource")
            self._sources.terminate_video()

            self.log.info("terminating Audio-TestSource")
            self._sources.terminate_audio()
        self._sources = None

        if self.serv is not None:
            self.log.info("terminating Server")
            self.serv.terminate(cov=True)

            if self.serv.proc:
                poll = self.serv.proc.poll()
                if poll == -11:
                    self.log.error("Server exited with Segmentation Fault")
                if poll != 0:
                    self.log.error("Server exited Error Eode %s", poll)

            log = open('server.log')
            print(log.read())

        self.serv = None


class IntegrationTestbaseMainloop(IntegrationTestbase):
    """Base class for integration tests that require a GLib-Mainloop.
    Used for Tests that register and wait for signals.
    """
    def setup_method(self, method):
        """Set up called automatically before every test_XXXX method."""
        super(IntegrationTestbaseMainloop, self).setup_method(method)
        self.mainloop = GLib.MainLoop()
        self.quit_count = 0

    def run_mainloop(self, timeout=5):
        """Start the MainLoop, set Quit-Counter to Zero"""
        self.quit_count = 0
        GLib.timeout_add_seconds(timeout, self.quit_mainloop)
        self.mainloop.run()

    def quit_mainloop(self, *_):
        """Quit the MainLoop, set Quit-Counter to Zero"""
        self.mainloop.quit()
        self.quit_count = 0

    def quit_mainloop_after(self, call_count):
        """Increment Quit-Counter, if it reaches call_count,
        Quit the MainLoop"""
        self.quit_count += 1
        if self.quit_count == call_count:
            self.quit_mainloop()


class IntegrationTestbaseCompare(IntegrationTestbase):
    """Base class for integration tests that require image capture
    and comparison. Used for Video- and Audio-Tests.
    """
    pass
