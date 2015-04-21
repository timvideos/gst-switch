"""
Integration Tests for the Video-Pipelines
"""

from __future__ import absolute_import, print_function, unicode_literals

from .baseclass import IntegrationTestbase, IntegrationTestbaseCompare
from gstswitch.testsource import VideoSrc
from gstswitch.controller import Controller


class IntegrationTestbaseVideo(IntegrationTestbaseCompare):
    """ Testbase used to test video-pipeline results
    """

    def setup_test(self):
        """Setup Server, Controller and two Video-Test-Sources"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting up 1st VideoSrc")
        self.new_test_video(pattern=VideoSrc.PATTERN_RED)

        self.log.info("starting up 2nd VideoSrc")
        self.new_test_video(pattern=VideoSrc.PATTERN_GREEN)

        self.log.info("starting up 3rd VideoSrc")
        self.new_test_video(pattern=VideoSrc.PATTERN_BLUE)

        self.serv.wait_for_output('tcpserversink name=sink', count=3)

        self.log.info("waiting for end of initial transition")
        self.serv.wait_for_output('ending transition')


class TestCompositeModes(IntegrationTestbaseVideo):
    """ Test the various composition modes and their results
    """

    def test_set_composite_mode_none(self):
        """Test set_composite_mode COMPOSITE_NONE"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        assert self.controller.set_composite_mode(
            Controller.COMPOSITE_NONE)

        self.expect_frame('COMPOSITE_NONE.png')

    def test_set_composite_mode_pip(self):
        """Test set_composite_mode COMPOSITE_PIP"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        assert self.controller.set_composite_mode(
            Controller.COMPOSITE_PIP)

        self.expect_frame('COMPOSITE_PIP.png')

    def test_set_composite_mode_preview(self):
        """Test set_composite_mode COMPOSITE_DUAL_PREVIEW"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        assert self.controller.set_composite_mode(
            Controller.COMPOSITE_DUAL_PREVIEW)

        self.expect_frame('COMPOSITE_DUAL_PREVIEW.png')

    def test_set_composite_mode_equal(self):
        """Test set_composite_mode COMPOSITE_DUAL_EQUAL"""
        self.setup_test()

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')
        # mode is unchanged -> returns false
        assert not self.controller.set_composite_mode(
            Controller.COMPOSITE_DUAL_EQUAL)

        self.expect_frame('COMPOSITE_DUAL_EQUAL.png')


class TestSwitch(IntegrationTestbaseVideo):
    """ Test switching between the available input sources
    """
    PORT_RED = 3003
    PORT_GREEN = 3004
    PORT_BLUE = 3005

    def test_switch_video_source_a(self):
        """Test set_composite_mode COMPOSITE_NONE"""
        self.setup_test()

        # default: A=RED, B=GREEN
        self.expect_frame('SWITCH_RED_GREEN.png')

        # switch: A=GREEN, [B=RED]
        assert self.controller.switch(
            Controller.VIDEO_CHANNEL_A, self.PORT_GREEN)

        self.expect_frame('SWITCH_GREEN_RED.png')

    def test_switch_video_source_b(self):
        """Test set_composite_mode COMPOSITE_NONE"""
        self.setup_test()

        # default: A=RED, B=GREEN
        self.expect_frame('SWITCH_RED_GREEN.png')

        # switch: [A=GREEN], B=RED
        assert self.controller.switch(
            Controller.VIDEO_CHANNEL_B, self.PORT_RED)

        self.expect_frame('SWITCH_GREEN_RED.png')

    def test_switch_video_source_ab(self):
        """Test set_composite_mode COMPOSITE_NONE"""
        self.setup_test()

        # default: A=RED, B=GREEN
        self.expect_frame('SWITCH_RED_GREEN.png')

        # switch: A=BLUE, B=GREEN
        assert self.controller.switch(
            Controller.VIDEO_CHANNEL_A, self.PORT_BLUE)

        self.expect_frame('SWITCH_BLUE_GREEN.png')

        # switch: B=BLUE, [A=GREEN]
        assert self.controller.switch(
            Controller.VIDEO_CHANNEL_B, self.PORT_BLUE)

        self.expect_frame('SWITCH_GREEN_BLUE.png')


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
