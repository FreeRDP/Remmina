# Quick and dirty guide for compiling remmina on ubuntu 16.04

These are instructions for people or software developers who want to contribute to the latest version of Remmina on Ubuntu 16.04.

If you are an end user and you want to install the latest version of remmina, please use the "Remmina Team Ubuntu PPA - next branch", as explained on the [homepage of the wiki](Home.md).

By following these instructions, you will get Remmina and FreeRDP compiled under the /opt/remmina_devel/ subdir, so they will not mess up your system too much. This is ideal for testing remmina.

You will also find the uninstall instructions at the bottom of this page.

**Changelog**
* Initial write: Mar 30 2016.
* Apr 19 2016: added apt-get remove for libwinpr and other packages

**1.** Install all packages required to build freerdp and remmina:
```
sudo apt-get install build-essential git-core cmake libssl-dev libx11-dev libxext-dev libxinerama-dev \
  libxcursor-dev libxdamage-dev libxv-dev libxkbfile-dev libasound2-dev libcups2-dev libxml2 libxml2-dev \
  libxrandr-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libxi-dev libavutil-dev \
  libavcodec-dev libxtst-dev libgtk-3-dev libgcrypt11-dev libssh-dev libpulse-dev \
  libvte-2.91-dev libxkbfile-dev libtelepathy-glib-dev libjpeg-dev \
  libgnutls-dev libgnome-keyring-dev libavahi-ui-gtk3-dev libvncserver-dev \
  libappindicator3-dev intltool libsecret-1-dev libwebkit2gtk-4.0-dev libsystemd-dev
```
**2.** Remove freerdp-x11 package and all packages containing the string remmina in the package name.
```
sudo apt-get --purge remove freerdp-x11 \
 remmina remmina-common remmina-plugin-rdp remmina-plugin-vnc remmina-plugin-gnome \
 remmina-plugin-nx remmina-plugin-telepathy remmina-plugin-xdmcp
sudo apt-get --purge remove libfreerdp-dev libfreerdp-plugins-standard libfreerdp1 \
 libfreerdp-utils1.1 libfreerdp-primitives1.1 libfreerdp-locale1.1 \
 libfreerdp-gdi1.1 libfreerdp-crypto1.1 libfreerdp-core1.1 libfreerdp-common1.1.0 \
 libfreerdp-codec1.1 libfreerdp-client1.1 libfreerdp-cache1.1
sudo apt-get --purge remove \
  libfreerdp-rail1.1 libwinpr-asn1-0.1 libwinpr-bcrypt0.1 libwinpr-credentials0.1 libwinpr-credui0.1 \
  libwinpr-crt0.1 libwinpr-crypto0.1 libwinpr-dev libwinpr-dsparse0.1 libwinpr-environment0.1 \
  libwinpr-error0.1 libwinpr-file0.1 libwinpr-handle0.1 libwinpr-heap0.1 libwinpr-input0.1 \
  libwinpr-interlocked0.1 libwinpr-io0.1 libwinpr-library0.1 libwinpr-path0.1 libwinpr-pipe0.1 \
  libwinpr-pool0.1 libwinpr-registry0.1 libwinpr-rpc0.1 libwinpr-sspi0.1 libwinpr-sspicli0.1 \
  libwinpr-synch0.1 libwinpr-sysinfo0.1 libwinpr-thread0.1 libwinpr-timezone0.1 libwinpr-utils0.1 \
  libwinpr-winhttp0.1 libwinpr-winsock0.1
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
make && sudo make install
```
**7.** Make your system dynamic loader aware of the new libraries you installed. For Ubuntu x64:
```
echo /opt/remmina_devel/freerdp/lib | sudo tee /etc/ld.so.conf.d/freerdp_devel.conf > /dev/null
sudo ldconfig
```

**8.** Create a symbolik link to the executable in /usr/local/bin
```
sudo ln -s /opt/remmina_devel/freerdp/bin/xfreerdp /usr/local/bin/
```
**9.** Test the new freerdp by connecting to a RDP host
```
xfreerdp +clipboard /sound:rate:44100,channel:2 /v:hostname /u:username
```

**10.** Now clone remmina repository, "next" branch, to your devel dir:
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
**13.** Create a symbolik link to the the executable
```
sudo ln -s /opt/remmina_devel/remmina/bin/remmina /usr/local/bin/
```
**14.** Run remmina
```
remmina
```
Please note that icons and launcher files are not installed, so don't search for remmina using Unity Dash.

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
