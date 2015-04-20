"""
Integration Tests for the dbus Controller
"""

from __future__ import absolute_import, print_function, unicode_literals
import os
import sys
import time
import random
from .baseclass import IntegrationTestbase, IntegrationTestbaseMainloop
from gstswitch.controller import Controller
from mock import Mock


class TestEstablishConnection(IntegrationTestbase):
    """ Test starting and connecting to the Server
    """

    def test_establish_connection(self):
        """ Start the Server and connect to it. Assert it starts up correctly
        and can be connected to via the python-controller.
        """
        self.setup_server()
        assert self.serv is not None
        assert self.serv.is_alive()

        self.setup_controller()
        assert self.controller.connection is not None


class TestGetComposePort(IntegrationTestbase):
    """ Test get_compose_port method
    """

    def wait_until_ready(self):
        """ Blocks until the Server has reported, that it's
        composite-output is started
        """
        self.log.info("waiting for Server to start composite-output")
        self.serv.wait_for_output('composite-output started')

    def test_compose_port(self):
        """
        Test if get_compose_port is returning the expected port
        """
        self.setup_server()
        self.setup_controller()
        self.wait_until_ready()

        self.log.info("calling get_compose_port")
        assert self.controller.get_compose_port() == 3001


class TestGetEncodePort(IntegrationTestbase):
    """ Test get_encode_port method
    """

    def wait_until_ready(self):
        """ Blocks until the Server has reported, that it's
        encoding-output is started
        """
        self.log.info("waiting for Server to start encoding-output")
        self.serv.wait_for_output('encoding-output started')

    def test_encode_port(self):
        """Test get_encode_port method returning the expected port"""
        self.setup_server()
        self.setup_controller()
        self.wait_until_ready()

        self.log.info("calling get_encode_port")
        assert self.controller.get_encode_port() == 3002


