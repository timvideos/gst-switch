"""
Integration Tests for the dbus Controller
"""

from __future__ import absolute_import, print_function, unicode_literals
from .baseclass import IntegrationTestbase


class TestEstablishConnection(IntegrationTestbase):
    """ Test starting and connecting to the Server
    """

    def test_establish_connection(self):
        """ Start the Server and connect to it. Assert it starts up correctly
        and can be connected to via the python-controller.
        """
        # setup_server asserts internally that the server started up correctly
        self.setup_server()

        # setup_controller asserts internally that the connection is okay
        self.setup_controller()


class TestGetComposePort(IntegrationTestbase):
    """ Test get_compose_port method
    """

    def wait_until_ready(self):
        """ Blocks until the Server has reported, that it's
        composite-output is started
        """
        self.log.info("waiting for Server to start composite-output")
        self.serv.wait_for_output('composite-output started')

    def test_compose_port(self):
        """
        Test if get_compose_port is returning the expected port
        """
        self.setup_server()
        self.setup_controller()
        self.wait_until_ready()

        self.log.info("calling get_compose_port")
        assert self.controller.get_compose_port() == 3001


class TestGetEncodePort(IntegrationTestbase):
    """ Test get_encode_port method
    """

    def wait_until_ready(self):
        """ Blocks until the Server has reported, that it's
        encoding-output is started
        """
        self.log.info("waiting for Server to start encoding-output")
        self.serv.wait_for_output('encoding-output started')

    def test_encode_port(self):
        """Test get_encode_port method returning the expected port"""
        self.setup_server()
        self.setup_controller()
        self.wait_until_ready()

        self.log.info("calling get_encode_port")
        assert self.controller.get_encode_port() == 3002


class TestGetAudioPort(IntegrationTestbase):
    """ Test get_audio_port method
    """
    def wait_until_ready(self):
        """ Blocks until the Server has reported, that it's
        audio-output has started
        """
        self.log.info("waiting for Server to start preview-port-outputs")
        self.serv.wait_for_output('tcpserversink')

    def test_preview_ports(self):
        """Test get_audio_port method returning the expected port"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting 2 test-audio sources")
        self.sources.new_test_audio()
        self.sources.new_test_audio()

        self.wait_until_ready()

        self.log.info("calling get_audio_port")
        assert self.controller.get_audio_port() == 3003


class TestGetPreviewPorts(IntegrationTestbase):
    """ Test get_preview_ports method
    """
    def wait_until_ready(self, count):
        """ Blocks until the Server has reported, that the right number of
        preview-ports are is started
        """
        self.log.info("waiting for Server to start preview-port-outputs")
        self.serv.wait_for_output('tcpserversink name=sink', count=count)

    def test_preview_ports(self):
        """Test get_preview_ports method returning the expected port"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting 1 test-video source")
        self.sources.new_test_video()

        self.wait_until_ready(1)

        self.log.info("calling get_preview_ports")
        assert self.controller.get_preview_ports() == [3003]

    def test_five_preview_ports(self):
        """Test get_preview_ports method returning the expected port"""
        self.setup_server()
        self.setup_controller()

        self.log.info("starting 5 test-video sources")
        self.sources.new_test_video()
        self.sources.new_test_video()
        self.sources.new_test_video()
        self.sources.new_test_video()
        self.sources.new_test_video()

        self.wait_until_ready(5)

        self.log.info("calling get_preview_ports")
        assert self.controller.get_preview_ports() == [3003, 3004, 3005, 3006, 3007]


class TestSignals(IntegrationTestbase):
    """ Test the various on_* signal-handler methods
    """
    pass


class TestNewRecord(IntegrationTestbase):
    """ Test new_record method
    """
    pass
