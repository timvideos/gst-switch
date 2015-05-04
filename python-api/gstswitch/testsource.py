"""
The testsource contains all gstreamer pipelines
It provides the abse for all the other gstreamer
components are build upon.
"""

from __future__ import absolute_import, print_function, unicode_literals

import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst

GObject.threads_init()
Gst.init(None)

from .exception import RangeError
import random

# from pipeline import *
# IMPORTS

# __all__ = ["Preview", "VideoSrc", "AudioSrc"]


class BasePipeline(Gst.Pipeline):

    """A Basic pipeline
    """

    def __init__(self):
        Gst.Pipeline.__init__(self)
        self._playing = False

    def play(self):
        """Set the pipeline as playing"""
        self._playing = True
        self.set_state(Gst.State.PLAYING)

    def pause(self):
        """Pause the pipeline"""
        self._playing = False
        self.set_state(Gst.State.PAUSED)

    def disable(self):
        """Disable the pipeline"""
        self._playing = False
        self.set_state(Gst.State.NULL)

    @classmethod
    def make(cls, elem, description=''):
        """Make a new gstreamer pipeline element
        :param elem: The name of the element to be made
        :param description: Description of the element to be made
        returns: The element which is made
        """
        element = Gst.ElementFactory.make(elem, description)
        return element


class VideoPipeline(BasePipeline):
    """A Video Pipeline which can be used by a Video Test Source
    :param port: The port of where the TCP stream will be sent
    Should be same as video port of gst-switch-src
    :param width: The width of the output video
    :param height: The height of the output video
    :param pattern: The videotestsrc pattern of the output video
    :param timeoverlay: True to enable a running time over video
    :param clockoverlay: True to enable current clock time over video
    """

    VIDEO_CAPS = """
video/x-raw,
  format=(string)I420, pixel-aspect-ratio=(fraction)1/1,
  width=(int){0}, height=(int){1},
  framerate=(fraction)25/1
"""

    def __init__(
            self,
            port,
            host='127.0.0.1',
            width=300,
            height=200,
            pattern=None,
            timeoverlay=False,
            clockoverlay=False):
        super(VideoPipeline, self).__init__()

        self.host = host

        src = self.make_videotestsrc(pattern)
        self.add(src)
        vfilter = self.make_capsfilter(width, height)
        self.add(vfilter)
        src.link(vfilter)
        gdppay = self.make_gdppay()
        self.add(gdppay)
        if timeoverlay:
            _timeoverlay = self.make_timeoverlay()
        if clockoverlay:
            _clockoverlay = self.make_clockoverlay()
        if timeoverlay and clockoverlay:
            self.add(_timeoverlay)
            self.add(_clockoverlay)
            vfilter.link(_timeoverlay)
            _timeoverlay.link(_clockoverlay)
            _clockoverlay.link(gdppay)
        elif timeoverlay:
            self.add(_timeoverlay)
            vfilter.link(_timeoverlay)
            _timeoverlay.link(gdppay)
        elif clockoverlay:
            self.add(_clockoverlay)
            vfilter.link(_clockoverlay)
            _clockoverlay.link(gdppay)
        else:
            vfilter.link(gdppay)

        sink = self.make_tcpclientsink(port)
        self.add(sink)
        gdppay.link(sink)

    def make_videotestsrc(self, pattern):
        """Return a videotestsrc element
        :param pattern: The video test source pattern (1-20)
        :return: A video test source element
        """
        element = self.make('videotestsrc', 'src')
        element.set_property('pattern', int(pattern))
        return element

    def make_capsfilter(self, width, height):
        """Return a caps filter
        :param width: The width of the caps
        :param height: The height of the caps
        :returns: A caps filter element
        """
        element = self.make("capsfilter", "vfilter")
        width = str(width)
        height = str(height)
        capsstring = self.VIDEO_CAPS.format(width, height)
        caps = Gst.Caps.from_string(capsstring)
        element.set_property('caps', caps)
        return element

    def make_gdppay(self):
        """Return a gdppay element
        :returns: A gdppay element
        """
        element = self.make('gdppay', 'gdppay')
        return element

    def make_timeoverlay(self):
        """Return a time overlay element (Verdana bold 50)
        :returns: a time overlay element
        """
        element = self.make('timeoverlay', 'timeoverlay')
        element.set_property('font-desc', "Verdana bold 50")
        return element

    def make_clockoverlay(self):
        """Return a clock overlay element (Verdana bold 50)
        :returns: a clock overlay element
        """
        element = self.make('clockoverlay', 'clockoverlay')
        element.set_property('font-desc', "Verdana bold 50")
        return element

    def make_tcpclientsink(self, port):
        """Return a TCP client sink element
        :port: Port to sink
        :returns: A TCP client sink element
        """
        element = self.make('tcpclientsink', 'tcpclientsink')
        element.set_property('host', self.host)
        element.set_property('port', int(port))
        return element


