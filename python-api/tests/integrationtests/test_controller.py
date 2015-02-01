"""
Integration Tests for the dbus Controller
"""

from __future__ import absolute_import, print_function, unicode_literals

import sys
import os
import pytest
import time
import datetime
import subprocess

from gi.repository import GLib
from mock import Mock
from integrationtests.compare import CompareVideo

sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))
from gstswitch.server import Server
from gstswitch.helpers import TestSources, PreviewSinks
from gstswitch.controller import Controller

# PATH = os.getenv("HOME") + '/gst/stage/bin/'
PATH = '../tools/'


class TestEstablishConnection(object):

    """Test establish_connection method"""
    NUM = 1
    # fails above 3

    def establish_connection(self):
        """Create Controller object and call establish_connection"""
        controller = Controller()
        controller.establish_connection()
        # print(controller.connection)
        assert controller.connection is not None

    def test_establish(self):
        """Test for establish_connection"""
        serv = Server(path=PATH, video_format="debug")
        try:
            serv.run()
            for i in range(self.NUM):
                print(i)
                self.establish_connection()
            serv.terminate(1)
        finally:
            serv.terminate_and_output_status(cov=True)


class TestGetComposePort(object):

    """Test get_compose_port method"""
    NUM = 1
    FACTOR = 1

    def get_compose_port(self):
        """Create Controller and call get_compose_port method"""
        res = []
        controller = Controller()
        controller.establish_connection()
        for _ in range(self.NUM * self.FACTOR):
            res.append(controller.get_compose_port())
        return res

    def test_compose_ports(self):
        """Test get_compose_port"""
        res = []
        expected_result = []
        for i in range(self.NUM):
            video_port = (i + 7) * 1000
            expected_result.append([video_port + 1] * self.NUM * self.FACTOR)
            serv = Server(path=PATH, video_port=video_port)
            try:
                serv.run()
                sources = TestSources(video_port=video_port)
                sources.new_test_video()
                sources.new_test_video()

                res.append(self.get_compose_port())
                sources.terminate_video()
                serv.terminate(1)
            finally:
                serv.terminate_and_output_status(cov=True)

        set_expected = [tuple(i) for i in expected_result]
        set_res = [tuple(i) for i in res]
        assert set(set_expected) == set(set_res)


class TestGetEncodePort(object):

    """Test get_encode_port method"""
    NUM = 1
    FACTOR = 1

    def get_encode_port(self):
        """Create a Controller object and call get_encode_port method"""
        res = []
        controller = Controller()
        controller.establish_connection()
        for _ in range(self.NUM * self.FACTOR):
            res.append(controller.get_encode_port())
        return res

    def test_encode_ports(self):
        """Test get_encode_port"""
        res = []
        expected_result = []
        for i in range(self.NUM):
            video_port = (i + 8) * 1000
            expected_result.append([video_port + 2] * self.NUM * self.FACTOR)
            serv = Server(path=PATH, video_port=video_port)
            try:
                serv.run()
                sources = TestSources(video_port=video_port)
                sources.new_test_video()
                sources.new_test_video()

                res.append(self.get_encode_port())
                sources.terminate_video()
                serv.terminate(1)
            finally:
                serv.terminate_and_output_status(cov=True)

        set_expected = [tuple(i) for i in expected_result]
        set_res = [tuple(i) for i in res]
        assert set(set_expected) == set(set_res)


class TestGetAudioPort(object):

    """Test get_audio_port method"""
    NUM = 1
    FACTOR = 1

    def get_audio_port(self):
        """Create Controller object and call get_audio_port method"""
        res = []
        controller = Controller()
        controller.establish_connection()
        for _ in range(self.NUM * self.FACTOR):
            res.append(controller.get_audio_port())
        return res

    def test_audio_ports(self):
        """Test get_audio_port"""
        res = []
        expected_result = []
        for i in range(1, self.NUM + 1):
            audio_port = (i + 10) * 1000
            expected_result.append([3003] * self.NUM * self.FACTOR)
            serv = Server(path=PATH, video_port=3000, audio_port=audio_port)
            try:
                serv.run()
                sources = TestSources(video_port=3000, audio_port=audio_port)
                sources.new_test_audio()

                res.append(self.get_audio_port())

                sources.terminate_audio()
                serv.terminate(1)

            finally:
                serv.terminate_and_output_status(cov=True)

        set_expected = [tuple(i) for i in expected_result]
        set_res = [tuple(i) for i in res]
        assert set(set_expected) == set(set_res)


