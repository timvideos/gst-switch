"""
Usage - 
    
    `python console.py`
    (Cmd)help

    Autocompletions should work...

"""
import sys
import os
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + '/python-api')

from gstswitch.server import Server
from gstswitch.helpers import *
from gstswitch.controller import Controller
import cmd
import inspect



class Console(cmd.Cmd):

    COMPOSITE_MAPPING = {
        'none': Controller.COMPOSITE_NONE,
        'pip': Controller.COMPOSITE_PIP,
        'dual_preview': Controller.COMPOSITE_DUAL_PREVIEW,
        'preview': Controller.COMPOSITE_DUAL_PREVIEW,
        'equal': Controller.COMPOSITE_DUAL_EQUAL,
        'dual_equal': Controller.COMPOSITE_DUAL_EQUAL
    }

    SWITCH_MAPPING = {
        'video-A': Controller.VIDEO_CHANNEL_A,
        'video-B': Controller.VIDEO_CHANNEL_B,
        'audio': Controller.AUDIO_CHANNEL        
    }

    def do_get_compose_port(self, line):
        c = Controller()
        c.establish_connection()
        print c.get_compose_port()

    def help_get_compose_port(self):
        print "Get the Compose Port"

    def do_get_encode_port(self, line):
        c = Controller()
        c.establish_connection()
        print c.get_encode_port()

    def do_get_audio_port(self, line):
        c = Controller()
        c.establish_connection()
        print c.get_audio_port()

    def do_get_preview_ports(self, line):
        c = Controller()
        c.establish_connection()
        print c.get_preview_ports()

    def do_set_composite_mode(self, line):
        try:
            val = self.COMPOSITE_MAPPING[line.lower()]
        except KeyError:
            print "Invalid"
            return
        c = Controller()
        c.establish_connection()
        print c.set_composite_mode(val)

    def help_set_composite_mode(self):
        print "Valid modes - {0}".format(", ".join([i for i in self.COMPOSITE_MAPPING]))

    def complete_set_composite_mode(self, text, line, begidx, endidx):
        return [i for i in self.COMPOSITE_MAPPING if i.startswith(text)]

    def do_adjust_pip(self, line):
        c = Controller()
        c.establish_connection()
        if len(line.split()) != 4:
            print "Inavlid number of arguments"
            return
        print c.adjust_pip(*map(int, line.split()))

    def do_switch(self, line):
        try:
            val = self.SWITCH_MAPPING[line.lower()]
        except KeyError:
            print "Invalid"
            return
        c = Controller()
        c.establish_connection()
        print c.switch(self.SWITCH_MAPPING[line.split()[0]], int(line.split()[1]))

    def complete_switch(self, text, line, begidx, endidx):
        return [i for i in self.SWITCH_MAPPING if i.startswith(text)]






if __name__ == '__main__':
    con = Console()
    con.cmdloop()
