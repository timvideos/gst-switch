"""
Integration Tests for the Video-Pipelines
"""

from __future__ import absolute_import, print_function, unicode_literals
import time
from mock import Mock

from .baseclass import IntegrationTestbase, IntegrationTestbaseCompare
from gstswitch.testsource import VideoSrc
from gstswitch.controller import Controller

class TestCompositeModes(IntegrationTestbaseCompare):
    """ Test the various composition modes and their results
    """

    def setup_test(self):
        self.setup_server()
        self.setup_controller()

        self.log.info("starting up 1st VideoSrc")
        self.new_test_video(pattern=VideoSrc.PATTERN_RED)
        self.serv.wait_for_output('tcpserversink name=sink', count=1)

        self.log.info("starting up 2nd VideoSrc")
        self.new_test_video(pattern=VideoSrc.PATTERN_BLUE)
        self.serv.wait_for_output('tcpserversink name=sink', count=2)

        self.log.info("waiting for end of initial transition")
        self.serv.wait_for_output('ending transition')

    def test_set_composite_mode_none(self):
        """Test set_composite_mode"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        assert self.controller.set_composite_mode(Controller.COMPOSITE_NONE)
        self.expect_frame('COMPOSITE_NONE.png')

    def test_set_composite_mode_pip(self):
        """Test set_composite_mode"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        assert self.controller.set_composite_mode(Controller.COMPOSITE_PIP)
        self.expect_frame('COMPOSITE_PIP.png')

    def test_set_composite_mode_preview(self):
        """Test set_composite_mode"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        assert self.controller.set_composite_mode(Controller.COMPOSITE_DUAL_PREVIEW)
        self.expect_frame('COMPOSITE_DUAL_PREVIEW.png')

    def test_set_composite_mode_equal(self):
        """Test set_composite_mode"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        assert self.controller.set_composite_mode(Controller.COMPOSITE_DUAL_EQUAL)
        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')


class TestSwitch(IntegrationTestbase):
    """ Test switching between the available input sources
    """
    pass


class TestAdjustPIP(IntegrationTestbase):
    """ Test adjusting the PIP Position via DBus API
    """
    pass


class TestCompositeOutput(IntegrationTestbase):
    """ Test the actual Output of the Composition-Port
    """
    def test_caps(self):
        """ Test if the compose-port is sending a video-stream with the
        expected caps
        """
        pass


class TestEncodedOutput(IntegrationTestbase):
    """ Test the actual Output of the Endoded-Port
    """
    def test_caps(self):
        """ Test if the encode-port is sending a video-stream with the
        expected caps
        """
        pass


# class TestClickVideo(object):
# class TestMarkFace(object):
# class TestMarkTracking(object):