class TestGetPreviewPorts(object):

    """Test get_preview_ports method"""
    NUM = 1
    FACTOR = 1

    def get_preview_ports(self):
        """Create Controller object and call get_preview_ports method"""
        res = []
        controller = Controller()
        controller.establish_connection()
        for _ in range(self.NUM * self.FACTOR):
            res.append(controller.get_preview_ports())
        return res

    def test_get_preview_ports(self):
        """Test get_preview_ports"""

        for _ in range(self.NUM):
            serv = Server(path=PATH, video_format="debug")
            try:
                serv.run()
                sources = TestSources(video_port=3000, audio_port=4000)
                for _ in range(self.NUM):
                    sources.new_test_audio()
                    sources.new_test_video()
                expected_result = map(
                    tuple,
                    [[x for x in range(3003, 3004 + self.NUM)]]
                    * self.NUM * self.FACTOR)
                res = map(tuple, self.get_preview_ports())
                print('\n', res, '\n')
                print(expected_result)
                assert set(expected_result) == set(res)
                sources.terminate_video()
                sources.terminate_audio()
                serv.terminate(1)
            finally:
                serv.terminate_and_output_status(cov=True)


class TestSignals(object):

    """Test on_*() methods"""

    def __init__(self):
        self.mainloop = GLib.MainLoop()
        self.quit_count = 0

    def run_mainloop(self):
        """Start the MainLoop, set Quit-Counter to Zero"""
        self.quit_count = 0
        self.mainloop.run()

    def quit_mainloop(self, *_):
        """Quit the MainLoop, set Quit-Counter to Zero"""
        self.mainloop.quit()
        self.quit_count = 0

    def quit_mainloop_after(self, num):
        """Increment Quit-Counter, if it reaches num, Quit the MainLoop"""
        self.quit_count += 1
        if self.quit_count == num:
            self.quit_mainloop()

    def test_on_new_mode_online(self):
        """Create a Controller object, call on_new_mode_online method and
        check that the callback fires
        """
        serv = Server(path=PATH)
        try:
            serv.run()

            controller = Controller()
            controller.establish_connection()

            test_cb = Mock(side_effect=self.quit_mainloop)
            controller.on_new_mode_online(test_cb)
            controller.set_composite_mode(0)

            GLib.timeout_add_seconds(5, self.quit_mainloop)
            self.run_mainloop()

            test_cb.assert_called_once_with(0)

            serv.terminate(1)
        finally:
            serv.terminate_and_output_status(cov=True)

    def test_on_preview_port_added(self):
        """Create a Controller object, call add a source method and
        check that the callback fires
        """
        serv = Server(path=PATH, video_port=3000)
        try:
            serv.run()

            controller = Controller()
            controller.establish_connection()

            test_cb = Mock(side_effect=lambda mode, serve, type:
                           self.quit_mainloop_after(2))
            controller.on_preview_port_added(test_cb)

            sources = TestSources(video_port=3000)
            sources.new_test_video()
            sources.new_test_video()

            GLib.timeout_add_seconds(5, self.quit_mainloop)
            self.run_mainloop()

            print(test_cb.call_args_list)
            test_cb.assert_any_call(3003, 1, 7)
            test_cb.assert_any_call(3004, 1, 8)
            assert test_cb.call_count == 2

            serv.terminate(1)
        finally:
            serv.terminate_and_output_status(cov=True)


