"""
Integration Tests for the Video-Pipelines
"""

from __future__ import absolute_import, print_function, unicode_literals
from .baseclass import IntegrationTestbase


class TestCompositeModes(object):
    """ Test the various composition modes and their results
    """
    pass


class TestSwitch(object):
    """ Test switching between the available input sources
    """
    pass


class TestAdjustPIP(object):
    """ Test adjusting the PIP Position via DBus API
    """
    pass


class TestCompositeOutput(IntegrationTestbase):
    def test_caps(self):
        """ Test if the compose-port is sending a video-stream with the
        expected caps
        """
        pass

class TestEncodedOutput(IntegrationTestbase):
    def test_caps(self):
        """ Test if the encode-port is sending a video-stream with the
        expected caps
        """
        pass



# class TestClickVideo(object):
# class TestMarkFace(object):
# class TestMarkTracking(object):
