# Quick and dirty guide for compiling remmina on Arch

These are the instructions for people who want to test the latest version of Remmina on Arch linux

You will obtain Remmina compiled under the /opt/remmina_devel/ subdir, so it will not mess up your system too much. This is ideal for testing remmina.

You will also find the uninstall instructions at the bottom of this page.

**ChangeLog**
- Initial write: Jul 16 2015


You must be **root** to follow this guide.

**1.** Update your system and install all packages required to build freerdp and remmina:
```
pacman -Syu
```
and reboot if needed.

Install packages needed for compilation:
```
pacman -S base-devel git libssh libvncserver gnome-keyring libgnome-keyring libpulse vte3 cmake
```
**2.** Remove installed versions of remmina and freerdp

```
pacman -Rs remmina freerdp
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
cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_SSE2=ON -DWITH_CUPS=on -DWITH_WAYLAND=off -DWITH_PULSE=on -DCMAKE_INSTALL_PREFIX:PATH=/opt/remmina_devel/freerdp .
```
Please note that the above line will make FreeRDP install in /opt/remmina_devel/freerdp

**6.** Compile FreeRDP and install
```
make && make install
```
**7.** Make your system dynamic loader aware of the new libraries you installed. For Ubuntu x64:
```
echo /opt/remmina_devel/freerdp/lib | sudo tee /etc/ld.so.conf.d/freerdp_devel.conf > /dev/null
sudo ldconfig
```
Please note: on arch 64 bit systems, the above lib directory could be different (`/opt/remmina_devel/freerdp/lib64`)

**8.** Link executable in /usr/local/bin
```
ln -s /opt/remmina_devel/freerdp/bin/xfreerdp /usr/local/bin/
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
cmake -DWITH_TELEPATHY=off -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=/opt/remmina_devel/remmina -DWITH_APPINDICATOR=off -DCMAKE_PREFIX_PATH=/opt/remmina_devel/freerdp --build=build .
```
**12.** Compile remmina and install it
```
make && make install
```
**13.** Link the executable
```
ln -s /opt/remmina_devel/remmina/bin/remmina /usr/local/bin/
```
**14.** Run remmina
```
remmina
```
NOTES for execution:
* Icons and .desktop files are not installed, so don't search for remmina in Gnome Shell. You can only launch it from a terminal or pressing ALT-F2 and typing remmina.
* Gnome Shell will never show you the system tray icon and menu. Press Super+M to see the remmina icon on the message bar.

## Uninstall everything
**1.** Remove the devel directory
```
rm -rf ~/remmina_devel/
```
**2.** Remove the binary directory and the symlink
```
rm -rf /opt/remmina_devel/ /usr/local/bin/remmina
```
