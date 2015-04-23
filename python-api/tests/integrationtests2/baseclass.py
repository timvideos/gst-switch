"""
Contains the Base class for integration tests.
"""

import sys
import os
import logging
import time
import subprocess
import numpy
import scipy.misc
import scipy.linalg
import gi
gi.require_version('Gst', '1.0')

sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))
from gstswitch.server import Server
from gstswitch.helpers import TestSources
from gstswitch.controller import Controller
from gi.repository import GLib, GObject, Gst
from gstswitch.testsource import VideoSrc, AudioSrc

GObject.threads_init()
Gst.init(None)

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

    def new_test_audio(self, freq=110, wave=AudioSrc.WAVE_SINE):
        """Start a new Audio-Testsource and wait until it's ready"""
        self.serv.wait_for_output('0.0.0.0:4000')
        self._sources.new_test_audio(freq=freq, wave=wave)

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

    def expect_frame(self, filename, port, shape, timeout=5):
        """Read frames from the server and compare them against filename.
        Return when a match is found or timeout seconds have passed"""

        filepath = os.path.join(
            os.path.dirname(__file__),
            'reference_frames',
            filename)

        self.log.debug("loading reference image %s", filepath)
        expected = scipy.misc.imread(filepath).astype(float)

        # build frame-fetching gstreamer pipeline
        appsink = self.get_appsink(port)

        # calculate timestamp to timeout at
        endtime = time.time() + timeout

        # initialize framecounter
        frame = 0

        self.log.debug("start pulling frames from server")
        while True:
            # increment framecounter
            frame += 1

            # pull sample from appsink (blocks)
            sample = appsink.emit('pull_sample')
            if sample is None:
                raise RuntimeError("pulled unexpected NULL-Frame from server")

            # extract RGB-Data out of the sample
            buf = sample.get_buffer()
            self.log.debug("got frame of %u bytes", buf.get_size())
            bytedata = buf.extract_dup(0, buf.get_size())

            # create numpy-array from RGB-Data
            img = numpy.frombuffer(bytedata, numpy.uint8).astype(float)
            img.shape = shape

            # Calulcate image-difference
            diff = abs(img - expected)

            # Calculate comparison-score
            #   # Manhattan norm
            #   m_norm = sum(abs(diff))
            #
            #   # Zero norm
            #   z_norm = scipy.linalg.norm(diff.ravel(), 0) / img.size

            # Maximal deviation between individual subpixels
            max_dev = max(diff.ravel())

            self.log.debug("comparison-results on frame %u: max_dev=%f",
                           frame, max_dev)

            # Compare calculated score
            if max_dev <= 3:
                # Score is small enough to succeed
                self.log.info("comparison succeeded on frame %u"
                              "with max_dev=%f", frame, max_dev)
                break

            # Calculate remaining runtime
            remaining = endtime - time.time()

            # If remaining runtime gets close to 0
            if remaining < 0.001:
                # Save last failed Frame and (in Travis) upload to imageurl
                self.save_images(filename, img, expected, diff)
                appsink.get_parent().set_state(Gst.State.NULL)

                # Throw an Error
                raise RuntimeError(
                    "Timeout while waiting for matching frame %s" % filename)

        appsink.get_parent().set_state(Gst.State.NULL)
        self.log.info("comparison succeeded after %u frames", frame)

    def expect_caps(self, port, expectedcapsstr):
        """Fetch a sample-frame from the server and return its
        caps as string"""
        self.log.debug("building sample-fetching gstreamer pipeline")
        pipeline = Gst.Pipeline()

        tcpsrc = Gst.ElementFactory.make('tcpclientsrc')
        depay = Gst.ElementFactory.make('gdpdepay')
        appsink = Gst.ElementFactory.make('appsink')

        pipeline.add(tcpsrc)
        pipeline.add(depay)
        pipeline.add(appsink)

        tcpsrc.link(depay)
        depay.link(appsink)

        tcpsrc.set_property('host', 'localhost')
        tcpsrc.set_property('port', port)
        appsink.set_property('max-buffers', 2)
        appsink.set_property('drop', True)
        pipeline.set_state(Gst.State.PLAYING)

        self.log.debug("pull a frame from server")
        sample = appsink.emit('pull_sample')
        assert isinstance(sample, Gst.Sample)

        pipeline.set_state(Gst.State.NULL)

        caps = sample.get_caps()
        assert isinstance(caps, Gst.Caps)

        capsstr = caps.to_string()
        self.log.info("got caps=%s", capsstr)
        self.log.info("expected caps=%s", expectedcapsstr)

        expected = Gst.Caps.from_string(expectedcapsstr)
        assert caps.is_subset(expected), \
            "%s is not a subset of %s" % (capsstr, expectedcapsstr)

    def is_running_in_ci(self):
        """Test if the testsuite is ran by Travis-CI"""
        return os.environ.get('CI', "False") == "true"

    def save_images(self, basename, img, expected, diff):
        """Save and (in Travis) upload Failed images"""

        filename = '%s-a_img.png' % (os.path.splitext(basename)[0])
        scipy.misc.imsave(
            filename,
            img.astype(numpy.uint8))

        if self.is_running_in_ci():
            self.log.warn("uploaded failed frame as: %s",
                          self.upload_image(filename))
        else:
            self.log.warn("saved failed frame as: %s",
                          filename)

        filename = '%s-b_expected.png' % (os.path.splitext(basename)[0])
        scipy.misc.imsave(
            filename,
            expected.astype(numpy.uint8))

        if self.is_running_in_ci():
            self.log.warn("uploaded expected frame as: %s",
                          self.upload_image(filename))
        else:
            self.log.warn("saved expected frame as: %s",
                          filename)

        filename = '%s-c_diff.png' % (os.path.splitext(basename)[0])
        scipy.misc.imsave(
            filename,
            diff.astype(numpy.uint8))

        if self.is_running_in_ci():
            self.log.warn("uploaded difference frame as: %s",
                          self.upload_image(filename))
        else:
            self.log.warn("saved difference frame as: %s",
                          filename)

    def upload_image(self, filename):
        """ upload the specified image to imgurl"""
        with open(os.devnull, 'w') as devnull:
            return subprocess.check_output([
                'bash',
                './imgurbash.sh',
                filename
            ], stderr=devnull).decode('utf-8').strip()


