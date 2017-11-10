Build instructions
------------------

1. Install `flatpak` and `flatpak-builder` ([instructions]). Remmina
   Flatpak manifest requires `flatpak-builder` >= 0.9.1.

[instructions]: http://flatpak.org/getting.html

2. Enable GNOME Flatpak repository:

        flatpak --user remote-add --if-not-exists --from gnome https://sdk.gnome.org/gnome.flatpakrepo

3. Install GNOME runtime (have a look to the file `org.remmina.Remmina.json`
   to get the required version, e.g, `"runtime-version": "3.26"`):

        flatpak --user install gnome org.gnome.Platform//3.26
        flatpak --user install gnome org.gnome.Sdk//3.26

4. Build Remmina:

        flatpak-builder --repo=repo/ appdir/ org.remmina.Remmina.json

   Remmina will be built in `appdir/` folder and the result will be exported
   to a local Flatpak repository in `repo/` folder.

5. Enable the local repository:

        flatpak --user remote-add --no-gpg-verify --if-not-exists my-repo repo/

6. Install Remmina from your repository:

        flatpak --user install my-repo org.remmina.Remmina

7. Launch Remmina

        flatpak run org.remmina.Remmina

Limitations
-----------

Several Remmina features are not enabled in Flatpak build:

* SPICE USB redirection uses a small suid wrapper
  (`spice-client-glib-usb-acl-helper`) which is inhibited by Flatpak
  `bubblewrap` sandboxing. Therefore, this feature is not enabled.

* Telepathy DBus activation would require some more files exported outside of
  Flatpak. Hence, Telepathy plugin is not compiled.

* File transfers of some plugins (SFTP, SPICE drag and drop...) are limited to
  the files located in the user home directory (see below).

Security considerations
-----------------------

* Remmina's Flatpak sandbox is configured to give access to the user home
  directory. You can share more folders or remove them by using the command
  `flatpak override --[no]filesystem=<folder>`.

* Because Xephyr version `1.17.0` and later don't work fine with `GtkSocket`
  (see downstream and upstream bugs), Remmina's Flatpak bundles an out of date
  version of Xephyr (`1.16.4`) for the XDMCP plugin.

    - https://github.com/FreeRDP/Remmina/issues/366
    - https://bugs.freedesktop.org/show_bug.cgi?id=91700
