# Systray menu
Currently there is not a common way to create a systray menu compatible with all desktops.

[GtkStatusIcon](https://developer.gnome.org/gtk3/stable/GtkStatusIcon.html) has been deprecated since GTK 3.14.
So we must move to something else. Unfortunately, this "something else" is different from a desktop environment to another.

E.g.: read [this discussion](https://trac.transmissionbt.com/ticket/3685) for the same problem on Transmission client.

Yet, we cannot use gtk_window_present() from a systray menu, see [issue #542](https://github.com/FreeRDP/Remmina/issues/542).

So, for each supported desktop environment, we must answer two questions: 

Q1. Where do we put the remmina systray menu, and which API do we use for it ?  
Q2. Does this place also allow the user to raise a hidden window via gtk_window_present() ?

### References

* [freedesktop.org system tray spec](http://www.freedesktop.org/wiki/Specifications/systemtray-spec/)
* [Libunity source code](http://bazaar.launchpad.net/~unity-team/libunity/trunk/files), written in vala.

# The desktop environments

### Unity
In Ubuntu's Unity, we currently do not use GtkStatusIcon, but libappindicator. This almost works, but when the remmina windows are not on the top, we are unable to present its windows to users. This is explained in [issue #542](https://github.com/FreeRDP/Remmina/issues/542) and it's a problem of the window manager which prevents gtk_window_present() to stealing focus to other applications.
We could use DBusMenu and the [Launcher API](https://wiki.ubuntu.com/Unity/LauncherAPI) to move all the current remmina systray menu into Unity Launcher icon as **dynamic quicklist entries**.
Compatibility with gtk_window_present() must be tested.

### Gnome Shell
Some functions of the [Shell](https://developer.gnome.org/shell/stable/) for ShellTrayIcon object should help for the icon (but no menu support ???). For correctly placing a menu, we can take a look to [Gnome Shell Extension Appindicator source code](https://github.com/rgcjonas/gnome-shell-extension-appindicator).

Currently Remmina detects Gnome Shell and, if detected, no Systray Menu will be used. We did this because Gnome Shells hides all deprecated GtkStatusIcons in a hidden place below the screen, and the user is unable to undestand when remmina is running.

### KDE
In KDE [KStatusNotifierItem](http://api.kde.org/frameworks-api/frameworks5-apidocs/knotifications/html/classKStatusNotifierItem.html)

### XFCE
In XFCE ???

### LXDE/LXQT
In LXDE ???

### MATE
In MATE ???

### EXTERNAL SOURCES ###

* [Proper implementation of system tray icons via libappindicator/StatusNotifierItem](https://code.google.com/p/chromium/issues/detail?id=419673) 
* [Updated Tray to use libappindicator in Linux](https://github.com/nwjs/nw.js/pull/2327) Discussion that propose the same Google Chrome fix.
   
