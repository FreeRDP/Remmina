Welcome to the Remmina wiki!

## FOR END USERS ##
Usually remmina is included in your linux distribution or in an external repository. Do not ask for distribution packages or precompiled binaries here. This is a development site.

### For Ubuntu users we have an official PPA with Remmina 1.2 ###
https://launchpad.net/~remmina-ppa-team/+archive/ubuntu/remmina-next

To install it, just copy and paste the following three lines on a terminal window
```
sudo apt-add-repository ppa:remmina-ppa-team/remmina-next
sudo apt-get update
sudo apt-get install remmina remmina-plugin-rdp libfreerdp-plugins-standard
```
Don't forget to exit/kill al your running instances of remmina. The best way to do it is to **reboot** your PC. Or, if you cannot reboot, kill all running remmina with
```
sudo killall remmina
```
By default the RDP, SSH and SFTP plugins are installed. You can view a list of available plugins with `apt-cache search remmina-plugin`

## FOR DEVELOPERS, PACKAGERS AND TESTERS ##
### CURRENT STATUS ###

Here, on github, we have three official branches:

"next" branch: the future 1.2.0 version, which compiles with current freerdp master branch and will compile with freerdp 2.0 when released. We will do code developments and bugfixes here, on the next branch.

"master" branch: the 1.1.2 stable version, which compiles with freerdp 1.1 stable. We will only provide security fixes or critical bugs (i.e.: data corruption) fixes on version 1.1.X.

"gtk2" branch: a special remmina 1.1.1 version compatible with GTK2, compiles with freerdp 1.1 stable

### REMMINA COMPILATION GUIDES ###

* How to compile "next" branch on [Ubuntu 14.04](Compile-on-Ubuntu-14.04.md) and [Ubuntu 16.04](Compile-on-Ubuntu-16.04.md).
* How to compile "next" branch on [Fedora 20](Compile-on-Fedora-20.md).
* For Debian 7.6 you can use the Ubuntu guide, but you must disable SSH with -DWITH_LIBSSH=OFF when executing cmake. libssh provided with debian 7 (v 5.4) is older than the needed version 6.x.

### Links ###
We also have a website: http://www.remmina.org
