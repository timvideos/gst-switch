"""Unittests for Connection class in connection.py"""
import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(__file__, "../../../")))

from gstswitch.connection import Connection
from gstswitch.exception import ConnectionError
import pytest
from gi.repository import Gio, GLib
from mock import Mock


class TestAddress(object):

    """Unittests for address parameter"""

    def test_address_null(self):
        """Test if address is null"""
        addresses = ['', None, [], {}]
        for address in addresses:
            with pytest.raises(ValueError):
                Connection(address=address)

    def test_address_colon(self):
        """Test if address has no colon"""
        address = 'abcdefghijk'
        with pytest.raises(ValueError):
            Connection(address=address)

    def test_address_normal(self):
        """"Test if address is valid"""
        address = ['tcp:host=127.0.0.1,port=5000', 'unix:temp=/tmp/abcd/xyz']
        for addr in address:
            conn = Connection(address=addr)
            assert conn.address == addr


class TestBusName(object):

    """Unittests for bus_name parameter"""

    def test_normal(self):
        """Test if bus_name is not null"""
        names = ['', 'abcd', 12345]
        for bus in names:
            conn = Connection(bus_name=bus)
            assert conn.bus_name == str(bus)

    def test_normal_none(self):
        """Test if bus_name is null"""
        name = None
        conn = Connection(bus_name=name)
        assert conn.bus_name == name


class TestObjectPath(object):

    """Unittests for object_path parameter"""

    def test_object_path_blank(self):
        """Test if object_path is null"""
        paths = [None, '', {}, []]
        for object_path in paths:
            with pytest.raises(ValueError):
                Connection(object_path=object_path)

    def test_object_path_slash(self):
        """Test when object_path doesn't have slash in start"""
        object_path = 'a/////////'
        with pytest.raises(ValueError):
            Connection(object_path=object_path)

    def test_object_path_normal(self):
        """Test of object_path is valid"""
        object_path = "/us/timvideos/gstswitch/SwitchController"
        conn = Connection(object_path=object_path)
        assert conn.object_path == object_path


class TestInterface(object):

    """Unittests for default_interface parameter"""

    def test_interface_none(self):
        """Test if default_interface is null"""
        default_interface = [None, '', [], {}]
        for interface_name in default_interface:
            with pytest.raises(ValueError):
                Connection(default_interface=interface_name)

    def test_interface_dot(self):
        """Test when the default_interface has <2 dots"""
        default_interface = ['.', 'info.', 'info']
        for interface_name in default_interface:
            with pytest.raises(ValueError):
                Connection(default_interface=interface_name)

    def test_interface_normal(self):
        """Test if default_interface is valid"""
        default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
        conn = Connection(default_interface=default_interface)
        assert default_interface == conn.default_interface


class TestConnectDBus(object):

    """Unittests for the connect_dbus method of Connection class"""

    def test_bad_address(self):
        """Test if wrong address is given - 1"""
        address = 'unix:path=gstswitch'
        conn = Connection(address=address)
        with pytest.raises(ConnectionError):
            conn.connect_dbus()

    def test_bad_address2(self):
        """Test if wrong address is given - 2"""
        address = 'unix:temp=gstswitch'
        conn = Connection(address=address)
        with pytest.raises(ConnectionError):
            conn.connect_dbus()

    def test_bad_address3(self):
        """Test if wrong address is given - 3"""
        address = 'unix:path'
        conn = Connection(address=address)
        with pytest.raises(ConnectionError):
            conn.connect_dbus()

    def test_mock1(self, monkeypatch):
        """Test GLib.GError exception"""
        monkeypatch.setattr(
            Gio.DBusConnection,
            'new_for_address_sync',
            Mock(side_effect=GLib.GError))
        conn = Connection()
        with pytest.raises(ConnectionError):
            conn.connect_dbus()

    def test_mock2(self, monkeypatch):
        """Test GLib.GError exception"""
        monkeypatch.setattr(
            Gio.DBusConnection, 'new_for_address_sync',
            Mock(return_value=1))
        conn = Connection()
        conn.connect_dbus()
        assert conn.connection is not None


class TestSignalSubscribe(object):

    """Unittests for signal_subscribe"""

    def test_handler_not_callable(self, monkeypatch):
        """Test if handler is not callable"""

        conn = Connection()
        test_cbs = ['', None, [], {}]
        for test_cb in test_cbs:
            with pytest.raises(ValueError):
                conn.signal_subscribe(test_cb)

    # pylint: disable=unpacking-non-sequence
    def test_handler_passed_to_gio(self, monkeypatch):
        """Test if handler is correctly passed to Gio"""

        monkeypatch.setattr(
            Gio.DBusConnection, 'new_for_address_sync',
            Mock(return_value=Gio.DBusConnection()))

        signal_subscribe_mock = Mock()
        monkeypatch.setattr(
            Gio.DBusConnection, 'signal_subscribe',
            signal_subscribe_mock)

        conn = Connection()
        conn.connect_dbus()
        test_cb = lambda: None
        conn.signal_subscribe(test_cb)

        # test that Gio's signal_subscribe method is called once
        # and passed the callback as-is
        signal_subscribe_mock.assert_called_once()
        args, _ = signal_subscribe_mock.call_args
        assert args[6] == test_cb

    def test_gio_error_is_converted(self, monkeypatch):
        """Test if handler is correctly passed to Gio"""

        monkeypatch.setattr(
            Gio.DBusConnection, 'new_for_address_sync',
            Mock(return_value=Gio.DBusConnection()))

        monkeypatch.setattr(
            Gio.DBusConnection, 'signal_subscribe',
            Mock(side_effect=GLib.GError('Boom!')))

        conn = Connection()
        conn.connect_dbus()
        test_cb = lambda: None
        with pytest.raises(ConnectionError):
            conn.signal_subscribe(test_cb)