class VideoFileSink(object):

    """Sink the video to a file
    """

    def __init__(self, port, filename):
        cmd = "gst-launch-1.0 tcpclientsrc port={0} ! gdpdepay !  jpegenc \
        ! avimux ! filesink location={1}".format(port, filename)
        with open(os.devnull, 'w') as tempf:
            self.proc = subprocess.Popen(
                cmd.split(),
                stdout=tempf,
                stderr=tempf,
                bufsize=-1,
                shell=False)

    def terminate(self):
        """Terminate sinking"""
        self.proc.terminate()


class TestSetCompositeMode(object):

    """Test set_composite_mode method"""
    NUM = 1
    FACTOR = 1

    def set_composite_mode(self, mode, generate_frames=False):
        """Create Controller object and call set_composite_mode method"""
        for _ in range(self.NUM):

            serv = Server(path=PATH, video_format="debug")
            try:
                serv.run()

                preview = PreviewSinks()
                preview.run()

                out_file = 'output-{0}.data'.format(mode)
                video_sink = VideoFileSink(serv.video_port + 1, out_file)

                sources = TestSources(video_port=3000)
                sources.new_test_video(pattern=4)
                sources.new_test_video(pattern=5)

                time.sleep(3)
                # expected_result = [mode != 3] * self.FACTOR
                # print(mode, expected_result)
                controller = Controller()
                res = controller.set_composite_mode(mode)
                print(res)
                time.sleep(3)
                video_sink.terminate()
                preview.terminate()
                sources.terminate_video()
                serv.terminate(1)
                if not generate_frames:
                    controller = Controller()
                    if mode == Controller.COMPOSITE_DUAL_EQUAL:
                        assert res is False
                    else:
                        assert res is True
                    assert self.verify_output(mode, out_file) is True
                # assert expected_result == res

            finally:
                serv.terminate_and_output_status(cov=True)

    def verify_output(self, mode, video):
        """Verify if the output is correct by comparing key frames"""
        test = 'composite_mode_{0}'.format(mode)
        cmpr = CompareVideo(test, video)
        res1, res2 = cmpr.compare()
        print("RESULTS", res1, res2)

        # In the CI environment, upload to imgur the results.
        if os.environ.get('CI', "False") == "true":
            folder = cmpr.test_frame_dir
            cmd = "./imgurbash.sh {0}/*.*".format(folder)
            print(cmd)
            proc = subprocess.Popen(
                cmd,
                bufsize=-1,
                shell=True)
            print(proc.wait())

        # Experimental Value
        if res1 <= 0.04 and res2 <= 0.04:
            return True
        return False

    def test_set_composite_mode_none(self):
        """Test set_composite_mode"""
        self.set_composite_mode(Controller.COMPOSITE_NONE)

    def test_set_composite_mode_pip(self):
        """Test set_composite_mode"""
        self.set_composite_mode(Controller.COMPOSITE_PIP)

    def test_set_composite_mode_preview(self):
        """Test set_composite_mode"""
        self.set_composite_mode(Controller.COMPOSITE_DUAL_PREVIEW)

    def test_set_composite_mode_equal(self):
        """Test set_composite_mode"""
        self.set_composite_mode(Controller.COMPOSITE_DUAL_EQUAL)


class TestNewRecord(object):

    """Test new_record method"""
    NUM = 1
    FACTOR = 1

    def new_record(self):
        """Create a Controller object and call new_record method"""
        res = []
        controller = Controller()
        for _ in range(self.NUM * self.FACTOR):
            res.append(controller.new_record())
        return res

    def test_new_record(self):
        """Test new_record"""
        for _ in range(self.NUM):
            serv = Server(path=PATH, record_file="test-%Y.data",
                          video_format="debug")
            try:
                serv.run()

                sources = TestSources(video_port=3000)
                sources.new_test_video()
                sources.new_test_video()

                curr_time = datetime.datetime.now()
                time_str = curr_time.strftime('%Y')
                test_filename = "test-{0}.data".format(time_str)

                res = self.new_record()
                print(res)
                sources.terminate_video()
                serv.terminate(1)
                assert os.path.exists(test_filename) is True
            finally:
                serv.terminate_and_output_status(cov=True)