class AudioPipeline(BasePipeline):
    """docstring for AudioPipeline"""

    AUDIO_CAPS = """
audio/x-raw,
  rate=48000, channels=2,
  format=S16LE, layout=interleaved
"""

    def __init__(
            self,
            port,
            host='127.0.0.1',
            freq=110,
            wave=None):
        super(AudioPipeline, self).__init__()

        self.host = host

        src = self.make_audiotestsrc(freq, wave)
        self.add(src)
        afilter = self.make_capsfilter()
        self.add(afilter)
        src.link(afilter)
        gdppay = self.make_gdppay()
        self.add(gdppay)
        afilter.link(gdppay)
        sink = self.make_tcpclientsink(port)
        self.add(sink)
        gdppay.link(sink)

    def make_capsfilter(self):
        """Return a caps filter
        :returns: A caps filter element
        """
        element = self.make("capsfilter", "afilter")
        capsstring = self.AUDIO_CAPS
        caps = Gst.Caps.from_string(capsstring)
        element.set_property('caps', caps)
        return element

    def make_audiotestsrc(self, freq, wave=None):
        """Return a Audio Source Element
        :freq: The Frequency
        :wave: The wave pattern (valid between 1 and 12)
        """
        element = self.make('audiotestsrc', 'src')
        element.set_property('freq', int(freq))
        element.set_property('wave', int(wave))
        return element

    def make_gdppay(self):
        """Return a gdppay element
        :returns: A gdppay element
        """
        element = self.make('gdppay', 'gdppay')
        return element

    def make_tcpclientsink(self, port):
        """Return a TCP client sink element
        :port: Port to sink
        :returns: A TCP client sink element
        """
        element = self.make('tcpclientsink', 'tcpclientsink')
        element.set_property('host', self.host)
        element.set_property('port', int(port))
        return element


class PreviewPipeline(BasePipeline):

    """Pipeline for usage by a Preview
    :param port: The preview port
    """

    def __init__(self, port):
        super(PreviewPipeline, self).__init__()
        self.host = '127.0.0.1'
        self.preview_port = port
        src = self.make_tcpclientsrc()
        self.add(src)

        gdpdepay = self.make_gdpdepay()
        self.add(gdpdepay)
        src.link(gdpdepay)

        conv1 = self.make_videoconvert("conv1")
        self.add(conv1)
        gdpdepay.link(conv1)

        cairo = self.make_cairooverlay()
        self.add(cairo)
        conv1.link(cairo)

        conv2 = self.make_videoconvert("conv2")
        self.add(conv2)
        cairo.link(conv2)

        sink = self.make_xvimagesink()
        self.add(sink)
        conv2.link(sink)

    def make_tcpclientsrc(self):
        """Return a TCP Client Source element
        :param port: The port of the server
        :returns: A TCP Client Source element
        """
        element = self.make('tcpclientsrc', 'tcpclientsrc')
        element.set_property('host', self.host)
        element.set_property('port', self.preview_port)
        return element

    def make_gdpdepay(self):
        """Return a gdpdepay element
        :returns: A gdpdepay element
        """
        element = self.make('gdpdepay', 'gdpdepay')
        return element

    def make_videoconvert(self, desc):
        """Return a videoconvert element
        :returns: A videoconvert element
        """
        element = self.make('videoconvert', desc)
        return element

    def make_cairooverlay(self):
        """Return a cairooverlay element
        :returns: A cairooverlay element
        """
        element = self.make('cairooverlay', 'cairooverlay')
        return element

    def make_xvimagesink(self):
        """Return a xvimagesink element
        :returns: A xvimagesink element
        """
        element = self.make('xvimagesink', 'xvimagesink')
        element.set_property('sync', 'false')
        return element


