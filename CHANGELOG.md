# Change Log

## [1.2.0-rcgit.8](https://github.com/FreeRDP/Remmina/tree/1.2.0-rcgit.8) (2016-01-04)

[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0-rcgit.7...1.2.0-rcgit.8)

**Closed issues:**

- tot remmina crashes when marking text in the client iff clipboard sync is disabled [\#695](https://github.com/FreeRDP/Remmina/issues/695)

**Merged pull requests:**

- Fix a bunch of memleaks [\#712](https://github.com/FreeRDP/Remmina/pull/712) ([jviksell](https://github.com/jviksell))

## [1.2.0-rcgit.7](https://github.com/FreeRDP/Remmina/tree/1.2.0-rcgit.7) (2015-12-17)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0-rcgit.6...1.2.0-rcgit.7)

**Implemented enhancements:**

- Icons not shown at the correct size in the main window's listview/treeview [\#701](https://github.com/FreeRDP/Remmina/issues/701)
- Reduce main window icons size fixes \#701 [\#702](https://github.com/FreeRDP/Remmina/pull/702) ([antenore](https://github.com/antenore))

**Closed issues:**

- VNC window immediately closes after connection attempt [\#699](https://github.com/FreeRDP/Remmina/issues/699)

**Merged pull requests:**

- Moved to markdown and updated text. [\#705](https://github.com/FreeRDP/Remmina/pull/705) ([antenore](https://github.com/antenore))
- Vnc exit with 8bpp issue699 [\#704](https://github.com/FreeRDP/Remmina/pull/704) ([antenore](https://github.com/antenore))
- Temporay fixes \#699 - Set default color depth to 15 \(high colors\) [\#703](https://github.com/FreeRDP/Remmina/pull/703) ([antenore](https://github.com/antenore))

## [1.2.0-rcgit.6](https://github.com/FreeRDP/Remmina/tree/1.2.0-rcgit.6) (2015-12-10)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0-rcgit.5...1.2.0-rcgit.6)

**Implemented enhancements:**

- Migrate from libgnome-keyring to libsecret [\#652](https://github.com/FreeRDP/Remmina/issues/652)
- Move ~/.remmina to a proper place \(follow XDG standards\) [\#197](https://github.com/FreeRDP/Remmina/issues/197)
- X2goplugin refactoring closes \#603 [\#655](https://github.com/FreeRDP/Remmina/pull/655) ([antenore](https://github.com/antenore))

**Closed issues:**

- Remmina windows open behind other desktop windows [\#691](https://github.com/FreeRDP/Remmina/issues/691)
- Indicator menu gone? [\#688](https://github.com/FreeRDP/Remmina/issues/688)
- fatal error: gst/gstconfig.h: No such file or directory [\#678](https://github.com/FreeRDP/Remmina/issues/678)
- remmina in gnome wayland \_XInternAtom\(\): remmina killed by SIGSEGV [\#677](https://github.com/FreeRDP/Remmina/issues/677)
- remmina\_rdp\_cliprdr\_monitor\_ready\(\): remmina killed by SIGSEGV [\#676](https://github.com/FreeRDP/Remmina/issues/676)
- remmina\_rdp\_event\_release\_key\(\): remmina killed by SIGSEGV [\#675](https://github.com/FreeRDP/Remmina/issues/675)
- remmina: remmina\_rdp\_event\_update\_scale\_factor\(\): remmina killed by SIGSEGV [\#674](https://github.com/FreeRDP/Remmina/issues/674)
- remmina: ringbuffer\_destroy\(\): remmina killed by SIGSEGV [\#673](https://github.com/FreeRDP/Remmina/issues/673)
- remmina: remmina\_connection\_holder\_toolbar\_preferences\_popdown\(\): remmina killed by SIGSEGV [\#672](https://github.com/FreeRDP/Remmina/issues/672)
- remmina: \_g\_log\_abort\(\): remmina killed by SIGTRAP [\#671](https://github.com/FreeRDP/Remmina/issues/671)
- Concurrent  remote desktop sessions on Win XP Pro [\#670](https://github.com/FreeRDP/Remmina/issues/670)
- Optimize screen space usage [\#661](https://github.com/FreeRDP/Remmina/issues/661)
- clipboard not synchonizing between RDP sessions and host [\#556](https://github.com/FreeRDP/Remmina/issues/556)
- FTBS 1.0.0 :  ld - undefined reference to symbol 'g\_module\_symbol' -- links.txt missing necessary libraries [\#182](https://github.com/FreeRDP/Remmina/issues/182)

**Merged pull requests:**

- Avoiding conditional directives that break statements [\#698](https://github.com/FreeRDP/Remmina/pull/698) ([RomeroMalaquias](https://github.com/RomeroMalaquias))
- Fix memory leaks in RDP plugin, fix window width in remmina\_connection\_window [\#690](https://github.com/FreeRDP/Remmina/pull/690) ([giox069](https://github.com/giox069))
- Disable wayland backend  [\#680](https://github.com/FreeRDP/Remmina/pull/680) ([antenore](https://github.com/antenore))
- Migrate from libgnome-keyring to libsecret \#652 - Inital import [\#653](https://github.com/FreeRDP/Remmina/pull/653) ([antenore](https://github.com/antenore))

## [1.2.0-rcgit.5](https://github.com/FreeRDP/Remmina/tree/1.2.0-rcgit.5) (2015-11-02)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.4...1.2.0-rcgit.5)

**Implemented enhancements:**

- Feature request: Provide a way to disable parsing of .ssh/config [\#648](https://github.com/FreeRDP/Remmina/issues/648)
- Toolbar drag and drop, for issue \#661 [\#668](https://github.com/FreeRDP/Remmina/pull/668) ([giox069](https://github.com/giox069))

**Closed issues:**

- rdp plugin does not load [\#667](https://github.com/FreeRDP/Remmina/issues/667)
- Remmina crashed with core dump while sharing RDP folder [\#659](https://github.com/FreeRDP/Remmina/issues/659)
- High DPI display scaling [\#654](https://github.com/FreeRDP/Remmina/issues/654)
- RDP Plugin Issue on Raspberry PI \(ARMv7\) [\#651](https://github.com/FreeRDP/Remmina/issues/651)
- Moving to an embedded version of FreeRDP [\#599](https://github.com/FreeRDP/Remmina/issues/599)
- please add support multi-hop ssh tunnels / read .ssh/config [\#302](https://github.com/FreeRDP/Remmina/issues/302)
- ForwardAgent [\#267](https://github.com/FreeRDP/Remmina/issues/267)
- SSH Tunneling [\#96](https://github.com/FreeRDP/Remmina/issues/96)
- Feature Request: SSH Tunnel with no authentication [\#83](https://github.com/FreeRDP/Remmina/issues/83)

**Merged pull requests:**

- Makes parsing of ~/.ssh/config optional closes \#648 [\#650](https://github.com/FreeRDP/Remmina/pull/650) ([antenore](https://github.com/antenore))

## [v1.2.0-rcgit.4](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.4) (2015-09-23)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.3...v1.2.0-rcgit.4)

**Implemented enhancements:**

- Remmina crashes using the Mir GTK backend [\#554](https://github.com/FreeRDP/Remmina/issues/554)
- Prior connections commands are executed in background [\#553](https://github.com/FreeRDP/Remmina/issues/553)
- Quick Search Textbox is Only 8 Characters Wide [\#547](https://github.com/FreeRDP/Remmina/issues/547)
- \[feature request\] agent forwarding with ssh! [\#395](https://github.com/FreeRDP/Remmina/issues/395)
- How about support XDG dir structure [\#129](https://github.com/FreeRDP/Remmina/issues/129)
- SSH and VTE imporvements [\#642](https://github.com/FreeRDP/Remmina/pull/642) ([antenore](https://github.com/antenore))
- Gtk3 -  Deprecation fixes [\#637](https://github.com/FreeRDP/Remmina/pull/637) ([antenore](https://github.com/antenore))

**Fixed bugs:**

- Prior connections commands are executed in background [\#553](https://github.com/FreeRDP/Remmina/issues/553)
- Remmina fullscreen is wrongly placed - not real fullscreen [\#525](https://github.com/FreeRDP/Remmina/issues/525)
- SSH not passing locale info [\#331](https://github.com/FreeRDP/Remmina/issues/331)

**Closed issues:**

- Remmina VS freerdp on 1.2.0 version [\#639](https://github.com/FreeRDP/Remmina/issues/639)
- Remmina does not connect to remote host by FQDN [\#632](https://github.com/FreeRDP/Remmina/issues/632)
- freeze when kvm setup finish [\#630](https://github.com/FreeRDP/Remmina/issues/630)
- Dependency problem on Ubuntu PPA remmina-next [\#629](https://github.com/FreeRDP/Remmina/issues/629)
- Remmina preferences are not being saved  [\#627](https://github.com/FreeRDP/Remmina/issues/627)
- Redirection of audio to local fails [\#626](https://github.com/FreeRDP/Remmina/issues/626)
- RDP component not installed [\#620](https://github.com/FreeRDP/Remmina/issues/620)
- Wrong path in /usr/share/applications/remmina.desktop since version 1.1.1-1+766+next+201507170316~ubuntu14.04.1 [\#616](https://github.com/FreeRDP/Remmina/issues/616)
- Reverse layout switching doesn't work [\#605](https://github.com/FreeRDP/Remmina/issues/605)
- Cursor disappear and doesn't refresh in RDP [\#598](https://github.com/FreeRDP/Remmina/issues/598)
- support VNC on UNIX sockets [\#596](https://github.com/FreeRDP/Remmina/issues/596)
- Problem with RDP graphics [\#591](https://github.com/FreeRDP/Remmina/issues/591)
- Crash when using TortoiseGit over a remote Windows through Remmina  [\#582](https://github.com/FreeRDP/Remmina/issues/582)
- Remmina 1.1.2 does not compile against FreeRDP 1.0.2 [\#579](https://github.com/FreeRDP/Remmina/issues/579)
- Sorting of hosts in notification pop-up area should not be case sensitive [\#574](https://github.com/FreeRDP/Remmina/issues/574)
- How to add custom keyboard layouts to RDP? [\#573](https://github.com/FreeRDP/Remmina/issues/573)
- provide Gnome 3 indicator [\#570](https://github.com/FreeRDP/Remmina/issues/570)
- New grouping mode: All-In-One [\#569](https://github.com/FreeRDP/Remmina/issues/569)
- Remmina nx won't connect with Ubuntu 15.04 [\#567](https://github.com/FreeRDP/Remmina/issues/567)
- Missing connection icons in tray menu [\#563](https://github.com/FreeRDP/Remmina/issues/563)
- undefined symbol: freerdp\_channels\_pop\_event in remmina-plugin-rdp.so [\#558](https://github.com/FreeRDP/Remmina/issues/558)
- Can't Sustain "Scale and Fill client window" Preference [\#557](https://github.com/FreeRDP/Remmina/issues/557)
- webbrowser support [\#551](https://github.com/FreeRDP/Remmina/issues/551)
- VNC connection crashes Remmina [\#546](https://github.com/FreeRDP/Remmina/issues/546)
- Remmina doesn't copy cells from libreoffice to WS2003R2 [\#541](https://github.com/FreeRDP/Remmina/issues/541)
- =net-misc/remmina-1.1.2 ssh quick connect always ask password, even if server don't support it [\#539](https://github.com/FreeRDP/Remmina/issues/539)
- Bring back Scaling WITHOUT respecting the remote destkop aspect ratio [\#537](https://github.com/FreeRDP/Remmina/issues/537)
- Remmina consistently crashes when I log out of an RDP Gateway session. [\#530](https://github.com/FreeRDP/Remmina/issues/530)
- remmina: gtk\_drag\_source\_info\_destroy\(\): remmina killed by SIGABRT [\#529](https://github.com/FreeRDP/Remmina/issues/529)
- Add ability to run an arbitrary command prior to connection. [\#520](https://github.com/FreeRDP/Remmina/issues/520)
- ssh key selection overwritten on edit [\#271](https://github.com/FreeRDP/Remmina/issues/271)
- VNC over SSH - Support for multiple keys for SSH authentication [\#256](https://github.com/FreeRDP/Remmina/issues/256)
- RDP: No caret on explorer address bar and white squares instead of small icons [\#251](https://github.com/FreeRDP/Remmina/issues/251)
- It does not move the text to scroll [\#208](https://github.com/FreeRDP/Remmina/issues/208)
- Crash in BitBlt\_SRCAND\_32bpp, BitBlt\_32bpp [\#186](https://github.com/FreeRDP/Remmina/issues/186)
- Can't attach to Windows console [\#177](https://github.com/FreeRDP/Remmina/issues/177)
- RDP disconnect from within Windows doesn't close session for 15-20 seconds [\#167](https://github.com/FreeRDP/Remmina/issues/167)
- No shared folder when connecting to console [\#161](https://github.com/FreeRDP/Remmina/issues/161)
- drive-client not copied/loaded properly [\#147](https://github.com/FreeRDP/Remmina/issues/147)
- Smartcard & sound redirection error [\#121](https://github.com/FreeRDP/Remmina/issues/121)
- Possibility to enter a remote command in SSH when I use it for VNC [\#94](https://github.com/FreeRDP/Remmina/issues/94)
- openpty\(3\) support for Remmina [\#73](https://github.com/FreeRDP/Remmina/issues/73)

**Merged pull requests:**

- libssh fixes - SSH\_OPTIONS\_LOG\_VERBOSITY [\#640](https://github.com/FreeRDP/Remmina/pull/640) ([antenore](https://github.com/antenore))
- Updated AUTHORS list [\#636](https://github.com/FreeRDP/Remmina/pull/636) ([antenore](https://github.com/antenore))
- Improve vnc rendering speed by using cairo surface directly [\#635](https://github.com/FreeRDP/Remmina/pull/635) ([mar-kolya](https://github.com/mar-kolya))
- CMake rules adjustments when compiling under linux [\#634](https://github.com/FreeRDP/Remmina/pull/634) ([fundawang](https://github.com/fundawang))
- Remove some legacy Xorg code, fixes \#554 [\#622](https://github.com/FreeRDP/Remmina/pull/622) ([giox069](https://github.com/giox069))
- fix regression introduced by 190ea2f98ab0443b8a05f70c79e2af037f9fca94 [\#617](https://github.com/FreeRDP/Remmina/pull/617) ([zfil](https://github.com/zfil))
- remmina.desktop: Use full path to execute remmina [\#612](https://github.com/FreeRDP/Remmina/pull/612) ([lanoxx](https://github.com/lanoxx))
- remmina.desktop: Use full path to execute remmina [\#607](https://github.com/FreeRDP/Remmina/pull/607) ([lanoxx](https://github.com/lanoxx))
- Filling out LoadBalanceInfo in the RDP settings struct [\#593](https://github.com/FreeRDP/Remmina/pull/593) ([qwertos](https://github.com/qwertos))
- Exception when precommad in NULL. Reference \#591 [\#592](https://github.com/FreeRDP/Remmina/pull/592) ([antenore](https://github.com/antenore))
- Xdg folders fixes \#129  \#197  [\#590](https://github.com/FreeRDP/Remmina/pull/590) ([antenore](https://github.com/antenore))
- coredump when precommand is not quoted. Resolve \#520 [\#589](https://github.com/FreeRDP/Remmina/pull/589) ([antenore](https://github.com/antenore))
- Support for ~/.ssh/config closes \#235 , libssh does not support most of the ssh options [\#588](https://github.com/FreeRDP/Remmina/pull/588) ([antenore](https://github.com/antenore))
- Revert "License update" [\#587](https://github.com/FreeRDP/Remmina/pull/587) ([antenore](https://github.com/antenore))
- Revert "License update" [\#586](https://github.com/FreeRDP/Remmina/pull/586) ([antenore](https://github.com/antenore))
- Remmina freerdp subtree + fixes for FreeBSD [\#585](https://github.com/FreeRDP/Remmina/pull/585) ([antenore](https://github.com/antenore))
- License update [\#581](https://github.com/FreeRDP/Remmina/pull/581) ([antenore](https://github.com/antenore))
- Fix GTK+2 build failure [\#565](https://github.com/FreeRDP/Remmina/pull/565) ([heptalium](https://github.com/heptalium))
- Prior Connection Command [\#562](https://github.com/FreeRDP/Remmina/pull/562) ([antenore](https://github.com/antenore))
- Update Uzbek translation [\#561](https://github.com/FreeRDP/Remmina/pull/561) ([ozbek](https://github.com/ozbek))
- Update for GTK+2 port [\#555](https://github.com/FreeRDP/Remmina/pull/555) ([repzilon](https://github.com/repzilon))

## [v1.2.0-rcgit.3](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.3) (2015-04-14)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.2...v1.2.0-rcgit.3)

**Implemented enhancements:**

- Show or hide the icons near the menu [\#504](https://github.com/FreeRDP/Remmina/issues/504)
- Show or hide the icons near the buttons [\#503](https://github.com/FreeRDP/Remmina/issues/503)
- Show or hide the icons near the buttons and the menus [\#505](https://github.com/FreeRDP/Remmina/pull/505) ([muflone](https://github.com/muflone))
- Fixes FreeRDP/Remmina\#473 - Customize button in the connection editor should  show the resolutions dialog [\#501](https://github.com/FreeRDP/Remmina/pull/501) ([antenore](https://github.com/antenore))
- The quick search doesn't list the folders [\#497](https://github.com/FreeRDP/Remmina/pull/497) ([muflone](https://github.com/muflone))
- Enable and disable buttons in the string list dialog [\#496](https://github.com/FreeRDP/Remmina/pull/496) ([muflone](https://github.com/muflone))
- Added Ctrl+F accelerator for quick search entry [\#494](https://github.com/FreeRDP/Remmina/pull/494) ([muflone](https://github.com/muflone))
- Removed the RemminaMain GType and used a static RemminaMain struct instead [\#461](https://github.com/FreeRDP/Remmina/pull/461) ([muflone](https://github.com/muflone))

**Fixed bugs:**

- Tab all connections [\#548](https://github.com/FreeRDP/Remmina/issues/548)
- Problem with shared  folder on Windows Server 2012 [\#523](https://github.com/FreeRDP/Remmina/issues/523)
- Can't share a folder in KDE, due to Gtk issue [\#518](https://github.com/FreeRDP/Remmina/issues/518)
- Connection to all servers stopped working with last update of remmina-next PPA [\#506](https://github.com/FreeRDP/Remmina/issues/506)
- Unsupported properties on GTK 3.10 [\#486](https://github.com/FreeRDP/Remmina/issues/486)
- Buttons in the string list dialog [\#474](https://github.com/FreeRDP/Remmina/issues/474)
- Customize button in the connection editor should show the resolutions dialog [\#473](https://github.com/FreeRDP/Remmina/issues/473)
- The search shouldn't show every group in the list [\#471](https://github.com/FreeRDP/Remmina/issues/471)
- Enable the use of the menu key [\#470](https://github.com/FreeRDP/Remmina/issues/470)
- Connections list is not refreshed upon update or copy \(duplicate an existing connection\) [\#460](https://github.com/FreeRDP/Remmina/issues/460)
- Auto-highlighted "Quick Connect" connection name within new connection dialog copies the words "Quick Connect" into PRIMARY selection [\#263](https://github.com/FreeRDP/Remmina/issues/263)
- Double-clicking a column header to sort list of saved connections actually attempts connection [\#250](https://github.com/FreeRDP/Remmina/issues/250)
- Fix small memory leaks and a NULL ptr dereference. [\#500](https://github.com/FreeRDP/Remmina/pull/500) ([KyleSanderson](https://github.com/KyleSanderson))
- Show the popup menu by pressing the menu key [\#495](https://github.com/FreeRDP/Remmina/pull/495) ([muflone](https://github.com/muflone))
- Use the correct label for the font section [\#466](https://github.com/FreeRDP/Remmina/pull/466) ([weberhofer](https://github.com/weberhofer))

**Closed issues:**

- Remmina main window can be opened multiple times [\#543](https://github.com/FreeRDP/Remmina/issues/543)
- =net-misc/remmina-1.1.2 Quick Connect should filter supported protocols similiarly with profile manager [\#540](https://github.com/FreeRDP/Remmina/issues/540)
- Remmina not showing entire desktop, Windows Remote Desktop Connection does. [\#534](https://github.com/FreeRDP/Remmina/issues/534)
- Copy and Paste between local and remote desktops failing in Ubuntu 15.04 [\#533](https://github.com/FreeRDP/Remmina/issues/533)
- Remmina does not report Host key Changed. [\#532](https://github.com/FreeRDP/Remmina/issues/532)
- compile with FreeRDP support failed on Funtoo Linux [\#526](https://github.com/FreeRDP/Remmina/issues/526)
- Exit VMWare Console via Remmina in 1.2.0 [\#522](https://github.com/FreeRDP/Remmina/issues/522)
- RDP Quality is not being stored [\#521](https://github.com/FreeRDP/Remmina/issues/521)
- Add the ability to place the floating toolbar on other sides of the screen [\#493](https://github.com/FreeRDP/Remmina/issues/493)
- Invisible toolbar in fullscreen mode grayed out [\#492](https://github.com/FreeRDP/Remmina/issues/492)
- Remarks to some setting's labels [\#489](https://github.com/FreeRDP/Remmina/issues/489)
- Open resolution list from connection editor [\#487](https://github.com/FreeRDP/Remmina/issues/487)
- Missing accelerators for search and quick connect entries [\#475](https://github.com/FreeRDP/Remmina/issues/475)
- Only one host key per domain name is allowed [\#465](https://github.com/FreeRDP/Remmina/issues/465)
- Latest changes causes compilation errors [\#464](https://github.com/FreeRDP/Remmina/issues/464)
- Host Name checking is erroneously case sensitive for RDP fingerprint [\#462](https://github.com/FreeRDP/Remmina/issues/462)
- Unable to connect "protocol security negotiation failure" [\#453](https://github.com/FreeRDP/Remmina/issues/453)
- Remmina crashes when using multiple RDP connections and closing one or more. [\#449](https://github.com/FreeRDP/Remmina/issues/449)
- Provide a way to send keys to the server connections [\#445](https://github.com/FreeRDP/Remmina/issues/445)
- Does \(or will\) Remmina support multi monitor RDP? [\#442](https://github.com/FreeRDP/Remmina/issues/442)
- xfreerdrp Crashes with segfault in find\_keyboard\_layout\_in\_xorg\_rules [\#441](https://github.com/FreeRDP/Remmina/issues/441)
- NX connection to freenx-client with custom key [\#436](https://github.com/FreeRDP/Remmina/issues/436)
- Unable to connect using \\server-name [\#435](https://github.com/FreeRDP/Remmina/issues/435)
- Regression: "Unknown authentication scheme from VNC server: 18" [\#433](https://github.com/FreeRDP/Remmina/issues/433)
- Copy file rdp [\#430](https://github.com/FreeRDP/Remmina/issues/430)
- Connection closes right away when trying to connect to Mac [\#427](https://github.com/FreeRDP/Remmina/issues/427)
- RDP clipboard and shared folder issue [\#406](https://github.com/FreeRDP/Remmina/issues/406)
- Crush RDP connect when copy file into clipboard on RDP server and clipboard sync ON [\#401](https://github.com/FreeRDP/Remmina/issues/401)
- Compilation failed on ubuntu 14.04 and 14.10 [\#381](https://github.com/FreeRDP/Remmina/issues/381)
- vertical text issue with excel [\#345](https://github.com/FreeRDP/Remmina/issues/345)
- Failure refresh image  [\#341](https://github.com/FreeRDP/Remmina/issues/341)
- Segfault in rf\_Pointer\_Free of rdp plugin [\#269](https://github.com/FreeRDP/Remmina/issues/269)
- Auto-fit stucks the window [\#257](https://github.com/FreeRDP/Remmina/issues/257)
- Remmina-plugins-rdp fails to compile correctly with the latest version of gcc & freerdp [\#244](https://github.com/FreeRDP/Remmina/issues/244)
- Remmina has stopped showing shared folders [\#243](https://github.com/FreeRDP/Remmina/issues/243)
- Lost toolbar in keyboard captured mode [\#242](https://github.com/FreeRDP/Remmina/issues/242)
- Better user credentials management [\#229](https://github.com/FreeRDP/Remmina/issues/229)
- Command Line Arguments [\#227](https://github.com/FreeRDP/Remmina/issues/227)
- Copy paste clipboard does not work [\#207](https://github.com/FreeRDP/Remmina/issues/207)
- remmina not compiling on cent os 6.4 x64 [\#201](https://github.com/FreeRDP/Remmina/issues/201)
- sometimes gives black screen [\#198](https://github.com/FreeRDP/Remmina/issues/198)
- remmina remote desktop -- erratic partial display issue [\#181](https://github.com/FreeRDP/Remmina/issues/181)
- remmina incorrectly handles saved terminal size for ssh-connection [\#169](https://github.com/FreeRDP/Remmina/issues/169)
- net-misc/remmina-1.0.0\_p20121004 fails rsa-key auth on X11Forwarding-enabled OpenSSH server [\#168](https://github.com/FreeRDP/Remmina/issues/168)
- Remmina will not load RDP plugins  [\#165](https://github.com/FreeRDP/Remmina/issues/165)
- cmake install prefix bug [\#160](https://github.com/FreeRDP/Remmina/issues/160)
- Graphical glitch with Total Commander and Remmina [\#157](https://github.com/FreeRDP/Remmina/issues/157)
- An Archer's tale - Unable to compile, compilation, followed by inability to path plugins. [\#152](https://github.com/FreeRDP/Remmina/issues/152)
- display off-center upon connection to RDP hosts [\#143](https://github.com/FreeRDP/Remmina/issues/143)
- RDP refresh/redraw problem  [\#138](https://github.com/FreeRDP/Remmina/issues/138)
- Blank/black window contents when opening a new connection [\#127](https://github.com/FreeRDP/Remmina/issues/127)
- patch for launching external tools [\#126](https://github.com/FreeRDP/Remmina/issues/126)
- New connection type: VNC Reverse Connection [\#108](https://github.com/FreeRDP/Remmina/issues/108)
- Cannot connect to shared OS X screen protected with password only. [\#104](https://github.com/FreeRDP/Remmina/issues/104)
- Remote screen has wrong offset when using hide-toolbar [\#103](https://github.com/FreeRDP/Remmina/issues/103)
- Remmina Won't Maintain Multiple RDP Sessions at the Same Time [\#99](https://github.com/FreeRDP/Remmina/issues/99)
- Screen capture software crashes remmina client [\#98](https://github.com/FreeRDP/Remmina/issues/98)
- SSH terminal color customization [\#91](https://github.com/FreeRDP/Remmina/issues/91)
- \[Enhancement\] Start in background [\#85](https://github.com/FreeRDP/Remmina/issues/85)
- SSH Freeze [\#84](https://github.com/FreeRDP/Remmina/issues/84)
- Initial screen wonky opening VNC client [\#69](https://github.com/FreeRDP/Remmina/issues/69)
- Issues with viewport fullscreen [\#44](https://github.com/FreeRDP/Remmina/issues/44)
- make sessions in the NX session dialog double-clickable [\#16](https://github.com/FreeRDP/Remmina/issues/16)

**Merged pull requests:**

- Added an option to execute commands just before to connect to a remote server closes  \#520 [\#552](https://github.com/FreeRDP/Remmina/pull/552) ([antenore](https://github.com/antenore))
- Uzbek Cyrillic: use proper naming convention for filename [\#549](https://github.com/FreeRDP/Remmina/pull/549) ([ozbek](https://github.com/ozbek))
- Uzbek Cyrillic: Add configure keystrokes and XDMCP feature text [\#538](https://github.com/FreeRDP/Remmina/pull/538) ([ozbek](https://github.com/ozbek))
- Remmina 1.2.0-rcgit.3: new floating toolbar for GTK\>=3.10 and many other fixes [\#536](https://github.com/FreeRDP/Remmina/pull/536) ([giox069](https://github.com/giox069))
- Fix for notebook tab drag and drop, fixes issues \#529 \#478 [\#531](https://github.com/FreeRDP/Remmina/pull/531) ([giox069](https://github.com/giox069))
- Fix race condition in VNC event queue [\#519](https://github.com/FreeRDP/Remmina/pull/519) ([mar-kolya](https://github.com/mar-kolya))
- Uzbek Cyrillic: apply latest additions [\#509](https://github.com/FreeRDP/Remmina/pull/509) ([ozbek](https://github.com/ozbek))
- Implemented custom keystrokes for plugins [\#508](https://github.com/FreeRDP/Remmina/pull/508) ([muflone](https://github.com/muflone))
- Add translations for Uzbek Cyrillic [\#502](https://github.com/FreeRDP/Remmina/pull/502) ([ozbek](https://github.com/ozbek))
- Update Spanish translation [\#490](https://github.com/FreeRDP/Remmina/pull/490) ([fitojb](https://github.com/fitojb))
- Fixes FreeRDP/Remmina\#460 [\#463](https://github.com/FreeRDP/Remmina/pull/463) ([antenore](https://github.com/antenore))
- Updated German translation [\#458](https://github.com/FreeRDP/Remmina/pull/458) ([weberhofer](https://github.com/weberhofer))
- Update es.po [\#455](https://github.com/FreeRDP/Remmina/pull/455) ([agdg](https://github.com/agdg))
- Update es.po [\#454](https://github.com/FreeRDP/Remmina/pull/454) ([agdg](https://github.com/agdg))
- Updated FSF address [\#451](https://github.com/FreeRDP/Remmina/pull/451) ([weberhofer](https://github.com/weberhofer))
- Rework of scaler code [\#447](https://github.com/FreeRDP/Remmina/pull/447) ([giox069](https://github.com/giox069))
- Added feature to send Ctrl+Alt+Del keys [\#446](https://github.com/FreeRDP/Remmina/pull/446) ([muflone](https://github.com/muflone))
- Moved the double click event and the enter buttons press in the row-activated signal handler [\#438](https://github.com/FreeRDP/Remmina/pull/438) ([muflone](https://github.com/muflone))
- Handle double click on the NX session rows to activate the default response [\#437](https://github.com/FreeRDP/Remmina/pull/437) ([muflone](https://github.com/muflone))
- SSH terminal color customization [\#432](https://github.com/FreeRDP/Remmina/pull/432) ([muflone](https://github.com/muflone))
- remmina: Actually install its headers. [\#428](https://github.com/FreeRDP/Remmina/pull/428) ([rakuco](https://github.com/rakuco))

## [v1.2.0-rcgit.2](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.2) (2014-12-30)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.1.2...v1.2.0-rcgit.2)

**Fixed bugs:**

- Position of popup menu is wrong [\#423](https://github.com/FreeRDP/Remmina/issues/423)
- Remmina crashes exporting a remote desktop file [\#412](https://github.com/FreeRDP/Remmina/issues/412)
- \[BUG\] RDP Preferences don't saving [\#391](https://github.com/FreeRDP/Remmina/issues/391)
- Command line help not showing all the options [\#327](https://github.com/FreeRDP/Remmina/issues/327)

**Closed issues:**

- Remmina crashes after Ctrl+X in a RDP session [\#422](https://github.com/FreeRDP/Remmina/issues/422)
- Remmina Crashes During RDP Session when Context Menu Item Clicked [\#418](https://github.com/FreeRDP/Remmina/issues/418)
- Host key change not indicated in Remmina GUI [\#407](https://github.com/FreeRDP/Remmina/issues/407)
- Copy/paste inop Windows to Mint 17 [\#404](https://github.com/FreeRDP/Remmina/issues/404)
- remmina/src/remmina\_ssh\_plugin.h should also check for HAVE\_LIBVTE [\#394](https://github.com/FreeRDP/Remmina/issues/394)
- I have to click on "Resize the window to fit remote resolution" to see my RDP session. [\#387](https://github.com/FreeRDP/Remmina/issues/387)
- Please create an AppData file for Remmina [\#272](https://github.com/FreeRDP/Remmina/issues/272)
- "Protocol plugin RDP is not installed."/The Application Not Recognizing the RDP Plugin  [\#249](https://github.com/FreeRDP/Remmina/issues/249)
- "Overwrite all" button [\#238](https://github.com/FreeRDP/Remmina/issues/238)

**Merged pull requests:**

- .po files cleanup and updates [\#425](https://github.com/FreeRDP/Remmina/pull/425) ([giox069](https://github.com/giox069))
- Fixes the position of popup menu [\#424](https://github.com/FreeRDP/Remmina/pull/424) ([muflone](https://github.com/muflone))
- Add a "Overwrite all" button [\#420](https://github.com/FreeRDP/Remmina/pull/420) ([muflone](https://github.com/muflone))
- New AppData specification file [\#419](https://github.com/FreeRDP/Remmina/pull/419) ([muflone](https://github.com/muflone))
- Expose the arguments in the help text and parse --help and -h arguments locally [\#417](https://github.com/FreeRDP/Remmina/pull/417) ([muflone](https://github.com/muflone))
- Clipboard: improved handling of time consuming clipboard transfer [\#416](https://github.com/FreeRDP/Remmina/pull/416) ([giox069](https://github.com/giox069))
- Clipboard fixes [\#415](https://github.com/FreeRDP/Remmina/pull/415) ([giox069](https://github.com/giox069))
- Next [\#414](https://github.com/FreeRDP/Remmina/pull/414) ([giox069](https://github.com/giox069))
- GTK3 fixes [\#409](https://github.com/FreeRDP/Remmina/pull/409) ([giox069](https://github.com/giox069))
-  GTK3 migration of remmina\_ftp\_client, fixes \#365 [\#408](https://github.com/FreeRDP/Remmina/pull/408) ([giox069](https://github.com/giox069))
- Update Remmina GTK+2 branch to 1.1.2 [\#405](https://github.com/FreeRDP/Remmina/pull/405) ([repzilon](https://github.com/repzilon))

## [v1.1.2](https://github.com/FreeRDP/Remmina/tree/v1.1.2) (2014-12-08)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.1.1-gtk2...v1.1.2)

**Fixed bugs:**

- NX plugin broken [\#369](https://github.com/FreeRDP/Remmina/issues/369)
- XDMCP plugin broken [\#366](https://github.com/FreeRDP/Remmina/issues/366)

**Closed issues:**

- resize remote to fit window option [\#398](https://github.com/FreeRDP/Remmina/issues/398)
- Rename README to README.md [\#304](https://github.com/FreeRDP/Remmina/issues/304)

**Merged pull requests:**

- Fix RDP race condifion, fixes \#394 \(missing HAVE\_LIBVTE\), removed unused function [\#399](https://github.com/FreeRDP/Remmina/pull/399) ([giox069](https://github.com/giox069))
- Merge pull request \#396 from FreeRDP/issue366 [\#397](https://github.com/FreeRDP/Remmina/pull/397) ([muflone](https://github.com/muflone))
- Issue \#366 [\#396](https://github.com/FreeRDP/Remmina/pull/396) ([muflone](https://github.com/muflone))

## [v1.1.1-gtk2](https://github.com/FreeRDP/Remmina/tree/v1.1.1-gtk2) (2014-12-07)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.1.1...v1.1.1-gtk2)

**Closed issues:**

- Password isn't saved [\#388](https://github.com/FreeRDP/Remmina/issues/388)
- undefined symbol: freerdp\_event\_new [\#379](https://github.com/FreeRDP/Remmina/issues/379)
- Compile failed [\#378](https://github.com/FreeRDP/Remmina/issues/378)
- Copy/Paste not working in 0.9.99.1 on Ubuntu 14.10 [\#377](https://github.com/FreeRDP/Remmina/issues/377)
- Crash when not accepting certificate for RDP server [\#375](https://github.com/FreeRDP/Remmina/issues/375)
- Can't get RDP to work in Remmina recently  [\#374](https://github.com/FreeRDP/Remmina/issues/374)
- One IP, more RDP ports [\#373](https://github.com/FreeRDP/Remmina/issues/373)
- about box reports 1.1.0 rather than 1.1.1 [\#370](https://github.com/FreeRDP/Remmina/issues/370)
- Complete GTK3 migration [\#365](https://github.com/FreeRDP/Remmina/issues/365)
- Build failure with remmina-rdp\_cliprdr\_\* [\#363](https://github.com/FreeRDP/Remmina/issues/363)
- Key with code 95 si incorrectly mapped  [\#361](https://github.com/FreeRDP/Remmina/issues/361)
- SSH connection not working: "ssh\_userauth\_password: Wrong state during pending SSH call" [\#305](https://github.com/FreeRDP/Remmina/issues/305)

**Merged pull requests:**

- Remmina 1.1.2 [\#402](https://github.com/FreeRDP/Remmina/pull/402) ([ic3d](https://github.com/ic3d))
- Varoius fixes [\#393](https://github.com/FreeRDP/Remmina/pull/393) ([giox069](https://github.com/giox069))
- install external tools to datadir, not user's home dir [\#385](https://github.com/FreeRDP/Remmina/pull/385) ([eworm-de](https://github.com/eworm-de))
- Fix rdp\_cliprd for freerdp 1.2. Fixes \#378 \#379 [\#384](https://github.com/FreeRDP/Remmina/pull/384) ([giox069](https://github.com/giox069))
- Update to work with vte 2.91 as well as 2.90 [\#372](https://github.com/FreeRDP/Remmina/pull/372) ([iainlane](https://github.com/iainlane))
- Update to work with vte 2.91 as well as 2.90 [\#371](https://github.com/FreeRDP/Remmina/pull/371) ([iainlane](https://github.com/iainlane))
- Added a Show quick connect menu item to show/hide the fast connection box [\#368](https://github.com/FreeRDP/Remmina/pull/368) ([muflone](https://github.com/muflone))
- Fixed GTK3 issue [\#367](https://github.com/FreeRDP/Remmina/pull/367) ([weberhofer](https://github.com/weberhofer))
- Rename relevant CB\_FORMAT to CF [\#364](https://github.com/FreeRDP/Remmina/pull/364) ([giox069](https://github.com/giox069))
- Add printer and smartcard redirection, fix freerdp connection closing [\#359](https://github.com/FreeRDP/Remmina/pull/359) ([giox069](https://github.com/giox069))
- Add printer and smartcard redirection, fix freerdp connection closing [\#358](https://github.com/FreeRDP/Remmina/pull/358) ([giox069](https://github.com/giox069))
- Screenshots page with real screenshots! [\#355](https://github.com/FreeRDP/Remmina/pull/355) ([ic3d](https://github.com/ic3d))
- Good News!  [\#354](https://github.com/FreeRDP/Remmina/pull/354) ([ic3d](https://github.com/ic3d))

## [v1.1.1](https://github.com/FreeRDP/Remmina/tree/v1.1.1) (2014-10-10)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.1...v1.1.1)

**Fixed bugs:**

- Share local printer doesn't work [\#324](https://github.com/FreeRDP/Remmina/issues/324)
- VNC plugin settings - scaler too small [\#316](https://github.com/FreeRDP/Remmina/issues/316)
- Protocol settings compact flag don't seem to be honored in the gtk3 branch [\#314](https://github.com/FreeRDP/Remmina/issues/314)
- gtk\_widget\_set\_opacity not supported by GTK 3.4.2 \(for Debian 7.0\) [\#299](https://github.com/FreeRDP/Remmina/issues/299)
- Clipboard Sync with RDP [\#280](https://github.com/FreeRDP/Remmina/issues/280)
- Terminal windows disappearing [\#274](https://github.com/FreeRDP/Remmina/issues/274)
- Remmina crash when trying to connect to remote VNC [\#252](https://github.com/FreeRDP/Remmina/issues/252)
- Fix segfault reported by issue \#1 \#280 \#131 \#135 \#199 \#270 [\#282](https://github.com/FreeRDP/Remmina/pull/282) ([antenore](https://github.com/antenore))

**Closed issues:**

- Stucked shift key [\#90](https://github.com/FreeRDP/Remmina/issues/90)
- New branch 'next' [\#313](https://github.com/FreeRDP/Remmina/issues/313)
- Laggy graphics with remmina [\#308](https://github.com/FreeRDP/Remmina/issues/308)
- does sound work for remmina RDP? [\#296](https://github.com/FreeRDP/Remmina/issues/296)
- Remmina install /usr/include/remmina/ empty [\#292](https://github.com/FreeRDP/Remmina/issues/292)
- Remmina freeze after system boots from suspend [\#284](https://github.com/FreeRDP/Remmina/issues/284)
- Sound connection not possible via RDP [\#281](https://github.com/FreeRDP/Remmina/issues/281)
- Blank window on connect - found whats causign it [\#273](https://github.com/FreeRDP/Remmina/issues/273)
- Shared folder on RDP does not work [\#270](https://github.com/FreeRDP/Remmina/issues/270)
- RDP connection cannot be established [\#248](https://github.com/FreeRDP/Remmina/issues/248)

**Merged pull requests:**

- Fix debian bug 764142 [\#353](https://github.com/FreeRDP/Remmina/pull/353) ([giox069](https://github.com/giox069))
- Internationalization fixes [\#320](https://github.com/FreeRDP/Remmina/pull/320) ([giox069](https://github.com/giox069))
- Merge antenore:master with FreeRDP:gtk3 - GTK3 migration - File editor [\#311](https://github.com/FreeRDP/Remmina/pull/311) ([antenore](https://github.com/antenore))
- Help GNOME SHELL to not hide the floating toolbar [\#309](https://github.com/FreeRDP/Remmina/pull/309) ([giox069](https://github.com/giox069))
- Enabled sound \(fixes \#296 \#281\) and improved disconnection detection. [\#297](https://github.com/FreeRDP/Remmina/pull/297) ([giox069](https://github.com/giox069))
- Corrections to keyboard grab/ungrab. Fixes \#245 [\#295](https://github.com/FreeRDP/Remmina/pull/295) ([giox069](https://github.com/giox069))
- Fixes for clipboard issues [\#294](https://github.com/FreeRDP/Remmina/pull/294) ([giox069](https://github.com/giox069))
- Fix GTK2 compatibility [\#293](https://github.com/FreeRDP/Remmina/pull/293) ([amon-sha](https://github.com/amon-sha))
- Indentation fix [\#290](https://github.com/FreeRDP/Remmina/pull/290) ([giox069](https://github.com/giox069))
- Fix \#288 \#143 and deprecated gtk\_widget\_reparent\(\) [\#289](https://github.com/FreeRDP/Remmina/pull/289) ([giox069](https://github.com/giox069))
- Issue \#286 - License OpenSSL linking exception [\#287](https://github.com/FreeRDP/Remmina/pull/287) ([antenore](https://github.com/antenore))
- GTK+ 2 compatibility [\#276](https://github.com/FreeRDP/Remmina/pull/276) ([repzilon](https://github.com/repzilon))
- Fix issue with invisible toolbar in fullscreen. [\#275](https://github.com/FreeRDP/Remmina/pull/275) ([jerrido](https://github.com/jerrido))
- Fix for issue \#251 [\#253](https://github.com/FreeRDP/Remmina/pull/253) ([giox069](https://github.com/giox069))

## [v1.2.0-rcgit.1](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.1) (2014-10-08)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.1.0...v1.2.0-rcgit.1)

**Closed issues:**

- undefined symbol: freerdp\_channels\_global\_init in remmina-plugin-rdp.so [\#278](https://github.com/FreeRDP/Remmina/issues/278)

**Merged pull requests:**

- Fix issue \#90 and website URL [\#352](https://github.com/FreeRDP/Remmina/pull/352) ([giox069](https://github.com/giox069))
- Fix issue \#90 and various updates [\#351](https://github.com/FreeRDP/Remmina/pull/351) ([giox069](https://github.com/giox069))
- Updated menu links [\#350](https://github.com/FreeRDP/Remmina/pull/350) ([ic3d](https://github.com/ic3d))
- Update links [\#349](https://github.com/FreeRDP/Remmina/pull/349) ([ic3d](https://github.com/ic3d))
- Changed links [\#348](https://github.com/FreeRDP/Remmina/pull/348) ([ic3d](https://github.com/ic3d))
- Remove call to freerdp\_get\_last\_error [\#346](https://github.com/FreeRDP/Remmina/pull/346) ([dktrkranz](https://github.com/dktrkranz))

## [v1.1.0](https://github.com/FreeRDP/Remmina/tree/v1.1.0) (2014-10-03)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.0.0...v1.1.0)

**Fixed bugs:**

- Black or white window when RDP connects at 32bpp [\#329](https://github.com/FreeRDP/Remmina/issues/329)
- Remmina RDP does not disconnect when closing the client window or tab [\#288](https://github.com/FreeRDP/Remmina/issues/288)
- Super/windows keypresses always present [\#7](https://github.com/FreeRDP/Remmina/issues/7)

**Closed issues:**

- Protocol plug-in RDP is not installed. [\#337](https://github.com/FreeRDP/Remmina/issues/337)
- FreeRDP / Remmina - is this a fork? [\#325](https://github.com/FreeRDP/Remmina/issues/325)
- RDP configuration tab garbled in GTK3 branch. [\#319](https://github.com/FreeRDP/Remmina/issues/319)
- Remmina hangs and doesn't let to switch to another program [\#310](https://github.com/FreeRDP/Remmina/issues/310)
- VNC plugin not available in Remmnia [\#307](https://github.com/FreeRDP/Remmina/issues/307)
- Identityfile not loaded properly [\#306](https://github.com/FreeRDP/Remmina/issues/306)
- Second Monitor in Portrait mode breaks dropdown. [\#298](https://github.com/FreeRDP/Remmina/issues/298)
- OpenSSL linking exception? [\#286](https://github.com/FreeRDP/Remmina/issues/286)
- Please tag 1.2.0-beta1 [\#285](https://github.com/FreeRDP/Remmina/issues/285)
- Remmina Remote Desktop Client doesn't save Quality option changes [\#283](https://github.com/FreeRDP/Remmina/issues/283)
- Cannot compile on Fedora 20 [\#277](https://github.com/FreeRDP/Remmina/issues/277)
- Cannot install Remmina [\#266](https://github.com/FreeRDP/Remmina/issues/266)
- Crash while trying to connect using iFreeRDP for iPad App. [\#264](https://github.com/FreeRDP/Remmina/issues/264)
- Public key should not be separately required for SSH PK authentication [\#262](https://github.com/FreeRDP/Remmina/issues/262)
- NX Session: Unresponsive to mouse input, window does not focus. [\#258](https://github.com/FreeRDP/Remmina/issues/258)
- GUI omits basic error information [\#247](https://github.com/FreeRDP/Remmina/issues/247)
- Super/ Windows key not grabbed [\#245](https://github.com/FreeRDP/Remmina/issues/245)
- Remmina fails to compile against latest freerdp on github [\#228](https://github.com/FreeRDP/Remmina/issues/228)
- Cannot connect to RDP over SSH to multiple hosts [\#223](https://github.com/FreeRDP/Remmina/issues/223)
- Bring Japanese translation from Ubuntu [\#216](https://github.com/FreeRDP/Remmina/issues/216)
- Bad colors on Ubuntu 13.04 [\#212](https://github.com/FreeRDP/Remmina/issues/212)
- Crash redirected folder / printer. [\#199](https://github.com/FreeRDP/Remmina/issues/199)
- FREERDP\_CLIENT\_LIBRARY, FREERDP\_LOCALE\_LIBRARY is not found [\#193](https://github.com/FreeRDP/Remmina/issues/193)
- Compile error building remmina-plugin-rdp [\#189](https://github.com/FreeRDP/Remmina/issues/189)
- error: unknown type name 'RDP\_EVENT' when compiled against current freerdp [\#187](https://github.com/FreeRDP/Remmina/issues/187)
- Warinig unimplemented brush style:2 and Beyond Compare in Win7 x64 [\#184](https://github.com/FreeRDP/Remmina/issues/184)
- Remmina crashes \(BSOD\) Windows NT 4 Terminal Server [\#183](https://github.com/FreeRDP/Remmina/issues/183)
- problems compiling [\#180](https://github.com/FreeRDP/Remmina/issues/180)
- SSH Problem to older linux servers. [\#175](https://github.com/FreeRDP/Remmina/issues/175)
- external\_tools directory should not be installed [\#171](https://github.com/FreeRDP/Remmina/issues/171)
- Unable to compile against current freerdp [\#159](https://github.com/FreeRDP/Remmina/issues/159)
- d5yt6guyhukijlkolpk;\['g [\#156](https://github.com/FreeRDP/Remmina/issues/156)
- Remmina full screen closes unexpectedly in a dual monitor setup [\#154](https://github.com/FreeRDP/Remmina/issues/154)
- Numeric keyboard doesn't sync [\#153](https://github.com/FreeRDP/Remmina/issues/153)
- ERRINFO\_DECRYPT\_FAILED and Invalid pointer gdi\_get\_bitmap\_pointer [\#151](https://github.com/FreeRDP/Remmina/issues/151)
- Segmentation fault when using Sessionbrooker and DNS round robin [\#150](https://github.com/FreeRDP/Remmina/issues/150)
- Failed to compile 2 [\#144](https://github.com/FreeRDP/Remmina/issues/144)
- Fails to compile [\#141](https://github.com/FreeRDP/Remmina/issues/141)
- Segfault connecting RDP [\#135](https://github.com/FreeRDP/Remmina/issues/135)
- RDP Plugin Not Found Linux Mint 14 [\#134](https://github.com/FreeRDP/Remmina/issues/134)
- icon fail to compile [\#133](https://github.com/FreeRDP/Remmina/issues/133)
- segfault on ubuntu 12.04 [\#131](https://github.com/FreeRDP/Remmina/issues/131)
- Segfault connecting RDP via SSH: invalid glyph / invalid brush \(0 bpp\) [\#130](https://github.com/FreeRDP/Remmina/issues/130)
- Saved Passwords Fail to Authenticate when .remmina Config is Symlinked [\#128](https://github.com/FreeRDP/Remmina/issues/128)
- can't compile, FREERDP\_CHANNELS\_LIBRARY not found [\#123](https://github.com/FreeRDP/Remmina/issues/123)
- The latest checkout fails to cmake --build=build . on ubuntu 12.10 [\#122](https://github.com/FreeRDP/Remmina/issues/122)
- avahi underlinking [\#120](https://github.com/FreeRDP/Remmina/issues/120)
- Compatibility with newest FreeRDP [\#119](https://github.com/FreeRDP/Remmina/issues/119)
- No visual indication of active tab [\#118](https://github.com/FreeRDP/Remmina/issues/118)
- Black Blocks and blocks misplaced [\#116](https://github.com/FreeRDP/Remmina/issues/116)
- avahi support not actually compiled in [\#113](https://github.com/FreeRDP/Remmina/issues/113)
- RDP session shows only white colors [\#111](https://github.com/FreeRDP/Remmina/issues/111)
- problems typing @ via RDP using pt-latin keyboard [\#110](https://github.com/FreeRDP/Remmina/issues/110)
- "Main Window" interface unusable \(odd toolbar, connection rendering\) [\#109](https://github.com/FreeRDP/Remmina/issues/109)
- remmina crash after rdp connection [\#107](https://github.com/FreeRDP/Remmina/issues/107)
- Wrong colours when connecting to a sunray terminal [\#102](https://github.com/FreeRDP/Remmina/issues/102)
- Add Chinese \(Taiwan\) translation [\#101](https://github.com/FreeRDP/Remmina/issues/101)
- Uses host cursors [\#100](https://github.com/FreeRDP/Remmina/issues/100)
- remmina new feature info [\#97](https://github.com/FreeRDP/Remmina/issues/97)
- Clipboard Sync from Mac OS X to Ubuntu over VNC Not Working [\#93](https://github.com/FreeRDP/Remmina/issues/93)
- SSH public key authentication failed: Public key file doesn't exist [\#92](https://github.com/FreeRDP/Remmina/issues/92)
- Remmina and numeric pad status [\#89](https://github.com/FreeRDP/Remmina/issues/89)
- Remmina disappears after minimizing, going fullscreen, or opening a 2nd window [\#87](https://github.com/FreeRDP/Remmina/issues/87)
- Timezone redirection not properly supported [\#80](https://github.com/FreeRDP/Remmina/issues/80)
- Wonky RDP display [\#79](https://github.com/FreeRDP/Remmina/issues/79)
- Unable to connect to RDP server XXX.XXX.XXX.XXX [\#78](https://github.com/FreeRDP/Remmina/issues/78)
- Remmina freezes when disconnecting vnc session with ssh tunnel [\#77](https://github.com/FreeRDP/Remmina/issues/77)
- Tray icon autostart option disappeared in 1.0 [\#76](https://github.com/FreeRDP/Remmina/issues/76)
- Unknown authentication scheme from VNC server: 5 [\#75](https://github.com/FreeRDP/Remmina/issues/75)
- Error Building Latest on Fedora 16 [\#74](https://github.com/FreeRDP/Remmina/issues/74)
- wake on lan  [\#72](https://github.com/FreeRDP/Remmina/issues/72)
- Add "Download for Ubuntu button" on the website [\#71](https://github.com/FreeRDP/Remmina/issues/71)
- Specify GLIB requirements [\#70](https://github.com/FreeRDP/Remmina/issues/70)
- Can not RDP connect if host key has changed [\#68](https://github.com/FreeRDP/Remmina/issues/68)
- remmina/xfreerdp crashes while trying to use 'remote control' [\#66](https://github.com/FreeRDP/Remmina/issues/66)
- Resize Window to Fit in Remote Resolution Broken [\#63](https://github.com/FreeRDP/Remmina/issues/63)
- OpenBSD OpenSSH & SSH\_AUTH\_METHOD\_INTERACTIVE [\#59](https://github.com/FreeRDP/Remmina/issues/59)
- Crashes on launch on 12.04 [\#57](https://github.com/FreeRDP/Remmina/issues/57)
- CMake Error at CMakeLists.txt:96 \(find\_suggested\_package\):   Unknown CMake command "find\_suggested\_package". [\#55](https://github.com/FreeRDP/Remmina/issues/55)
- Remmina/FreeRDP \(both v1.x\) - protocol security negotiation failure \(to any Windows host\) [\#54](https://github.com/FreeRDP/Remmina/issues/54)
- patch for support building with gtk-2.22 [\#46](https://github.com/FreeRDP/Remmina/issues/46)
- numlock setting is not exported [\#45](https://github.com/FreeRDP/Remmina/issues/45)
- use Enter key to open connection, not only Doubleclick [\#43](https://github.com/FreeRDP/Remmina/issues/43)
- FindGnuTLS.cmake with cmake 2.6  [\#42](https://github.com/FreeRDP/Remmina/issues/42)
- SSH connection with transparent background support [\#41](https://github.com/FreeRDP/Remmina/issues/41)
- Help\>About version is out of date [\#35](https://github.com/FreeRDP/Remmina/issues/35)
- Build error with -DWITH\_GETTEXT=OFF option [\#23](https://github.com/FreeRDP/Remmina/issues/23)
- .desktop.in file is no longer handled [\#21](https://github.com/FreeRDP/Remmina/issues/21)
- don't forget about the old bugs at SF.net [\#19](https://github.com/FreeRDP/Remmina/issues/19)
- Don't use convenience copy of libvncserver [\#15](https://github.com/FreeRDP/Remmina/issues/15)
- Start minimized to systray [\#14](https://github.com/FreeRDP/Remmina/issues/14)
- Clipboard redirect is not working. [\#13](https://github.com/FreeRDP/Remmina/issues/13)
- Accelerator is the same for "Cancel" and "Connect" [\#12](https://github.com/FreeRDP/Remmina/issues/12)
- linker problem [\#9](https://github.com/FreeRDP/Remmina/issues/9)
- rdp plugin crashes [\#1](https://github.com/FreeRDP/Remmina/issues/1)

**Merged pull requests:**

- Frdp 1.1 [\#344](https://github.com/FreeRDP/Remmina/pull/344) ([giox069](https://github.com/giox069))
- Fixes for latest FreeRDP [\#343](https://github.com/FreeRDP/Remmina/pull/343) ([dktrkranz](https://github.com/dktrkranz))
- GTK3 migration - about [\#339](https://github.com/FreeRDP/Remmina/pull/339) ([antenore](https://github.com/antenore))
- GTK3 migration + gix069 fixes [\#338](https://github.com/FreeRDP/Remmina/pull/338) ([antenore](https://github.com/antenore))
- Added more translations from launchpad, fixes \#216 [\#336](https://github.com/FreeRDP/Remmina/pull/336) ([giox069](https://github.com/giox069))
- Make 8bpp and 32bpp working again. Fixes \#329 [\#334](https://github.com/FreeRDP/Remmina/pull/334) ([giox069](https://github.com/giox069))
- Fix compilation issues with latest FreeRDP [\#333](https://github.com/FreeRDP/Remmina/pull/333) ([giox069](https://github.com/giox069))
- Various SSH fixes, fixes \#262 and \#223 [\#330](https://github.com/FreeRDP/Remmina/pull/330) ([giox069](https://github.com/giox069))
- Fix missing extended keycode and 'up' action when releasing all keys [\#328](https://github.com/FreeRDP/Remmina/pull/328) ([giox069](https://github.com/giox069))
- Fix compilation problems with latest version of FreeRDP/FreeRDP master [\#326](https://github.com/FreeRDP/Remmina/pull/326) ([giox069](https://github.com/giox069))
- GTK3 migration - preferences dialog upggraded to Grid Layout [\#322](https://github.com/FreeRDP/Remmina/pull/322) ([antenore](https://github.com/antenore))
- GTK3 Migration - Move from GtkTable to GtkGrid - Chat window and auth dialogs [\#321](https://github.com/FreeRDP/Remmina/pull/321) ([antenore](https://github.com/antenore))
- gtk\_widget\_set\_margin\_end too new as reported in \#commitcomment-7689638 [\#318](https://github.com/FreeRDP/Remmina/pull/318) ([antenore](https://github.com/antenore))
- Scaler layout improvement - gtk\_widget\_set\_margin\_end [\#317](https://github.com/FreeRDP/Remmina/pull/317) ([antenore](https://github.com/antenore))
- Should fix issue \#314 and other related problems [\#315](https://github.com/FreeRDP/Remmina/pull/315) ([antenore](https://github.com/antenore))
- Fixes for ssh, floating toolbar and version number change [\#312](https://github.com/FreeRDP/Remmina/pull/312) ([giox069](https://github.com/giox069))
- ssh, minimize to tray and file sharing fixes [\#303](https://github.com/FreeRDP/Remmina/pull/303) ([giox069](https://github.com/giox069))
- Minor fixes to floating toolbar [\#301](https://github.com/FreeRDP/Remmina/pull/301) ([giox069](https://github.com/giox069))
- GTK+ \< 3.8 compatibility, fixes \#299 [\#300](https://github.com/FreeRDP/Remmina/pull/300) ([giox069](https://github.com/giox069))
- Added one-liner apt-get to install all dependencies, for the lazy people [\#240](https://github.com/FreeRDP/Remmina/pull/240) ([Photonios](https://github.com/Photonios))
- Typo in preference name broke key mapping in VNC [\#236](https://github.com/FreeRDP/Remmina/pull/236) ([nopdotcom](https://github.com/nopdotcom))
- fixes \#193: Instructions for compiling against master FreeRDP [\#231](https://github.com/FreeRDP/Remmina/pull/231) ([krlmlr](https://github.com/krlmlr))
- rename context\_size to ContextSize to match changes in FreeRDP [\#215](https://github.com/FreeRDP/Remmina/pull/215) ([benkohler](https://github.com/benkohler))
- Finish replacing the old stream macros [\#206](https://github.com/FreeRDP/Remmina/pull/206) ([floppym](https://github.com/floppym))
- Fix compilation against freerdp master [\#191](https://github.com/FreeRDP/Remmina/pull/191) ([darklajid](https://github.com/darklajid))
- Remove call to rfx\_context\_set\_cpu\_opt, which was removed from FreeRDP [\#172](https://github.com/FreeRDP/Remmina/pull/172) ([floppym](https://github.com/floppym))
- DWORD is the replacement for RDP\_SCANCODE [\#166](https://github.com/FreeRDP/Remmina/pull/166) ([dktrkranz](https://github.com/dktrkranz))
- Do not define any SONAME for the plugins [\#149](https://github.com/FreeRDP/Remmina/pull/149) ([dktrkranz](https://github.com/dktrkranz))
- external tools [\#132](https://github.com/FreeRDP/Remmina/pull/132) ([loki36](https://github.com/loki36))
- Couple of packaging fixes [\#114](https://github.com/FreeRDP/Remmina/pull/114) ([floppym](https://github.com/floppym))
- Some improvements [\#105](https://github.com/FreeRDP/Remmina/pull/105) ([dupondje](https://github.com/dupondje))
- Multiple fixes + Quickconnect [\#95](https://github.com/FreeRDP/Remmina/pull/95) ([dupondje](https://github.com/dupondje))
- Porting to Cairo and some bugfixes/new features [\#67](https://github.com/FreeRDP/Remmina/pull/67) ([dupondje](https://github.com/dupondje))
- GTK2 compatibility [\#65](https://github.com/FreeRDP/Remmina/pull/65) ([dupondje](https://github.com/dupondje))
- A couple of build fixes [\#64](https://github.com/FreeRDP/Remmina/pull/64) ([floppym](https://github.com/floppym))
- last clipboard commits [\#62](https://github.com/FreeRDP/Remmina/pull/62) ([dupondje](https://github.com/dupondje))
- Some more clipboard fixes [\#60](https://github.com/FreeRDP/Remmina/pull/60) ([dupondje](https://github.com/dupondje))
- Patches [\#58](https://github.com/FreeRDP/Remmina/pull/58) ([dupondje](https://github.com/dupondje))
- Clipboard support [\#56](https://github.com/FreeRDP/Remmina/pull/56) ([dupondje](https://github.com/dupondje))
- Some fixes [\#53](https://github.com/FreeRDP/Remmina/pull/53) ([dupondje](https://github.com/dupondje))
- Fix all deprecated function calls [\#52](https://github.com/FreeRDP/Remmina/pull/52) ([dupondje](https://github.com/dupondje))
- Fixes compiling on ubuntu [\#51](https://github.com/FreeRDP/Remmina/pull/51) ([dupondje](https://github.com/dupondje))
- Fix for scrolling in Remmina [\#50](https://github.com/FreeRDP/Remmina/pull/50) ([dupondje](https://github.com/dupondje))
- Fix app indicator when using custom install prefix [\#48](https://github.com/FreeRDP/Remmina/pull/48) ([rawlinc](https://github.com/rawlinc))
- Issue \#9 [\#39](https://github.com/FreeRDP/Remmina/pull/39) ([Gankov](https://github.com/Gankov))
- bump version to 1.0.0. fixed \#35  [\#38](https://github.com/FreeRDP/Remmina/pull/38) ([chihchun](https://github.com/chihchun))
- Fixed missing system pointer update skeletons. [\#37](https://github.com/FreeRDP/Remmina/pull/37) ([chihchun](https://github.com/chihchun))
- Add an option to remmina to redirect smartcard over rdp [\#36](https://github.com/FreeRDP/Remmina/pull/36) ([absmall](https://github.com/absmall))
- Corrected mistake in desktop file [\#34](https://github.com/FreeRDP/Remmina/pull/34) ([krnekhelesh](https://github.com/krnekhelesh))
- Updated Quicklists [\#33](https://github.com/FreeRDP/Remmina/pull/33) ([krnekhelesh](https://github.com/krnekhelesh))
- update french translation [\#32](https://github.com/FreeRDP/Remmina/pull/32) ([emmanuelgrognet](https://github.com/emmanuelgrognet))
- Fixes for recent git FreeRDP headers [\#31](https://github.com/FreeRDP/Remmina/pull/31) ([maelnor](https://github.com/maelnor))
- GTK cleanup in remmina\_main.c [\#30](https://github.com/FreeRDP/Remmina/pull/30) ([floppym](https://github.com/floppym))
- Install plugins under CMAKE\_INSTALL\_LIBDIR. [\#29](https://github.com/FreeRDP/Remmina/pull/29) ([floppym](https://github.com/floppym))
- Implementing X-GNOME-FullName [\#28](https://github.com/FreeRDP/Remmina/pull/28) ([dktrkranz](https://github.com/dktrkranz))
- Make Gnome Keyring an optional dependency. [\#27](https://github.com/FreeRDP/Remmina/pull/27) ([floppym](https://github.com/floppym))
- Fix installation of desktop file and related icons. [\#26](https://github.com/FreeRDP/Remmina/pull/26) ([floppym](https://github.com/floppym))
- Mark 'Connect' and 'New' as important tool items so they always have labels [\#25](https://github.com/FreeRDP/Remmina/pull/25) ([robert-ancell](https://github.com/robert-ancell))
- Mark main toolbar as a primary toolbar \(so is correctly themed\) [\#24](https://github.com/FreeRDP/Remmina/pull/24) ([robert-ancell](https://github.com/robert-ancell))
- Do not use convenience copy of libvncserver [\#22](https://github.com/FreeRDP/Remmina/pull/22) ([dktrkranz](https://github.com/dktrkranz))
- remmina: install .desktop file [\#20](https://github.com/FreeRDP/Remmina/pull/20) ([jbicha](https://github.com/jbicha))

## [1.0.0](https://github.com/FreeRDP/Remmina/tree/1.0.0) (2012-02-10)
**Closed issues:**

- Can't compile with GTK+ [\#3](https://github.com/FreeRDP/Remmina/issues/3)

**Merged pull requests:**

- A few minor issues [\#11](https://github.com/FreeRDP/Remmina/pull/11) ([doctaweeks](https://github.com/doctaweeks))
- remmina: menu separator fix if avahi is disabled [\#10](https://github.com/FreeRDP/Remmina/pull/10) ([doctaweeks](https://github.com/doctaweeks))
- GTK2/3 issue + re-enable ssh [\#8](https://github.com/FreeRDP/Remmina/pull/8) ([doctaweeks](https://github.com/doctaweeks))
- Fix missed header path, closes \#4 [\#5](https://github.com/FreeRDP/Remmina/pull/5) ([chihchun](https://github.com/chihchun))
- CMake Migration [\#2](https://github.com/FreeRDP/Remmina/pull/2) ([awakecoding](https://github.com/awakecoding))



\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*