class TestAdjustPIP(object):

    """Test adjust_pip method"""
    NUM = 1
    FACTOR = 1

    def adjust_pip(self,
                   xpos,
                   ypos,
                   width,
                   heigth,
                   index,
                   generate_frames=False):
        """Create Controller object and call adjust_pip"""
        for _ in range(self.NUM):
            serv = Server(path=PATH, video_format="debug")
            try:
                serv.run()
                sources = TestSources(video_port=3000)
                preview = PreviewSinks()
                preview.run()
                out_file = "output-{0}.data".format(index)
                video_sink = VideoFileSink(3001, out_file)
                sources.new_test_video(pattern=4)
                sources.new_test_video(pattern=5)
                controller = Controller()
                controller.set_composite_mode(Controller.COMPOSITE_PIP)
                time.sleep(3)
                res = controller.adjust_pip(xpos, ypos, width, heigth)
                time.sleep(3)
                sources.terminate_video()
                preview.terminate()
                video_sink.terminate()
                serv.terminate(1)
                if not generate_frames:
                    assert res is not None
                    assert self.verify_output(index, out_file) is True

            finally:
                serv.terminate_and_output_status(cov=True)

    def verify_output(self, index, video):
        """Verify if the output is correct by comparing key frames"""
        test = 'adjust_pip_{0}'.format(index)
        cmpr = CompareVideo(test, video)
        res1, res2 = cmpr.compare()
        print("RESULTS", res1, res2)
        #   Experimental Value
        if res1 <= 0.04 and res2 <= 0.04:
            return True
        return False

    @pytest.mark.xfail
    def test_adjust_pip(self):
        """Test adjust_pip"""
        dic = [
            [50, 75, 0, 0],
        ]
        for i in range(4, 5):
            self.adjust_pip(
                dic[i - 4][0],
                dic[i - 4][1],
                dic[i - 4][2],
                dic[i - 4][3],
                i)


class TestSwitch(object):

    """Test switch method"""

    NUM = 1

    def switch(self, channel, port, index):
        """Create Controller object and call switch method"""
        for _ in range(self.NUM):
            serv = Server(path=PATH, video_format="debug")
            try:
                serv.run()

                sources = TestSources(3000)
                sources.new_test_video(pattern=4)
                sources.new_test_video(pattern=5)
                preview = PreviewSinks(3001)
                preview.run()
                out_file = "output-{0}.data".format(index)
                video_sink = VideoFileSink(3001, out_file)
                time.sleep(3)
                controller = Controller()
                res = controller.switch(channel, port)
                print(res)
                time.sleep(3)
                video_sink.terminate()
                sources.terminate_video()
                preview.terminate()
                serv.terminate(1)

            finally:
                serv.terminate_and_output_status(cov=True)

    def test_switch(self):
        """Test switch"""
        dic = [
            [Controller.VIDEO_CHANNEL_A, 3004]
        ]
        start = 5
        for i in range(start, 6):
            self.switch(dic[i - start][0], dic[i - start][1], i)


class TestClickVideo(object):

    """Test click_video method"""
    NUM = 1
    FACTOR = 1

    def click_video(self,
                    xpos,
                    ypos,
                    width,
                    heigth,
                    index,
                    generate_frames=False):
        """Create Controller object and call click_video method"""
        for _ in range(self.NUM):
            serv = Server(path=PATH, video_format="debug")
            try:
                serv.run()
                sources = TestSources(video_port=3000)
                preview = PreviewSinks()
                preview.run()
                out_file = "output-{0}.data".format(index)
                video_sink = VideoFileSink(3001, out_file)
                sources.new_test_video(pattern=4)
                sources.new_test_video(pattern=5)
                controller = Controller()
                time.sleep(1)
                res = controller.click_video(xpos, ypos, width, heigth)
                print(res)
                time.sleep(1)
                sources.terminate_video()
                preview.terminate()
                video_sink.terminate()
                serv.terminate(1)
                if not generate_frames:
                    assert res is not None
                    assert self.verify_output(index, out_file) is True

            finally:
                serv.terminate_and_output_status(cov=True)

    def verify_output(self, index, video):
        """Verify if the output is correct by comparing key frames"""
        test = 'click_video_{0}'.format(index)
        cmpr = CompareVideo(test, video)
        res1, res2 = cmpr.compare()
        print("RESULTS", res1, res2)
        #   Experimental Value
        if res1 <= 0.04 and res2 <= 0.04:
            return True
        return False

    def test_click_video(self):
        """Test click_video"""
        dic = [
            [0, 0, 10, 10],
        ]
        start = 6
        for i in range(start, 7):
            self.click_video(
                dic[i - start][0],
                dic[i - start][1],
                dic[i - start][2],
                dic[i - start][3],
                i,
                True)


