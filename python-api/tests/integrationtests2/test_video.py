"""
Integration Tests for the Video-Pipelines
"""

from __future__ import absolute_import, print_function, unicode_literals
from .baseclass import IntegrationTestbase


class TestCompositeModes(IntegrationTestbase):
    """ Test the various composition modes and their results
    """
    pass


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
