Precompiled binaries
-----------------

See [the wiki](https://gitlab.com/Remmina/Remmina/wikis/home#for-users-with-a-distro-that-supports-flatpak-including-ubuntu) for instructions. The recipe here is always pointed to the `master` branch. The recipe in [the flathub repository](https://github.com/flathub/org.remmina.Remmina/blob/master/org.remmina.Remmina.json) is pointed to the latest tagged release. Files except the JSON file in this directory is used for automatic builds.

Build instructions
------------------

0. Update fluthub shared modules

From time to time update the shared-modules provided by flathub

```shell
git submodule update --remote --merge
```

1. Install `flatpak` and `flatpak-builder` ([instructions]). Remmina
   Flatpak manifest recommends the latest version of `flatpak-builder`.

[instructions]: http://flatpak.org/getting.html

2. Enable the Flatpak repository maintained by Flathub:

```shell
flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

3. Build Remmina:

```shell
git submodule init -- shared-modules/
git submodule update -- shared-modules/
flatpak-builder --user --sandbox --install-deps-from=flathub --repo=repo/ appdir/ org.remmina.Remmina.json
```

   Remmina will be built in `appdir/` folder and the result will be exported
   to a local Flatpak repository in `repo/` folder.

4. Enable the local repository:

```shell
flatpak --user remote-add --no-gpg-verify --if-not-exists my-repo repo/
```

5. Install Remmina from your repository:

```shell
flatpak --user install my-repo org.remmina.Remmina
```

6. Launch Remmina

```shell
flatpak run org.remmina.Remmina
```

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

    - https://gitlab.com/Remmina/Remmina/issues/366
    - https://bugs.freedesktop.org/show_bug.cgi?id=91700
