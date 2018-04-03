# Change Log

## [v1.2.0-rcgit.28](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.28) (2018-04-03)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.27...v1.2.0-rcgit.28)

**Implemented enhancements:**

- window has no focus when open ssh sessions [\#1530](https://github.com/FreeRDP/Remmina/issues/1530)
- Enhancement - Variables for pre- and post-commands [\#1485](https://github.com/FreeRDP/Remmina/issues/1485)
- Register and support opening rdp files - Mime improvements [\#1497](https://github.com/FreeRDP/Remmina/pull/1497) ([antenore](https://github.com/antenore))
- Profile and group name niddles in the pre/post commands [\#1492](https://github.com/FreeRDP/Remmina/pull/1492) ([antenore](https://github.com/antenore))
- Add xdmcp protocol to Keywords [\#1491](https://github.com/FreeRDP/Remmina/pull/1491) ([mfvescovi](https://github.com/mfvescovi))
- flatpak: update manifest file [\#1480](https://github.com/FreeRDP/Remmina/pull/1480) ([larchunix](https://github.com/larchunix))

**Fixed bugs:**

- Remmina fails to connect to SSH server without compression since 1.2.0-rcgit-27 \(git rcgit-27\) [\#1505](https://github.com/FreeRDP/Remmina/issues/1505)
- Pasting something that was copied in another VM that has since been closed causes crash [\#1484](https://github.com/FreeRDP/Remmina/issues/1484)
- SSH password authentication failed Wrong state during pending SSH call [\#1428](https://github.com/FreeRDP/Remmina/issues/1428)
- Fix clipboard cleanup, fixes issue \#1484 [\#1486](https://github.com/FreeRDP/Remmina/pull/1486) ([giox069](https://github.com/giox069))

**Closed issues:**

- Is it possible to install to SailfishOS ? [\#1540](https://github.com/FreeRDP/Remmina/issues/1540)
- Icon missing in Gnome Activities on Fedora 27 [\#1534](https://github.com/FreeRDP/Remmina/issues/1534)
- Wrong state during pending SSH call - only one particular server affected [\#1525](https://github.com/FreeRDP/Remmina/issues/1525)
- Remmina Application - unable to find when searching or pin to launcher [\#1521](https://github.com/FreeRDP/Remmina/issues/1521)
- AppStream metadata [\#1520](https://github.com/FreeRDP/Remmina/issues/1520)
- Problem connecting to Windows machine since this last patch Tuesday [\#1515](https://github.com/FreeRDP/Remmina/issues/1515)
- No RDP connections to Windows 10 1803 [\#1512](https://github.com/FreeRDP/Remmina/issues/1512)
- RDP to Windows on custom port [\#1509](https://github.com/FreeRDP/Remmina/issues/1509)
- Unable to connect to RDP [\#1501](https://github.com/FreeRDP/Remmina/issues/1501)
- Dependency Error installing v1.2 on Debian Stretch [\#1481](https://github.com/FreeRDP/Remmina/issues/1481)
- Can't build version 1.2.0-rcgit27 on openSUSE Tumbleweed [\#1476](https://github.com/FreeRDP/Remmina/issues/1476)
- Crash on RDP connection [\#1475](https://github.com/FreeRDP/Remmina/issues/1475)
- this error message occur. i can not install remmina at tails. [\#1473](https://github.com/FreeRDP/Remmina/issues/1473)
- RDP on Custom Port [\#1465](https://github.com/FreeRDP/Remmina/issues/1465)
- Remmina does not save password [\#1440](https://github.com/FreeRDP/Remmina/issues/1440)
- Telepathy plugin can not be built on openSUSE using 1.2.0-rcgit.26 [\#1432](https://github.com/FreeRDP/Remmina/issues/1432)
- Please don't change releases after you publish them [\#1231](https://github.com/FreeRDP/Remmina/issues/1231)
- clipboard not synchonizing between RDP sessions and host [\#556](https://github.com/FreeRDP/Remmina/issues/556)

**Merged pull requests:**

- Minor improvements on secure plugin and SNAP welcome message [\#1545](https://github.com/FreeRDP/Remmina/pull/1545) ([giox069](https://github.com/giox069))
- Remove SNAP build from Travis [\#1544](https://github.com/FreeRDP/Remmina/pull/1544) ([giox069](https://github.com/giox069))
- telepathy: add dbus-glib-1 to link flags [\#1543](https://github.com/FreeRDP/Remmina/pull/1543) ([larchunix](https://github.com/larchunix))
- stats: detect flatpak sandbox at runtime [\#1541](https://github.com/FreeRDP/Remmina/pull/1541) ([larchunix](https://github.com/larchunix))
- Refactoring [\#1537](https://github.com/FreeRDP/Remmina/pull/1537) ([antenore](https://github.com/antenore))
- Redesign - Removed icons where not needed [\#1535](https://github.com/FreeRDP/Remmina/pull/1535) ([antenore](https://github.com/antenore))
- Delay and use gtk\_window\_present\_with\_time\(\) [\#1533](https://github.com/FreeRDP/Remmina/pull/1533) ([giox069](https://github.com/giox069))
- Make compilation and stats collection FLATPAK aware [\#1527](https://github.com/FreeRDP/Remmina/pull/1527) ([giox069](https://github.com/giox069))
- Update flatpak manifest according to feedback from Flathub [\#1526](https://github.com/FreeRDP/Remmina/pull/1526) ([larchunix](https://github.com/larchunix))
- Update fr.po [\#1524](https://github.com/FreeRDP/Remmina/pull/1524) ([DevDef](https://github.com/DevDef))
- Fixes for fedora bugs - desktop file and AppStream metadata [\#1523](https://github.com/FreeRDP/Remmina/pull/1523) ([antenore](https://github.com/antenore))
- Desktop data fixes [\#1522](https://github.com/FreeRDP/Remmina/pull/1522) ([antenore](https://github.com/antenore))
- Fixes for snapcraft.yaml [\#1519](https://github.com/FreeRDP/Remmina/pull/1519) ([giox069](https://github.com/giox069))
- Updated Hungarian translation [\#1517](https://github.com/FreeRDP/Remmina/pull/1517) ([meskobalazs](https://github.com/meskobalazs))
- Removed compression option as not compatible with all SSH servers [\#1506](https://github.com/FreeRDP/Remmina/pull/1506) ([antenore](https://github.com/antenore))
- Fixes segmentation fault reported by \#1499 [\#1503](https://github.com/FreeRDP/Remmina/pull/1503) ([antenore](https://github.com/antenore))

## [v1.2.0-rcgit.27](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.27) (2018-02-14)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.26...v1.2.0-rcgit.27)

**Implemented enhancements:**

- Remmina usage statistics collection. [\#1472](https://github.com/FreeRDP/Remmina/pull/1472) ([antenore](https://github.com/antenore))
- Remmina stats: Profiles and protocols counter [\#1463](https://github.com/FreeRDP/Remmina/pull/1463) ([antenore](https://github.com/antenore))
- Updated freerdp on flatpak definition file [\#1451](https://github.com/FreeRDP/Remmina/pull/1451) ([daper](https://github.com/daper))

**Fixed bugs:**

- RDP + DYNRES: Screen is black immediately after connection and becames visible only later when DISP\_DVC\_CHANNEL\_NAME is connected [\#1442](https://github.com/FreeRDP/Remmina/issues/1442)

**Closed issues:**

- Please help, ERRCONNECT\_LOGON\_TYPE\_NOT\_GRANTED [\#1457](https://github.com/FreeRDP/Remmina/issues/1457)
- Remmina: black flash and connection closed \(cannot connect to Windows 10 17074.1002 with Remmina 1.2.0-rcgit-26 \(git rcgit-26\)\) [\#1456](https://github.com/FreeRDP/Remmina/issues/1456)
- Artifacts and Glitches with PeopleSoft App Designer [\#1452](https://github.com/FreeRDP/Remmina/issues/1452)
- Support for AVC 444 mode [\#1449](https://github.com/FreeRDP/Remmina/issues/1449)
- "save password" option not work [\#1444](https://github.com/FreeRDP/Remmina/issues/1444)
- Shared clipboard not work both Gnome and Unity [\#1443](https://github.com/FreeRDP/Remmina/issues/1443)
- segfault [\#1425](https://github.com/FreeRDP/Remmina/issues/1425)
- Remmina SNAP can't still access keyring to save passwords [\#1404](https://github.com/FreeRDP/Remmina/issues/1404)
- Remmina 1.2 displays lots artifacts when scrolling/zooming in MS Access print preview, but latest xfreerdp and old Remmina 1.1.2 works correctly [\#1387](https://github.com/FreeRDP/Remmina/issues/1387)

**Merged pull requests:**

- Add OS info for Solus [\#1474](https://github.com/FreeRDP/Remmina/pull/1474) ([der-eismann](https://github.com/der-eismann))
- Stats - Bugfixing [\#1471](https://github.com/FreeRDP/Remmina/pull/1471) ([antenore](https://github.com/antenore))
- Stats - Last time  each protocol has been used.  [\#1470](https://github.com/FreeRDP/Remmina/pull/1470) ([antenore](https://github.com/antenore))
- Add initial \[MS-RDPEGFX\] software support [\#1468](https://github.com/FreeRDP/Remmina/pull/1468) ([giox069](https://github.com/giox069))
- Added lsb functions [\#1464](https://github.com/FreeRDP/Remmina/pull/1464) ([antenore](https://github.com/antenore))
- Permit SCALE\_MODE\_DYNRES before DISP\_DVC\_CHANNEL\_NAME is enabled. [\#1446](https://github.com/FreeRDP/Remmina/pull/1446) ([giox069](https://github.com/giox069))
- OS stats and code cleaning [\#1445](https://github.com/FreeRDP/Remmina/pull/1445) ([antenore](https://github.com/antenore))
- Ensure the return value is initialized [\#1433](https://github.com/FreeRDP/Remmina/pull/1433) ([weberhofer](https://github.com/weberhofer))

## [v1.2.0-rcgit.26](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.26) (2017-12-28)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.25...v1.2.0-rcgit.26)

**Fixed bugs:**

- SSH not working [\#1418](https://github.com/FreeRDP/Remmina/issues/1418)

**Closed issues:**

- "Segmentation fault" during start at Xubuntu 17.10 [\#1419](https://github.com/FreeRDP/Remmina/issues/1419)

## [v1.2.0-rcgit.25](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.25) (2017-12-27)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.24...v1.2.0-rcgit.25)

**Implemented enhancements:**

- Allow external commands without protocols in create connection dialog [\#1391](https://github.com/FreeRDP/Remmina/issues/1391)
- Feature request - Option to completely hide the floating toolbar when in fullscreen. [\#1379](https://github.com/FreeRDP/Remmina/issues/1379)
- Option to run Pre Command before ANYTHING else [\#1363](https://github.com/FreeRDP/Remmina/issues/1363)
-  Rename the plugin 'remmina-plugins-gnome' in 'remmina-plugins-secret' [\#1343](https://github.com/FreeRDP/Remmina/issues/1343)
- Enhancement - Variables for pre- and post-commands [\#849](https://github.com/FreeRDP/Remmina/issues/849)
- Failed to load plugin: remmina-plugin-telepathy.so - undefined symbol: remmina\_tp\_handler\_new [\#714](https://github.com/FreeRDP/Remmina/issues/714)
- Implement an exec protocol plugin [\#1406](https://github.com/FreeRDP/Remmina/pull/1406) ([antenore](https://github.com/antenore))
- Add SPICE Native WebDAV shared folder support [\#1401](https://github.com/FreeRDP/Remmina/pull/1401) ([larchunix](https://github.com/larchunix))
-  Added encryption algorithms options for SSH  [\#1397](https://github.com/FreeRDP/Remmina/pull/1397) ([antenore](https://github.com/antenore))
- SSH tunnel and pre/post commands improvements  [\#1385](https://github.com/FreeRDP/Remmina/pull/1385) ([antenore](https://github.com/antenore))
- Prior commands improvements [\#1378](https://github.com/FreeRDP/Remmina/pull/1378) ([antenore](https://github.com/antenore))
- flatpak: add manifest for flatpak-builder [\#1368](https://github.com/FreeRDP/Remmina/pull/1368) ([larchunix](https://github.com/larchunix))
- telepathy: properly setup dbus activation [\#1365](https://github.com/FreeRDP/Remmina/pull/1365) ([larchunix](https://github.com/larchunix))
- Add avahi host discovery for ssh and sftp plugins [\#1355](https://github.com/FreeRDP/Remmina/pull/1355) ([larchunix](https://github.com/larchunix))
- Refactoring - Rename remmina-plugins-gnome in remmina-plugin-secret [\#1348](https://github.com/FreeRDP/Remmina/pull/1348) ([antenore](https://github.com/antenore))
- Refactoring - Part 1 [\#1336](https://github.com/FreeRDP/Remmina/pull/1336) ([antenore](https://github.com/antenore))

**Fixed bugs:**

- Un-check of "Fullscreen on the same monitor as the connection window" won't save [\#1344](https://github.com/FreeRDP/Remmina/issues/1344)
- Remmina resets screen resolution settings in RDP shortcut [\#1323](https://github.com/FreeRDP/Remmina/issues/1323)
- SSH Tunneling is broken with SSH Agent with public key [\#1228](https://github.com/FreeRDP/Remmina/issues/1228)
- Failed to load plugin: remmina-plugin-telepathy.so - undefined symbol: remmina\\_tp\\_handler\\_new [\#714](https://github.com/FreeRDP/Remmina/issues/714)
- Align SFTP and SSH plugins authentication and tunnel functionalities.  [\#1393](https://github.com/FreeRDP/Remmina/pull/1393) ([antenore](https://github.com/antenore))
- Fix Telepathy plugin compilation [\#1356](https://github.com/FreeRDP/Remmina/pull/1356) ([larchunix](https://github.com/larchunix))
- \_\_func\_\_ keyword must not be quoted [\#1350](https://github.com/FreeRDP/Remmina/pull/1350) ([larchunix](https://github.com/larchunix))

**Closed issues:**

- Unable to reject new or changed RDP certificate [\#1413](https://github.com/FreeRDP/Remmina/issues/1413)
- Trying to open an aplication but it fails all the time [\#1412](https://github.com/FreeRDP/Remmina/issues/1412)
- The password in the connections is not saved after upgrading Remmina [\#1402](https://github.com/FreeRDP/Remmina/issues/1402)
- remmina fails to open sftp window, connected to ssh2 server with public key auth [\#1392](https://github.com/FreeRDP/Remmina/issues/1392)
- Please add hostbased mechanism support for ssh [\#1373](https://github.com/FreeRDP/Remmina/issues/1373)
- Remmina tray icon not visible anymore [\#1371](https://github.com/FreeRDP/Remmina/issues/1371)
- Spacebar button bug with "use client mapping" RDP option enabled [\#1364](https://github.com/FreeRDP/Remmina/issues/1364)
- Window 0x555b79d3d650 has not been made visible in GdkSeatGrabPrepareFunc [\#1359](https://github.com/FreeRDP/Remmina/issues/1359)
- WARNING: the "resolution" setting in .pref files is deprecated [\#1358](https://github.com/FreeRDP/Remmina/issues/1358)
- Wrong keyboard layout in VNC [\#1352](https://github.com/FreeRDP/Remmina/issues/1352)
- remmina shows pop-up notification only for first screenshot [\#1347](https://github.com/FreeRDP/Remmina/issues/1347)
- SFTP identity File [\#1301](https://github.com/FreeRDP/Remmina/issues/1301)
- Password not saved [\#1047](https://github.com/FreeRDP/Remmina/issues/1047)
- Starting the Remmina connection from commandline in full screen [\#941](https://github.com/FreeRDP/Remmina/issues/941)
- minimize button does not function well [\#921](https://github.com/FreeRDP/Remmina/issues/921)

**Merged pull requests:**

- Ssh tunnel fixes for \#1228  [\#1417](https://github.com/FreeRDP/Remmina/pull/1417) ([antenore](https://github.com/antenore))
- Fixes 2017 christmas [\#1416](https://github.com/FreeRDP/Remmina/pull/1416) ([antenore](https://github.com/antenore))
- New Spanish file venezuela [\#1415](https://github.com/FreeRDP/Remmina/pull/1415) ([jgjimenez](https://github.com/jgjimenez))
- Update da.po [\#1411](https://github.com/FreeRDP/Remmina/pull/1411) ([scootergrisen](https://github.com/scootergrisen))
- Update Spanish translation [\#1410](https://github.com/FreeRDP/Remmina/pull/1410) ([fitojb](https://github.com/fitojb))
- Updated French po file [\#1409](https://github.com/FreeRDP/Remmina/pull/1409) ([DevDef](https://github.com/DevDef))
- Updated README.MD with Debian install instructions [\#1399](https://github.com/FreeRDP/Remmina/pull/1399) ([MagicFab](https://github.com/MagicFab))
- update simplified Chinese translations. [\#1367](https://github.com/FreeRDP/Remmina/pull/1367) ([sotux](https://github.com/sotux))
- Fix 'consistant' -\> 'consistent' typo [\#1362](https://github.com/FreeRDP/Remmina/pull/1362) ([mfvescovi](https://github.com/mfvescovi))
- ssh\_userauth\_publickey\_auto: should accept empty passphrase [\#1361](https://github.com/FreeRDP/Remmina/pull/1361) ([rayrapetyan](https://github.com/rayrapetyan))
- Remove a couple of legacy "resolution" fields, fixes \#1358 [\#1360](https://github.com/FreeRDP/Remmina/pull/1360) ([giox069](https://github.com/giox069))
- Un-check of 'Fullscreen on the same monitor' [\#1349](https://github.com/FreeRDP/Remmina/pull/1349) ([antenore](https://github.com/antenore))
- Create CODE\_OF\_CONDUCT [\#1341](https://github.com/FreeRDP/Remmina/pull/1341) ([antenore](https://github.com/antenore))

## [v1.2.0-rcgit.24](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.24) (2017-10-25)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.23...v1.2.0-rcgit.24)

**Closed issues:**

- vte no longer optional [\#1327](https://github.com/FreeRDP/Remmina/issues/1327)
- libwinpr.so.2 dependcy missing [\#1312](https://github.com/FreeRDP/Remmina/issues/1312)

**Merged pull requests:**

- Another fix for \#1323 [\#1339](https://github.com/FreeRDP/Remmina/pull/1339) ([giox069](https://github.com/giox069))

## [v1.2.0-rcgit.23](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.23) (2017-10-23)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.22...v1.2.0-rcgit.23)

**Implemented enhancements:**

- SSH session log to file [\#1320](https://github.com/FreeRDP/Remmina/issues/1320)
- Save SSH session to file \#1320  [\#1333](https://github.com/FreeRDP/Remmina/pull/1333) ([antenore](https://github.com/antenore))

**Merged pull requests:**

- Fixes for issue \#1327, optional VTE an SSH [\#1335](https://github.com/FreeRDP/Remmina/pull/1335) ([giox069](https://github.com/giox069))
- snap: add password-manager-service interface for gnome-keyring [\#1334](https://github.com/FreeRDP/Remmina/pull/1334) ([dfiloni](https://github.com/dfiloni))
- Fix window state saving for remmina\_connection\_window and main window [\#1331](https://github.com/FreeRDP/Remmina/pull/1331) ([giox069](https://github.com/giox069))
- Update da.po \(fully translated\) [\#1330](https://github.com/FreeRDP/Remmina/pull/1330) ([scootergrisen](https://github.com/scootergrisen))
- Fixes for deprecated "resolution" setting, issue \#1323 [\#1329](https://github.com/FreeRDP/Remmina/pull/1329) ([giox069](https://github.com/giox069))

## [v1.2.0-rcgit.22](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.22) (2017-10-20)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.21...v1.2.0-rcgit.22)

**Implemented enhancements:**

- Add support for ayatana-appindicators [\#1309](https://github.com/FreeRDP/Remmina/pull/1309) ([giox069](https://github.com/giox069))
- \[snap\] Use the new desktop, desktop-legacy and wayland interfaces [\#1306](https://github.com/FreeRDP/Remmina/pull/1306) ([kenvandine](https://github.com/kenvandine))

**Fixed bugs:**

- Crashes when I try opening Preferences [\#1313](https://github.com/FreeRDP/Remmina/issues/1313)
- Resolution and Passwords Not Saving in Connection Profiles [\#1307](https://github.com/FreeRDP/Remmina/issues/1307)
- Add background and foreground colors for old versions of VTE [\#1318](https://github.com/FreeRDP/Remmina/pull/1318) ([antenore](https://github.com/antenore))
- Make remmina\_file\_editor\_on\_save behave correcthly [\#1308](https://github.com/FreeRDP/Remmina/pull/1308) ([antenore](https://github.com/antenore))

**Closed issues:**

- RDP connection profile settings are not used when opening session [\#1325](https://github.com/FreeRDP/Remmina/issues/1325)
- Remmina removes the desktop application bar when placed in fullscreen mode \(Linux Mint v18.2\) [\#1317](https://github.com/FreeRDP/Remmina/issues/1317)
- Remmina only shows black screen without any text after connection to an SSH server [\#1316](https://github.com/FreeRDP/Remmina/issues/1316)
- Shared file timestamp [\#1310](https://github.com/FreeRDP/Remmina/issues/1310)
- Tab size - SSH Sessions [\#1305](https://github.com/FreeRDP/Remmina/issues/1305)
- no rdp in the window option and no remmina command available in terminal [\#1302](https://github.com/FreeRDP/Remmina/issues/1302)
- Unsolvable dependencies on Debian 9 [\#1298](https://github.com/FreeRDP/Remmina/issues/1298)
- Remmina doesn't respect the `SSH Tunnel` `SSH authentication` `User name` setting [\#1278](https://github.com/FreeRDP/Remmina/issues/1278)
- Debian 9 - Remmina - RDP Plugin [\#1201](https://github.com/FreeRDP/Remmina/issues/1201)
- Win key stays pressed in remote desktop when remmina loses focus [\#1058](https://github.com/FreeRDP/Remmina/issues/1058)
- Remmina crashes after reconnection attempt after entering wrong password [\#1054](https://github.com/FreeRDP/Remmina/issues/1054)
- Remmina is not aware of additional screen added [\#938](https://github.com/FreeRDP/Remmina/issues/938)

**Merged pull requests:**

- Focus-out-event should call UNFOCUS plugin feature, fixes \#1058 [\#1315](https://github.com/FreeRDP/Remmina/pull/1315) ([giox069](https://github.com/giox069))
- Fix some memory leaks [\#1314](https://github.com/FreeRDP/Remmina/pull/1314) ([giox069](https://github.com/giox069))
- Dev documentation and Copyright update [\#1311](https://github.com/FreeRDP/Remmina/pull/1311) ([antenore](https://github.com/antenore))

## [v1.2.0-rcgit.21](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.21) (2017-10-08)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.20...v1.2.0-rcgit.21)

**Implemented enhancements:**

- Feature Request: Create custom Terminal Color Schemes for SSH [\#1235](https://github.com/FreeRDP/Remmina/issues/1235)
- Enter key doesn't "Connect" when in Edit menu and password field is in focus [\#1233](https://github.com/FreeRDP/Remmina/issues/1233)
- Feature request - Option - Floating Desktop Name \[$5\] [\#815](https://github.com/FreeRDP/Remmina/issues/815)
- Don't close connection windows when main window is closed [\#785](https://github.com/FreeRDP/Remmina/issues/785)
- File association for connections [\#485](https://github.com/FreeRDP/Remmina/issues/485)
- Implementation of dynamic resolution update for RDP [\#1292](https://github.com/FreeRDP/Remmina/pull/1292) ([giox069](https://github.com/giox069))
- Shortcutkey viewonly [\#1289](https://github.com/FreeRDP/Remmina/pull/1289) ([antenore](https://github.com/antenore))
- Simplify pthreads library detection [\#1274](https://github.com/FreeRDP/Remmina/pull/1274) ([jabl](https://github.com/jabl))
- Custom Color schemes for SSH [\#1272](https://github.com/FreeRDP/Remmina/pull/1272) ([antenore](https://github.com/antenore))
- RDP client keyboard mapping with GTK3 [\#1265](https://github.com/FreeRDP/Remmina/pull/1265) ([giox069](https://github.com/giox069))
- Make building the VNC plugin optional [\#1263](https://github.com/FreeRDP/Remmina/pull/1263) ([diogocp](https://github.com/diogocp))

**Fixed bugs:**

- copy / paste and charset [\#1300](https://github.com/FreeRDP/Remmina/issues/1300)
- Changes user name ssh tunnel \(vnc over ssh\) do not save. [\#1255](https://github.com/FreeRDP/Remmina/issues/1255)
- SSH username ignored for Tunnels for RDP  [\#1254](https://github.com/FreeRDP/Remmina/issues/1254)
- Can't Specify X :display for XDMCP.  [\#1251](https://github.com/FreeRDP/Remmina/issues/1251)
- "Remember last view mode for each connection" not respected in v1.2.0-rcgit.19 [\#1247](https://github.com/FreeRDP/Remmina/issues/1247)
- Remmina always remember password [\#1224](https://github.com/FreeRDP/Remmina/issues/1224)
- Host key not working with SPICE plugin connected to KVM client.  [\#1035](https://github.com/FreeRDP/Remmina/issues/1035)
- Shortcutkey viewonly [\#1289](https://github.com/FreeRDP/Remmina/pull/1289) ([antenore](https://github.com/antenore))

**Closed issues:**

- Remmina VNC viewer window become freeze or unresponsive if remote Ubuntu pc  connection dropped or shutdown [\#1296](https://github.com/FreeRDP/Remmina/issues/1296)
- support opensuse [\#1284](https://github.com/FreeRDP/Remmina/issues/1284)
- Remmina 1.2.0-rcgit.20 fails to build from source on sparc64 box [\#1283](https://github.com/FreeRDP/Remmina/issues/1283)
- WINPR\_INCLUDE\_DIR Set to not found [\#1281](https://github.com/FreeRDP/Remmina/issues/1281)
- Problem with latest pthread cmake patch and address sanitizer [\#1276](https://github.com/FreeRDP/Remmina/issues/1276)
- Remmina on Raspbian 9 Stretch and Remmina RemoteApp support [\#1269](https://github.com/FreeRDP/Remmina/issues/1269)
- Large file corruption over shared folder on RDP connection. [\#1266](https://github.com/FreeRDP/Remmina/issues/1266)
- Autoreconnect function with same credentials [\#1264](https://github.com/FreeRDP/Remmina/issues/1264)
- VNC plugin on Debian 9 \(stretch\) no longer available / working [\#1248](https://github.com/FreeRDP/Remmina/issues/1248)
- Graphics Flickering [\#1194](https://github.com/FreeRDP/Remmina/issues/1194)
- Can't connect to x509vnc on Fedora 25 [\#1171](https://github.com/FreeRDP/Remmina/issues/1171)
- RDP remote files are listed with huge wrong size under 32bit client platforms [\#1166](https://github.com/FreeRDP/Remmina/issues/1166)
- Printing PDF via Remmina and share local Printers on Linux Mint dont work [\#1158](https://github.com/FreeRDP/Remmina/issues/1158)
- Segfault when mousing out of latest remmina. [\#1152](https://github.com/FreeRDP/Remmina/issues/1152)
- Build 1.2.0-rcgit.x on Debian Jessie [\#1147](https://github.com/FreeRDP/Remmina/issues/1147)
- RDP to Windows 10 freezes/hangs [\#1139](https://github.com/FreeRDP/Remmina/issues/1139)
- FTBFS with both libfreerdp 1.2 and 2.0 installed [\#1137](https://github.com/FreeRDP/Remmina/issues/1137)
- Share folder date & time [\#934](https://github.com/FreeRDP/Remmina/issues/934)
- Copy/paste between Windows 10 guest and Remmina 1.2.0-rcgit.14 \(git n/a\) host [\#916](https://github.com/FreeRDP/Remmina/issues/916)
- Duplicate of issue\#99 on Remina 1.2.0 [\#844](https://github.com/FreeRDP/Remmina/issues/844)
- Shared clipboard with rdp [\#806](https://github.com/FreeRDP/Remmina/issues/806)
- Remmina crashes after 1st auth failure [\#798](https://github.com/FreeRDP/Remmina/issues/798)
- RDP Plugin Not Found [\#751](https://github.com/FreeRDP/Remmina/issues/751)
- Unable to connect to remote desktop after last update, error \(SIGSEGV\) [\#747](https://github.com/FreeRDP/Remmina/issues/747)
- Bug: unable to connect through gateway since 1.2.0-rcgit.8 [\#722](https://github.com/FreeRDP/Remmina/issues/722)
- not printing with remiina client 1.2 [\#707](https://github.com/FreeRDP/Remmina/issues/707)
- Copy&Paste of formatted text not working. [\#693](https://github.com/FreeRDP/Remmina/issues/693)
- \\tsclient shares disconnect when accessed by certain programs on server [\#686](https://github.com/FreeRDP/Remmina/issues/686)
- copy / paste and charset [\#685](https://github.com/FreeRDP/Remmina/issues/685)
- Cannot compile Remmina 1.0 on Linux mint 17.2 [\#658](https://github.com/FreeRDP/Remmina/issues/658)
- Minus "-" instead of slash "/" from numpad [\#656](https://github.com/FreeRDP/Remmina/issues/656)
- version 1.2 RDP/VNC "viewport fullscreen mode" causes problems when scaling is enabled [\#357](https://github.com/FreeRDP/Remmina/issues/357)

**Merged pull requests:**

- Fix charset conversion in VNC clipboard [\#1303](https://github.com/FreeRDP/Remmina/pull/1303) ([giox069](https://github.com/giox069))
- Add support for application/x-remmina MIME type, fixes \#485 [\#1299](https://github.com/FreeRDP/Remmina/pull/1299) ([giox069](https://github.com/giox069))
- Reverted the notfication for the floating toolbar [\#1297](https://github.com/FreeRDP/Remmina/pull/1297) ([antenore](https://github.com/antenore))
- Fixes for notifications [\#1294](https://github.com/FreeRDP/Remmina/pull/1294) ([giox069](https://github.com/giox069))
- Server name is notified to the user everytime we enter a tab [\#1290](https://github.com/FreeRDP/Remmina/pull/1290) ([antenore](https://github.com/antenore))
- Exit strategy for Gnome Shell 3.26 [\#1287](https://github.com/FreeRDP/Remmina/pull/1287) ([giox069](https://github.com/giox069))
- Fix gtk redrawing in RDP plugin [\#1286](https://github.com/FreeRDP/Remmina/pull/1286) ([giox069](https://github.com/giox069))
- Gtk deprecations [\#1285](https://github.com/FreeRDP/Remmina/pull/1285) ([giox069](https://github.com/giox069))
- Add Keywords entry [\#1277](https://github.com/FreeRDP/Remmina/pull/1277) ([mfvescovi](https://github.com/mfvescovi))
- Runtime paths for AppImage [\#1271](https://github.com/FreeRDP/Remmina/pull/1271) ([giox069](https://github.com/giox069))
- Get ssh\_username from remmina profile - fixes \#1255 [\#1267](https://github.com/FreeRDP/Remmina/pull/1267) ([antenore](https://github.com/antenore))
- Better button labelling and new save button [\#1250](https://github.com/FreeRDP/Remmina/pull/1250) ([antenore](https://github.com/antenore))

## [v1.2.0-rcgit.20](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.20) (2017-08-25)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.19...v1.2.0-rcgit.20)

**Implemented enhancements:**

- Set Linux as the default terminal color scheme. Fixes \#1238 [\#1243](https://github.com/FreeRDP/Remmina/pull/1243) ([antenore](https://github.com/antenore))

**Fixed bugs:**

- Remmina crashes each time I use FTP transfer [\#1257](https://github.com/FreeRDP/Remmina/issues/1257)
- Problems with some color schemes [\#1225](https://github.com/FreeRDP/Remmina/issues/1225)

**Closed issues:**

- Color on SSH terminal window being applied [\#1238](https://github.com/FreeRDP/Remmina/issues/1238)
- Tray applet closing when last window gets closed [\#1236](https://github.com/FreeRDP/Remmina/issues/1236)
- Systray icon closes when closing 'Main Window' or RDP connection [\#1229](https://github.com/FreeRDP/Remmina/issues/1229)
- Missing icons after compiling latest Git release [\#1221](https://github.com/FreeRDP/Remmina/issues/1221)
- RDP quality settings not saved [\#1216](https://github.com/FreeRDP/Remmina/issues/1216)
- Cannot connect after upgrading Ubuntu to 16.04 [\#946](https://github.com/FreeRDP/Remmina/issues/946)

**Merged pull requests:**

- passphrase not yet used in sftp connection - closes \#1257 [\#1259](https://github.com/FreeRDP/Remmina/pull/1259) ([antenore](https://github.com/antenore))
- Added ssh\_agent in the list of authorized method, fixes \#1228 [\#1246](https://github.com/FreeRDP/Remmina/pull/1246) ([antenore](https://github.com/antenore))
- Connect when pressing enter in the password field [\#1242](https://github.com/FreeRDP/Remmina/pull/1242) ([antenore](https://github.com/antenore))
- Fixed terminal color palettes [\#1240](https://github.com/FreeRDP/Remmina/pull/1240) ([antenore](https://github.com/antenore))
- French translation update and other po files update with new strings to be translated [\#1239](https://github.com/FreeRDP/Remmina/pull/1239) ([DevDef](https://github.com/DevDef))
- SSH Kerberos GSSAPI Auth [\#1237](https://github.com/FreeRDP/Remmina/pull/1237) ([giox069](https://github.com/giox069))
- Enter key doesn't "Connect" when in Edit menu and password field is in focus  - \#1233 [\#1234](https://github.com/FreeRDP/Remmina/pull/1234) ([erichoog](https://github.com/erichoog))
- Fix link for issue \#367 [\#1232](https://github.com/FreeRDP/Remmina/pull/1232) ([erichoog](https://github.com/erichoog))
- Change Status Icon availability for gnome 3.16 [\#1230](https://github.com/FreeRDP/Remmina/pull/1230) ([giox069](https://github.com/giox069))

## [v1.2.0-rcgit.19](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.19) (2017-07-24)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.18...v1.2.0-rcgit.19)

**Implemented enhancements:**

- SSH tunneling does not work with RDP server redirection [\#1175](https://github.com/FreeRDP/Remmina/issues/1175)
- Enhancement - Another User connected to the server, forcing \[...\] pop-up [\#1141](https://github.com/FreeRDP/Remmina/issues/1141)
- RD Gateway Doesn't Support different username/password [\#933](https://github.com/FreeRDP/Remmina/issues/933)
- the checkbox, 'Save SSH Password',  greyed out. [\#708](https://github.com/FreeRDP/Remmina/issues/708)
- RD Gateway authentication doesn't work [\#511](https://github.com/FreeRDP/Remmina/issues/511)
- Allow the user to hide the toolbar inside a remmina connection window [\#413](https://github.com/FreeRDP/Remmina/issues/413)
- Support for RDP Gateway [\#347](https://github.com/FreeRDP/Remmina/issues/347)
- Feature request: Implement "Credential Groups" [\#82](https://github.com/FreeRDP/Remmina/issues/82)
- Ssh improvements [\#1196](https://github.com/FreeRDP/Remmina/pull/1196) ([antenore](https://github.com/antenore))
- Implementing separate user and password for RD Gateway [\#1193](https://github.com/FreeRDP/Remmina/pull/1193) ([antenore](https://github.com/antenore))
- RDP: remove server hostname DNS check [\#1190](https://github.com/FreeRDP/Remmina/pull/1190) ([giox069](https://github.com/giox069))
- Pressing Enter in the Domain entry of Auth dialog submits [\#1163](https://github.com/FreeRDP/Remmina/pull/1163) ([iivorait](https://github.com/iivorait))
- Add server name to popup warning [\#1142](https://github.com/FreeRDP/Remmina/pull/1142) ([antenore](https://github.com/antenore))
- Add an option to completely disable the floating toolbar in fullscreen mode [\#1135](https://github.com/FreeRDP/Remmina/pull/1135) ([transistor1](https://github.com/transistor1))
- The host key should not affect subsequent key operations [\#1132](https://github.com/FreeRDP/Remmina/pull/1132) ([nanxiongchao](https://github.com/nanxiongchao))

**Fixed bugs:**

- Remmina profile editor scrolling is broken [\#1179](https://github.com/FreeRDP/Remmina/issues/1179)
- Graphical issues when using byobu/tmux [\#1151](https://github.com/FreeRDP/Remmina/issues/1151)
- I always got an error message "SSH public key has changed!" [\#1129](https://github.com/FreeRDP/Remmina/issues/1129)
- problem with tui interfaces in remmina [\#1076](https://github.com/FreeRDP/Remmina/issues/1076)
- test bounty source [\#1048](https://github.com/FreeRDP/Remmina/issues/1048)
- Importing rdp file not successful [\#1039](https://github.com/FreeRDP/Remmina/issues/1039)
- Implementing separate user and password for RD Gateway [\#1193](https://github.com/FreeRDP/Remmina/pull/1193) ([antenore](https://github.com/antenore))

**Closed issues:**

- Invalid \(too large\) file size in rdp shared folder [\#1220](https://github.com/FreeRDP/Remmina/issues/1220)
- RDP to Win10 makes ToolBar clear [\#1209](https://github.com/FreeRDP/Remmina/issues/1209)
- Upgrading to 1.2 failed [\#1202](https://github.com/FreeRDP/Remmina/issues/1202)
- Multiple NICs [\#1188](https://github.com/FreeRDP/Remmina/issues/1188)
- "Public Key \(Automatic\)" option does not work with ed25519 keys [\#1187](https://github.com/FreeRDP/Remmina/issues/1187)
- Cannot connect to Windows 10 after Creators Update \(1703\) [\#1186](https://github.com/FreeRDP/Remmina/issues/1186)
- Crashing on Load: Segfault [\#1185](https://github.com/FreeRDP/Remmina/issues/1185)
- Files in mounted sharefolders become terrabytes big. [\#1174](https://github.com/FreeRDP/Remmina/issues/1174)
- compilation on Debian 9 [\#1165](https://github.com/FreeRDP/Remmina/issues/1165)
- Clipboard sync is not working [\#1164](https://github.com/FreeRDP/Remmina/issues/1164)
- File Transfers to Share Folder Crashing Remmina 1.2.0-rcgit-18 Each Try [\#1159](https://github.com/FreeRDP/Remmina/issues/1159)
- Segfault when using the host SMB sharefolder on Windows \(from Linux client\) [\#1157](https://github.com/FreeRDP/Remmina/issues/1157)
- Custom RDP settings? [\#1146](https://github.com/FreeRDP/Remmina/issues/1146)
- Missing icons in remote session toolbar [\#1136](https://github.com/FreeRDP/Remmina/issues/1136)
- Opacity for pop up tool bar is too see-through [\#1131](https://github.com/FreeRDP/Remmina/issues/1131)
- option to remove fullscreen toolbar completely [\#1128](https://github.com/FreeRDP/Remmina/issues/1128)
- SSH terminal doesn't work  [\#1125](https://github.com/FreeRDP/Remmina/issues/1125)
- Start remmina with fixed language [\#1119](https://github.com/FreeRDP/Remmina/issues/1119)
- Remmina Next is Corrupting the Unity Desktop [\#1111](https://github.com/FreeRDP/Remmina/issues/1111)
- Remove systemd dependency [\#1100](https://github.com/FreeRDP/Remmina/issues/1100)
- Keyboard Events [\#1096](https://github.com/FreeRDP/Remmina/issues/1096)
- No move or resize terminal window [\#1087](https://github.com/FreeRDP/Remmina/issues/1087)
- Connection Timeout - vfprintf.c no such file or directory - Segfault [\#1080](https://github.com/FreeRDP/Remmina/issues/1080)
- SSH password can't save , terminal auto disconnection [\#1078](https://github.com/FreeRDP/Remmina/issues/1078)
- Remmina getting down when used ssh & mc [\#1075](https://github.com/FreeRDP/Remmina/issues/1075)
- VNC over SSH tunnel very slow [\#713](https://github.com/FreeRDP/Remmina/issues/713)
- Domain/Username/Password database [\#711](https://github.com/FreeRDP/Remmina/issues/711)
- Remmina RDP hangs after second certificate confirmation when using RD gateway [\#706](https://github.com/FreeRDP/Remmina/issues/706)
- SSH agent forwarding, tunnels and other nice stuff [\#692](https://github.com/FreeRDP/Remmina/issues/692)
- When I open a RDP connexion on Microsoft Windows PC, SSH Client display is messing [\#663](https://github.com/FreeRDP/Remmina/issues/663)
- Remmina doesn't respect ssh config files... [\#235](https://github.com/FreeRDP/Remmina/issues/235)
- SSH: missed check availability of PubkicKey Auth on remote host before asking passphrase [\#176](https://github.com/FreeRDP/Remmina/issues/176)

**Merged pull requests:**

- Fixes for the exit strategy [\#1219](https://github.com/FreeRDP/Remmina/pull/1219) ([giox069](https://github.com/giox069))
- Issues 785 966 [\#1214](https://github.com/FreeRDP/Remmina/pull/1214) ([giox069](https://github.com/giox069))
- Ssh protocol plugin implementation [\#1206](https://github.com/FreeRDP/Remmina/pull/1206) ([antenore](https://github.com/antenore))
- The multi password changer [\#1203](https://github.com/FreeRDP/Remmina/pull/1203) ([giox069](https://github.com/giox069))
- Fix snap issues [\#1200](https://github.com/FreeRDP/Remmina/pull/1200) ([dfiloni](https://github.com/dfiloni))
- Terminal functionalities to make Remmina behave correctly with ncurses [\#1198](https://github.com/FreeRDP/Remmina/pull/1198) ([antenore](https://github.com/antenore))
- Fix multiple typos of 'transfered' word [\#1173](https://github.com/FreeRDP/Remmina/pull/1173) ([mfvescovi](https://github.com/mfvescovi))
- RDP plugin: add password expired message and update po files [\#1170](https://github.com/FreeRDP/Remmina/pull/1170) ([giox069](https://github.com/giox069))
- Remmina --full-version command line option [\#1169](https://github.com/FreeRDP/Remmina/pull/1169) ([antenore](https://github.com/antenore))
- snapcraft: use snap-preload to get dynamic access to /snap path [\#1161](https://github.com/FreeRDP/Remmina/pull/1161) ([3v1n0](https://github.com/3v1n0))
- Inproved CodeTriage and Bountysource buttons [\#1143](https://github.com/FreeRDP/Remmina/pull/1143) ([weberhofer](https://github.com/weberhofer))
- Cmake clean [\#1140](https://github.com/FreeRDP/Remmina/pull/1140) ([antenore](https://github.com/antenore))
- Give precedence to libfreerdp2 and winpr2 libs [\#1138](https://github.com/FreeRDP/Remmina/pull/1138) ([antenore](https://github.com/antenore))
- travis: build the snap for PRs in Debug mode [\#1130](https://github.com/FreeRDP/Remmina/pull/1130) ([3v1n0](https://github.com/3v1n0))
- snap: optionally push all the built snaps on PRs to transfer.sh [\#1126](https://github.com/FreeRDP/Remmina/pull/1126) ([3v1n0](https://github.com/3v1n0))
- Small English nit \(fullscreen\_on\_auto\) [\#1124](https://github.com/FreeRDP/Remmina/pull/1124) ([lnicola](https://github.com/lnicola))
- Fix german translation [\#1120](https://github.com/FreeRDP/Remmina/pull/1120) ([giox069](https://github.com/giox069))

## [v1.2.0-rcgit.18](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.18) (2017-02-13)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.17...v1.2.0-rcgit.18)

**Implemented enhancements:**

- No prompt information while another login using the same account  [\#1114](https://github.com/FreeRDP/Remmina/pull/1114) ([nanxiongchao](https://github.com/nanxiongchao))
- snap: add CMake generated snapcraft.yaml and `make snap` [\#1102](https://github.com/FreeRDP/Remmina/pull/1102) ([3v1n0](https://github.com/3v1n0))

**Fixed bugs:**

- 
full screen window placement with multiple monitors [\#124](https://github.com/FreeRDP/Remmina/issues/124)

**Closed issues:**

- Auto-Reconnect function [\#1099](https://github.com/FreeRDP/Remmina/issues/1099)
- RDP Plugin not found after Update [\#1094](https://github.com/FreeRDP/Remmina/issues/1094)
- Remmina-1.2.0-rcgit.17 build error [\#1090](https://github.com/FreeRDP/Remmina/issues/1090)
- Remote Windows program crashes when accessed by Remmina RDP [\#1083](https://github.com/FreeRDP/Remmina/issues/1083)
- Crashing since upgraded to 1.2.0-rcgit-17 [\#1077](https://github.com/FreeRDP/Remmina/issues/1077)
- Remmina windows unmovable/unresizable [\#1073](https://github.com/FreeRDP/Remmina/issues/1073)
- Russian translation fixes [\#1071](https://github.com/FreeRDP/Remmina/issues/1071)
- English fixes [\#1040](https://github.com/FreeRDP/Remmina/issues/1040)
- Compile failed on Archlinux [\#1012](https://github.com/FreeRDP/Remmina/issues/1012)
- RDP connections or whole Remmina are crashing regularly [\#778](https://github.com/FreeRDP/Remmina/issues/778)
- xfce4 applet gone? [\#609](https://github.com/FreeRDP/Remmina/issues/609)
- Bad port stored into known\_hosts2 [\#604](https://github.com/FreeRDP/Remmina/issues/604)
- no puedo ver la impresora por remmnia [\#578](https://github.com/FreeRDP/Remmina/issues/578)
- Uzbek language support [\#560](https://github.com/FreeRDP/Remmina/issues/560)
- Getting message that VNC plugin is not installed even though it is [\#559](https://github.com/FreeRDP/Remmina/issues/559)

**Merged pull requests:**

- snap: use snapcraft 2.26 features [\#1115](https://github.com/FreeRDP/Remmina/pull/1115) ([3v1n0](https://github.com/3v1n0))
- Keyboard capture fixes, \#1087 \#1096 \#1111 [\#1113](https://github.com/FreeRDP/Remmina/pull/1113) ([giox069](https://github.com/giox069))
- Keyboard capture changes, \#1087 \#1096 \#1111 [\#1112](https://github.com/FreeRDP/Remmina/pull/1112) ([giox069](https://github.com/giox069))
- travis: add parallel builds to build from debs and generate snap packages [\#1104](https://github.com/FreeRDP/Remmina/pull/1104) ([3v1n0](https://github.com/3v1n0))
- GUI enhancements  [\#1103](https://github.com/FreeRDP/Remmina/pull/1103) ([antenore](https://github.com/antenore))
- Add descriptions for some freerdp exit status code [\#1101](https://github.com/FreeRDP/Remmina/pull/1101) ([giox069](https://github.com/giox069))
- FindFREERDP.cmake: update library names to match upstream [\#1095](https://github.com/FreeRDP/Remmina/pull/1095) ([3v1n0](https://github.com/3v1n0))
- Update fr.po [\#1089](https://github.com/FreeRDP/Remmina/pull/1089) ([DevDef](https://github.com/DevDef))
- Fix english typos as per \#1040 [\#1088](https://github.com/FreeRDP/Remmina/pull/1088) ([antenore](https://github.com/antenore))
- Make full screen in the same monitor where the connection window reside [\#1084](https://github.com/FreeRDP/Remmina/pull/1084) ([antenore](https://github.com/antenore))
- Fix russian translations as per \#1070 [\#1072](https://github.com/FreeRDP/Remmina/pull/1072) ([antenore](https://github.com/antenore))

## [v1.2.0-rcgit.17](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.17) (2016-12-22)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.16...v1.2.0-rcgit.17)

**Implemented enhancements:**

- Remove survey as too bloated.  [\#1064](https://github.com/FreeRDP/Remmina/issues/1064)
- Cannot connect to neoma-bs RDP server [\#1056](https://github.com/FreeRDP/Remmina/issues/1056)
- Import gateay values from rdp files - \#1056 [\#1068](https://github.com/FreeRDP/Remmina/pull/1068) ([antenore](https://github.com/antenore))
- No debian/ubuntu distributions files included. [\#1062](https://github.com/FreeRDP/Remmina/pull/1062) ([nastasi](https://github.com/nastasi))
- Fix fullscreen window position \(\#873\) [\#1060](https://github.com/FreeRDP/Remmina/pull/1060) ([spasche](https://github.com/spasche))

**Fixed bugs:**

- When "Server" GtkComboBox is selected, TAB key doesn't work [\#1049](https://github.com/FreeRDP/Remmina/issues/1049)

**Closed issues:**

- Advanced Settings for RDP connections [\#1043](https://github.com/FreeRDP/Remmina/issues/1043)
- error: ‘rdpGdi {aka struct rdp\_gdi}’ has no member named ‘bytesPerPixel’ [\#1028](https://github.com/FreeRDP/Remmina/issues/1028)
- Remmina crashes attempting VNC connection to Mac OS X Yosemite [\#517](https://github.com/FreeRDP/Remmina/issues/517)
- Remmina can no longer recognize RDP authentication failure [\#507](https://github.com/FreeRDP/Remmina/issues/507)
- SSH-Tunneled VNC connection randomly hangs [\#480](https://github.com/FreeRDP/Remmina/issues/480)
- Long connection times when forwarding RDP connections through SSH [\#452](https://github.com/FreeRDP/Remmina/issues/452)
- Issue with PPA mentioned in Wiki [\#439](https://github.com/FreeRDP/Remmina/issues/439)
- Remmina crashes copying from remoted computer and pasting into remoting one [\#411](https://github.com/FreeRDP/Remmina/issues/411)
- font smoothing - some fonts are not smoothed [\#382](https://github.com/FreeRDP/Remmina/issues/382)
- Remmina blocks and I have to disconnect every 20 minutes [\#332](https://github.com/FreeRDP/Remmina/issues/332)
- Keyboard Mapping  [\#261](https://github.com/FreeRDP/Remmina/issues/261)
- Remmina crash when running towards server with xrdp 0.7.0 [\#234](https://github.com/FreeRDP/Remmina/issues/234)
- Resolution of Client Viewport not functioning correct [\#205](https://github.com/FreeRDP/Remmina/issues/205)

**Merged pull requests:**

- Updated Russian translations [\#1070](https://github.com/FreeRDP/Remmina/pull/1070) ([antenore](https://github.com/antenore))
- Libfreerdp updates [\#1067](https://github.com/FreeRDP/Remmina/pull/1067) ([giox069](https://github.com/giox069))
- Removed survey to clean up the code - API CHANGE [\#1065](https://github.com/FreeRDP/Remmina/pull/1065) ([antenore](https://github.com/antenore))
- Updated German translation [\#1063](https://github.com/FreeRDP/Remmina/pull/1063) ([theraser](https://github.com/theraser))
- When 'Server' GtkComboBox is selected, TAB doesn't work [\#1050](https://github.com/FreeRDP/Remmina/pull/1050) ([antenore](https://github.com/antenore))
- Customizable app name and locations [\#1046](https://github.com/FreeRDP/Remmina/pull/1046) ([3v1n0](https://github.com/3v1n0))
- remmina.desktop: add Quit desktop action for Unity [\#1045](https://github.com/FreeRDP/Remmina/pull/1045) ([3v1n0](https://github.com/3v1n0))
- Update Spanish Translation [\#1044](https://github.com/FreeRDP/Remmina/pull/1044) ([jgjimenez](https://github.com/jgjimenez))
- Update fr.po [\#1038](https://github.com/FreeRDP/Remmina/pull/1038) ([DevDef](https://github.com/DevDef))
- Uzbek cyrillic: update translations [\#1037](https://github.com/FreeRDP/Remmina/pull/1037) ([ozbek](https://github.com/ozbek))

## [v1.2.0-rcgit.16](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.16) (2016-10-31)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.15...v1.2.0-rcgit.16)

**Implemented enhancements:**

- Add RDP scaling factor as implemented by FreeRDP  [\#969](https://github.com/FreeRDP/Remmina/issues/969)
- Please add man pages for remmina. [\#952](https://github.com/FreeRDP/Remmina/issues/952)
- Please support XDG base directory spec [\#818](https://github.com/FreeRDP/Remmina/issues/818)
- Auto-Highlight Quick Search Text Upon Each Remote Session Launch [\#544](https://github.com/FreeRDP/Remmina/issues/544)
- spice support [\#117](https://github.com/FreeRDP/Remmina/issues/117)
- CMAKE: fix GNUInstallDirs usage [\#1004](https://github.com/FreeRDP/Remmina/pull/1004) ([hasufell](https://github.com/hasufell))
- XDG base directory spec [\#1003](https://github.com/FreeRDP/Remmina/pull/1003) ([antenore](https://github.com/antenore))
- When focus-in inside the quick search, select the whole text as per \#544 [\#1001](https://github.com/FreeRDP/Remmina/pull/1001) ([antenore](https://github.com/antenore))
- spice: send a notification at the end of file transfers [\#994](https://github.com/FreeRDP/Remmina/pull/994) ([larchunix](https://github.com/larchunix))
- spice: show progress of drag & drop file transfers [\#993](https://github.com/FreeRDP/Remmina/pull/993) ([larchunix](https://github.com/larchunix))

**Fixed bugs:**

- SSH - Secure Shell Issue [\#936](https://github.com/FreeRDP/Remmina/issues/936)
- Auto-highlighted "Quick Connect" connection name within new connection dialog copies the words "Quick Connect" into PRIMARY selection [\#263](https://github.com/FreeRDP/Remmina/issues/263)
- can't connect to 2008r2 when set "negotiation" security mode [\#202](https://github.com/FreeRDP/Remmina/issues/202)
- crash of remmina when terminal bell rings [\#163](https://github.com/FreeRDP/Remmina/issues/163)
- Support for left-handed mouse [\#136](https://github.com/FreeRDP/Remmina/issues/136)

**Closed issues:**

- Segmentation error on Gnome Desktop with Wayland  [\#1034](https://github.com/FreeRDP/Remmina/issues/1034)
- untranslated lines missed  [\#1029](https://github.com/FreeRDP/Remmina/issues/1029)
- Security Issue? [\#1027](https://github.com/FreeRDP/Remmina/issues/1027)
- remmina fails to compile against freerdp [\#1015](https://github.com/FreeRDP/Remmina/issues/1015)
- VNC connection doesn't work if color is set to 256 colors \(8 bpp\) [\#989](https://github.com/FreeRDP/Remmina/issues/989)
- remmina.org down [\#988](https://github.com/FreeRDP/Remmina/issues/988)
- Random remmina exits \(no crash, just plain close\) [\#978](https://github.com/FreeRDP/Remmina/issues/978)
- spice plugin missing on Ubuntu PPA [\#958](https://github.com/FreeRDP/Remmina/issues/958)
- Arch Linux: Unable to load RDP plugin [\#931](https://github.com/FreeRDP/Remmina/issues/931)
- Not work Copy&paste between Keepass2 and Remmina [\#900](https://github.com/FreeRDP/Remmina/issues/900)
- Can't connect to certain hosts after update [\#855](https://github.com/FreeRDP/Remmina/issues/855)
- Huge icons in toolbar in some desktop environments [\#826](https://github.com/FreeRDP/Remmina/issues/826)
- Regression in RDP plugin - no ipv6 support [\#528](https://github.com/FreeRDP/Remmina/issues/528)
- RDP on Custom Port [\#237](https://github.com/FreeRDP/Remmina/issues/237)
- Provide a different icon for the tray icon [\#225](https://github.com/FreeRDP/Remmina/issues/225)
- Segfault after login by rdp [\#200](https://github.com/FreeRDP/Remmina/issues/200)
- Remmina in Fullscreen does hide Panels on other unused monitors in gnome [\#188](https://github.com/FreeRDP/Remmina/issues/188)
- Regression: Connections scrolling dissapeared [\#185](https://github.com/FreeRDP/Remmina/issues/185)
- Remmina Hangs/Stucks at second login to RD Server [\#179](https://github.com/FreeRDP/Remmina/issues/179)
- Cant see the Windows login screen while RDPing to a machine [\#174](https://github.com/FreeRDP/Remmina/issues/174)
- LIBVNCSERVER\_WITH\_CLIENT\_TLS is never defined [\#173](https://github.com/FreeRDP/Remmina/issues/173)
- Cannot connect to VNC servers using IPv6 [\#170](https://github.com/FreeRDP/Remmina/issues/170)
- Connection window disappears when minimized  [\#155](https://github.com/FreeRDP/Remmina/issues/155)
- build fails with -DWITH\_GETTEXT=OFF [\#142](https://github.com/FreeRDP/Remmina/issues/142)
- remmina rdp session freeze or hide [\#137](https://github.com/FreeRDP/Remmina/issues/137)
- Cut & Paste [\#106](https://github.com/FreeRDP/Remmina/issues/106)
- Vertical resolution of RDP host not being determined correctly [\#81](https://github.com/FreeRDP/Remmina/issues/81)
- Regression: In remmina 1.0 tray menu right click editing broken [\#61](https://github.com/FreeRDP/Remmina/issues/61)
- do ssh server host key check with the NX plugin [\#18](https://github.com/FreeRDP/Remmina/issues/18)

**Merged pull requests:**

- Fixes workspace detection outside X11 backend. [\#1036](https://github.com/FreeRDP/Remmina/pull/1036) ([giox069](https://github.com/giox069))
- Man Pages as per \#952 request [\#1033](https://github.com/FreeRDP/Remmina/pull/1033) ([antenore](https://github.com/antenore))
- Updated Hungarian translation [\#1032](https://github.com/FreeRDP/Remmina/pull/1032) ([meskobalazs](https://github.com/meskobalazs))
- updated german translation [\#1031](https://github.com/FreeRDP/Remmina/pull/1031) ([morph027](https://github.com/morph027))
- New gettext strings, closes \#1029 [\#1030](https://github.com/FreeRDP/Remmina/pull/1030) ([antenore](https://github.com/antenore))
- Improves focus in/out detection when keyboard is captured [\#1025](https://github.com/FreeRDP/Remmina/pull/1025) ([giox069](https://github.com/giox069))
- Show information on upgrade libssh to 0.7.X [\#1017](https://github.com/FreeRDP/Remmina/pull/1017) ([e-alfred](https://github.com/e-alfred))
- Update French translation [\#1014](https://github.com/FreeRDP/Remmina/pull/1014) ([DevDef](https://github.com/DevDef))
- Implementing RDP remote scaling and orientation [\#979](https://github.com/FreeRDP/Remmina/pull/979) ([giox069](https://github.com/giox069))
- Grab focus without selecting text in the remmina file editor [\#976](https://github.com/FreeRDP/Remmina/pull/976) ([antenore](https://github.com/antenore))
- make gettext \*really\* behave correctly [\#972](https://github.com/FreeRDP/Remmina/pull/972) ([diogocp](https://github.com/diogocp))
- Update name [\#971](https://github.com/FreeRDP/Remmina/pull/971) ([qwertos](https://github.com/qwertos))

## [v1.2.0-rcgit.15](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.15) (2016-08-17)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/v1.2.0-rcgit.14...v1.2.0-rcgit.15)

**Implemented enhancements:**

- No dark tray icon [\#905](https://github.com/FreeRDP/Remmina/issues/905)

**Fixed bugs:**

- Systray does not show the remmina applet icon in Plasma 5.7 - therefore no connect menu [\#944](https://github.com/FreeRDP/Remmina/issues/944)

**Closed issues:**

- Unicode copy leads to crash [\#967](https://github.com/FreeRDP/Remmina/issues/967)
- Remmina can no longer detect bad RDP credentials [\#960](https://github.com/FreeRDP/Remmina/issues/960)
- Import Plugin Dialog [\#954](https://github.com/FreeRDP/Remmina/issues/954)
- SSH does not try IPv4 after IPv6 fails \(when DNS has addresses for both\) [\#953](https://github.com/FreeRDP/Remmina/issues/953)
- RDP reconnect extra warning at the end [\#929](https://github.com/FreeRDP/Remmina/issues/929)
- Invisible Add-Button due to color [\#924](https://github.com/FreeRDP/Remmina/issues/924)
- View bug [\#920](https://github.com/FreeRDP/Remmina/issues/920)
- SSH - Blank Window After Upgrading Fedora 23 to 24 [\#913](https://github.com/FreeRDP/Remmina/issues/913)
- Don't compile on FreeBSD [\#911](https://github.com/FreeRDP/Remmina/issues/911)
- missing remmina settings icon ubuntu 14.04 [\#906](https://github.com/FreeRDP/Remmina/issues/906)
- apt-get fresh install error on Ubuntu MATE 15.10 [\#903](https://github.com/FreeRDP/Remmina/issues/903)
- remmina-1.2 SSH support in Fedora-24beta totally broken [\#899](https://github.com/FreeRDP/Remmina/issues/899)
- Segmentation Fault on FreeBSD using SPICE [\#876](https://github.com/FreeRDP/Remmina/issues/876)
- Error when using clipboard sync wiht windows 2012R2 [\#821](https://github.com/FreeRDP/Remmina/issues/821)
- RDP Clipboard issue with 1.2.0-rcgit.10 [\#809](https://github.com/FreeRDP/Remmina/issues/809)
- FreeBSD - error: no member named 'sftp\_client\_confirm\_res ume' in 'union remmina\_masterthread\_exec\_data [\#431](https://github.com/FreeRDP/Remmina/issues/431)

**Merged pull requests:**

- Spice package and integrated debian packaging. [\#964](https://github.com/FreeRDP/Remmina/pull/964) ([nastasi](https://github.com/nastasi))
- .travis.yml: Add a missing dependency [\#963](https://github.com/FreeRDP/Remmina/pull/963) ([dshcherb](https://github.com/dshcherb))
- spice: add support for tls encrypted connections [\#962](https://github.com/FreeRDP/Remmina/pull/962) ([larchunix](https://github.com/larchunix))
- Update AUTHORS [\#959](https://github.com/FreeRDP/Remmina/pull/959) ([antenore](https://github.com/antenore))
- Fix import label [\#957](https://github.com/FreeRDP/Remmina/pull/957) ([Justinzobel](https://github.com/Justinzobel))
- Updated Hungarian translation [\#949](https://github.com/FreeRDP/Remmina/pull/949) ([meskobalazs](https://github.com/meskobalazs))
- Add missing ClientFormatListResponse\(\) call in RDP plugin clipboard, … [\#948](https://github.com/FreeRDP/Remmina/pull/948) ([giox069](https://github.com/giox069))
- RDP: allow disabling auto reconnection in .remmina file [\#947](https://github.com/FreeRDP/Remmina/pull/947) ([xhaakon](https://github.com/xhaakon))
- Embed docs [\#945](https://github.com/FreeRDP/Remmina/pull/945) ([nastasi](https://github.com/nastasi))
- Allow disabling libsecret dependency [\#942](https://github.com/FreeRDP/Remmina/pull/942) ([diogocp](https://github.com/diogocp))
- add shortcuts to show remote desktop edges [\#940](https://github.com/FreeRDP/Remmina/pull/940) ([nastasi](https://github.com/nastasi))
- Fix a possible crash when changing gtk\_tree\_model [\#928](https://github.com/FreeRDP/Remmina/pull/928) ([giox069](https://github.com/giox069))
- Inverted tray icon for light theme [\#907](https://github.com/FreeRDP/Remmina/pull/907) ([antenore](https://github.com/antenore))

## [v1.2.0-rcgit.14](https://github.com/FreeRDP/Remmina/tree/v1.2.0-rcgit.14) (2016-06-15)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0.rcgit.13...v1.2.0-rcgit.14)

**Implemented enhancements:**

- No more keyboard shortcuts for quick search [\#887](https://github.com/FreeRDP/Remmina/issues/887)
- Support opening rdp files [\#47](https://github.com/FreeRDP/Remmina/issues/47)

**Fixed bugs:**

- Remmina closes even if I press "No" when it asks if I'm sure [\#888](https://github.com/FreeRDP/Remmina/issues/888)

**Closed issues:**

- RemoteApp support [\#898](https://github.com/FreeRDP/Remmina/issues/898)
- I do not see the button "Creane a new connection profile" [\#897](https://github.com/FreeRDP/Remmina/issues/897)
- Error when compiling Remmina [\#848](https://github.com/FreeRDP/Remmina/issues/848)

**Merged pull requests:**

- Adds monochrome tray icons [\#901](https://github.com/FreeRDP/Remmina/pull/901) ([wa4557](https://github.com/wa4557))
- Don't quit Remmina if the user denies closing the connection window [\#895](https://github.com/FreeRDP/Remmina/pull/895) ([larchunix](https://github.com/larchunix))
- Adds support for themable panel icons [\#894](https://github.com/FreeRDP/Remmina/pull/894) ([wa4557](https://github.com/wa4557))
- added \n in the credits [\#893](https://github.com/FreeRDP/Remmina/pull/893) ([jgjimenez](https://github.com/jgjimenez))

## [1.2.0.rcgit.13](https://github.com/FreeRDP/Remmina/tree/1.2.0.rcgit.13) (2016-06-02)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0.rcgit.12...1.2.0.rcgit.13)

**Implemented enhancements:**

- RDP: Fix extended mouse event to register the click for forward/back buttons [\#638](https://github.com/FreeRDP/Remmina/issues/638)
- Remmina main fixes [\#891](https://github.com/FreeRDP/Remmina/pull/891) ([antenore](https://github.com/antenore))
- Remmina Main Window Refactoring [\#875](https://github.com/FreeRDP/Remmina/pull/875) ([antenore](https://github.com/antenore))

**Fixed bugs:**

- \[remmina\_main\] Quick connect bar not visible [\#878](https://github.com/FreeRDP/Remmina/issues/878)
- Remmina doesn't support ssh-rsa hostkeys [\#647](https://github.com/FreeRDP/Remmina/issues/647)
- Toolbar buttons initially enabled in the main window [\#467](https://github.com/FreeRDP/Remmina/issues/467)

**Closed issues:**

- Remmina SPICE doesn't show the connections anymore [\#885](https://github.com/FreeRDP/Remmina/issues/885)
- Only access via Administrator account \(Raspbian -\> Windows 10\) [\#880](https://github.com/FreeRDP/Remmina/issues/880)
- rdp attempt from ubuntu xenial to win 10 [\#868](https://github.com/FreeRDP/Remmina/issues/868)
- Location of saved connections is wrong in Wiki [\#866](https://github.com/FreeRDP/Remmina/issues/866)
- compile remmina not working [\#771](https://github.com/FreeRDP/Remmina/issues/771)
- Grayed buttons if no entry or if none selected [\#488](https://github.com/FreeRDP/Remmina/issues/488)
- Hide quick connect bar by default [\#421](https://github.com/FreeRDP/Remmina/issues/421)
- Can't execute the last GIT with GTK2 \(undefined symbol: gdk\_pixbuf\_get\_from\_surface\) [\#220](https://github.com/FreeRDP/Remmina/issues/220)

**Merged pull requests:**

- Fix some memory leaks [\#890](https://github.com/FreeRDP/Remmina/pull/890) ([giox069](https://github.com/giox069))
- Translations ready [\#889](https://github.com/FreeRDP/Remmina/pull/889) ([jgjimenez](https://github.com/jgjimenez))
- Translations [\#884](https://github.com/FreeRDP/Remmina/pull/884) ([antenore](https://github.com/antenore))
- Spice plugin: smartcard redirection support [\#882](https://github.com/FreeRDP/Remmina/pull/882) ([larchunix](https://github.com/larchunix))
- SPICE plugin: USB redirection support + minor fixes [\#881](https://github.com/FreeRDP/Remmina/pull/881) ([larchunix](https://github.com/larchunix))
- Spice plugin: scaling support  [\#879](https://github.com/FreeRDP/Remmina/pull/879) ([larchunix](https://github.com/larchunix))
- SPICE plugin improvements \(again\) [\#877](https://github.com/FreeRDP/Remmina/pull/877) ([larchunix](https://github.com/larchunix))
- More SPICE plugin improvements [\#874](https://github.com/FreeRDP/Remmina/pull/874) ([larchunix](https://github.com/larchunix))
- SPICE plugin improvements [\#872](https://github.com/FreeRDP/Remmina/pull/872) ([larchunix](https://github.com/larchunix))
- Initial support for Wayland and Mir [\#871](https://github.com/FreeRDP/Remmina/pull/871) ([giox069](https://github.com/giox069))
- New plugin with basic support for the SPICE protocol [\#870](https://github.com/FreeRDP/Remmina/pull/870) ([larchunix](https://github.com/larchunix))

## [1.2.0.rcgit.12](https://github.com/FreeRDP/Remmina/tree/1.2.0.rcgit.12) (2016-05-17)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0.rcgit.11...1.2.0.rcgit.12)

**Implemented enhancements:**

- \[Feature Request\] Focus on 'server' box during 'New Connection' [\#838](https://github.com/FreeRDP/Remmina/issues/838)
- GTK 3.20 VTE3 Remmina shows blank terminal [\#835](https://github.com/FreeRDP/Remmina/issues/835)
- How do I select all text in SSH terminal session ? [\#833](https://github.com/FreeRDP/Remmina/issues/833)
- Add a issues-templates to the project [\#822](https://github.com/FreeRDP/Remmina/issues/822)
- Quick Find cleanups [\#791](https://github.com/FreeRDP/Remmina/issues/791)
- Remmina crashed when Windows Server TS Session is Remote Controlled [\#621](https://github.com/FreeRDP/Remmina/issues/621)
- Quickfind fixes - Close \#791 [\#811](https://github.com/FreeRDP/Remmina/pull/811) ([antenore](https://github.com/antenore))

**Fixed bugs:**

- 100% CPU Usage [\#842](https://github.com/FreeRDP/Remmina/issues/842)
- Screenshot crash under Ubuntu 16.04 [\#836](https://github.com/FreeRDP/Remmina/issues/836)
- Remmina main connection window loose it's minimum height when the toolbar is hidden [\#829](https://github.com/FreeRDP/Remmina/issues/829)
- Unable to connect to Win7 PCs via RDESKTOP with Remmina 1.2.0-rcgit.11 [\#823](https://github.com/FreeRDP/Remmina/issues/823)
- Problem with changing key assignment in the "Keyboard" tab \(rcgit.11\) [\#819](https://github.com/FreeRDP/Remmina/issues/819)
- Remmina forces its windows to appear at the center of the screen [\#817](https://github.com/FreeRDP/Remmina/issues/817)
- Quick Find cleanups [\#791](https://github.com/FreeRDP/Remmina/issues/791)
- Quickfind fixes - Close \\#791 [\#811](https://github.com/FreeRDP/Remmina/pull/811) ([antenore](https://github.com/antenore))

**Closed issues:**

- Can't move tabs between windows [\#861](https://github.com/FreeRDP/Remmina/issues/861)
- Ctrl + c will not stop processes anymore \(SSH\) [\#858](https://github.com/FreeRDP/Remmina/issues/858)
- Continual flicker due to redraw \(VNC from Windows to Ubuntu\) [\#857](https://github.com/FreeRDP/Remmina/issues/857)
- vim doesn't look well in fullscreen mode [\#856](https://github.com/FreeRDP/Remmina/issues/856)
- Quick find is slow with a lot of profiles [\#852](https://github.com/FreeRDP/Remmina/issues/852)
- Cannot break from a command using CTRL + C since enhancement 833 was implemented [\#847](https://github.com/FreeRDP/Remmina/issues/847)
- Cannot redirect microphone to VDI- Win10 HyperV- Raspberry Pi3- Rasbian Jessie [\#846](https://github.com/FreeRDP/Remmina/issues/846)
- Drop libvte-2.90 support? [\#843](https://github.com/FreeRDP/Remmina/issues/843)
- Crashing after update [\#840](https://github.com/FreeRDP/Remmina/issues/840)
- Please make survey & webkit-gtk dependency optional [\#812](https://github.com/FreeRDP/Remmina/issues/812)
- Japanese keyboard mapping [\#805](https://github.com/FreeRDP/Remmina/issues/805)
- Remmina crashes taking a screenshot [\#803](https://github.com/FreeRDP/Remmina/issues/803)
- Remmina crash on start [\#800](https://github.com/FreeRDP/Remmina/issues/800)
- \\tsclient shares list blank files and folders [\#799](https://github.com/FreeRDP/Remmina/issues/799)
- Remmina Crashes [\#797](https://github.com/FreeRDP/Remmina/issues/797)
- Can't hear remote audio \(local\) [\#790](https://github.com/FreeRDP/Remmina/issues/790)
- RDP Connection fails, but only the first time [\#789](https://github.com/FreeRDP/Remmina/issues/789)
- Interface bug: edit window [\#491](https://github.com/FreeRDP/Remmina/issues/491)

**Merged pull requests:**

- Fix a few typos in README.md [\#851](https://github.com/FreeRDP/Remmina/pull/851) ([ivuk](https://github.com/ivuk))
- Focus server box [\#845](https://github.com/FreeRDP/Remmina/pull/845) ([antenore](https://github.com/antenore))
- Screenshot segfault [\#841](https://github.com/FreeRDP/Remmina/pull/841) ([giox069](https://github.com/giox069))
- Vte improvements - popup menu and terminal selection [\#834](https://github.com/FreeRDP/Remmina/pull/834) ([antenore](https://github.com/antenore))
- fix loadbalanceinfo in import [\#828](https://github.com/FreeRDP/Remmina/pull/828) ([koter84](https://github.com/koter84))
- WM hints and Window positioning [\#827](https://github.com/FreeRDP/Remmina/pull/827) ([antenore](https://github.com/antenore))
- Added issue-template [\#825](https://github.com/FreeRDP/Remmina/pull/825) ([weberhofer](https://github.com/weberhofer))
- Update Uzbek translation [\#824](https://github.com/FreeRDP/Remmina/pull/824) ([ozbek](https://github.com/ozbek))
- Make survey & webkit-gtk dependency optional [\#813](https://github.com/FreeRDP/Remmina/pull/813) ([antenore](https://github.com/antenore))
- VNC - HandleRFBServerMessage return status [\#804](https://github.com/FreeRDP/Remmina/pull/804) ([antenore](https://github.com/antenore))

## [1.2.0.rcgit.11](https://github.com/FreeRDP/Remmina/tree/1.2.0.rcgit.11) (2016-03-17)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0.rcgit.10...1.2.0.rcgit.11)

**Implemented enhancements:**

- Scrollable profile editor [\#801](https://github.com/FreeRDP/Remmina/issues/801)
- Screenshot of remote machine \[feature request\] [\#644](https://github.com/FreeRDP/Remmina/issues/644)

**Closed issues:**

- Remmina is unable to access saved passwords immediately after local PC logon [\#795](https://github.com/FreeRDP/Remmina/issues/795)
- 1.2.0-rcgit.10 \(git n/a\) vnc connector not installed [\#794](https://github.com/FreeRDP/Remmina/issues/794)
- \[1.2.0-rcgit.9\] Unable to Compile in Centos 7 [\#793](https://github.com/FreeRDP/Remmina/issues/793)
- Remmina crash on having files or images in clipboard [\#792](https://github.com/FreeRDP/Remmina/issues/792)
- Connection search doesn't work [\#773](https://github.com/FreeRDP/Remmina/issues/773)

**Merged pull requests:**

- Remote machine screenshot \[feature request\] \#644 [\#802](https://github.com/FreeRDP/Remmina/pull/802) ([antenore](https://github.com/antenore))
- Don't unlock keyring when libsecret is \< 0.18 [\#796](https://github.com/FreeRDP/Remmina/pull/796) ([giox069](https://github.com/giox069))

## [1.2.0.rcgit.10](https://github.com/FreeRDP/Remmina/tree/1.2.0.rcgit.10) (2016-03-08)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0-rcgit.9...1.2.0.rcgit.10)

**Fixed bugs:**

- Selected item in the remmina\_main window is lost after editing a profile [\#786](https://github.com/FreeRDP/Remmina/issues/786)
- Clipboard + Windows 2008 Server R2 SP1 + Google Chrome [\#583](https://github.com/FreeRDP/Remmina/issues/583)

**Closed issues:**

- Is remmina between Ubuntu and Windows 10 secure? [\#782](https://github.com/FreeRDP/Remmina/issues/782)
- Latest version of remmina segfaults when closing remmina's main windows while having an open connection [\#744](https://github.com/FreeRDP/Remmina/issues/744)
- Clipboard doesn't work [\#730](https://github.com/FreeRDP/Remmina/issues/730)

**Merged pull requests:**

- Added ctrl+f to Quick Search to supersed the standard GTK accelerator [\#788](https://github.com/FreeRDP/Remmina/pull/788) ([antenore](https://github.com/antenore))
- Selected item is lost after editing a profile. Closes \#786 [\#787](https://github.com/FreeRDP/Remmina/pull/787) ([antenore](https://github.com/antenore))
- formats is not nulled upon failure [\#781](https://github.com/FreeRDP/Remmina/pull/781) ([weberhofer](https://github.com/weberhofer))
- pos\_cache not nulled upon realloc failure [\#780](https://github.com/FreeRDP/Remmina/pull/780) ([weberhofer](https://github.com/weberhofer))
- Freebsd support [\#779](https://github.com/FreeRDP/Remmina/pull/779) ([antenore](https://github.com/antenore))

## [1.2.0-rcgit.9](https://github.com/FreeRDP/Remmina/tree/1.2.0-rcgit.9) (2016-02-28)
[Full Changelog](https://github.com/FreeRDP/Remmina/compare/1.2.0-rcgit.8...1.2.0-rcgit.9)

**Fixed bugs:**

- Num lock off [\#389](https://github.com/FreeRDP/Remmina/issues/389)

**Closed issues:**

- Share some usage statistics dialog cannot be disabled!!! [\#772](https://github.com/FreeRDP/Remmina/issues/772)
- vnc plugin not found [\#768](https://github.com/FreeRDP/Remmina/issues/768)
- Selected file after editing is wrong [\#761](https://github.com/FreeRDP/Remmina/issues/761)
- SSH cursor and scrolling / display issues [\#760](https://github.com/FreeRDP/Remmina/issues/760)
- NumLock in RDP sessions [\#758](https://github.com/FreeRDP/Remmina/issues/758)
- Buttons not shown inside RDP Advanced tab on smal screens [\#757](https://github.com/FreeRDP/Remmina/issues/757)
- Can't SSH tunnel over Remmina \(RDP\) [\#756](https://github.com/FreeRDP/Remmina/issues/756)
- Remmina closes unexpectedly [\#753](https://github.com/FreeRDP/Remmina/issues/753)
- Feature Request: After Command \(opposite to precommand\) [\#746](https://github.com/FreeRDP/Remmina/issues/746)
- mistake. [\#739](https://github.com/FreeRDP/Remmina/issues/739)
- Currently rdp\_event.c.o can not be built [\#732](https://github.com/FreeRDP/Remmina/issues/732)
- Scrolled fullscreen mode does not work [\#729](https://github.com/FreeRDP/Remmina/issues/729)
- RDP session is dropped from time to time, reproducible situation. [\#723](https://github.com/FreeRDP/Remmina/issues/723)
- RDP plugin fails to load [\#721](https://github.com/FreeRDP/Remmina/issues/721)
- \[Info Req\] RDP v7  [\#719](https://github.com/FreeRDP/Remmina/issues/719)
- Suddenly fails to RDP to any server  [\#717](https://github.com/FreeRDP/Remmina/issues/717)
- Constantly and often breaks the connection. It started about 1-2 months ago. [\#710](https://github.com/FreeRDP/Remmina/issues/710)
- No RDP connection after latest update Ubuntu [\#657](https://github.com/FreeRDP/Remmina/issues/657)
- Fullscreen windows open on the monitor next to them [\#580](https://github.com/FreeRDP/Remmina/issues/580)
- fullscreen multiple screens [\#577](https://github.com/FreeRDP/Remmina/issues/577)
- Missing controls tab from top/center of window in Gnome [\#481](https://github.com/FreeRDP/Remmina/issues/481)
- ALT + F4 closes Remina remote Window [\#125](https://github.com/FreeRDP/Remmina/issues/125)

**Merged pull requests:**

- Autoreconnect [\#776](https://github.com/FreeRDP/Remmina/pull/776) ([giox069](https://github.com/giox069))
- Fix for a black border \(GTK undershoot\) appering from GTK 3.18 [\#767](https://github.com/FreeRDP/Remmina/pull/767) ([giox069](https://github.com/giox069))
- Using compact settings for RDP plugin closes Issue 757 [\#759](https://github.com/FreeRDP/Remmina/pull/759) ([antenore](https://github.com/antenore))
- Fixes for issue \#744 [\#752](https://github.com/FreeRDP/Remmina/pull/752) ([giox069](https://github.com/giox069))
- adjust lenght of strings [\#749](https://github.com/FreeRDP/Remmina/pull/749) ([weberhofer](https://github.com/weberhofer))
- Feature Request: After Command \#746 [\#748](https://github.com/FreeRDP/Remmina/pull/748) ([antenore](https://github.com/antenore))
- Fix compiler warnings [\#743](https://github.com/FreeRDP/Remmina/pull/743) ([weberhofer](https://github.com/weberhofer))
- rdp-plugin requires x11 libraries [\#742](https://github.com/FreeRDP/Remmina/pull/742) ([weberhofer](https://github.com/weberhofer))
- Match remmina\_survey\_start declarations [\#741](https://github.com/FreeRDP/Remmina/pull/741) ([weberhofer](https://github.com/weberhofer))
- Exec if only when "name" has been initialized [\#740](https://github.com/FreeRDP/Remmina/pull/740) ([weberhofer](https://github.com/weberhofer))
- XDG - make remmina user data dir global [\#738](https://github.com/FreeRDP/Remmina/pull/738) ([antenore](https://github.com/antenore))
- Fixes for Ubuntu 14.04 compatibility [\#737](https://github.com/FreeRDP/Remmina/pull/737) ([giox069](https://github.com/giox069))
- Make tool\_hello\_world compliant with our plugins model [\#735](https://github.com/FreeRDP/Remmina/pull/735) ([antenore](https://github.com/antenore))
- Added user survey, community links and fixed all deprecations warnings [\#734](https://github.com/FreeRDP/Remmina/pull/734) ([antenore](https://github.com/antenore))
- Align library name with latest freerdp master [\#731](https://github.com/FreeRDP/Remmina/pull/731) ([weberhofer](https://github.com/weberhofer))
- Added microphone redirection [\#727](https://github.com/FreeRDP/Remmina/pull/727) ([akallabeth](https://github.com/akallabeth))
- RDP plugin update client main loop to new API [\#726](https://github.com/FreeRDP/Remmina/pull/726) ([akallabeth](https://github.com/akallabeth))
- Make Remmina really exit when you choose Exit or Quit [\#720](https://github.com/FreeRDP/Remmina/pull/720) ([giox069](https://github.com/giox069))
- Remove some compiler warnings [\#716](https://github.com/FreeRDP/Remmina/pull/716) ([jviksell](https://github.com/jviksell))

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
- Fix issue with invisible toolbar in fullscreen. [\#275](https://github.com/FreeRDP/Remmina/pull/275) ([jeebsorg](https://github.com/jeebsorg))
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