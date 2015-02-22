"""Unittests for Server class in server.py"""
import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))

from gstswitch.server import Server
import pytest
from gstswitch.exception import ServerProcessError
import subprocess
from distutils import spawn
from mock import Mock, patch


PATH = '/usr/bin/'


class TestPath(object):

    """Test the path parameter"""
    # Path Tests

    def test_path_provided_slash(self):
        """Test if a path is provided"""
        def mock_method(arg):
            """Mocking _start_process"""
            return arg
        path = '/usr/'
        serv = Server(path=path)
        serv._start_process = mock_method
        assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000".split()

    def test_path_provided_no_slash(self):
        """Test if a path is provided"""
        def mock_method(arg):
            """Mocking _start_process"""
            return arg
        path = '/usr'
        serv = Server(path=path)
        serv._start_process = mock_method
        assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000".split()

    def test_path_empty(self, monkeypatch):
        """Test if null path is given"""

        def mock_method(arg):
            "Mocking _start_process"
            return arg

        def mockreturn(path):
            "Mocking distutils.spawn.find_executable"
            return '/usr/gst-switch-srv'
        monkeypatch.setattr(spawn, 'find_executable', mockreturn)
        paths = [None, '']
        for path in paths:
            serv = Server(path=path)
            serv._start_process = mock_method
            assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000".split()


class TestVideoPort(object):

    """Test for video_port parameter"""
    # Video Port Tests

    def test_invalid_video_port_null(self):
        """Test when the video_port is null"""
        video_ports = [None, '', [], {}]
        for video_port in video_ports:
            with pytest.raises(ValueError):
                Server(path=PATH, video_port=video_port)

    def test_invalid_video_port_type(self):
        """Test when the video port given is not a valid
        integral value"""
        video_ports = [[1, 2, 3], {1: 2, 2: 3}]
        for video_port in video_ports:
            with pytest.raises(TypeError):
                Server(path=PATH, video_port=video_port)

    def test_invalid_video_port_range(self):
        """Test when the video port is not in range"""
        video_ports = [-99, -1, 1e6]
        for video_port in video_ports:
            with pytest.raises(ValueError):
                Server(path=PATH, video_port=video_port)


class TestAudioPort(object):

    """Test for audio_port parameter"""
    # Audio Port Tests

    def test_invalid_audio_port_null(self):
        """Test when the audio_port is null"""
        audio_ports = [None, '', [], {}]
        for audio_port in audio_ports:
            with pytest.raises(ValueError):
                Server(path=PATH, audio_port=audio_port)

    def test_invalid_audio_port_type(self):
        """Test when the audio port given is not a valid
        integral value"""
        audio_ports = [[1, 2, 3], {1: 2, 2: 3}]
        for audio_port in audio_ports:
            with pytest.raises(TypeError):
                Server(path=PATH, audio_port=audio_port)

    def test_invalid_audio_port_range(self):
        """Test when the audio port is not in range"""
        audio_ports = [-99, -1, 1e6]
        for audio_port in audio_ports:
            with pytest.raises(ValueError):
                Server(path=PATH, audio_port=audio_port)


class TestControllerAddress(object):

    """Test the controller_address parameter"""
    # Control Address Tests

    def test_invalid_controller_address_null(self):
        """Test when the control port is null"""
        controller_addresses = [None, '', [], {}]
        for controller_address in controller_addresses:
            with pytest.raises(ValueError):
                Server(path=PATH, controller_address=controller_address)

    def test_invalid_controller_address_type(self):
        """Test when the control port is not a valid
        integral value"""
        controller_addresses = [[1, 2, 3], {1: 2, 2: 3}]
        for controller_address in controller_addresses:
            with pytest.raises(TypeError):
                Server(path=PATH, controller_address=controller_address)

    def test_invalid_controller_address_range(self):
        """Test when the control port does not contain a colon"""
        controller_addresses = ['42', 'port=42', '127.0.0.1']
        for controller_address in controller_addresses:
            with pytest.raises(ValueError):
                Server(path=PATH, controller_address=controller_address)


class TestRecordFile(object):

    """Test the record_file parameter"""
    # Record File

    def test_record_file_false(self):
        """Test if record file is False"""
        def mock_method(arg):
            """Mocking _start_process"""
            return arg
        path = '/usr'
        serv = Server(path=path, record_file=False)
        serv._start_process = mock_method
        assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000".split()

    def test_record_file_true(self):
        """Test if record file is True"""
        def mock_method(arg):
            """Mocking _start_process"""
            return arg
        path = '/usr'
        serv = Server(path=path, record_file=True)
        serv._start_process = mock_method
        assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000 -r".split()

    def test_record_file_valid(self):
        """Test if record file is valid"""
        def mock_method(arg):
            """Mocking _start_process"""
            return arg
        path = '/usr'
        serv = Server(path=path, record_file="record.data")
        serv._start_process = mock_method
        assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000 --record=record.data".split()

    def test_record_file_valid_date(self):
        """Test if record file is valid"""
        def mock_method(arg):
            """Mocking _start_process"""
            return arg
        path = '/usr'
        serv = Server(path=path, record_file="record_%Y.data")
        serv._start_process = mock_method
        assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000 \
--record=record_%Y.data".split()

    def test_record_file_valid_space(self):
        """Test if record file is valid and has a space"""
        def mock_method(arg):
            """Mocking _start_process"""
            return arg
        path = '/usr'
        serv = Server(path=path, record_file='record 1.data')
        serv._start_process = mock_method
        assert serv._run_process() == "/usr/gst-switch-srv \
--video-input-port=3000 --audio-input-port=4000 \
--controller-address=tcp:host=0.0.0.0,port=5000".split() + \
            ["--record=record 1.data"]

    def test_record_file_invalid(self):
        """Test when the record_file is invalid"""
        files = ['', None, [], {}]
        for record_file in files:
            with pytest.raises(ValueError):
                Server(path=PATH, record_file=record_file)

    def test_record_file_slashes(self):
        """Test when the record_file has forward slashes"""
        filename = 'abcd/xyz/'
        with pytest.raises(ValueError):

            Server(path=PATH, record_file=filename)


