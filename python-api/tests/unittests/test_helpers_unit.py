"""Unittests for helper.py"""
import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))

from gstswitch.helpers import TestSources, PreviewSinks
from gstswitch.exception import RangeError, InvalidIndexError
import pytest
from gstswitch import testsource


class TestTestSourcesVideoPort(object):

    """Test for video_port parameter"""

    def test_range(self):
        """Test when out of range"""
        tests = [-100, 1e7, 65536]
        for test in tests:
            with pytest.raises(RangeError):
                TestSources(video_port=test)

    def test_invalid(self):
        """Test when not integral value"""
        tests = [[1, 2, 3, 4], {1: 2, 2: 3}, '1e10']
        for test in tests:
            with pytest.raises(TypeError):
                TestSources(video_port=test)

    def test_normal(self):
        """Test when valid"""
        tests = [1, 65535, 1000]
        for test in tests:
            src = TestSources(video_port=test)
            assert src.video_port == test


class TestTestSourcesAudioPort(object):

    """Test for audio_port parameter"""

    def test_range(self):
        """Test when out of range"""
        tests = [-100, 1e7, 65536]
        for test in tests:
            with pytest.raises(RangeError):
                TestSources(audio_port=test)

    def test_invalid(self):
        """Test when not integral value"""
        tests = [[1, 2, 3, 4], {1: 2, 2: 3}, '1e10']
        for test in tests:
            with pytest.raises(TypeError):
                TestSources(audio_port=test)

    def test_normal(self):
        """Test when valid"""
        tests = [1, 65535, 1000]
        for test in tests:
            src = TestSources(audio_port=test)
            assert src.audio_port == test


class TestTestSources(object):

    """Test for testsources"""
    class MockVideoSrc(object):

        """A mock video source"""

        def __init__(self,
                     port,
                     width=300,
                     height=200,
                     pattern=None,
                     timeoverlay=False,
                     clockoverlay=False):
            pass

        def run(self):
            """Run the mock video source"""
            pass

    def test_new_test_video(self, monkeypatch):
        """Test for new_test_video"""
        test = TestSources(video_port=3000)
        monkeypatch.setattr(testsource, 'VideoSrc', self.MockVideoSrc)
        test.new_test_video()
        assert test.running_tests_video[0] is not None
        assert len(test.running_tests_video) != 0

    class MockTest(object):

        """A mock test"""

        def __init__(self, pattern):
            self.pattern = pattern

        def end(self):
            """End the test"""
            pass

    def test_terminate_index_error_video(self):
        """Test for terminate_video"""
        testsrc = TestSources(video_port=3000)
        testsrc.running_tests_video = [
            self.MockTest(1),
            self.MockTest(2),
            self.MockTest(3),
            self.MockTest(19)]
        tests = [-100, 20, 1e10, "hi", [1, 2, 3]]
        for test in tests:
            with pytest.raises(InvalidIndexError):
                testsrc.terminate_index_video(test)

    def test_terminate_index_normal_video(self):
        """Test terminate_index_video"""
        test = TestSources(video_port=3000)
        test.running_tests_video = [
            self.MockTest(1),
            self.MockTest(2),
            self.MockTest(3),
            self.MockTest(19)]
        test.terminate_index_video(0)

    def test_terminate1_video(self):
        """Test terminate_video multiple"""
        test = TestSources(video_port=3000)
        test.running_tests_video = [
            self.MockTest(1),
            self.MockTest(2),
            self.MockTest(3),
            self.MockTest(19)]
        test.terminate_video()

    def test_terminate2_video(self):
        """Test terminate_video none present"""
        test = TestSources(video_port=3000)
        test.running_tests_video = []
        test.terminate_video()

    class MockAudioSrc(object):

        """A Mock audio source"""

        def __init__(self, port, freq=110, wave=None):
            pass

        def run(self):
            """Run the source"""
            pass

    def test_new_test_audio(self, monkeypatch):
        """Test new_test_audio"""
        test = TestSources(audio_port=3000)
        monkeypatch.setattr(testsource, 'AudioSrc', self.MockAudioSrc)
        test.new_test_audio()
        assert test.running_tests_audio[0] is not None
        assert len(test.running_tests_audio) != 0

    class MockTest2(object):

        """A mock audio source"""

        def __init__(self, wave):
            self.wave = wave

        def end(self):
            """End the source"""
            pass

    def test_get_test_audio(self):
        """Test get_test_audio"""
        test = TestSources(audio_port=4000)
        test.running_tests_audio = [
            self.MockTest2(1),
            self.MockTest2(2),
            self.MockTest2(3),
            self.MockTest2(10)]
        test.get_test_audio()

    def test_terminate_index_error_audio(self):
        """Test terminate_index_audio - invalid index"""
        testsrc = TestSources(audio_port=4000)
        testsrc.running_tests_audio = [
            self.MockTest2(1),
            self.MockTest2(2),
            self.MockTest2(3),
            self.MockTest2(10)]
        tests = [-100, 20, 1e10, "hi", [1, 2, 3]]
        for test in tests:
            with pytest.raises(InvalidIndexError):
                testsrc.terminate_index_audio(test)

    def test_terminate_index_normal_audio(self):
        """Test terminate_index_audio valid index"""
        test = TestSources(audio_port=4000)
        test.running_tests_audio = [
            self.MockTest2(1),
            self.MockTest2(2),
            self.MockTest2(3),
            self.MockTest2(10)]
        test.terminate_index_audio(0)

    def test_terminate1_audio(self):
        """Test terminate_audio multiple"""
        test = TestSources(audio_port=4000)
        test.running_tests_audio = [
            self.MockTest2(1),
            self.MockTest2(2),
            self.MockTest2(3),
            self.MockTest2(10)]
        test.terminate_audio()

    def test_terminate2_audio(self):
        """Test terminate_audio none present"""
        test = TestSources(audio_port=4000)
        test.running_tests_audio = []
        test.terminate_audio()