class TestGetAudioPort(IntegrationTestbase):
    """ Test get_audio_port method
    """
    def wait_until_ready(self):
        """ Blocks until the Server has reported, that it's
        audio-output has started
        """
        self.log.info("waiting for Server to start preview-port-outputs")
        self.serv.wait_for_output('tcpserversink name=sink')

    def test_preview_ports(self):
        """Test get_audio_port method returning the expected port"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting 2 test-audio sources")
        self.new_test_audio()
        self.new_test_audio()

        self.wait_until_ready()

        self.log.info("calling get_audio_port")
        assert self.controller.get_audio_port() == 3003


class TestGetPreviewPorts(IntegrationTestbase):
    """ Test get_preview_ports method
    """
    def wait_for_sources(self, count):
        """ Blocks until the Server has reported, that the right number of
        preview-ports are is started
        """
        self.log.info("waiting for Server to start preview-port-outputs")
        self.serv.wait_for_output('tcpserversink name=sink', count=count)

    def test_one_preview_port(self):
        """Test get_preview_ports method returning the expected port"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting 1 test-video source")
        self.new_test_video()

        self.wait_for_sources(count=1)

        self.log.info("calling get_preview_ports")
        assert self.controller.get_preview_ports() == [3003]

    def test_five_preview_ports(self):
        """Test get_preview_ports method returning the expected port"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting 5 test-video sources")
        self.new_test_video()
        self.new_test_video()
        self.new_test_video()
        self.new_test_video()
        self.new_test_video()

        self.log.info("waiting for the test-video sources to come up")
        self.wait_for_sources(count=5)

        self.log.info("calling get_preview_ports")
        assert self.controller.get_preview_ports() == [
            3003, 3004, 3005, 3006, 3007]


class TestSignals(IntegrationTestbaseMainloop):
    """ Test the various on_* signal-handler methods
    """
    def wait_for_sources(self, count):
        """ Blocks until the Server has reported, that the right number of
        preview-ports are is started
        """
        self.log.info("waiting for Server to start preview-port-outputs")
        self.serv.wait_for_output('tcpserversink name=sink', count=count)

    def setup_sources(self):
        """ Starts some Test-Video streams and waits until they are ready
        """
        self.log.info("starting 2 test-video sources")
        self.new_test_video()
        self.new_test_video()

        self.log.info("waiting for the test-video sources to come up")
        self.wait_for_sources(count=2)

    def test_initial_mode_callback(self):
        """Create a Controller object, call on_new_mode_online method and
        check that the callback fires initially when the sources are set up
        """
        self.setup_server()
        self.setup_controller()

        self.log.info("setting callback")
        test_cb = Mock(side_effect=self.quit_mainloop)
        self.controller.on_new_mode_online(test_cb)

        self.log.info("starting test-sources")
        self.setup_sources()

        self.log.info("waiting for initial callback with"
                      "default-mode COMPOSITE_DUAL_EQUAL")
        self.run_mainloop(timeout=5)
        test_cb.assert_called_once_with(Controller.COMPOSITE_DUAL_EQUAL)

    def test_new_mode_callback(self):
        """Create a Controller object, call on_new_mode_online method and
        check that the callback fires when set_composite_mode is called
        """
        self.setup_server()
        self.setup_controller()

        self.log.info("setting callback")
        test_cb = Mock(side_effect=self.quit_mainloop)
        self.controller.on_new_mode_online(test_cb)

        self.setup_sources()

        self.log.info("waiting for initial callback with default-mode COMPOSITE_DUAL_EQUAL")
        self.run_mainloop(timeout=5)
        test_cb.assert_called_once_with(Controller.COMPOSITE_DUAL_EQUAL)
        test_cb.reset_mock()

        self.log.info("setting new composite mode COMPOSITE_NONE")
        assert self.controller.set_composite_mode(Controller.COMPOSITE_NONE)
        self.run_mainloop(timeout=5)

        self.log.info("waiting for callback with new mode COMPOSITE_NONE")
        test_cb.assert_called_once_with(Controller.COMPOSITE_NONE)

    def test_same_mode_no_callback(self):
        """Create a Controller object, call on_new_mode_online method and
        check that the callback fires, but does not fire when
        set_composite_mode is called multiple times with the same format
        """
        self.setup_server()
        self.setup_controller()

        self.log.info("setting callback")
        test_cb = Mock(side_effect=self.quit_mainloop)
        self.controller.on_new_mode_online(test_cb)

        self.setup_sources()

        self.log.info("waiting for initial callback with"
                      "default-mode COMPOSITE_DUAL_EQUAL")
        self.run_mainloop(timeout=5)
        test_cb.assert_called_once_with(Controller.COMPOSITE_DUAL_EQUAL)
        test_cb.reset_mock()

        self.log.info("setting the same composite mode"
                      "(COMPOSITE_DUAL_EQUAL) again")
        assert not self.controller.set_composite_mode(
            Controller.COMPOSITE_DUAL_EQUAL)

        self.log.info("setting a new composite-mode COMPOSITE_PIP")
        assert self.controller.set_composite_mode(Controller.COMPOSITE_PIP)
        self.run_mainloop(timeout=5)

        # just waiting for the timeout to verify no incoming call
        # would slow down the tests remarebly, so we send another
        # mode-change, knowing that when it arrives without an
        # intermediate call, there was no duplicate callback

        self.log.info("waiting for callback with new mode COMPOSITE_PIP")
        test_cb.assert_called_once_with(Controller.COMPOSITE_PIP)

    def test_preview_port_added_cb(self):
        """Create a Controller object, call on_preview_port_added method and
        check that the callback fires correctly when new video-sources
        are added
        """
        self.setup_server()
        self.setup_controller()

        test_cb = Mock(side_effect=lambda mode, serve, type:
                       self.quit_mainloop_after(call_count=2))
        self.controller.on_preview_port_added(test_cb)

        self.setup_sources()
        self.run_mainloop(timeout=5)

        self.log.info("test_cb called with: %s", test_cb.call_args_list)
        test_cb.assert_any_call(3003, 1, 7)
        test_cb.assert_any_call(3004, 1, 8)
        assert test_cb.call_count == 2


class TestNewRecord(IntegrationTestbase):
    """ Test new_record method
    """

    def wait_for_sources(self, count):
        """ Blocks until the Server has reported, that the right number of
        preview-ports are is started
        """
        self.log.info("waiting for Server to start preview-port-outputs")
        self.serv.wait_for_output('tcpserversink name=sink', count=count)

    def setup_sources(self):
        """ Starts some Test-Video streams and waits until they are ready
        """
        self.log.info("starting 2 test-video sources")
        self.new_test_video()
        self.new_test_video()

        self.log.info("waiting for the test-video sources to come up")
        self.wait_for_sources(count=2)

    def test_filename_changes(self):
        """ Tests, that the Server creates a new File each time
        new_record is called
        """
        test_filename = "gst-test-{0}.data".format(
            random.randint(0, sys.maxsize))

        self.log.info("asserting recording-file are not aready existing"
                      "(test_filename=%s)", test_filename)
        assert not os.path.exists(test_filename)
        assert not os.path.exists(test_filename + '.000')

        self.setup_server(record_file=test_filename)
        self.setup_controller()
        self.setup_sources()

        self.log.info("asserting server created a recording-file")
        assert os.path.exists(test_filename)
        assert not os.path.exists(test_filename + '.000')

        self.log.info("starting a new recording")
        assert self.controller.new_record()

        self.log.info("asserting server created a new recording-file")
        assert os.path.exists(test_filename)
        assert os.path.exists(test_filename + '.000')

        os.remove(test_filename)
        os.remove(test_filename + '.000')

    def test_record_file_grows(self):
        """ Tests, that the Server actually writes Data into the record-file
        """
        test_filename = "gst-test-{0}.data".format(
            random.randint(0, sys.maxsize))

        self.log.info("asserting recording-file are not aready existing"
                      "(test_filename=%s)", test_filename)
        assert not os.path.exists(test_filename)

        self.setup_server(record_file=test_filename)
        self.setup_controller()
        self.setup_sources()

        self.log.info("asserting server created a recording-file")
        assert os.path.exists(test_filename)

        self.log.info("sleeping for 5 frames")
        time.sleep(5 / 25)

        self.log.info("starting a new recording")
        assert self.controller.new_record()

        self.log.info("asserting the old recording-file was flushed"
                      "and is >0 bytes")
        size = os.path.getsize(test_filename)
        assert size > 0

        os.remove(test_filename)
        os.remove(test_filename + '.000')