class VideoSrc(object):

    """A Test Video Source
    :param width: The width of the output video
    :param height: The height of the output video
    :param pattern: The videotestsrc pattern of the output video
    None for random
    :param timeoverlay: True to enable a running time over video
    :param clockoverlay: True to enable current clock time over video
    """
    HOST = '127.0.0.1'

    PATTERN_SMPTE = 0
    PATTERN_SNOW = 1
    PATTERN_BLACK = 2
    PATTERN_WHITE = 3
    PATTERN_RED = 4
    PATTERN_GREEN = 5
    PATTERN_BLUE = 6
    PATTERN_CHECKERS1 = 7
    PATTERN_CHECKERS2 = 8
    PATTERN_CHECKERS4 = 9
    PATTERN_CHECKERS8 = 10
    PATTERN_CIRCULAR = 11
    PATTERN_BLINK = 12
    PATTERN_SMPTE75 = 13
    PATTERN_ZONE_PLATE = 14
    PATTERN_GAMUT = 15
    PATTERN_CHROMA_ZONE_PLATE = 16
    PATTERN_SOLID_COLOR = 17
    PATTERN_BALL = 18
    PATTERN_SMPTE100 = 19
    PATTERN_BAR = 20
    PATTERN_PINWHEEL = 21
    PATTERN_SPOKES = 22

    def __init__(
            self,
            port,
            width=300,
            height=200,
            pattern=None,
            timeoverlay=False,
            clockoverlay=False):
        super(VideoSrc, self).__init__()
        self._port = None
        self._width = None
        self._height = None
        self._pattern = None
        self._timeoverlay = None
        self._clockoverlay = None

        self.port = port
        self.width = width
        self.height = height
        self.pattern = self.generate_pattern(pattern)
        self.timeoverlay = timeoverlay
        self.clockoverlay = clockoverlay
        self.pipeline = VideoPipeline(
            self.port,
            self.HOST,
            self.width,
            self.height,
            self.pattern,
            self.timeoverlay,
            self.clockoverlay)

    @property
    def port(self):
        """Get the Video Port"""
        return self._port

    @port.setter
    def port(self, port):
        """Set the video Port
        :raises RangeError: Port not in range 1 to 65535
        :raises TypeError: Port cannot be converted to integer
        :raises ValueError: Port cannot be left blank
        """
        if not port:
            raise ValueError("Port: '{0}' cannot be blank"
                             .format(port))
        else:
            try:
                i = int(port)
                if i < 1 or i > 65535:
                    raise RangeError('Port must be in range 1 to 65535')
                else:
                    self._port = port
            except TypeError:
                raise TypeError("Port must be a string or number,"
                                "not, '{0}'".format(type(port)))
            except ValueError:
                raise TypeError("Port must be a valid number")

    @property
    def width(self):
        """Get the width"""
        return self._width

    @width.setter
    def width(self, width):
        """Set the Width
        raises ValueError: Width must be a positive value
        raises ValueError: Width must not be blank
        raises TypeError: Width must be convertable into a float
        """
        if not width:
            raise ValueError("Width: '{0}' cannot be blank"
                             .format(width))
        try:
            i = float(width)
            if i <= 0:
                raise ValueError("Width: '{0}' must be a valid positive value")
            else:
                self._width = width
        except TypeError:
            raise TypeError("Width must be a valid value not '{0}'"
                            .format(type(width)))

    @property
    def height(self):
        """Get the height"""
        return self._height

    @height.setter
    def height(self, height):
        """Set the height
        raises ValueError: Height must be a positive value
        raises ValueError: Height must not be blank
        raises TypeError: Height must be convertable into a float
        """
        if not height:
            raise ValueError("Height: '{0}' cannot be blank"
                             .format(height))
        try:
            i = float(height)
            if i <= 0:
                raise ValueError(
                    "Height: '{0}' must be a valid positive value")
            else:
                self._height = height
        except TypeError:
            raise TypeError("Height must be a valid value not '{0}'"
                            .format(type(height)))

    @property
    def pattern(self):
        """Get the Pattern"""
        return self._pattern

    @pattern.setter
    def pattern(self, pattern):
        """Set the Pattern
        :raises RangeError: Pattern must be in range 0 to 19
        :raises TypeError: Pattern must be convertable to an integer
        """
        try:
            i = int(float(pattern))
            if i < 0 or i > 19:
                raise RangeError('Pattern must be in range 0 and 19')
            else:
                self._pattern = pattern
        except ValueError:
            raise TypeError("Pattern must be a valid number")

    @property
    def timeoverlay(self):
        """Get the timeoverlay"""
        return self._timeoverlay

    @timeoverlay.setter
    def timeoverlay(self, timeoverlay):
        """Set the timeoverlay
        :raises ValueError: Timeoverlay must be True or False
        """
        timeover = str(timeoverlay)
        if timeover == 'True' or timeover == 'False':
            self._timeoverlay = timeoverlay
        else:
            raise ValueError("Timeoverlay: '{0}' must be True of False"
                             .format(timeoverlay))

    @property
    def clockoverlay(self):
        """Get the Clockoverlay"""
        return self._clockoverlay

    @clockoverlay.setter
    def clockoverlay(self, clockoverlay):
        """Set the Clockoverlay
        :raises ValueError: Clockoverlay must be True or False
        """
        clockover = str(clockoverlay)
        if clockover == 'True' or clockover == 'False':
            self._clockoverlay = clockoverlay
        else:
            raise ValueError("Clockoverlay: '{0}' must be True of False"
                             .format(clockoverlay))

    def run(self):
        """Run the pipeline"""
        self.pipeline.play()

    def pause(self):
        """End the pipeline"""
        self.pipeline.pause()

    def end(self):
        """End/disable the pipeline"""
        self.pipeline.disable()

    @classmethod
    def generate_pattern(cls, pattern):
        """Generate a random pattern if not specified
        """
        if pattern is None:
            pattern = random.randint(0, 19)
        pattern = str(pattern)
        return pattern