class TestPreviewSinksPreviewPort(object):

    """Test preview_port parameter"""

    def test_blank(self):
        """Test when preview_port is blank"""
        tests = ['', None, [], {}]
        for test in tests:
            with pytest.raises(ValueError):
                PreviewSinks(preview_port=test)

    def test_range(self):
        """Test when preview_port is out of range"""
        tests = [-100, 1e7, 65536]
        for test in tests:
            with pytest.raises(RangeError):
                PreviewSinks(preview_port=test)

    def test_invalid(self):
        """Test when preview_port is not a integral value"""
        tests = [[1, 2, 3, 4], {1: 2, 2: 3}, '1e10']
        for test in tests:
            with pytest.raises(TypeError):
                PreviewSinks(preview_port=test)

    def test_normal(self):
        """Test when preview_port is valid"""
        tests = [1, 65535, 1000]
        for test in tests:
            src = PreviewSinks(preview_port=test)
            assert src.preview_port == test


class TestPreviewSinks(object):

    """Test Preview Sinks"""
    class MockPreview(object):

        """A mock preview class"""

        def __init__(self, preview_port=3001):
            pass

        def run(self):
            """Run the preview"""
            pass

        def end(self):
            """End the preview"""
            pass

    def test_run(self, monkeypatch):
        """Test running preview"""
        preview = PreviewSinks()
        monkeypatch.setattr(testsource, 'Preview', self.MockPreview)
        preview.run()
        assert preview.preview is not None

    def test_terminate_fail(self):
        """Test terminating a preview when none exists"""
        preview = PreviewSinks()
        with pytest.raises(AttributeError):
            preview.terminate()

    def test_terminate_normal(self, monkeypatch):
        """Test terminating a preview when valid exists"""
        preview = PreviewSinks()
        preview.preview = self.MockPreview()
        preview.terminate()
        assert preview.preview is None
