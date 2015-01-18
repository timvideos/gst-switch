#!usr/bin/env python

from __future__ import absolute_import, print_function, unicode_literals

import dbus
import dbus.mainloop.glib
import os
import time
import gobject

def main():
    loop = gobject.MainLoop()
    bus = dbus.SessionBus()
    obj = bus.get_object("info.duzy.gst_switch.SwitchControllerInterface","/info/duzy/gst_switch/SwitchController")
    print(obj.Introspect())
    loop.run()

if __name__=="__main__":
    os.environ["DBUS_SESSION_BUS_ADDRESS"] = "unix:abstract=gstswitch"
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    main()