class MockConnection(object):

    """A class which mocks the Connection class"""

    funs = {
        'get_compose_port': (3001,),
        'get_encode_port': (3002,),
        'get_audio_port': (4000,),
        'get_preview_ports': ('[(3002, 1, 7), (3003, 1, 8)]',),
        'set_composite_mode': (False,),
        'get_composite_mode': (0,),
        'set_encode_mode': (False,),
        'new_record': (False,),
        'adjust_pip': (1,),
        'switch': (True,),
        'click_video': (True,),
        'mark_face': None,
        'mark_tracking': None
    }

    def __init__(self, method):
        self.method = method
        self.return_result = self.funs[method]

    def call_sync(
            self,
            bus_name,
            object_path,
            interface_name,
            method_name,
            parameters,
            reply_type, flags,
            timeout_msec,
            cancellable):
        """Mock of call_sync method,
        raises GLib.GError if interface_name invalid"""
        if interface_name == (
                "us.timvideos.gstswitch.SwitchControllerInterface"):
            return self.return_result
        else:
            raise GLib.GError('{0}: Test Failed'.format(self.method))


def test_get_compose_port():
    """Test the get_compose_port method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_compose_port')
    with pytest.raises(ConnectionError):
        conn.get_compose_port()

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_compose_port')
    assert conn.get_compose_port() == (3001,)


def test_get_encode_port():
    """Test the get_encode_port method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_encode_port')
    with pytest.raises(ConnectionError):
        conn.get_encode_port()

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_encode_port')
    assert conn.get_encode_port() == (3002,)


def test_get_audio_port():
    """Test the get_audio_port method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_audio_port')
    with pytest.raises(ConnectionError):
        conn.get_audio_port()

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_audio_port')
    assert conn.get_audio_port() == (4000,)


def test_get_preview_ports():
    """Test the get_preview_ports method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_preview_ports')
    with pytest.raises(ConnectionError):
        conn.get_preview_ports()

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_preview_ports')
    assert conn.get_preview_ports() == ('[(3002, 1, 7), (3003, 1, 8)]',)


def test_set_composite_mode():
    """Test the set_composite_mode method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('set_composite_mode')
    with pytest.raises(ConnectionError):
        conn.set_composite_mode(2)

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('set_composite_mode')
    assert conn.set_composite_mode(2) == (False,)


def test_get_composite_mode():
    """Test the set_composite_mode method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_composite_mode')
    with pytest.raises(ConnectionError):
        conn.get_composite_mode()

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('get_composite_mode')
    assert conn.get_composite_mode() == (0,)


def test_set_encode_mode():
    """Test the set_encode_mode method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('set_encode_mode')
    with pytest.raises(ConnectionError):
        conn.set_encode_mode(2)

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('set_encode_mode')
    assert conn.set_encode_mode(2) == (False,)


def test_new_record():
    """Test the new_record method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('new_record')
    with pytest.raises(ConnectionError):
        conn.new_record()

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('new_record')
    assert conn.new_record() == (False,)


def test_adjust_pip():
    """Test the adjust_pip method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('adjust_pip')
    with pytest.raises(ConnectionError):
        conn.adjust_pip(1, 2, 3, 4)

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('adjust_pip')
    assert conn.adjust_pip(1, 2, 3, 4) == (1,)


def test_switch():
    """Test the switch method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('switch')
    with pytest.raises(ConnectionError):
        conn.switch(1, 2)

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('switch')
    assert conn.switch(1, 2) == (True,)


def test_click_video():
    """Test the click_video method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('click_video')
    with pytest.raises(ConnectionError):
        conn.click_video(1, 2, 3, 4)

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('click_video')
    assert conn.click_video(1, 2, 3, 4) == (True,)


def test_mark_face():
    """Test the mark_face method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('mark_face')
    with pytest.raises(ConnectionError):
        face = [(1, 1, 1, 1), (2, 2, 2, 2)]
        conn.mark_face(face)

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('mark_face')
    face = [(1, 1, 1, 1), (2, 2, 2, 2)]
    assert conn.mark_face(face) is None


def test_mark_tracking():
    """Test the mark_tracking method"""
    default_interface = "us.timvideos.gstswitch"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('mark_tracking')
    with pytest.raises(ConnectionError):
        face = [(1, 1, 1, 1), (2, 2, 2, 2)]
        conn.mark_tracking(face)

    default_interface = "us.timvideos.gstswitch.SwitchControllerInterface"
    conn = Connection(default_interface=default_interface)
    conn.connection = MockConnection('mark_tracking')
    face = [(1, 1, 1, 1), (2, 2, 2, 2)]
    assert conn.mark_tracking(face) is None
