from gi.repository import Gio, GLib
from time import sleep

address = "tcp:host=127.0.0.1,port=5000"
name = None
object_path = "/info/duzy/gst/switch/SwitchController"


# a = connection.call_sync(
#             name, object_path, 'org.freedesktop.DBus.Introspectable', 'Introspect',
#             None, GLib.VariantType.new("(s)"), Gio.DBusCallFlags.NONE, -1,
#             None)
#mode = 1
#print type(mode), mode
mode=0
while mode<=3:
    print "mode:", mode
    connection = Gio.DBusConnection.new_for_address_sync(
                    address,
                    Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT,
                    None, None)
    args = GLib.Variant('(i)',(mode,))
    print "args:", args
    b =  connection.call_sync(
                name, object_path, 'info.duzy.gst.switch.SwitchControllerInterface', 'set_composite_mode',
                args, GLib.VariantType.new("(b)"), Gio.DBusCallFlags.NONE, -1,
                None)
    print b.unpack()[0],type(b.unpack()[0])
    mode += 1
    if mode==4:
        mode=0
    sleep(0.5)