class TestKillTerminate(object):

    """Test kill, terminate and gcov_flush methods"""
    # OS Errors

    def test_terminate_fail(self):
        """Test when terminate fails"""
        class FakeProc(object):

            """A mock process"""

            def __init__(self):
                pass

            def terminate(self):
                """Terminate the mock process"""
                raise OSError

        serv = Server(path=PATH)
        serv.proc = FakeProc()
        with pytest.raises(ServerProcessError):
            serv.terminate()

    def test_kill_fail(self):
        """Test when kill fails"""
        serv = Server(path=PATH)
        serv.proc = 1
        serv.pid = -300
        with pytest.raises(ServerProcessError):
            serv.kill()

    def test_no_process_kill(self):
        """Test when no process exists and kill is called"""
        serv = Server(path=PATH)
        with pytest.raises(ServerProcessError):
            serv.kill()

    def test_no_process_terminate(self):
        """Test when no process exists and terminate is called"""
        serv = Server(path=PATH)
        with pytest.raises(ServerProcessError):
            serv.terminate()

    def test_no_process_gov_flush(self):
        """Test when no process exists and gcov_flush is called"""
        serv = Server(path=PATH)
        with pytest.raises(ServerProcessError):
            serv.gcov_flush()


class TestRun(object):

    """Test running the server"""

    def test_run(self):
        """Test the run method"""
        serv = Server(path='abc')
        serv._run_process = Mock(return_value=MockProcess())
        serv.run()
        assert serv.pid == 1
        assert serv.proc is not None

    def test_run_process(self):
        """Test _run_process method"""
        serv = Server(path='abc')
        serv._start_process = Mock(return_value=MockProcess())
        serv.gst_option_string = ''
        ret = serv._run_process()
        assert ret is not None

    def test_start_process_error(self, monkeypatch):
        """Test _start_process method"""
        with patch('subprocess.Popen.__init__') as mock:
            mock.side_effect = OSError
            serv = Server(path='abc')
            with pytest.raises(ServerProcessError):
                serv._start_process('def')

    def test_start_process_normal(self, monkeypatch):
        """Test _start_process normally"""
        with patch('subprocess.Popen.__init__') as mock:
            mock.return_value = MockProcess()
            serv = Server(path='abc')
            serv._start_process('def')


class MockProcess(object):

    """A mock process"""

    def __init__(self, mode=True):
        self.mode = mode
        self.pid = 1

    def terminate(self):
        """Terminate the mock process"""
        if self.mode is True:
            pass
        if self.mode is False:
            raise OSError('Testing terminate')

    def kill(self):
        """Kill the mock process"""
        if self.mode is True:
            pass
        if self.mode is False:
            raise OSError('Testing kill')

    def make_coverage(self):
        """Dump coverage"""
        pass


class MockPopen(object):

    """Mock Popen method"""

    def __init__(self, cmd, bufsize, shell):
        pass

    def communicate(self):
        """Mock communicate method of Popen"""
        return 0, 0


class TestNormal(object):
    # Normal Functioning Tests

    """Test the functioning of tests under valid conditions"""

    def test_normal_terminate(self):
        """Test terminal when normally called"""
        serv = Server(path='abc')
        serv.proc = MockProcess(True)
        serv.terminate()
        assert serv.proc is None

    def test_normal_kill(self, monkeypatch):
        """Test kill when normally called"""
        serv = Server(path='abc')
        serv.proc = Mock()
        monkeypatch.setattr(os, 'kill', Mock())
        res = serv.kill()
        assert res is True
        assert serv.proc is None

    def test_terminate(self):
        """Test terminate ServerProcessError"""
        serv = Server(path='abc')
        serv.proc = MockProcess(False)
        with pytest.raises(ServerProcessError):
            serv.terminate()

    def test_terminate_cov(self):
        """Test terminate and gcov_flush ServerProcessError"""
        serv = Server(path='abc')
        serv.proc = MockProcess(False)
        serv.gcov_flush = Mock()
        serv.make_coverage = Mock()
        with pytest.raises(ServerProcessError):
            serv.terminate(True)

    def test_kill(self, monkeypatch):
        """Test kill ServerProcessError"""
        serv = Server(path='abc')
        serv.proc = Mock()
        monkeypatch.setattr(os, 'kill', Mock(side_effect=OSError))
        with pytest.raises(ServerProcessError):
            serv.kill()

    def test_kill_cov(self, monkeypatch):
        """Test kill and gcov_flush ServerProcessError"""
        serv = Server(path='abc')
        serv.proc = MockProcess(False)
        serv.gcov_flush = Mock()
        serv.make_coverage = Mock()
        monkeypatch.setattr(os, 'kill', Mock(side_effect=OSError))
        with pytest.raises(ServerProcessError):
            serv.kill(True)

    def test_normal_gcov_flush(self, monkeypatch):
        """Test gcov_flush"""
        serv = Server(path='abc')
        serv.proc = Mock()
        monkeypatch.setattr(os, 'kill', Mock())
        res = serv.gcov_flush()
        assert res is True
        assert serv.proc is not None

    def test_make_coverage(self, monkeypatch):
        """Test dumping coverage"""
        serv = Server(path='abc')
        monkeypatch.setattr(subprocess, 'Popen', MockPopen)
        serv.make_coverage()
