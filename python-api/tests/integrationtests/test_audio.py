"""
Integration Tests for the Audio-Pipelines
"""

from __future__ import absolute_import, print_function, unicode_literals

from .baseclass import IntegrationTestbaseAudio
from gstswitch.controller import Controller
import pytest
import os


class TestSwitch(IntegrationTestbaseAudio):
    """ Test switching between multiple audio sources
    """
    @pytest.mark.xfail(os.environ.get('CI', "false") == "true",
                       reason="issue #207 (travis only)")
    def test_audio_passes(self):
        """ Test if audio passed into the gst-switch-server comes
        out the other end

        This test could be removed if test_switch_audio_source
        would pass (aka after fixing #201)
        """
        self.setup_test()

        self.expect_audio_spectrascope('SPECTRASCOPE_110.png')

    @pytest.mark.xfail(reason="issue #201")
    def test_switch_audio_source(self):
        """ Test switching between two audio sources
        """
        self.setup_test()

        self.expect_audio_spectrascope('SPECTRASCOPE_110.png')

        # this fails because there is no PORT_220 to select
        # adding it is disabled in the baseclass, because of issue #201
        assert self.controller.switch(
            Controller.AUDIO_CHANNEL, self.PORT_220)

        self.expect_audio_spectrascope('SPECTRASCOPE_220.png')


class TestPreviewPorts(IntegrationTestbaseAudio):
    """ Test the actual Output of the Preview-Ports
    """
    def test_caps(self):
        """ Test if the preview-ports is sending a video-stream with the
        expected caps
        """

        self.setup_test()
        caps = ('audio/x-raw, rate=(int)48000, channels=(int)2, '
                'format=(string)S16LE, layout=(string)interleaved')

        for port in self.PORTS:
            self.log.info("testing caps of preview-port %u", port)
            self.expect_caps(port, caps)
