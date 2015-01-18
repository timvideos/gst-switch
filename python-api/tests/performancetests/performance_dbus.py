"""
Performance/Torture test for DBus methods
"""

from __future__ import absolute_import, print_function, unicode_literals

import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))

from gstswitch.server import Server
from gstswitch.helpers import TestSources, PreviewSinks
from gstswitch.controller import Controller
import time

PATH = '../tools/'



def get_compose_port(num):
        """Test get_compose_port - num times"""
        video_port = 8000
        serv = Server(path=PATH, video_port=video_port)
        try:
            serv.run()
            sources = TestSources(video_port=video_port)
            sources.new_test_video()

            expected_res = [video_port + 1] * num

            controller = Controller()
            controller.establish_connection()
            res = []
            for _ in range(num):
                res.append(controller.get_compose_port())

            assert expected_res == res
        finally:
            if serv.proc:
                    poll = serv.proc.poll()
                    if poll == -11:
                        print("SEGMENTATION FAULT OCCURRED")
                    print("ERROR CODE - {0}".format(poll))
                    serv.terminate(1)
                    log = open('server.log')
                    print(log.read())


class TestGetComposePort(object):
    """Performance test for get_compose_port method - 100 times"""

    def test_100(self):
        """Performance test for get_compose_port method"""
        number_test_runs = 100
        get_compose_port(number_test_runs)


    def test_200(self):
        """Performance test for get_compose_port method"""
        number_test_runs = 200
        get_compose_port(number_test_runs)


    def test_300(self):
        """Performance test for get_compose_port method"""
        number_test_runs = 300
        get_compose_port(number_test_runs)


    def test_400(self):
        """Performance test for get_compose_port method"""
        number_test_runs = 400
        get_compose_port(number_test_runs)

    def test_500(self):
        """Performance test for get_compose_port method"""
        number_test_runs = 500
        get_compose_port(number_test_runs)


def get_encode_port(num):
        """Test get_encode_port - num times"""
        video_port = 3000
        serv = Server(path=PATH, video_port=video_port)
        try:
            serv.run()
            sources = TestSources(video_port=video_port)
            sources.new_test_video()

            expected_res = [video_port + 2] * num

            controller = Controller()
            controller.establish_connection()
            res = []
            for _ in range(num):
                res.append(controller.get_encode_port())

            assert expected_res == res
        finally:
            if serv.proc:
                    poll = serv.proc.poll()
                    if poll == -11:
                        print("SEGMENTATION FAULT OCCURRED")
                    print("ERROR CODE - {0}".format(poll))
                    serv.terminate(1)
                    log = open('server.log')
                    print(log.read())


class TestGetEncodePort(object):
    """Performance test for get_encode_port method"""

    def test_100(self):
        """Performance test for get_encode_port method - 100 times"""
        number_test_runs = 100
        get_encode_port(number_test_runs)

    def test_200(self):
        """Performance test for get_encode_port method - 200 times"""
        number_test_runs = 200
        get_encode_port(number_test_runs)

    def test_300(self):
        """Performance test for get_encode_port method - 300 times"""
        number_test_runs = 300
        get_encode_port(number_test_runs)

    def test_400(self):
        """Performance test for get_encode_port method - 400 times"""
        number_test_runs = 400
        get_encode_port(number_test_runs)

    def test_500(self):
        """Performance test for get_encode_port method - 500 times"""
        number_test_runs = 500
        get_encode_port(number_test_runs)



def get_audio_port(num):
        """Test get_audio_port"""
        audio_port = 8000
        serv = Server(path=PATH, audio_port=audio_port)
        try:
            serv.run()
            sources = TestSources(audio_port=audio_port)
            sources.new_test_audio()

            expected_res = [3003] * num

            controller = Controller()
            controller.establish_connection()
            res = []
            for _ in range(num):
                res.append(controller.get_audio_port())

            assert expected_res == res

        finally:
            if serv.proc:
                    poll = serv.proc.poll()
                    if poll == -11:
                        print("SEGMENTATION FAULT OCCURRED")
                    print("ERROR CODE - {0}".format(poll))
                    serv.terminate(1)
                    log = open('server.log')
                    print(log.read())


class TestGetAudioPort(object):

    def test_100(self):
        """Performance test for get_audio_port method - 100 times"""
        number_test_runs = 100
        get_audio_port(number_test_runs)


    def test_200(self):
        """Performance test for get_audio_port method - 200 times"""
        number_test_runs = 200
        get_audio_port(number_test_runs)


    def test_300(self):
        """Performance test for get_audio_port method - 300 times"""
        number_test_runs = 300
        get_audio_port(number_test_runs)


    def test_400(self):
        """Performance test for get_audio_port method - 400 times"""
        number_test_runs = 400
        get_audio_port(number_test_runs)


    def test_500(self):
        """Performance test for get_audio_port method - 500 times"""
        number_test_runs = 500
        get_audio_port(number_test_runs)


