# Quick and dirty guide for compiling remmina on Fedora 20

These are the instructions for people who want to test the latest version of Remmina on Fedora 20

You will obtain Remmina and FreeRDP compiled under the /opt/remmina_devel/ subdir, so they will not mess up your system too much. This is ideal for testing remmina.

You will also find the uninstall instructions at the bottom of this page.

**ChangeLog**
- Initial write: Aug 23 2014.
- Updated: Aug 30 2014 (compile with libappindicator)
- Updated: Sep 2 2014 (notes for Gnome Shell Users)
- Update: Oct 3 2014 (change from branch "gtk3" to "next")
- Update Oct 15 2014: addedd -DWITH_CUPS=on -DWITH_WAYLAND=off to freerdp parameters

*UNTESTED*: use at your own risk

**1.** Update your system and install all packages required to build freerdp and remmina:
```
sudo yum -y update
```
and reboot if needed. Then install packages for freerdp
```
sudo yum install gcc gcc-c++ cmake openssl-devel libX11-devel libXext-devel \
  libXinerama-devel libXcursor-devel \
  libXdamage-devel libXv-devel libxkbfile-devel alsa-lib-devel cups-devel ffmpeg-devel glib2-devel \
  pulseaudio-libs-devel git libssh-devel libXi-devel libXtst-devel xmlto gstreamer1-devel \
  libXrandr-devel gstreamer1-plugins-base-devel 
```
and then install packages for remmina
```
sudo yum install gtk3-devel libgcrypt-devel libssh-devel libxkbfile-devel openjpeg2-devel \
 gnutls-devel libgnome-keyring-devel avahi-ui-devel avahi-ui-gtk3 \
 libvncserver-devel vte3-devel libappindicator-devel libappindicator-gtk3 libappindicator-gtk3-devel \
 telepathy-glib-devel libSM-devel webkitgtk4-devel
```
**2.** Remove freerdp-x11 package and all packages containing the string remmina in the package name.

```
sudo rpm -e remmina remmina-devel remmina-plugins-gnome remmina-plugins-nx remmina-plugins-rdp remmina-plugins-telepathy remmina-plugins-vnc remmina-plugins-xdmcp
```

**3.** Create a new directory for development in your home directory, and cd into it
```
mkdir ~/remmina_devel
cd ~/remmina_devel
```
**4.** Download the latest source code of FreeRDP from its master branch
```
git clone https://github.com/FreeRDP/FreeRDP.git
cd FreeRDP
```
**5.** Configure FreeRDP for compilation (don't forget to include -DWITH_PULSE=ON)
```
cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_SSE2=ON -DWITH_PULSE=ON -DWITH_CUPS=on -DWITH_WAYLAND=off -DCMAKE_INSTALL_PREFIX:PATH=/opt/remmina_devel/freerdp .
```
Please note that the above line will make FreeRDP install in /opt/remmina_devel/freerdp

**6.** Compile FreeRDP and install
```
make && sudo make install
```
**7.** Make your system dynamic loader aware of the new libraries you installed. For Fedora x64:
```
echo /opt/remmina_devel/freerdp/lib64/ | sudo tee /etc/ld.so.conf.d/freerdp_devel.conf > /dev/null
sudo ldconfig
```
For fedora 32 bit (i686) you have to change the path of the source lib folder in the first line.

**8.** Link executable in /usr/local/bin
```
sudo ln -s /opt/remmina_devel/freerdp/bin/xfreerdp /usr/local/bin/
```
**9.** Test the new freerdp by connecting to a RDP host
```
xfreerdp +clipboard /sound:rate:44100,channel:2 /v:hostname /u:username
```

**10.** Now clone remmina repository, branch "next" to your devel dir:
```
cd ~/remmina_devel
git clone https://github.com/FreeRDP/Remmina.git -b next
```

**11.** Configure Remmina for compilation
```
cd Remmina
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=/opt/remmina_devel/remmina -DCMAKE_PREFIX_PATH=/opt/remmina_devel/freerdp --build=build .
```
**12.** Compile remmina and install it
```
make && sudo make install
```
**13.** Link the executable
```
sudo ln -s /opt/remmina_devel/remmina/bin/remmina /usr/local/bin/
```
**14.** Run remmina
```
remmina
```
NOTES for execution:
* Icons and .desktop files are not installed, so don't search for remmina in Gnome Shell. You can only launch it from a terminal or pressing ALT-F2 and typing remmina.
* Gnome Shell will never show you the system tray icon and menu. Press Super+M to see the remmina icon on the message bar. Or install this extension to have full access to a remmina appindicator icon: [Appindicator Support Gnome Shell Extension](https://extensions.gnome.org/extension/615/appindicator-support/)
* XFCE and other desktop without appindicator support, will never show you the system tray icon if you are executing remmina from its compilation direcotry (~/remmina_devel/Remmina) because it contains a directory named remmina. See [Bug #1363277 on launchpad](https://bugs.launchpad.net/libappindicator/+bug/1363277)

## Uninstall everything
**1.** Remove the devel directory
```
rm -rf ~/remmina_devel/
```
**2.** Remove the binary directory
```
sudo rm -rf /opt/remmina_devel/
```
**3.** Cleanup symlinks and dynamic loader
```
sudo rm /etc/ld.so.conf.d/freerdp_devel.conf /usr/local/bin/remmina /usr/local/bin/xfreerdp
sudo ldconfig
```
