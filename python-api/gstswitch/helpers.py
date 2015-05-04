"""
Has helper classes which create test video and audio sources.
It is also possible to create a preview out source showing the
compose port output.
"""

from __future__ import absolute_import, print_function, unicode_literals

import logging
from gstswitch import testsource
from .exception import RangeError, InvalidIndexError


__all__ = ["TestSources", "PreviewSinks"]


class TestSources(object):

    """A Controller of test sources feeding into the
    gst-switch-srv
    :param width: The width of the output video
    :param height: The height of the output video
    :param pattern: The videotestsrc pattern of the output video
    :param timeoverlay: True to enable a running time over video
    :param clockoverlay: True to enable current clock time over video
    """

    def __init__(self, video_port=None, audio_port=None):
        super(TestSources, self).__init__()
        self._running_tests_video = []
        self._running_tests_audio = []
        self._audio_port = None
        self._video_port = None

        self.log = logging.getLogger('testsources')

        if video_port:
            self.video_port = video_port
        if audio_port:
            self.audio_port = audio_port

    @property
    def video_port(self):
        """Get the video port"""
        return self._video_port

    @video_port.setter
    def video_port(self, video_port):
        """Set the video port
        :raises RangeError: Video Port must be range 1 to 65535
        :raises TypeError: should be a integer or should be able to
        covert to an integer
        """
        try:
            i = int(video_port)
            if i < 1 or i > 65535:
                raise RangeError('video_port must be in range 1 to 65535')
            else:
                self._video_port = video_port
        except TypeError:
            raise TypeError("video_port must be a string or number,"
                            "not, '{0}'".format(type(video_port)))
        except ValueError:
            raise TypeError("Port must be a valid number")

    @property
    def audio_port(self):
        """Get the audio port"""
        return self._audio_port

    @audio_port.setter
    def audio_port(self, audio_port):
        """Set the audio port
        :raises RangeError: audio port must be range 1 to 65535
        :raises TypeError: should be a integer or should be able to
        covert to an integer
        """
        try:
            i = int(audio_port)
            if i < 1 or i > 65535:
                raise RangeError('audio port must be in range 1 to 65535')
            else:
                self._audio_port = audio_port
        except TypeError:
            raise TypeError("audio port must be a string or number,"
                            "not, '{0}'".format(type(audio_port)))
        except ValueError:
            raise TypeError("Port must be a valid number")

    @property
    def running_tests_video(self):
        """Get the currently running test video list"""
        return list(self._running_tests_video)

    @running_tests_video.setter
    def running_tests_video(self, tests):
        """Set the running test video list
        """
        self._running_tests_video = tests

    @property
    def running_tests_audio(self):
        """Get the currently running test audio list"""
        return list(self._running_tests_audio)

    @running_tests_audio.setter
    def running_tests_audio(self, tests):
        """Set the currently running test audio list"""
        self._running_tests_audio = tests

    def new_test_video(self,
                       width=300,
                       height=200,
                       pattern=None,
                       timeoverlay=False,
                       clockoverlay=False):
        """Start a new test video
        :param port: The port of where the TCP stream will be sent
        Should be same as video port of gst-switch-src
        :param width: The width of the output video
        :param height: The height of the output video
        :param pattern: The videotestsrc pattern of the output video
        :param timeoverlay: True to enable a running time over video
        :param clockoverlay: True to enable current clock time over video
        """
        testsrc = testsource.VideoSrc(
            self.video_port,
            width,
            height,
            pattern,
            timeoverlay,
            clockoverlay)
        testsrc.run()
        self._running_tests_video.append(testsrc)

    def terminate_index_video(self, index):
        """Terminate video test source specified by index
        :param index: The index of the video source to terminate
        Use get_test_video for finding the index
        """
        try:
            testsrc = self._running_tests_video[int(index)]
        except IndexError:
            raise InvalidIndexError(
                ("No video source with index:{0},"
                 "use get_test_video() to determine index")
                .format(index))
        except ValueError:
            raise InvalidIndexError("Index should be a valid integer")
        except TypeError:
            raise InvalidIndexError("Index should be a valid integer")

        self.log.debug(
            'terminating video-source with pattern %s',
            testsrc.pattern)

        testsrc.end()
        self._running_tests_video.remove(self._running_tests_video[index])

    def terminate_video(self):
        """Terminate all test video sources
        """
        for _ in range(len(self._running_tests_video)):
            self.terminate_index_video(0)

    def new_test_audio(self, freq=110, wave=None):
        """Start a new test audio
        :param port: The port of where the TCP stream will be sent
        Should be same as audio port of gst-switch-src
        :param width: The width of the output audio
        :param height: The height of the output audio
        :param pattern: The audiotestsrc pattern of the output audio
        :param timeoverlay: True to enable a running time over audio
        :param clockoverlay: True to enable current clock time over audio
        """
        testsrc = testsource.AudioSrc(
            self.audio_port,
            freq,
            wave)
        testsrc.run()
        self._running_tests_audio.append(testsrc)

    def get_test_audio(self):
        """Returns a list of processes acting as audio test sources running
        :returns: A list containing all audio test sources running
        """
        return self._running_tests_audio

    def terminate_index_audio(self, index):
        """Terminate audio test source specified by index
        :param index: The index of the audio source to terminate
        Use get_test_audio for finding the index
        """
        try:
            testsrc = self._running_tests_audio[int(index)]
        except IndexError:
            raise InvalidIndexError(
                ("No audio source with index:{0},"
                 "use get_test_audio() to determine index")
                .format(index))
        except ValueError:
            raise InvalidIndexError("Index should be a valid integer")
        except TypeError:
            raise InvalidIndexError("Index should be a valid integer")

        self.log.debug('terminating audio-source with wave %s', testsrc.wave)
        testsrc.end()
        self._running_tests_audio.remove(self._running_tests_audio[index])

    def terminate_audio(self):
        """Terminate all test audio sources
        """
        for _ in range(len(self._running_tests_audio)):
            self.terminate_index_audio(0)


class PreviewSinks(object):

    """A Controller for preview sinks to preview ports of gst-switch-srv
    :param preview_port: The preview port to get the preview
    """

    def __init__(self, preview_port=3001):
        super(PreviewSinks, self).__init__()
        self._preview_port = None
        self.preview = None

        self.log = logging.getLogger('previewsinks')

        self.preview_port = preview_port

    @property
    def preview_port(self):
        """Get the preview port"""
        return self._preview_port

    @preview_port.setter
    def preview_port(self, preview_port):
        """Set the preview port
        :raises RangeError: preview port must be in range 1 to 65535
        :raises TypeError: preview port must be a integer or convertable to
        an integer
        """
        if not preview_port:
            raise ValueError("Preview_port: '{0}' cannot be blank"
                             .format(preview_port))
        else:
            try:
                i = int(preview_port)
                if i < 1 or i > 65535:
                    raise RangeError(
                        'Preview port must be in range 1 to 65535')
                else:
                    self._preview_port = preview_port
            except TypeError:
                raise TypeError("Preview_port must be a string or number,"
                                "not, '{0}'".format(type(preview_port)))
            except ValueError:
                raise TypeError("Port must be a valid number")

    def run(self):
        """Run the Preview Sink"""
        self.preview = testsource.Preview(self.preview_port)
        self.log.debug('starting preview')
        self.preview.run()

    def terminate(self):
        """End/Terminate the Preview Sink"""
        try:
            self.log.debug('ending preview')
            self.preview.end()
            self.preview = None
        except AttributeError:
            raise AttributeError("No preview Sink to terminate")
