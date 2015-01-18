"""
connection deals with all low level method calls over dbus
The Connection object is capable of invoking remote methods over dbus
"""

from __future__ import absolute_import, print_function, unicode_literals


from gi.repository import Gio, GLib
from .exception import ConnectionError
import sys

__all__ = ["Connection", ]


class Connection(object):

    """Class which makes all remote object class.
    Deals with lower level connection and remote method invoking

    :default bus-address: unix:abstract=gstswitch

    :param: None
    """
    CONNECTION_FLAGS = Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT

    def __init__(
            self,
            address="unix:abstract=gstswitch",
            bus_name='info.duzy.gst.switch.SwitchController',
            object_path="/info/duzy/gst/switch/SwitchController",
            default_interface=("info.duzy.gst.switch."
                               "SwitchControllerInterface")):

        super(Connection, self).__init__()
        self.connection = None
        self._address = None
        self._bus_name = None
        self._object_path = None
        self._default_interface = None

        self.address = address
        self.bus_name = bus_name
        self.object_path = object_path
        self.default_interface = default_interface

    @property
    def address(self):
        """Get the address
        """
        return self._address

    @address.setter
    def address(self, address):
        """Set the Address
        http://dbus.freedesktop.org/doc/dbus-specification.html#addresses
        """
        if not address:
            raise ValueError("Address '{0}' cannot be blank")
        else:
            adr = str(address)
            if adr.find(':') > 0:
                self._address = adr
            else:
                raise ValueError("Address must follow specifications mentioned"
                                 " at http://dbus.freedesktop.org/doc/"
                                 "dbus-specification.html#addresses")

    @property
    def bus_name(self):
        """Get the bus name"""
        if self._bus_name is None:
            return None
        return self._bus_name

    @bus_name.setter
    def bus_name(self, bus_name):
        """Set the Bus Name
        http://dbus.freedesktop.org/doc/dbus-specification.html\
        #message-protocol-names-bus
        """
        if bus_name is None:
            self._bus_name = None
            return
        bus = str(bus_name)
        self._bus_name = bus

    @property
    def object_path(self):
        """Get the object path"""
        return self._object_path

    @object_path.setter
    def object_path(self, object_path):
        """Set the object_path
        http://dbus.freedesktop.org/doc/dbus-specification.html\
        #message-protocol-marshaling-object-path
        """
        if not object_path:
            raise ValueError("object_path '{0} cannot be blank'")
        else:
            obj = str(object_path)
            if obj[0] == '/':
                self._object_path = obj
            else:
                raise ValueError("object_path must follow specifications"
                                 " mentioned at "
                                 "http://dbus.freedesktop.org/doc/"
                                 "dbus-specification.html"
                                 "#message-protocol-marshaling-object-path""")

    @property
    def default_interface(self):
        """Get the default interface"""
        return self._default_interface

    @default_interface.setter
    def default_interface(self, default_interface):
        """Set the default_interface
        http://dbus.freedesktop.org/doc/dbus-specification.html\
        #message-protocol-names-interface
        """
        if not default_interface:
            raise ValueError("default_interface '{0} cannot be blank'")
        else:
            intr = str(default_interface)
            if intr.count('.') > 1:
                self._default_interface = intr
            else:
                raise ValueError("default_interface must follow "
                                 "specifications mentioned at "
                                 "http://dbus.freedesktop.org/"
                                 "doc/dbus-specification.html"
                                 "#message-protocol-names-interface")

    def connect_dbus(self):
        """Make a new connection using the parameters belonging to the class
        to the gst-switch-srv over dbus.
        Sets the self.connection

        :params: None
        :returns: Nothing
        :raises ConnectionError: GError occurs while making a connection
        """
        try:
            connection = Gio.DBusConnection.new_for_address_sync(
                self.address,
                self.CONNECTION_FLAGS,
                None,
                None)
            self.connection = connection
        except GLib.GError as error:
            message = error.message
            new_message = "{1} ({0})".format(message, self.address)
            raise ConnectionError(new_message)

    def get_compose_port(self):
        """get_compose_port(out i port);
        Calls get_compose_port remotely

        :param: None
        :returns: tuple with first element compose port number
        """
        try:
            args = None
            connection = self.connection
            port = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'get_compose_port',
                args, GLib.VariantType.new("(i)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return port
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "get_compose_port")
            raise ConnectionError(new_message)

    def get_encode_port(self):
        """get_encode_port(out i port);
        Calls get_encode_port remotely

        :param: None
        :returns: tuple with first element encode port number
        """
        try:
            args = None
            connection = self.connection
            port = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'get_encode_port',
                args,
                GLib.VariantType.new("(i)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return port
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "get_encode_port")
            raise ConnectionError(new_message)

    def get_audio_port(self):
        """get_audio_port(out i port);
        Calls get_audio_port remotely

        :param: None
        :returns: tuple wit first element audio port number
        """
        try:
            args = None
            connection = self.connection
            port = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'get_audio_port',
                args,
                GLib.VariantType.new("(i)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return port
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "get_audio_port")
            raise ConnectionError(new_message)

    def get_preview_ports(self):
        """get_preview_ports(out s ports);
        Calls get_preview_ports remotely

        :param: None
        :returns: tuple with first element a string in the form of
        '[(3002, 1, 7), (3003, 1, 8)]'
        """
        try:
            args = None
            connection = self.connection
            ports = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'get_preview_ports',
                args,
                GLib.VariantType.new("(s)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return ports
        except GLib.GError as error:
            error = sys.exc_info()[1]
            message = error.message
            new_message = "{0}: {1}".format(message, "get_preview_ports")
            raise ConnectionError(new_message)

    def set_composite_mode(self, mode):
        """set_composite_mode(in  i channel,
                                out b result);
        Calls set_composite_mode remotely

        :param mode: new composite mode
        :returns: tuple with first element True if requested
        """
        try:
            args = GLib.Variant('(i)', (mode,))
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'set_composite_mode',
                args,
                GLib.VariantType.new("(b)"),
                Gio.DBusCallFlags.NONE, -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "set_composite_mode")
            raise ConnectionError(new_message)

    def get_composite_mode(self):
        """get_composite_mode(out b result);
        Calls get_composite_mode remotely

        :returns: tuple with first element being the current compsition mode
        """
        try:
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'get_composite_mode',
                None,
                GLib.VariantType.new("(i)"),
                Gio.DBusCallFlags.NONE, -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "get_composite_mode")
            raise ConnectionError(new_message)

    def set_encode_mode(self, channel):
        """set_encode_mode(in  i channel,
                            out b result);
        Calls set_encode_mode remotely
        **Does not do anything**

        :param: channel
        :returns: tuple with first element True if requested
        """
        try:
            args = GLib.Variant('(i)', (channel,))
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'set_encode_mode',
                args,
                GLib.VariantType.new("(b)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "set_encode_mode")
            raise ConnectionError(new_message)

    def new_record(self):
        """new_record(out b result);
        Calls new_record remotely

        :param: None:
        returns: tuple with first element True if requested
        """
        try:
            args = None
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'new_record',
                args,
                GLib.VariantType.new("(b)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "new_record")
            raise ConnectionError(new_message)

    def adjust_pip(self, xpos, ypos, width, height):
        """adjust_pip(in i dx,
                           in  i dy,
                           in  i dw,
                           in  i dh,
                           out u result);
        Calls adjust_pip remotely

        :param xpos: the X position of the PIP
        :param ypos: the Y position of the PIP
        :param width: the width of the PIP
        :param height: the height of the PIP
        :returns: tuple with first element as result -
        PIP has been changed succefully
        """
        try:
            args = GLib.Variant('(iiii)', (xpos, ypos, width, height,))
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'adjust_pip',
                args,
                GLib.VariantType.new("(u)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "adjust_pip")
            raise ConnectionError(new_message)

    def switch(self, channel, port):
        """switch(in  i channel,
                       in  i port,
                       out b result);
        Calls switch remotely

        :param channel: The channel to be switched, 'A', 'B', 'a'
        :param port: The target port number
        :returns: tuple with first element True if requested
        """
        try:
            args = GLib.Variant('(ii)', (channel, port,))
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'switch',
                args,
                GLib.VariantType.new("(b)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "switch")
            raise ConnectionError(new_message)

    def click_video(self, xpos, ypos, width, height):
        """click_video(in  i x,
                            in  i y,
                            in  i fw,
                            in  i fh,
                            out b result);
        Calls click_video remotely

        :param xpos:
        :param ypos:
        :param width:
        :param height:
        :returns: tuple with first element True if requested
        """
        try:
            args = GLib.Variant('(iiii)', (xpos, ypos, width, height,))
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'click_video',
                args,
                GLib.VariantType.new("(b)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "click_video")
            raise ConnectionError(new_message)

    def mark_face(self, faces):
        """mark_face(in  a(iiii) faces);
        Calls mark_face remotely

        :param faces: tuple having four elements
        :returns: tuple with first element True if requested
        """
        try:
            args = GLib.Variant('a(iiii)', faces)
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'mark_face',
                args,
                GLib.VariantType.new("(b)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "mark_face")
            raise ConnectionError(new_message)

    def mark_tracking(self, faces):
        """mark_tracking(in  a(iiii) faces);
        Calls mark_tracking remotely

        :param faces: tuple having four elements
        :returns: tuple with first element True if requested
        """
        try:
            args = GLib.Variant('a(iiii)', faces)
            connection = self.connection
            result = connection.call_sync(
                self.bus_name,
                self.object_path,
                self.default_interface,
                'mark_tracking',
                args,
                GLib.VariantType.new("(b)"),
                Gio.DBusCallFlags.NONE,
                -1,
                None)
            return result
        except GLib.GError as error:
            message = error.message
            new_message = "{0}: {1}".format(message, "mark_tracking")
            raise ConnectionError(new_message)
