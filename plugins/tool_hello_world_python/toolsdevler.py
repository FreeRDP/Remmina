import sys

if not hasattr(sys, 'argv'):
    sys.argv = ['']

import remmina
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib
import psutil

class HelloPlugin:
    def __init__(self):
        self.name = "Hello"
        self.type = "protocol"
        self.description = "Hello World!"
        self.version  = "1.0"
        self.icon_name = "remmina-tool"
        self.icon_name_ssh = "remmina-tool"
        self.btn = Gtk.Button(label="Hello!")
        self.btn.connect("clicked", self.callback_add, "hello")
        pass

    def callback_add(self, widget, data):
        print("Click :)")

    def name(self):
        return "Hello"

    def init(self):
        print("Init!")
        return True


    def open_connection(self, viewport):
        print("open_connection!")
        def foreach_child(child):
            child.add(self.btn)
            self.btn.show()
        viewport.foreach(foreach_child)
        print("Connected!")

        remmina.log_print("[%s]: Plugin open connection\n" % self.name)
        return True

    def draw(self, widget, cr, color):
        cr.rectangle(0, 0, 100, 100)
        cr.set_source_rgb(color[0], color[1], color[2])
        cr.fill()
        cr.queue_draw_area(0, 0, 100, 100)

        return True

    def close_connection(self, viewport):
        print("close_connection!")
        remmina.log_print("[%s]: Plugin close connection\n" % self.name)
        return True

    def query_feature(self):
        pass

    def call_feature(self):
        pass

    def send_keystrokes(self):
        pass

    def get_plugin_screenshot(self):
        pass

myPlugin = HelloPlugin()
remmina.register_plugin(myPlugin)
