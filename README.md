[![](https://img.shields.io/liberapay/receives/Remmina.svg?logo=liberapay)](https://liberapay.com/Remmina/donate)
[![Build Status](https://gitlab.com/Remmina/Remmina/badges/master/build.svg)](https://gitlab.com/Remmina/Remmina/pipelines)
[![CodeTriage](https://www.codetriage.com/freerdp/remmina/badges/users.svg)](https://www.codetriage.com/freerdp/remmina)
[![Translation status](https://hosted.weblate.org/widgets/remmina/-/remmina/svg-badge.svg)](https://hosted.weblate.org/engage/remmina/?utm_source=widget)

# IMPORTANT NOTICE

Remmina is now here on [GitLab](https://gitlab.com/Remmina/Remmina)

As of the 1st of July 2018, the following links have to be used instead of respective GitHub counterparts.

* [Issues URL](https://gitlab.com/Remmina/Remmina/issues)
* [Wiki URL](https://gitlab.com/Remmina/Remmina/wikis/home)

# Remmina: The GTK Remote Desktop Client

Initially developed by [Vic Lee](https://github.com/llyzs).

## Description
**Remmina** is a remote desktop client written in GTK, aiming to be useful for
system administrators and travellers, who need to work with lots of remote
computers in front of either large or tiny screens. Remmina supports
multiple network protocols, in an integrated and consistent user interface.
Currently RDP, VNC, SPICE, NX, XDMCP, SSH and EXEC are supported.

Remmina is released in separated source packages:
* "remmina", the main GTK application
* "remmina-plugins", a set of plugins

Remmina is copylefted libre software, licensed GNU GPLv2+.

## Installation

### Binary distributions
Usually Remmina is included in your Linux|GNU distribution, or in an external repository.
This is not the place to ask for distribution packages or precompiled binaries.

### Debian GNU/Linux ###

Remmina is not available in the default Debian 9 (Stretch) repositories. It can be installed from the backports repository.

The [Debian Backports](https://backports.debian.org/Instructions/) repository must be enabled to install it.

To install Remmina from Debian Backports, copy and paste the following lines into a terminal window:

```
echo 'deb https://ftp.debian.org/debian stretch-backports main' | sudo tee --append /etc/apt/sources.list.d/stretch-backports.list >> /dev/null
sudo apt update
sudo apt install -t stretch-backports remmina remmina-plugin-rdp remmina-plugin-secret
```

### Ubuntu

#### Using Snap Package (also for other [supported distros](https://snapcraft.io/docs/core/install))

You can install the last release from the Ubuntu Software center. Look for `remmina`, otherwise install it from a terminal with:

```sh
sudo snap install remmina
```

If you want to install latest Git revision of Remmina, you can get it from the `edge` channel:

```sh
sudo snap install remmina  --edge
```

Or update the current installed version with the selected channel:

```sh
sudo snap refresh remmina --channel=edge # use --channel=stable otherwise
```

For advanced features such as
* `mount-control` (to manage mount positions),
* `avahi-observer` (to automatically look for local servers to connect to),
* `cups-control` (to manage printing),
* `password-manager-service` (to use gnome-keyring)

enter this in a terminal:

```sh
sudo snap connect remmina:avahi-observe :avahi-observe # servers discovery
sudo snap connect remmina:cups-control :cups-control # printing
sudo snap connect remmina:mount-observe :mount-observe # mount management
sudo snap connect remmina:password-manager-service :password-manager-service # gnome-keyring
```

Snap packages are automatically updated, and include both the latest `FreeRDP` Git, and the latest `libssh 0.7` release (for better security).

#### From PPA

[Ubuntu ppa:remmina-ppa-team/remmina-next](https://launchpad.net/~remmina-ppa-team/+archive/ubuntu/remmina-next)

To install, copy and paste the following three lines in a terminal:
```sh
sudo apt-add-repository ppa:remmina-ppa-team/remmina-next
sudo apt-get update
sudo apt-get install remmina remmina-plugin-rdp libfreerdp-plugins-standard
```
By default, the RDP, SSH and SFTP plugins are installed. You can view a list of available plugins with `apt-cache search remmina-plugin`

If you want to connect to more securely configured SSH servers on Ubuntu 16.04 and below, you have to upgrade libssh to 0.7.X.
This can be achieved by adding the following PPA by David Kedves, which contains libssh 0.7.X and then upgrading your packages:
```sh
sudo add-apt-repository ppa:kedazo/libssh-0.7.x
sudo apt-get update
```

### Fedora and Red Hat

As of March 2018, Remmina is available on Fedora testing and stable, and still a (not updated) Copr provided by [Hubbitus](https://github.com/Hubbitus) (Pavel Alexeev),
to install just put this in a terminal, as root:

```sh
dnf copr enable hubbitus/remmina-next
dnf upgrade --refresh 'remmina*' 'freerdp*'
```

~~On Red Hat, you can enable the EPEL repository:~~

Remmina [is now in EPEL](https://rpms.remirepo.net/rpmphp/zoom.php?rpm=remmina).

```sh
wget https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
rpm -ivh epel-release-latest-7.noarch.rpm
```

### Arch Linux based

Install [remmina-git](https://aur.archlinux.org/packages/remmina-git) from [AUR](https://aur.archlinux.org/).

### openSUSE

Remmina is in the official repositories for all openSUSE distributions.
In case the version in the released stable branch of openSUSE is too old, you can install the latest one from the [devel project](https://build.opensuse.org/package/show/X11%3ARemoteDesktop/remmina) via:

```
zypper ar -f obs://X11:RemoteDesktop/remmina remmina
zypper ref
zypper in remmina
```

### For users with a distro that supports [Flatpak](https://flathub.org/), including Ubuntu ###

## Development snapshot

Download [this](https://gitlab.com/Remmina/Remmina/-/jobs/artifacts/master/raw/flatpak/remmina-dev.flatpak\?job\=flatpak:test) Flatpak
and install it as per:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user --bundle remmina-dev.flatpak
flatpak run org.remmina.Remmina
```

## Last stable official build on [Flathub](https://flathub.org/apps/details/org.remmina.Remmina)

Run the following commands:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.remmina.Remmina
flatpak run org.remmina.Remmina
```
If you use an SSH agent (https://github.com/flatpak/flatpak/issues/1438)

```sh
flatpak run --filesystem=$SSH_AUTH_SOCK --env=SSH_AUTH_SOCK=$SSH_AUTH_SOCK org.remmina.Remmina
```

Just be aware that Flatpak stores data for installed applications (the XDG config/data folders) under ```$HOME/.var```
For instance, if you previously have installed Remmina with another package manager,
you will have to transfer what was under ```$HOME/.config/remmina``` and ```$HOME/.local/share/remmina``` under,
respectively ```~/.var/app/org.remmina.Remmina/config/remmina``` and ```~/.var/app/org.remmina.Remmina/data/remmina```

### External not supported plugins

There are also some external, unsupported plugins, provided by [Muflone](https://github.com/muflone):

* [remmina-plugin-folder](https://aur.archlinux.org/packages/remmina-plugin-folder/) A protocol plugin for Remmina to open a folder.
* [remmina-plugin-open](https://aur.archlinux.org/packages/remmina-plugin-open/) A protocol plugin for Remmina to open a document with its associated application.
* [remmina-plugin-rdesktop](https://aur.archlinux.org/packages/remmina-plugin-rdesktop/) A protocol plugin for Remmina to open a RDP connection with rdesktop.
* [remmina-plugin-teamviewer](https://aur.archlinux.org/packages/remmina-plugin-teamviewer/) A protocol plugin for Remmina to launch a TeamViewer connection.
* [remmina-plugin-ultravnc](https://aur.archlinux.org/packages/remmina-plugin-ultravnc/) A protocol plugin for Remmina to connect via VNC using the UltraVNC viewer.
* [remmina-plugin-url](https://aur.archlinux.org/packages/remmina-plugin-url/) A protocol plugin for Remmina to open an URL in an external browser.
* [remmina-plugin-webkit](https://aur.archlinux.org/packages/remmina-plugin-webkit/) A protocol plugin for Remmina to launch a GTK Webkit browser.

### From the source code

Follow the guides available on the wiki:
* [Wiki and compilation instructions](https://gitlab.com/Remmina/Remmina/wikis/home)

## Usage

Just select Remmina from your application menu, or execute Remmina from the command-line:

Remmina also supports the following options:

```sh
  -a, --about                 Show the "About" dialog
  -c, --connect=FILE          Connect to a .remmina file
  -e, --edit=FILE             Edit a .remmina file
  -n, --new                   Create a new connection profile
  -p, --pref=PAGENR           Show preferences dialog page
  -x, --plugin=PLUGIN         Execute the plugin
  -q, --quit                  Quit the application
  -s, --server=SERVER         Use default server name
  -t, --protocol=PROTOCOL     Use default protocol
  -i, --icon                  Start in tray
  -v, --version               Show the application version
  -V, --full-version          Show the application and plugin version
  --display=DISPLAY           X display to use
```

## Configuration

You can configure everything from the graphical interface, or editing by hand the files under $HOME/.remmina or $HOME/.config/remmina

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for a better overview.

If you want to contribute code:

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create a new merge request

If you want to contribute in other ways, get in contact on IRC (#remmina on freenode), or send an e-mail using the form provided on the website.

### Donations

If you want to contribute to Remmina by sending money, you are more than welcome.
More info about this on [Remmina website donation page](https://remmina.org/wp/donations/).

See the [THANKS.md](THANKS.md) file for a list of all supporters.

## Authors

Remmina is maintained by:

 * [Antenore Gatta](https://gitlab.com/antenore)
 * [Giovanni Panozzo](https://gitlab.com/giox069)
 * [Dario Cavedon](https://gitlab.com/ic3d)

See the [AUTHORS](AUTHORS) for an exhaustive list.
If you are not listed and have contributed, feel free to update it.

## Resources

 * [Website](https://www.remmina.org/)
 * IRC room on freenode.net, in the #remmina channel, you can also use a [web client](https://kiwiirc.com/client/irc.freenode.net/?nick=remminer|?#remmina/).

## License

Licensed [GPLv2+](https://gitlab.com/Remmina/Remmina/blob/master/COPYING).