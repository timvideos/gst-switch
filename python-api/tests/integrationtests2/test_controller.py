"""
Integration Tests for the dbus Controller
"""

from __future__ import absolute_import, print_function, unicode_literals
from .baseclass import IntegrationTestbase

class TestEstablishConnection(IntegrationTestbase):
    """Test starting and connecting to Server"""

    def test_establish_connection(self):
        """Test starting and connecting to Server"""
        # setup_server asserts internally that the server started up correctly
        self.setup_server()

        # setup_controller asserts internally that the connection is okay
        self.setup_controller()


class TestGetComposePort(IntegrationTestbase):
    """Test get_compose_port method"""

    def test_compose_port(self):
        """Test get_compose_port method returning the expected port"""
        self.setup_server()
        self.setup_controller()

        assert self.controller.get_compose_port() == 3001

    def test_compose_caps(self):
        """Test get_compose_port method returning the expected
        video-stream-caps"""
        pass


class TestGetEncodePort(IntegrationTestbase):
    """Test get_encode_port method"""

    def test_encode_port(self):
        """Test get_encode_port method returning the expected port"""
        self.setup_server()
        self.setup_controller()
        assert self.controller.get_encode_port() == 3002


class TestGetAudioPort(object):
    pass


class TestGetPreviewPorts(object):
    pass


class TestSignals(object):
    pass


class TestNewRecord(object):
    pass