class AudioSrc(object):

    """docstring for AudioSrc"""

    HOST = '127.0.0.1'

    WAVE_SINE = 0
    WAVE_SQUARE = 1
    WAVE_SAW = 2
    WAVE_TRIANGLE = 3
    WAVE_SILENCE = 4
    WAVE_WHITE_NOISE = 5
    WAVE_PINK_NOISE = 6
    WAVE_SINE_TABLE = 7
    WAVE_TICKS = 8
    WAVE_GAUSSIAN_NOISE = 9
    WAVE_RED_NOISE = 10
    WAVE_BLUE_NOISE = 11
    WAVE_VIOLET_NOISE = 12

    def __init__(
            self,
            port,
            freq=110,
            wave=None):
        super(AudioSrc, self).__init__()
        self._port = None
        self._freq = None
        self._wave = None

        self.port = port
        self.freq = freq
        self.wave = self.generate_wave(wave)
        self.pipeline = AudioPipeline(
            self.port,
            self.HOST,
            self.freq,
            self.wave)

    @property
    def port(self):
        """Get the Audio Port"""
        return self._port

    @port.setter
    def port(self, port):
        """Set the Audio Port
        :raises RangeError: Port not in range 1 to 65535
        :raises TypeError: Port cannot be converted to integer
        :raises ValueError: Port cannot be left blank
        """
        if not port:
            raise ValueError("Port: '{0}' cannot be blank"
                             .format(port))
        else:
            try:
                i = int(port)
                if i < 1 or i > 65535:
                    raise RangeError('Port must be in range 1 to 65535')
                else:
                    self._port = port
            except TypeError:
                raise TypeError("Port must be a string or number,"
                                "not, '{0}'".format(type(port)))
            except ValueError:
                raise TypeError("Port must be a valid number")

    @property
    def freq(self):
        """Get the Frequency"""
        return self._freq

    @freq.setter
    def freq(self, freq):
        """Set the Frequency
        raises ValueError: Width must be a positive value
        raises ValueError: Width must not be blank
        raises TypeError: Width must be convertable into a float
        """
        if not freq:
            raise ValueError("Frequency: '{0}' cannot be blank"
                             .format(freq))
        else:
            try:
                i = int(freq)
                if i < 1:
                    raise RangeError("Frequency must be a positive value")
                else:
                    self._freq = freq
            except TypeError:
                raise TypeError("Frequency must be a string or number,"
                                "not, '{0}'".format(type(freq)))
            except ValueError:
                raise TypeError("Frequency must be a valid number")

    @property
    def wave(self):
        """Get the wave number"""
        return self._wave

    @wave.setter
    def wave(self, wave):
        """Set the Wave number
        :raises RangeError: Wave must be in range 0 to 12
        :raises TypeError: Wave must be convertable to integer
        """
        try:
            i = int(float(wave))
            if i < 0 or i > 12:
                raise RangeError('Wave must be in range 0 and 12')
            else:
                self._wave = wave
        except ValueError:
            raise TypeError("Wave must be a valid number")

    def run(self):
        """Run the pipeline"""
        self.pipeline.play()

    def pause(self):
        """End the pipeline"""
        self.pipeline.pause()

    def end(self):
        """End/disable the pipeline"""
        self.pipeline.disable()

    @classmethod
    def generate_wave(cls, wave):
        """Generate a random wave if not specified
        """
        if wave is None:
            wave = random.randint(0, 12)
        wave = str(wave)
        return wave