def get_preview_ports(num):
    """Get Preview Ports when num number of sources are added"""
    video_port = 3000
    serv = Server(path=PATH, video_port=video_port)
    try:
        serv.run()
        sources = TestSources(video_port=video_port)


        expected_res = [ i for i in range(3003, 3003 + num)]

        controller = Controller()

        res = []
        for _ in range(num):
            sources.new_test_video()
        controller.establish_connection()
        res = controller.get_preview_ports()
        print(res)

        assert expected_res == res
    finally:
        if serv.proc:
                poll = serv.proc.poll()
                if poll == -11:
                    print("SEGMENTATION FAULT OCCURRED")
                print("ERROR CODE - {0}".format(poll))
                serv.terminate(1)
                log = open('server.log')
                print(log.read())


class _TestGetPreviewPorts(object):

    def test_50(self):
        """Test when 50 sources are added"""
        number_souces = 50
        get_preview_ports(number_souces)

    def test_100(self):
        """Test when 100 sources are added"""
        number_souces = 100
        get_preview_ports(number_souces)

    def test_200(self):
        """Test when 200 sources are added"""
        number_souces = 200
        get_preview_ports(number_souces)

    def test_500(self):
        """Test when 500 sources are added"""
        number_souces = 500
        get_preview_ports(number_souces)

    def test_1000(self):
        """Test when 1000 sources are added"""
        number_souces = 1000
        get_preview_ports(number_souces)


def permutate_composite_mode(num, delay):
    """Change composite mode num number of times"""
    import random

    video_port = 3000
    serv = Server(path=PATH, video_port=video_port)
    try:
        serv.run()
        sources = TestSources(video_port=video_port)
        sources.new_test_video(pattern=6)
        sources.new_test_video(pattern=5)
        preview = PreviewSinks()
        preview.run()
        controller = Controller()
        res = controller.set_composite_mode(0)
        assert res is True

        prev_mode = 0
        for _ in range(num-1):

            time.sleep(delay)
            mode = random.randint(0,3)
            expected_res = bool(prev_mode ^ mode)
            prev_mode = mode
            assert expected_res == controller.set_composite_mode(mode)
        preview.terminate()
        sources.terminate_video()

    finally:
        if serv.proc:
                poll = serv.proc.poll()
                if poll == -11:
                    print("SEGMENTATION FAULT OCCURRED")
                print("ERROR CODE - {0}".format(poll))
                serv.terminate(1)
                log = open('server.log')
                print(log.read())

class TestSetCompositeMode(object):
    """Performance Tests for set_composite_mode"""

    def test_num_20_delay_1(self):
        delay = 1
        num = 20
        permutate_composite_mode(num, delay)

    def test_num_20_delay_point_6(self):
        delay = 0.6
        num = 20
        permutate_composite_mode(num, delay)

    def test_num_20_delay_point_5(self):
        delay = 0.5
        num = 20
        permutate_composite_mode(num, delay)

    def test_num_20_delay_point_2(self):
        delay = 0.2
        num = 20
        permutate_composite_mode(num, delay)


def permutate_adjust_pip(num, delay):
    """Adjust_pip num number of times"""
    import random

    video_port = 3000
    serv = Server(path=PATH, video_port=video_port)
    try:
        serv.run()
        sources = TestSources(video_port=video_port)
        sources.new_test_video(pattern=6)
        sources.new_test_video(pattern=5)
        preview = PreviewSinks()
        preview.run()
        controller = Controller()

        for _ in range(num):

            xpos = random.randrange(-20, 20)
            ypos = random.randrange(-20, 20)
            res = controller.adjust_pip(xpos, ypos, 0, 0)
            time.sleep(delay)
            assert res is not None
        preview.terminate()
        sources.terminate_video()

    finally:
        if serv.proc:
                poll = serv.proc.poll()
                if poll == -11:
                    print("SEGMENTATION FAULT OCCURRED")
                print("ERROR CODE - {0}".format(poll))
                serv.terminate(1)
                log = open('server.log')
                print(log.read())

class TestAdjustPip(object):
    """Performance Tests for set_adjust_pip"""

    def test_num_20_delay_1(self):
        delay = 1
        num = 20
        permutate_adjust_pip(num, delay)

    def test_num_20_delay_point_6(self):
        delay = 0.6
        num = 20
        permutate_adjust_pip(num, delay)

    def test_num_20_delay_point_5(self):
        delay = 0.5
        num = 20
        permutate_adjust_pip(num, delay)

    def test_num_20_delay_point_2(self):
        delay = 0.2
        num = 20
        permutate_adjust_pip(num, delay)
