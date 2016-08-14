# Remmina dependencies.

In order to use Remmina, you need at least one plugin, here we explain you how to build Remmina with most plugins but mainly with FreerDP support.

## How to compile FreeRDP

Follow the next procedure to install FreerDPP

### Install FreeRDP dependencies

```sh
pkg install cmake cmake-modules gccmakedep git pkgconf libX11 libXext libXinerama libXcursor libXdamage libXv libxkbfile alsa-lib cups ffmpeg pulseaudio libssh libXi libXtst libXrandr xmlto gstreamer1 gstreamer1-plugins
```
### Get FreeRDP code in you development environment
**1.** Prepare your dev environment
```
mkdir ~/remmina_devel
cd ~/remmina_devel
```
**2.** Get the source code
```
git clone https://github.com/FreeRDP/FreeRDP.git
cd FreeRDP
```
**3.** Configure FreeRDP
```
cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_SSE2=ON -DWITH_PULSE=ON -DWITH_CUPS=on -DWITH_WAYLAND=off -DCMAKE_INSTALL_PREFIX:PATH=/opt/remmina_devel/freerdp .
```
**4.** Compile and install

```
make && sudo make install
```
**5.** Add the freerdp library path to ldconfig
Edit the file `/etc/ld-elf.so.conf`, create it if it does not exits. A add the line
```
/opt/remmina_devel/freerdp/lib/
```
And then restart ldconfig with
```
service ldconfig restart
```
**6.** Link the xfreerdp executable

If you use Gnome, you probably have FreerDP installed under /usr/local. Gnome needs vinagre that needs FreerDP, so you won't be able to remove FreeRDP, that shouldn't be an issue as we installed the git version under /opt... But keep this in mind if you'll have troubles.
If you don't have Gnome, you can link xfreerdp under /usr/local/bin
```
sudo ln -s /opt/remmina_devel/freerdp/bin/xfreerdp /usr/local/bin/
```
**7.** Test xfreerdp
```
xfreerdp +clipboard /sound:rate:44100,channel:2 /v:hostname /u:username
```
## How to compile Remmina
**1.** Install Remmina dependencies.
```
pkg install avahi-gtk3 gtk3 libgcrypt webkit2-gtk3 webkit-gtk3 gnutls avahi vte3-290 vte3 telepathy-glib libSM openjpeg libvncserver
```
**2.** Install gnome keyring or similar password storage for your specific desktop
```
pkg install gnome-keyring
```
**3.** Clone Remmina
```
cd ~/remmina_devel
git clone https://github.com/FreeRDP/Remmina.git -b next
```
**4.** Configure compile settings
```
cd Remmina
cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_APPINDICATOR=OFF -DCMAKE_INSTALL_PREFIX:PATH=/opt/remmina_devel/remmina -DCMAKE_PREFIX_PATH=/opt/remmina_devel/freerdp --build=build .
```
**5.** Compile remmina and install it
```
make && make install
```
**6.** Run Remmina
```
/opt/remmina_devel/remmina/bin/remmina
```
