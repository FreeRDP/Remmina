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

### From the source code

Follow the following guides:

* How to compile "next" branch on Ubuntu 14.04 and 14.10: [Compile-on-Ubuntu-14.04-and-14.10](Compile-on-Ubuntu-14.04-and-14.10)
* How to compile "next" branch on Fedora 20: [Compile-on-Fedora-20](Compile-on-Fedora-20)
* For Debian 7.6 you can use the Ubuntu guide, but you must disable SSH with -DWITH_LIBSSH=OFF when executing cmake. libssh provided with debian 7 (v 5.4) is older than the needed version 6.x.

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

[![paypal](https://www.paypalobjects.com/en_US/CH/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZBD87JG52PTZC)

## Authors

Remmina is maintained by:

 * [Antenore Gatta](https://github.com/antenore)
 * [Giovanni Panozzo](https://github.com/giox069)

See the [AUTHORS](https://raw.githubusercontent.com/FreeRDP/Remmina/next/AUTHORS) for an exhaustive list.
If you are not listed and you have contributed, feel free to update that file.

## Resources

 * [Wiki and compilation instructions](https://github.com/FreeRDP/Remmina/wiki)
 * [Mailing List](https://lists.sourceforge.net/mailman/listinfo/remmina-common)
 * [Website](http://www.remmina.org)

## License

Licensed under the [GPLv2](http://www.opensource.org/licenses/GPL-2.0) license
