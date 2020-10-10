import sys

if not hasattr(sys, 'argv'):
    sys.argv = ['']

import remmina
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib
import psutil

class HelloPlugin(remmina.ProtocolPlugin):
    def __init__(self):
        super().__init__("Hello", "Just a Plugin saying hello :)", "1.0", "")

    def init(self, service):
        self.service = service
        print("Init!")
        self.show_gui()
        super.init()

    def open_connection(self):
        self.service.log_printf("[%s]: Plugin open connection\n" % self.name)
        super.open_connection()

    def close_connection(self):
        self.service.log_printf("[%s]: Plugin close connection\n" % self.name)

    def query_feature(self):
        pass

    def call_feature(self):
        pass

    def send_keystrokes(self):
        pass

    def get_plugin_screenshot(self):
        pass

    def show_gui(self):
        self.window = Gtk.Window(title="Hello World")
        self.lblMemUsage = Gtk.Label(label="CPU Usage: ")
        self.window.add(self.lblMemUsage)
        self.window.show_all()
        self.window.connect("destroy", Gtk.main_quit)
        GLib.timeout_add_seconds(1, self.threaded_mem_usage)
        Gtk.main()

    def threaded_mem_usage(self):
        self.lblMemUsage.set_text("CPU Usage: %s" % psutil.cpu_percent())
        return True

myPlugin = HelloPlugin()
#myPlugin.show_gui()
