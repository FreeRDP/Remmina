# Quick and dirty guide for compiling remmina on ubuntu 14.04

These are instructions for people or software developers who want to contribute to the latest version of Remmina on Ubuntu 14.04.

If you are an end user and you want to install the latest version of remmina, please use the "Remmina Team Ubuntu PPA - next branch", as explained on the [homepage of the wiki](Home.md).

By following these instructions, you will get Remmina and FreeRDP compiled under the /opt/remmina_devel/ subdir, so they will not mess up your system too much. This is ideal for testing remmina.

You will also find the uninstall instructions at the bottom of this page.

**Changelog**
* Initial write: Aug 20 2014.
* Update Oct 3 2014: changed branch name from gtk3 to next
* Update Oct 15 2014: addedd -DWITH_CUPS=on -DWITH_WAYLAND=off to freerdp parameters
* Update Oct 23 2014: tested on ubuntu 14.10
* Update Oct 29 2014: tested on Mine 17.2 (based on Ubuntu 14.04)
* Update Oct 29 2015: Found a load of install issued on step 1, changed recommendation from apt-get to aptitude as it reports issues far more lucidly and they all boil down to my having later versions of packages these depend on.
* Update Nov 23 2015: Added libsecret-1-dev to packages to be installed
* Update Jan 16 2016: Added libsystemd-dev
* Update Jan 23 2016: Added libwebkit2gtk-3.0-dev
* Update Mar 12 2016: Added apt-get remove of some freerdp packages installed from the PPA, removed ubuntu 14.10

**1.** Install all packages required to build freerdp and remmina:
```
sudo aptitude install build-essential git-core cmake libssl-dev libx11-dev libxext-dev libxinerama-dev \
  libxcursor-dev libxdamage-dev libxv-dev libxkbfile-dev libasound2-dev libcups2-dev libxml2 libxml2-dev \
  libxrandr-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libxi-dev libavutil-dev \
  libavcodec-dev libxtst-dev libgtk-3-dev libgcrypt11-dev libssh-dev libpulse-dev \
  libvte-2.90-dev libxkbfile-dev libfreerdp-dev libtelepathy-glib-dev libjpeg-dev \
  libgnutls-dev libgnome-keyring-dev libavahi-ui-gtk3-dev libvncserver-dev \
  libappindicator3-dev intltool libsecret-1-dev libwebkit2gtk-3.0-dev
```
And try also to install libsystemd-dev (available only in newer ubuntu)
```
sudo aptitude install libsystemd-dev
```
**2.** Remove freerdp-x11 package and all packages containing the string remmina in the package name.
```
sudo apt-get --purge remove freerdp-x11 \
 remmina remmina-common remmina-plugin-rdp remmina-plugin-vnc remmina-plugin-gnome \
 remmina-plugin-nx remmina-plugin-telepathy remmina-plugin-xdmcp
sudo apt-get --purge remove libfreerdp-dev libfreerdp-plugins-standard libfreerdp1
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
For ubuntu 32 bit you have to change the path of the source lib folder in the first line.

**8.** Link executable in /usr/local/bin
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
**13.** Link the executable
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
