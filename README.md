[![Build Status](https://travis-ci.org/FreeRDP/Remmina.png)](https://travis-ci.org/FreeRDP/Remmina) [![Bountysource](https://img.shields.io/bountysource/team/remmina/activity.svg)]()

# Remmina: The GTK+ Remote Desktop Client

Initially developed by [Vic Lee](https://github.com/llyzs)

## Description
**Remmina** is a remote desktop client written in GTK+, aiming to be useful for
system administrators and travellers, who need to work with lots of remote
computers in front of either large monitors or tiny netbooks. Remmina supports
multiple network protocols in an integrated and consistant user interface.
Currently RDP, VNC, SPICE, NX, XDMCP and SSH are supported.

Remmina is released in separated source packages:
* "remmina", the main GTK+ application
* "remmina-plugins", a set of plugins

Remmina is free and open-source software, released under GNU GPL license.

## Installation

### Binary distributions
Usually remmina is included in your linux distribution or in an external repository.
Do not ask for distribution packages or precompiled binaries here.
This is a development site.

### Ubuntu

[Ubuntu ppa:remmina-ppa-team/remmina-next](https://launchpad.net/~remmina-ppa-team/+archive/ubuntu/remmina-next)

To install it, just copy and paste the following three lines on a terminal window
```sh
sudo apt-add-repository ppa:remmina-ppa-team/remmina-next
sudo apt-get update
sudo apt-get install remmina remmina-plugin-rdp libfreerdp-plugins-standard
```
By default the RDP, SSH and SFTP plugins are installed. You can view a list of available plugins with `apt-cache search remmina-plugin`

If you want to connect to more securely configured SSH servers on Ubuntu 16.04 and below, you have to upgrade libssh to 0.7.X. This can be achieved by adding the following PPA containing libssh 0.7.X by David Kedves and upgrading your packages:
```sh
sudo add-apt-repository ppa:kedazo/libssh-0.7.x
sudo apt-get update
```

### Fedora

[Hubbitus](https://github.com/Hubbitus) (Pavel Alexeev) provided us a copr, to install just execute as root:

```sh
# dnf copr enable hubbitus/remmina-next
# dnf upgrade --refresh 'remmina*' 'freerdp*'
```

### Arch Linux based

Install [remmina-git](https://aur.archlinux.org/packages/remmina-git) from [AUR](https://aur.archlinux.org/)

There are also some external, not supported plugins provided by [Muflone](https://github.com/muflone) :

* [remmina-plugin-exec](https://aur.archlinux.org/packages/remmina-plugin-exec/) A protocol plugin for Remmina to execute an external process. 
* [remmina-plugin-folder](https://aur.archlinux.org/packages/remmina-plugin-folder/) A protocol plugin for Remmina to open a folder. 
* [remmina-plugin-open](https://aur.archlinux.org/packages/remmina-plugin-open/) A protocol plugin for Remmina to open a document with its associated application. 
* [remmina-plugin-rdesktop](https://aur.archlinux.org/packages/remmina-plugin-rdesktop/) A protocol plugin for Remmina to open a RDP connection with rdesktop.  
* [remmina-plugin-teamviewer](https://aur.archlinux.org/packages/remmina-plugin-teamviewer/) A protocol plugin for Remmina to launch a TeamViewer connection.  
* [remmina-plugin-ultravnc](https://aur.archlinux.org/packages/remmina-plugin-ultravnc/) A protocol plugin for Remmina to connect via VNC using UltraVNC viewer.  
* [remmina-plugin-url](https://aur.archlinux.org/packages/remmina-plugin-url/) A protocol plugin for Remmina to open an URL in an external browser.  
* [remmina-plugin-webkit](https://aur.archlinux.org/packages/remmina-plugin-webkit/) A protocol plugin for Remmina to launch a GTK+ Webkit browser.  

### From the source code

Follow the guides available on the wiki:
* [Wiki and compilation instructions](https://github.com/FreeRDP/Remmina/wiki)

## Usage

Just select Remmina from your application menu or execute remmina from the command line

Remmina support also the following options:

```sh
  -a, --about                 Show about dialog
  -c, --connect=FILE          Connect to a .remmina file
  -e, --edit=FILE             Edit a .remmina file
  -n, --new                   Create a new connection profile
  -p, --pref=PAGENR           Show preferences dialog page
  -x, --plugin=PLUGIN         Execute the plugin
  -q, --quit                  Quit the application
  -s, --server=SERVER         Use default server name
  -t, --protocol=PROTOCOL     Use default protocol
  -i, --icon                  Start as tray icon
  -v, --version               Show the application's version
  --display=DISPLAY           X display to use
```

## Configuration

You can configure everything from the graphical interface or editing by hand the files under $HOME/.remmina or $HOME/.config/remmina 

## Contributing

If you want to contribute with code:

1. [Fork it](https://github.com/FreeRDP/Remmina#fork-destination-box)
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

If you want to contribute in other ways, drop us an email using the form provided in our web site.

### Donations

If you rather prefer to contribute to Remmina with money you are more than welcome.

For more informations See the [Remmina web site donation page](http://remmina.org/wp/donations).

See the [THANKS.md](https://raw.githubusercontent.com/FreeRDP/Remmina/next/THANKS.md) file for an exhaustive list of supporters.

#### Paypal

[![paypal](https://www.paypalobjects.com/en_US/CH/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZBD87JG52PTZC) 

#### Bitcoin

[![bitcoin](http://www.remmina.org/wp/wp-content/uploads/2016/06/bitcoin_1298H2vaxcbDQRuR-e1465504491655.png)](bitcoin:1298H2vaxcbDQRuRYkDjfFbvGEgxE1CNjk?label=Remmina%20Donation)

If clicking on the line above does not work, use this payment info: 

- Remmina bitcoin address:  1298H2vaxcbDQRuRYkDjfFbvGEgxE1CNjk  
- Message: Remmina Donation  

## Authors

Remmina is maintained by:

 * [Antenore Gatta](https://github.com/antenore)
 * [Giovanni Panozzo](https://github.com/giox069)

See the [AUTHORS](https://raw.githubusercontent.com/FreeRDP/Remmina/next/AUTHORS) for an exhaustive list.
If you are not listed and you have contributed, feel free to update that file.

## Resources

 * [Wiki and compilation instructions](https://github.com/FreeRDP/Remmina/wiki)
 * [G+ Remmina community](https://plus.google.com/communities/106276095923371962010)
 * [Website](http://www.remmina.org)
 * IRC we are on freenode.net , in the channel #remmina, you can also use a [web client](https://webchat.freenode.net/) in case

## License

Licensed under the [GPLv2](http://www.opensource.org/licenses/GPL-2.0) license
