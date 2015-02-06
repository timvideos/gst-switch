"""
Integration Tests for the Audio-Pipelines
"""

from __future__ import absolute_import, print_function, unicode_literals


class TestAudioPassed(object):
    """ Test if audio passed into the gst-switch-server comes
    out the other end
    """

    # gst-launch-1.0 tcpclientsrc num-buffers=5 port=3004 !\
    #     gdpdepay !\
    #     wavescope style=dots !\
    #     videoconvert !\
    #     pngenc !\
    #     filesink location=test.png
    pass
