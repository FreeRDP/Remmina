import sys

if not hasattr(sys, 'argv'):
    sys.argv = ['']

import remmina

class HelloPlugin(remmina.Plugin):
    def __init__(self):
        self.type = remmina.PLUGIN_TYPE_PROTOCOL
        self.name = "Hello"
        self.description = "Just a Plugin saying hello :)"
        self.version = "1.0"
        self.appicon = ""
        self.service = None
        self.protocol_setting = None

    def init(self, service):
        self.service = service
        service.log_printf("[%s]: Plugin init\n" % self.name)

    def open_connection(self):
        service.log_printf("[%s]: Plugin open connection\n" % self.name)


    def close_connection(self):
        service.log_printf("[%s]: Plugin close connection\n" % self.name)

    def query_feature(self):
        pass

    def call_feature(self):
        pass

    def send_keystrokes(self):
        pass

    def get_plugin_screenshot(self):
        pass

print("import gi")
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

window = Gtk.Window(title="Hello World")
window.show()
window.connect("destroy", Gtk.main_quit)
Gtk.main()