class Preview(object):

    """A Preview Element
    :param port: The preview port
    """

    def __init__(self, port):
        super(Preview, self).__init__()
        self._preview_port = None

        self.preview_port = port
        self.pipeline = PreviewPipeline(self.preview_port)

    @property
    def preview_port(self):
        """Get the Preview Port"""
        return self._preview_port

    @preview_port.setter
    def preview_port(self, preview_port):
        """Set the Preview Port
        :raises RangeError: Port not in range 1 to 65535
        :raises TypeError: Port cannot be converted to integer
        :raises ValueError: Port cannot be left blank
        """
        if not preview_port:
            raise ValueError("preview_port: '{0}' cannot be blank"
                             .format(preview_port))
        else:
            try:
                i = int(preview_port)
                if i < 1 or i > 65535:
                    raise RangeError(
                        'preview_port must be in range 1 to 65535')
                else:
                    self._preview_port = preview_port
            except TypeError:
                raise TypeError("preview_port must be a string or number,"
                                "not, '{0}'".format(type(preview_port)))
            except ValueError:
                raise TypeError("Port must be a valid number")

    def run(self):
        """Run the pipeline"""
        self.pipeline.play()

    def pause(self):
        """Pause the pipeline"""
        self.pipeline.pause()

    def end(self):
        """End/disable the pipeline"""
        self.pipeline.disable()