class TestMarkFace(object):

    """Test mark_face method"""
    NUM = 1

    def mark_face(self, faces, index, generate_frames=False):
        """Create the Controller object and call mark_face method"""
        for _ in range(self.NUM):
            serv = Server(path=PATH, video_format="debug")
            try:
                serv.run()
                sources = TestSources(video_port=3000)
                preview = PreviewSinks()
                preview.run()
                out_file = "output-{0}.data".format(index)
                video_sink = VideoFileSink(3001, out_file)
                sources.new_test_video(pattern=4)
                sources.new_test_video(pattern=5)
                controller = Controller()
                time.sleep(1)
                res = controller.mark_face(faces)
                print(res)
                time.sleep(1)
                sources.terminate_video()
                preview.terminate()
                video_sink.terminate()
                serv.terminate(1)
                if not generate_frames:
                    assert res is not None
                    assert self.verify_output(index, out_file) is True

            finally:
                serv.terminate_and_output_status(cov=True)

    def verify_output(self, index, video):
        """Verify if the output is correct by comparing key frames"""
        test = 'mark_face_{0}'.format(index)
        cmpr = CompareVideo(test, video)
        res1, res2 = cmpr.compare()
        print("RESULTS", res1, res2)
        #   Experimental Value
        if res1 <= 0.04 and res2 <= 0.04:
            return True
        return False

    def test_mark_face(self):
        """Test mark_face"""
        dic = [
            [(1, 1, 1, 1), (10, 10, 1, 1)],
        ]
        start = 7
        for i in range(start, 8):
            self.mark_face(dic[i - start], i, True)


class TestMarkTracking(object):

    """Test mark_tracking method"""
    NUM = 1

    def mark_tracking(self, faces, index, generate_frames=False):
        """Create Controller object and call mark_tracking method"""
        for _ in range(self.NUM):
            serv = Server(path=PATH, video_format="debug")
            try:
                serv.run()
                sources = TestSources(video_port=3000)
                preview = PreviewSinks()
                preview.run()
                out_file = "output-{0}.data".format(index)
                video_sink = VideoFileSink(3001, out_file)
                sources.new_test_video(pattern=4)
                sources.new_test_video(pattern=5)
                controller = Controller()
                time.sleep(1)
                res = controller.mark_tracking(faces)
                print(res)
                time.sleep(1)
                sources.terminate_video()
                preview.terminate()
                video_sink.terminate()
                serv.terminate(1)
                if not generate_frames:
                    assert res is not None
                    assert self.verify_output(index, out_file) is True

            finally:
                serv.terminate_and_output_status(cov=True)

    def verify_output(self, index, video):
        """Verify if the output is correct by comparing key frames"""
        test = 'mark_tracking_{0}'.format(index)
        cmpr = CompareVideo(test, video)
        res1, res2 = cmpr.compare()
        print("RESULTS", res1, res2)
        #   Experimental Value
        if res1 <= 0.04 and res2 <= 0.04:
            return True
        return False

    def test_mark_tracking(self):
        """Test mark_tracking"""
        dic = [
            [(1, 1, 1, 1), (10, 10, 1, 1)],
        ]
        start = 7
        for i in range(start, 8):
            self.mark_tracking(dic[i - start], i, True)
