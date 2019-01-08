[![](https://img.shields.io/liberapay/patrons/:entity.svg)](https://liberapay.com/Remmina/)
[![Snap Status](https://build.snapcraft.io/badge/FreeRDP/Remmina.svg)](https://build.snapcraft.io/user/FreeRDP/Remmina)
[![Build Status](https://gitlab.com/Remmina/Remmina/badges/master/build.svg)](https://gitlab.com/Remmina/Remmina/pipelines)
[![Bountysource](https://img.shields.io/bountysource/team/remmina/activity.svg)](https://www.bountysource.com/teams/remmina)
[![CodeTriage](https://www.codetriage.com/freerdp/remmina/badges/users.svg)](https://www.codetriage.com/freerdp/remmina)

# IMPORTANT NOTICE

We have moved to [gitlab](https://gitlab.com/Remmina/Remmina)

Since the 1st of July 2018 the following links have to be used instead of GitHub.

* [issues URL](https://gitlab.com/Remmina/Remmina/issues)
* [wiki URL](https://gitlab.com/Remmina/Remmina/wikis/home)

# Remmina: The GTK+ Remote Desktop Client

Initially developed by [Vic Lee](https://github.com/llyzs)

## Description
**Remmina** is a remote desktop client written in GTK+, aiming to be useful for
system administrators and travellers, who need to work with lots of remote
computers in front of either large monitors or tiny netbooks. Remmina supports
multiple network protocols in an integrated and consistent user interface.
Currently RDP, VNC, SPICE, NX, XDMCP, SSH and EXEC are supported.

Remmina is released in separated source packages:
* "remmina", the main GTK+ application
* "remmina-plugins", a set of plugins

Remmina is free and open-source software, released under GNU GPL license.

## Installation

### Binary distributions
Usually remmina is included in your linux distribution or in an external repository.
Do not ask for distribution packages or precompiled binaries here.
This is a development site.

### Debian ###

Remmina is not available on the default Debian 9 (Stretch) repositories. It can be installed from the Backports repository.

The [Debian Backports](https://backports.debian.org/Instructions/) repository must be enabled to install it, see [this blog post](https://www.remmina.org/wp/debian-the-boys-are-backport-in-town/) for more information.

To install Remmina from Debian Backports, just copy and paste the following lines on a terminal window:

```
echo 'deb http://ftp.debian.org/debian stretch-backports main' | sudo tee --append /etc/apt/sources.list.d/stretch-backports.list >> /dev/null
sudo apt update
sudo apt install -t stretch-backports remmina remmina-plugin-rdp remmina-plugin-secret
```

### Ubuntu

#### Using Snap Package (also for other [supported distros](https://snapcraft.io/docs/core/install))

You can install the last release from the Ubuntu Software center, looking for `remmina`, otherwise you can install it from terminal with:

```sh
sudo snap install remmina
```

If you want to install latest git revision of remmina, you can use it from the `edge` channel:

```sh
sudo snap install remmina  --edge
```

Or update the current installed version with the selected channel:

```sh
sudo snap refresh remmina --channel=edge # use --channel=stable otherwise
```

To enable some advanced features such as `mount-control` (to manage mount positions), `avahi-observer` (to automatically look for local servers to connect to), `cups-control` (to manage printing), `password-manager-service` (to use gnome-keyring) you should run something like:

```sh
sudo snap connect remmina:avahi-observe :avahi-observe # servers discovery
sudo snap connect remmina:cups-control :cups-control # printing
sudo snap connect remmina:mount-observe :mount-observe # mount management
sudo snap connect remmina:password-manager-service :password-manager-service # gnome-keyring
```

Snap packages will be updated automatically and will include both latest `FreeRDP` git and latest `libssh 0.7` release (for better security).

#### From PPA

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

### Fedora and Red Hat

As of March 2018 Remmina is available on most fedora testing and stable, we still have a (not updated) copr provided by [Hubbitus](https://github.com/Hubbitus) (Pavel Alexeev), to install just execute as root:

```sh
dnf copr enable hubbitus/remmina-next
dnf upgrade --refresh 'remmina*' 'freerdp*'
```

~~On Red Hat you can enable the EPEL repository:~~

Note: Unlucky Remmina is not yet in EPEL, you can help submitting a request on the [Red Hat bugzilla](https://bugzilla.redhat.com/).

```sh
wget http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
rpm -ivh epel-release-latest-7.noarch.rpm
```

### Arch Linux based

Install [remmina-git](https://aur.archlinux.org/packages/remmina-git) from [AUR](https://aur.archlinux.org/)

### openSUSE

Remmina is in the offical repositories for all openSUSE distributions.
In case the version in the released stable branch of openSUSE is too old you can install the latest one from the [devel project](https://build.opensuse.org/package/show/X11%3ARemoteDesktop/remmina) via:

```
zypper ar -f obs://X11:RemoteDesktop/remmina remmina
zypper ref
zypper in remmina
```

### For users with a distro that supports [Flatpak](https://flathub.org/), including Ubuntu ###

## Development snapshot

Download [this](https://gitlab.com/Remmina/Remmina/-/jobs/artifacts/master/raw/flatpak/remmina-dev.flatpak\?job\=flatpak:test) flatpak
and install it as described here

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user --bundle remmina-dev.flatpak
flatpak run org.remmina.Remmina
```

## Last stable official build on [FlatHub](https://flathub.org/apps/details/org.remmina.Remmina)

Execute the following commands.

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.remmina.Remmina
flatpak run org.remmina.Remmina
```
If you use SSH agent (https://github.com/flatpak/flatpak/issues/1438 )

```sh
flatpak run --filesystem=$SSH_AUTH_SOCK --env=SSH_AUTH_SOCK=$SSH_AUTH_SOCK org.remmina.Remmina
```

Just be aware that flatpak store data for installed applications (the XDG config/data folders) under ```$HOME/.var```
So for instance, if you previously have installed remmina with another package manager, you will have to transfer what was under ```$HOME/.config/remmina``` and ```$HOME/.local/share/remmina``` under, respectively ```~/.var/app/org.remmina.Remmina/config/remmina``` and ```~/.var/app/org.remmina.Remmina/data/remmina```

### External not supported plugins

There are also some external, not supported plugins provided by [Muflone](https://github.com/muflone) :

* [remmina-plugin-folder](https://aur.archlinux.org/packages/remmina-plugin-folder/) A protocol plugin for Remmina to open a folder.
* [remmina-plugin-open](https://aur.archlinux.org/packages/remmina-plugin-open/) A protocol plugin for Remmina to open a document with its associated application.
* [remmina-plugin-rdesktop](https://aur.archlinux.org/packages/remmina-plugin-rdesktop/) A protocol plugin for Remmina to open a RDP connection with rdesktop.
* [remmina-plugin-teamviewer](https://aur.archlinux.org/packages/remmina-plugin-teamviewer/) A protocol plugin for Remmina to launch a TeamViewer connection.
* [remmina-plugin-ultravnc](https://aur.archlinux.org/packages/remmina-plugin-ultravnc/) A protocol plugin for Remmina to connect via VNC using UltraVNC viewer.
* [remmina-plugin-url](https://aur.archlinux.org/packages/remmina-plugin-url/) A protocol plugin for Remmina to open an URL in an external browser.
* [remmina-plugin-webkit](https://aur.archlinux.org/packages/remmina-plugin-webkit/) A protocol plugin for Remmina to launch a GTK+ Webkit browser.

### From the source code

Follow the guides available on the wiki:
* [Wiki and compilation instructions](https://gitlab.com/Remmina/Remmina/wikis/home)

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
  -V, --full-version          Show the application's version, including the plugin versions
  --display=DISPLAY           X display to use
```

## Configuration

You can configure everything from the graphical interface or editing by hand the files under $HOME/.remmina or $HOME/.config/remmina

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for a better overview.

If you want to contribute with code:

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

If you want to contribute in other ways, drop us an email using the form provided in our web site.

### Donations

If you rather prefer to contribute to Remmina with money you are more than welcome.

For more informations See the [Remmina web site donation page](http://remmina.org/wp/donations).

See the [THANKS.md](THANKS.md) file for an exhaustive list of supporters.

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
 * [Dario Cavedon](https://github.com/ic3d)

See the [AUTHORS](AUTHORS) for an exhaustive list.
If you are not listed and you have contributed, feel free to update that file.

## Resources

 * [Wiki and compilation instructions](https://gitlab.com/Remmina/Remmina/wikis/home)
 * [G+ Remmina community](https://plus.google.com/communities/106276095923371962010)
 * [Website](http://www.remmina.org)
 * IRC we are on freenode.net , in the channel #remmina, you can also use a [web client](https://webchat.freenode.net/) in case

## License

Licensed under the [GPLv2](http://www.opensource.org/licenses/GPL-2.0) license