class IntegrationTestbaseVideo(IntegrationTestbaseCompare):
    """ Testbase used to test video-pipeline results
    """

    PORT_RED = 3003
    PORT_GREEN = 3004
    PORT_BLUE = 3005
    PORTS = (PORT_RED, PORT_GREEN, PORT_BLUE, )

    def expect_video_frame(self, filename, port=3001, timeout=5):
        """Read frames from the server and compare them against filename.
        Return when a match is found or timeout seconds have passed
        Source-Port defaults to 3001=video compose-port"""

        self.expect_frame(
            filename, port,
            shape=(200, 300, 3), timeout=timeout)

    def get_appsink(self, port):
        """Construct a Gstreamer-Pipeline which receives frames,
        converts them to RGB and pushes them to an Appsink.
        The Appsink is returned."""

        self.log.debug("building video-frame-fetching gstreamer pipeline")
        pipeline = Gst.Pipeline()

        tcpsrc = Gst.ElementFactory.make('tcpclientsrc')
        depay = Gst.ElementFactory.make('gdpdepay')
        conv = Gst.ElementFactory.make('videoconvert')
        appsink = Gst.ElementFactory.make('appsink')

        pipeline.add(tcpsrc)
        pipeline.add(depay)
        pipeline.add(conv)
        pipeline.add(appsink)

        tcpsrc.link(depay)
        depay.link(conv)
        conv.link_filtered(
            appsink,
            Gst.Caps.from_string('video/x-raw,format=RGB'))

        tcpsrc.set_property('host', 'localhost')
        tcpsrc.set_property('port', port)
        appsink.set_property('max-buffers', 2)
        appsink.set_property('drop', True)
        pipeline.set_state(Gst.State.PLAYING)

        return appsink

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


class IntegrationTestbaseAudio(IntegrationTestbaseCompare):
    """ Testbase used to test video-pipeline results
    """

    PORT_110 = 3003
    # PORT_220 = 3004
    PORTS = (PORT_110, )

    def expect_audio_wavescope(self, filename, port=3003, timeout=5):
        """Read frames from the server and compare them against filename.
        Return when a match is found or timeout seconds have passed
        Source-Port defaults to 3001=video compose-port"""

        self.expect_frame(
            filename, port,
            shape=(200, 400, 3), timeout=timeout)

    def get_appsink(self, port):
        """Construct a Gstreamer-Pipeline which receives frames,
        converts them to RGB and pushes them to an Appsink.
        The Appsink is returned."""

        self.log.debug("building audio-sample-fetching gstreamer pipeline")
        pipeline = Gst.Pipeline()

        tcpsrc = Gst.ElementFactory.make('tcpclientsrc')
        depay = Gst.ElementFactory.make('gdpdepay')
        wavescope = Gst.ElementFactory.make('spectrascope')
        conv = Gst.ElementFactory.make('videoconvert')
        appsink = Gst.ElementFactory.make('appsink')

        pipeline.add(tcpsrc)
        pipeline.add(depay)
        pipeline.add(wavescope)
        pipeline.add(conv)
        pipeline.add(appsink)

        tcpsrc.link(depay)
        depay.link(wavescope)
        wavescope.link_filtered(
            conv,
            Gst.Caps.from_string('video/x-raw,width=400,height=200'))
        conv.link_filtered(
            appsink,
            Gst.Caps.from_string('video/x-raw,format=RGB'))

        tcpsrc.set_property('host', 'localhost')
        tcpsrc.set_property('port', port)
        appsink.set_property('max-buffers', 2)
        appsink.set_property('drop', True)
        wavescope.set_property('shader', 'none')
        pipeline.set_state(Gst.State.PLAYING)

        return appsink

    def setup_test(self):
        """Setup Server, Controller and two Video-Test-Sources"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting up 1st AudioSrc")
        self.new_test_audio(freq=110, wave=AudioSrc.WAVE_SINE)

        # self.log.info("starting up 2nd AudioSrc")
        # self.new_test_audio(freq=220, wave=AudioSrc.WAVE_TICKS)

        self.serv.wait_for_output('tcpserversink name=sink', count=1)
