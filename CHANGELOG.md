## v1.4.20
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.19...v1.4.20)

* Mark appindicator as required [!2290](https://gitlab.com/Remmina/Remmina/merge_requests/2290) *@antenore*
* Disabling XDMCP, NX, and ST [!2291](https://gitlab.com/Remmina/Remmina/merge_requests/2291) *@antenore*
* Remove plugins/st,xdmcp,nx for submodule replacement [!2292](https://gitlab.com/Remmina/Remmina/merge_requests/2292) *@antenore*
* SSH tunnel MFA [!2293](https://gitlab.com/Remmina/Remmina/merge_requests/2293) *@antenore*
* Adding connection profiles menu into the toolbar [!2295](https://gitlab.com/Remmina/Remmina/merge_requests/2295) *@antenore*
* Resolve "Preferences buttons not working since v1.4.19" [!2296](https://gitlab.com/Remmina/Remmina/merge_requests/2296) *@antenore*

## v1.4.19
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.18...v1.4.19)

* Fix Freerdp Git Revision [!2277](https://gitlab.com/Remmina/Remmina/merge_requests/2277) *@matty-r*
* UI improvements and cleanup [!2278](https://gitlab.com/Remmina/Remmina/merge_requests/2278) *@antenore*
* Desktop integration for the Remmina SNAP [!2279](https://gitlab.com/Remmina/Remmina/merge_requests/2279) *@antenore*
* Add process-control to the remmina snap [!2276](https://gitlab.com/Remmina/Remmina/merge_requests/2276) *@antenore*
* Adding SSH_AGENT support to the snap package [!2280](https://gitlab.com/Remmina/Remmina/merge_requests/2280) *@antenore*
* Adding option to disable smooth scrolling [!2281](https://gitlab.com/Remmina/Remmina/merge_requests/2281) *@antenore*
* Scrolled Viewport: use viewport_motion_handler as the only timeout indicator [!2282](https://gitlab.com/Remmina/Remmina/merge_requests/2282) *@cth451*
* Adding TCP redirection through rdp2tcp [!2283](https://gitlab.com/Remmina/Remmina/merge_requests/2283) *@antenore*
* Added setting for RDP number of reconnect attempts [!2284](https://gitlab.com/Remmina/Remmina/merge_requests/2284) *@antenore*
* Add RDP reconnect interrupt on window close, fix crash introduced with 7c13b918. Should fix #2079 [!2286](https://gitlab.com/Remmina/Remmina/merge_requests/2286) *@giox069*
* Removing GtkStatusIcon as deprecated [!2285](https://gitlab.com/Remmina/Remmina/merge_requests/2285) *@antenore*
* Adding advanced option to share multiple folders [!2287](https://gitlab.com/Remmina/Remmina/merge_requests/2287) *@antenore*
* Profile list grabs the focus when search is hidden [!2288](https://gitlab.com/Remmina/Remmina/merge_requests/2288) *@antenore*

## v1.4.18
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.17...v1.4.18)

* [SNAP] Removing unsupported architectures [!2268](https://gitlab.com/Remmina/Remmina/merge_requests/2268) *@antenore*
* Try more shells as launcher if default isn't found [!2269](https://gitlab.com/Remmina/Remmina/merge_requests/2269) *@cirelli94*
* Minor fixes for v1.4.17 [!2270](https://gitlab.com/Remmina/Remmina/merge_requests/2270) *@antenore*
* SSH session improvements [!2271](https://gitlab.com/Remmina/Remmina/merge_requests/2271) *@antenore*
* Fixes - Auto-start file created on tray icon disabled [!2272](https://gitlab.com/Remmina/Remmina/merge_requests/2272) *@antenore*
* RDP: Remove older usage of ClientHostname *@giox069*
* Fix libfreerdp version check *@giox069*
* Explicitly set user resolution to a multiple of 4 [!2273](https://gitlab.com/Remmina/Remmina/merge_requests/2273) *@antenore*
* Code refactoring - ASAN exceptions [!2274](https://gitlab.com/Remmina/Remmina/merge_requests/2274) *@antenore*

## v1.4.17
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.16...v1.4.17)

* Fix build with musl libc [!2259](https://gitlab.com/Remmina/Remmina/merge_requests/2259) *@ncopa*
* Fix typos [!2260](https://gitlab.com/Remmina/Remmina/merge_requests/2260) *@mfvescovi*
* Improving CI cache [!2257](https://gitlab.com/Remmina/Remmina/merge_requests/2257) *@antenore*
* Fix System Tray Icon Broken/Missing [!2261](https://gitlab.com/Remmina/Remmina/merge_requests/2261) *@antenore*
* VNC quality deafults now to good [!2264](https://gitlab.com/Remmina/Remmina/merge_requests/2264) *@antenore*
* Flatpak refactoring [!2262](https://gitlab.com/Remmina/Remmina/merge_requests/2262) *@antenore*
* Adding Gateway websocket support [!2263](https://gitlab.com/Remmina/Remmina/merge_requests/2263) *@antenore*
* Revert "Linking snap and flatpak to FreeRDP 2.3.1" [!2265](https://gitlab.com/Remmina/Remmina/merge_requests/2265) *@antenore*
* Set FreeRDP config path to Remmina profiles path [!2266](https://gitlab.com/Remmina/Remmina/merge_requests/2266) *@antenore*

## v1.4.16
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.15...v1.4.16)

* Fix Data PATH for the FreeRDP files bcf24360e05d2f9b60f9f0adaf56dede66497a42 @antenore

## v1.4.15
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.14...v1.4.15)

* Fixing SSH plugin color palette initialization. [!2255](https://gitlab.com/Remmina/Remmina/merge_requests/2255) *@antenore*

## v1.4.14
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.13...v1.4.14)

* [VNC] - Ignore remote Bell option and other fixes [!2237](https://gitlab.com/Remmina/Remmina/merge_requests/2237) *@antenore*
* Fixing color palette size for themed SSH [!2253](https://gitlab.com/Remmina/Remmina/merge_requests/2253) *@antenore*
* Bump FreeRDP version to 2.3.2 [!2226](https://gitlab.com/Remmina/Remmina/merge_requests/2226) *@antenore*
* Fixes search bar shortcuts wrong bahavior [!2227](https://gitlab.com/Remmina/Remmina/merge_requests/2227) *@antenore*
* Honour theme settings when run from command line [!2251](https://gitlab.com/Remmina/Remmina/merge_requests/2251) *@antenore*
* FTP UI improvements [!2228](https://gitlab.com/Remmina/Remmina/merge_requests/2228) *@antenore*
* Experimental VNC plugin using GTK-VNC [!2248](https://gitlab.com/Remmina/Remmina/merge_requests/2248) *@antenore*
* Config SSH tunnel username takes precedence. [!2231](https://gitlab.com/Remmina/Remmina/merge_requests/2231) *@matir*
* Allow groups to be expanded and collapsed by using the keyboard [!2232](https://gitlab.com/Remmina/Remmina/merge_requests/2232) *@xsmile*
* Fixing VNC repeater logic. [!2243](https://gitlab.com/Remmina/Remmina/merge_requests/2243) *@antenore*
* Send text clipboard content as keystrokes [!2238](https://gitlab.com/Remmina/Remmina/merge_requests/2238) *@antenore*
* scrolled viewport: explicitly recheck whether the timeout is active [!2233](https://gitlab.com/Remmina/Remmina/merge_requests/2233) *@cth451*
* Resolve Host+Page_Down triggers search text in SSH [!2240](https://gitlab.com/Remmina/Remmina/merge_requests/2240) *@antenore*
* UNIX sockets initial support [!2250](https://gitlab.com/Remmina/Remmina/merge_requests/2250) *@antenore*
* Fixed wrong freerdp_settings function use [!2234](https://gitlab.com/Remmina/Remmina/merge_requests/2234) *@akallabeth*
* Fixing RemminaConnectionWindow map/unmap events [!2245](https://gitlab.com/Remmina/Remmina/merge_requests/2245) *@antenore*
* Spelling: Comma-separated, List monitor IDs [!2235](https://gitlab.com/Remmina/Remmina/merge_requests/2235) *@kingu*
* Set Remmina specific FreeRDP config data folder [!2236](https://gitlab.com/Remmina/Remmina/merge_requests/2236) *@antenore*
* Optional port connection instead of server [!2239](https://gitlab.com/Remmina/Remmina/merge_requests/2239) *@kingu*
* Resolve "Use LZO compression for Snap to improve startup speed" [!2241](https://gitlab.com/Remmina/Remmina/merge_requests/2241) *@antenore*
* Make wayland not mandatory during compile time [!2246](https://gitlab.com/Remmina/Remmina/merge_requests/2246) *@antenore*
* Do not use alpha as it is not used for the Desktop [!2247](https://gitlab.com/Remmina/Remmina/merge_requests/2247) *@antenore*
* Refactoring: Deprecations and warnings [!2249](https://gitlab.com/Remmina/Remmina/merge_requests/2249) *@antenore*
* Removing unneeded widgets in the headerbar [!2252](https://gitlab.com/Remmina/Remmina/merge_requests/2252) *@antenore*

## v1.4.13
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.12...v1.4.13)

* Resolve "Can't build Remmina-v1.4.12 on openSUSE versions" [!2217](https://gitlab.com/Remmina/Remmina/merge_requests/2217) *@antenore*
* Adding wayland include dirs [!2218](https://gitlab.com/Remmina/Remmina/merge_requests/2218) *@antenore*
* Use freerdp_settings_get|set API [!2216](https://gitlab.com/Remmina/Remmina/merge_requests/2216) *@akallabeth*
* Fixing Can't build Remmina-v1.4.12 on openSUSE versions / SPICE [!2219](https://gitlab.com/Remmina/Remmina/merge_requests/2219) *@antenore*
* Fixed typo in reconnection attempt in the norwegian translation [!2221](https://gitlab.com/Remmina/Remmina/merge_requests/2221) *@ragnarstolsmark*
* Change "The news are turned off" to the more grammatically correct "News is... [!2220](https://gitlab.com/Remmina/Remmina/merge_requests/2220) *@ncguk*
* rdp: Allow autoreconnect for ERRINFO_GRAPHICS_SUBSYSTEM_FAILED [!2222](https://gitlab.com/Remmina/Remmina/merge_requests/2222) *@Fantu*
* Disable cert file auth when libssh < 0.9.0 [!2223](https://gitlab.com/Remmina/Remmina/merge_requests/2223) *@antenore*
* Removing redundant subtitle [!2224](https://gitlab.com/Remmina/Remmina/merge_requests/2224) *@antenore*
* Removing redundant ssh_userauth_none [!2225](https://gitlab.com/Remmina/Remmina/merge_requests/2225) *@antenore*

## v1.4.12
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.11...v1.4.12)

* Multi monitor support preview [!2184](https://gitlab.com/Remmina/Remmina/merge_requests/2184) *@antenore*
* Resolve "Left-handed mouse support" [!2200](https://gitlab.com/Remmina/Remmina/merge_requests/2200) *@antenore*
* Refactoring SSH themes [!2205](https://gitlab.com/Remmina/Remmina/merge_requests/2205) *@antenore*
* Crystal clear sound made simple. [!2207](https://gitlab.com/Remmina/Remmina/merge_requests/2207) *@antenore*
* Adding SSH certificate authentication [!2208](https://gitlab.com/Remmina/Remmina/merge_requests/2208) *@antenore*
* Refactoring SSH tunnel authentication. #2414 [!2211](https://gitlab.com/Remmina/Remmina/merge_requests/2211) *@antenore*
* Refactoring: GResource based UI elements [!2201](https://gitlab.com/Remmina/Remmina/merge_requests/2201) *@antenore*
* Invalid connection option ":port" removed [!2193](https://gitlab.com/Remmina/Remmina/merge_requests/2193) *@kingu*
* Add ifdefs for spice version less then 0.38 (fix #2408) [!2195](https://gitlab.com/Remmina/Remmina/merge_requests/2195) *@hadogenes*
* Fix for #2408 building with older SPICE libraries [!2194](https://gitlab.com/Remmina/Remmina/merge_requests/2194) *@jweberhofer*
* Spelling: Bug triaging and labeling labels reworked. [!2192](https://gitlab.com/Remmina/Remmina/merge_requests/2192) *@kingu*
* Ignoring glib functions if glib version older then 2.56 [!2196](https://gitlab.com/Remmina/Remmina/merge_requests/2196) *@antenore*
* Some refactoring and fixes for 1.4.11 [!2198](https://gitlab.com/Remmina/Remmina/merge_requests/2198) *@antenore*
* Revert "rdp/event: Fix wheel value for GDK_SCROLL_DOWN events" [!2199](https://gitlab.com/Remmina/Remmina/merge_requests/2199) *@pnowack*
* Fixing compiler errors related to Python plugin support on master [!2178](https://gitlab.com/Remmina/Remmina/merge_requests/2178) *@ToolsDevler*
* Fixes for multi monitor and weblate [!2202](https://gitlab.com/Remmina/Remmina/merge_requests/2202) *@antenore*
* Spelling: desktop [!2203](https://gitlab.com/Remmina/Remmina/merge_requests/2203) *@kingu*
* Spelling: Colour [!2204](https://gitlab.com/Remmina/Remmina/merge_requests/2204) *@kingu*
* Fixing memory leaks and minor bugs [!2206](https://gitlab.com/Remmina/Remmina/merge_requests/2206) *@antenore*
* Fix minor typos [!2209](https://gitlab.com/Remmina/Remmina/merge_requests/2209) *@yurchor*
* Typo fix, certificat -> certificate [!2210](https://gitlab.com/Remmina/Remmina/merge_requests/2210) *@yarons*
* Improve pre-connection FreeRDP channel initializations [!2212](https://gitlab.com/Remmina/Remmina/merge_requests/2212) *@antenore*

## v1.4.11
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.10...v1.4.11)

* Implementing simple SSH multi factor authentication. [!2162](https://gitlab.com/Remmina/Remmina/merge_requests/2162) *@antenore*
* rdp/cliprdr: Fix header of FormatList message [!2147](https://gitlab.com/Remmina/Remmina/merge_requests/2147) *@pnowack*
* rdp/event: Fix wheel value for GDK_SCROLL_DOWN events [!2149](https://gitlab.com/Remmina/Remmina/merge_requests/2149) *@pnowack*
* Implementing dynamic resolution in SPICE plugin [!2150](https://gitlab.com/Remmina/Remmina/merge_requests/2150) *@hadogenes*
* remove triplicate entry in changelog [!2151](https://gitlab.com/Remmina/Remmina/merge_requests/2151) *@Fantu*
* Resolve "Remmina does not handle `file:///some/path/to/file.rdp` syntax" [!2152](https://gitlab.com/Remmina/Remmina/merge_requests/2152) *@antenore*
* Resolve "SSH tunneling, honoring ssh_config (User, HostKeyAlias, ProxyJump, HostKeyAlgorithms, IdentitiesOnly, etc.)" [!2154](https://gitlab.com/Remmina/Remmina/merge_requests/2154) *@antenore*
* Resolve "While in the main window, bind F10 keyboard shortcut to toggling the main menubutton" [!2156](https://gitlab.com/Remmina/Remmina/merge_requests/2156) *@antenore*
* Add capability to load Python plugins (not finished). [!2157](https://gitlab.com/Remmina/Remmina/merge_requests/2157) *@ToolsDevler*
* Disabling Python support by default [!2158](https://gitlab.com/Remmina/Remmina/merge_requests/2158) *@antenore*
* Suppress Output PDU when the RDP window is not visible [!2159](https://gitlab.com/Remmina/Remmina/merge_requests/2159) *@antenore*
* Refactoring SSH plugin [!2160](https://gitlab.com/Remmina/Remmina/merge_requests/2160) *@antenore*
* Spelling: SGR 1 attribute colour and bold switching [!2163](https://gitlab.com/Remmina/Remmina/merge_requests/2163) *@kingu*
* Remove extra parenthesis [!2164](https://gitlab.com/Remmina/Remmina/merge_requests/2164) *@yurchor*
* Spice option to choose Prefered Video Codec and Image Compressor [!2165](https://gitlab.com/Remmina/Remmina/merge_requests/2165) *@hadogenes*
* Spelling: Protocol info and view strings reworked [!2166](https://gitlab.com/Remmina/Remmina/merge_requests/2166) *@kingu*
* Spelling: Server name placeholder moved [!2167](https://gitlab.com/Remmina/Remmina/merge_requests/2167) *@kingu*
* Spelling: Glade [!2170](https://gitlab.com/Remmina/Remmina/merge_requests/2170) *@kingu*
* Spelling: Start-up [!2171](https://gitlab.com/Remmina/Remmina/merge_requests/2171) *@kingu*
* Spelling: ISO, date and time [!2172](https://gitlab.com/Remmina/Remmina/merge_requests/2172) *@kingu*
* Spelling: Auto-start, auto-scroll, auto-detect [!2173](https://gitlab.com/Remmina/Remmina/merge_requests/2173) *@kingu*
* Spelling: Colour, could not, turn off, don't send [!2168](https://gitlab.com/Remmina/Remmina/merge_requests/2168) *@kingu*
* Spelling: SSH key, first, checksum of, either [!2169](https://gitlab.com/Remmina/Remmina/merge_requests/2169) *@kingu*
* Implementing simple SSH multi factor authentication. [!2162](https://gitlab.com/Remmina/Remmina/merge_requests/2162) *@antenore*
* Correct iterating lines in string - address sanitizer fix #2390 [!2174](https://gitlab.com/Remmina/Remmina/merge_requests/2174) *@hadogenes*
* Correct freeing memory in spice [!2175](https://gitlab.com/Remmina/Remmina/merge_requests/2175) *@hadogenes*
* Issue/2391 randomness [!2176](https://gitlab.com/Remmina/Remmina/merge_requests/2176) *@antenore*
* Resolve "Missing keyboard shortcuts to toggle search (Ctrl+F, Escape), and closing the search doesn't clear the search" [!2179](https://gitlab.com/Remmina/Remmina/merge_requests/2179) *@antenore*
* Resolve "Always false contition in remmina_ssh.c" [!2180](https://gitlab.com/Remmina/Remmina/merge_requests/2180) *@antenore*
* Improving error detection [!2181](https://gitlab.com/Remmina/Remmina/merge_requests/2181) *@antenore*
* Using curly double quotes where possible [!2182](https://gitlab.com/Remmina/Remmina/merge_requests/2182) *@antenore*
* Spelling: SPICE, GStreamer, No, video-codec error [!2183](https://gitlab.com/Remmina/Remmina/merge_requests/2183) *@kingu*
* Command line help improvements [!2185](https://gitlab.com/Remmina/Remmina/merge_requests/2185) *@antenore*
* Fix minor typo [!2186](https://gitlab.com/Remmina/Remmina/merge_requests/2186) *@yurchor*
* Small typos fixed [!2187](https://gitlab.com/Remmina/Remmina/merge_requests/2187) *@kingu*
* Resolve "Remmina Crashes when opening the preferences with the accelerator" [!2189](https://gitlab.com/Remmina/Remmina/merge_requests/2189) *@antenore*
* Fixing #2401 - crash when using ctrl+p [!2190](https://gitlab.com/Remmina/Remmina/merge_requests/2190) *@antenore*
* Refactorin the RCW toolbar to use the right tool items types [!2188](https://gitlab.com/Remmina/Remmina/merge_requests/2188) *@antenore*

## v1.4.10
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.9...v1.4.10)

* Resolve "RDP Black Screen on connection" [!2123](https://gitlab.com/Remmina/Remmina/merge_requests/2123) *@antenore*
* Correctly importing and exporting audiocapturemode, closes #2349 [!2124](https://gitlab.com/Remmina/Remmina/merge_requests/2124) *@antenore*
* Resolve "Auto accept changes to fingerprints and auto accept certificates" [!2126](https://gitlab.com/Remmina/Remmina/merge_requests/2126) *@antenore*
* "Fingerprinters" corrected to "fingerprints". [!2127](https://gitlab.com/Remmina/Remmina/merge_requests/2127) *@kingu*
* Implementing network type option [!2128](https://gitlab.com/Remmina/Remmina/merge_requests/2128) *@antenore*
* Improving the terminal colour file picker [!2129](https://gitlab.com/Remmina/Remmina/merge_requests/2129) *@antenore*
* Resolve "[RDP] Since v1.4.9 Audio is no longer working" [!2130](https://gitlab.com/Remmina/Remmina/merge_requests/2130) *@antenore*
* New connection strings corrected [!2131](https://gitlab.com/Remmina/Remmina/merge_requests/2131) *@kingu*
* Correct location of Terminal colour scheme setting [!2132](https://gitlab.com/Remmina/Remmina/merge_requests/2132) *@kingu*
* Set .gitlab-ci.yml to enable or configure SAST [!2134](https://gitlab.com/Remmina/Remmina/merge_requests/2134) *@antenore*
* Adding missing components in the snap [!2133](https://gitlab.com/Remmina/Remmina/merge_requests/2133) *@antenore*
* Fixing pulseaudio LD_LIBRARY_PATH and staging PA libraries [!2136](https://gitlab.com/Remmina/Remmina/merge_requests/2136) *@antenore*
* fix incorrect name date log sessions ssh [!2137](https://gitlab.com/Remmina/Remmina/merge_requests/2137) *@acendrou*
* Resolve "Strange padding in main window" [!2138](https://gitlab.com/Remmina/Remmina/merge_requests/2138) *@antenore*
* Remove legacy rfx code [!2139](https://gitlab.com/Remmina/Remmina/merge_requests/2139) *@antenore*
* Resolve "RDP export features does not properly include gatewayhostname" [!2140](https://gitlab.com/Remmina/Remmina/merge_requests/2140) *@antenore*
* Fixing snap's pulseaudio and wayland issues [!2142](https://gitlab.com/Remmina/Remmina/merge_requests/2142) *@antenore*
* RDP log filters keep previous value across connections [!2143](https://gitlab.com/Remmina/Remmina/merge_requests/2143) *@antenore*
* RDP: Add Use base credential for RD gateway authentication [!2135](https://gitlab.com/Remmina/Remmina/merge_requests/2135) *@Fantu*
* Emit warning if libkf5wallet missing but required by -DWITH_KF5WALLET=ON [!2144](https://gitlab.com/Remmina/Remmina/merge_requests/2144) *@giox069*
* Do not activate performance optimisations based on network type unless explicitly requested. [!2145](https://gitlab.com/Remmina/Remmina/merge_requests/2145) *@antenore*

## v1.4.9
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.8...v1.4.9)

* Snap Pulseaudio integration [!2106](https://gitlab.com/Remmina/Remmina/merge_requests/2106) *@antenore*
* Updated color schemes from iTerm2-Color-Schemes [!2107](https://gitlab.com/Remmina/Remmina/merge_requests/2107) *@antenore*
* Use previously defined DATADIR to install Kiosk files [!2108](https://gitlab.com/Remmina/Remmina/merge_requests/2108) *@kapsh*
* Snap language simplified [!2110](https://gitlab.com/Remmina/Remmina/merge_requests/2110) *@kingu*
* RDP and VNC: Fix smooth scrolling when abs(delta) < 1.0, see issue #2273 [!2111](https://gitlab.com/Remmina/Remmina/merge_requests/2111) *@giox069*
* Alexander Kapshuna added to remmina_about.glade [!2109](https://gitlab.com/Remmina/Remmina/merge_requests/2109) *@kingu*
* remmina_main_quickconnect: recognize ip when textbox has ip:port in it, and strip whitespaces [!2112](https://gitlab.com/Remmina/Remmina/merge_requests/2112) *@bwack*
* Implementing text search in the SSH plugin [!2113](https://gitlab.com/Remmina/Remmina/merge_requests/2113) *@antenore*
* Spelling: Plugin manager language reworked [!2114](https://gitlab.com/Remmina/Remmina/merge_requests/2114) *@kingu*
* Save screenshot_name and use correct seconds format [!2115](https://gitlab.com/Remmina/Remmina/merge_requests/2115) *@antenore*
* Resolve "Autostart checkbox setting not saved" [!2116](https://gitlab.com/Remmina/Remmina/merge_requests/2116) *@antenore*
* rdp: document freerdp Performance Flags setted by quality setting [!2117](https://gitlab.com/Remmina/Remmina/merge_requests/2117) *@Fantu*
* Resolve "Terminal general preferences are not saved" [!2119](https://gitlab.com/Remmina/Remmina/merge_requests/2119) *@antenore*
* Resolve "Type in FindFREERDP3.cmake" [!2120](https://gitlab.com/Remmina/Remmina/merge_requests/2120) *@antenore*
* rdp: add freerdp log filters setting [!2118](https://gitlab.com/Remmina/Remmina/merge_requests/2118) *@Fantu*
* Resolve "Extra underline character in "_Preferences" tooptip text" [!2121](https://gitlab.com/Remmina/Remmina/merge_requests/2121) *@antenore*

## v1.4.8
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.7...v1.4.8)

* Adding timout option for RDP connections. [!2091](https://gitlab.com/Remmina/Remmina/merge_requests/2091) *@antenore*
* Avoid quickconnect to empty hostnames. Fixes #2240. [!2092](https://gitlab.com/Remmina/Remmina/merge_requests/2092) *@giox069*
* Using full paths instead of variables [!2094](https://gitlab.com/Remmina/Remmina/merge_requests/2094) *@antenore*
* Add support for quick connecting to RDP, VNC and SPICE from the command line [!2093](https://gitlab.com/Remmina/Remmina/merge_requests/2093) *@espentveit*
* Add SSH support to the protocol handler [!2095](https://gitlab.com/Remmina/Remmina/merge_requests/2095) *@espentveit*
* Restart SSH session when user has provided new username or password to allow for changing SSH user [!2096](https://gitlab.com/Remmina/Remmina/merge_requests/2096) *@espentveit*
* Use </image> inline with AppStream 0.12 specification. [!2097](https://gitlab.com/Remmina/Remmina/merge_requests/2097) *@ghost1*
* Enabled GDK_SCROLL_SMOOTH for RDP/VNC [!2098](https://gitlab.com/Remmina/Remmina/merge_requests/2098) *@kenansun0*
* Some fixes for the RDP backend [!2099](https://gitlab.com/Remmina/Remmina/merge_requests/2099) *@pnowack*
* Trim white from ip addresses input into quick connect bar [!2100](https://gitlab.com/Remmina/Remmina/merge_requests/2100) *@daxkelson*
* Enhancing the SNAP info dialog box [!2102](https://gitlab.com/Remmina/Remmina/merge_requests/2102) *@antenore*
* Adding FreeRDP log level setting [!2103](https://gitlab.com/Remmina/Remmina/merge_requests/2103) *@antenore*
* Patron tally badge added to README [!2090](https://gitlab.com/Remmina/Remmina/merge_requests/2090) *@kingu*

## v1.4.7
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.6...v1.4.7)

* Allow compilation with libwinpr (freerdp) pre commit 8c5d96784d [!2083](https://gitlab.com/Remmina/Remmina/merge_requests/2083) *@giox069*
* Bug fixing v1.4.6 [!2082](https://gitlab.com/Remmina/Remmina/merge_requests/2082) *@antenore*
* Spelling: Automatic negotiation [!2084](https://gitlab.com/Remmina/Remmina/merge_requests/2084) *@kingu*
* Spelling: GNOME Shell, opt-in desc, comments [!2085](https://gitlab.com/Remmina/Remmina/merge_requests/2085) *@kingu*
* Memory leaks fixes [!2086](https://gitlab.com/Remmina/Remmina/merge_requests/2086) *@antenore*
* RDP: Replacing deprecated freerdp function VeryfyCertificate [!2087](https://gitlab.com/Remmina/Remmina/merge_requests/2087) *@antenore*

## v1.4.6
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.5...v1.4.6)

* Hotkey language fixed [!2064](https://gitlab.com/Remmina/Remmina/merge_requests/2064) *@kingu*
* TRANSLATION file for l10n [!2065](https://gitlab.com/Remmina/Remmina/merge_requests/2065) *@kingu*
* Fetch news from remmina.org optional [!2066](https://gitlab.com/Remmina/Remmina/merge_requests/2066) *@antenore*
* allow saving notes in connection profile [!2067](https://gitlab.com/Remmina/Remmina/merge_requests/2067) *@juarez.rudsatz*
* RDP: Improving hardware option parsing [!2068](https://gitlab.com/Remmina/Remmina/merge_requests/2068) *@antenore*
* Rearranged widgets in a new Behavior tab [!2069](https://gitlab.com/Remmina/Remmina/merge_requests/2069) *@juarez.rudsatz*
* Add separator only when there are saved profiles. Fixes #1914 [!2070](https://gitlab.com/Remmina/Remmina/merge_requests/2070) *@antenore*
* Implementing resume all for FTP file transfer, should fix #2210 [!2072](https://gitlab.com/Remmina/Remmina/merge_requests/2072) *@antenore*
* Spelling: Overwrite all file transfers [!2073](https://gitlab.com/Remmina/Remmina/merge_requests/2073) *@kingu*
* Spelling: Options for redirection x2, -: [!2074](https://gitlab.com/Remmina/Remmina/merge_requests/2074) *@kingu*
* Edit or connect using multiple profile files from the command line [!2075](https://gitlab.com/Remmina/Remmina/merge_requests/2075) *@antenore*
* Desktop session files for Remmina kiosk are optional [!2076](https://gitlab.com/Remmina/Remmina/merge_requests/2076) *@antenore*
* Update Ukrainian translation for the desktop file [!2077](https://gitlab.com/Remmina/Remmina/merge_requests/2077) *@yurchor*
* Remove 'translatable="yes"' from the fake label in remmina_spinner.glade [!2079](https://gitlab.com/Remmina/Remmina/merge_requests/2079) *@yurchor*
* Remove 'translatable="yes"' from the fake label [!2078](https://gitlab.com/Remmina/Remmina/merge_requests/2078) *@yurchor*
* Feat/lebowski [!2080](https://gitlab.com/Remmina/Remmina/merge_requests/2080) *@antenore*

## v1.4.5
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.4...v1.4.5)

* SSH plugin - adding font resize - closes #2201 [!2059](https://gitlab.com/Remmina/Remmina/merge_requests/2059) *@antenore*
* Fixing keyboard grabbing issues with screenshot tool [!2062](https://gitlab.com/Remmina/Remmina/merge_requests/2062) *@giox069*
* Refactoring remmina_debug to avoid memory leaks and overhead, should fix #2202 [!2061](https://gitlab.com/Remmina/Remmina/merge_requests/2061) *@antenore*
* Using directory only to expose artifacts [!2060](https://gitlab.com/Remmina/Remmina/merge_requests/2060) *@antenore*

## v1.4.4
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.3...v1.4.4)

* RDP Plugin - Adding UDP support, implements #2153 [!2038](https://gitlab.com/Remmina/Remmina/merge_requests/2038) *@antenore*
* Adding proxy and local storage support [!2039](https://gitlab.com/Remmina/Remmina/merge_requests/2039) *@antenore*
* RDP option to prefer IPv6 AAAA record over IPv4 A records [!2040](https://gitlab.com/Remmina/Remmina/merge_requests/2040) *@antenore*
* Snap: Install icons system-wide and use them [!2042](https://gitlab.com/Remmina/Remmina/merge_requests/2042) *@ed10vi*
* Allow users to override the app ID [!2044](https://gitlab.com/Remmina/Remmina/merge_requests/2044) *@garymoon*
* Use icon name instead of localizable string in gtk_image_new_from_icon_name() [!2045](https://gitlab.com/Remmina/Remmina/merge_requests/2045) *@yurchor*
* Fix minor typos [!2046](https://gitlab.com/Remmina/Remmina/merge_requests/2046) *@yurchor*
* Don't grab when window has no focus, issue #2165 [!2047](https://gitlab.com/Remmina/Remmina/merge_requests/2047) *@giox069*
* SSH tunnel and VNC fixes [!2048](https://gitlab.com/Remmina/Remmina/merge_requests/2048) *@antenore*
* Adding explicitely trueColour in the client format structure. Fixes #2181 and #810 [!2049](https://gitlab.com/Remmina/Remmina/merge_requests/2049) *@antenore*
* Code refactoring. [!2050](https://gitlab.com/Remmina/Remmina/merge_requests/2050) *@antenore*
* Extract subtitle for translation [!2051](https://gitlab.com/Remmina/Remmina/merge_requests/2051) *@yurchor*
* [SSH] Connection pre/post command not replacing SSH tunnel parameters [!2053](https://gitlab.com/Remmina/Remmina/merge_requests/2053) *@antenore*
* SNAP: Remove libssh, available in distro [!2052](https://gitlab.com/Remmina/Remmina/merge_requests/2052) *@ed10vi*
*  2189 [!2054](https://gitlab.com/Remmina/Remmina/merge_requests/2054) *@giox069*
* Adding remmina_debug function to simplify log reporting [!2055](https://gitlab.com/Remmina/Remmina/merge_requests/2055) *@antenore*
* Removing OnlyShowIn as deprecated in the latest freedesktop spec. Closes #2198 [!2056](https://gitlab.com/Remmina/Remmina/merge_requests/2056) *@antenore*
* Cleaning up glib deprecations [!2058](https://gitlab.com/Remmina/Remmina/merge_requests/2058) *@antenore*


## v1.4.3
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.2...v1.4.3)

* KB grabbing fixes (mostly for Wayland) [!2036](https://gitlab.com/Remmina/Remmina/merge_requests/2036) *@giox069*
* Adding Freerdp 3 compile option and using FreeRDP tag 2.0.0 as default [!2034](https://gitlab.com/Remmina/Remmina/merge_requests/2034) *@antenore*
* Adding remmina terminal dependencies [!2035](https://gitlab.com/Remmina/Remmina/merge_requests/2035) *@antenore*
* Translated using Weblate (Turkish) by Oğuz Ersen <oguzersen@protonmail.com>
* Translated using Weblate (Swedish) by Allan Nordhøy <epost@anotheragency.no>
* Translated using Weblate (Albanian) by Allan Nordhøy <epost@anotheragency.no>
* Translated using Weblate (Dutch) by Jennifer <jen@elypia.org>
* Translated using Weblate (Norwegian Bokmål) by Allan Nordhøy <epost@anotheragency.no>
* Translated using Weblate (Burmese) by Allan Nordhøy <epost@anotheragency.no>
* Translated using Weblate (Korean) by Justin Song <mcsong@gmail.com>
* Translated using Weblate (Japanese) by anonymous <noreply@weblate.org>
* Translated using Weblate (Japanese) by FeLvi_zzz <felvi.zzz.coffee@gmail.com>
* Translated using Weblate (Italian) by Allan Nordhøy <epost@anotheragency.no>
* Translated using Weblate (Basque) by Osoitz <oelkoro@gmail.com>
* Translated using Weblate (Spanish) by Adolfo Jayme Barrientos <fitojb@ubuntu.com>
* Translated using Weblate (English (United Kingdom)) by Barbul Gergő <barbul.gergo@stud.u-szeged.hu>
* Translated using Weblate (Arabic) by ButterflyOfFire <ButterflyOfFire@protonmail.com>
* Translated using Weblate (Spanish) by Allan Nordhøy <epost@anotheragency.no>
* Translated using Weblate (Hebrew) by Yaron Shahrabani <sh.yaron@gmail.com>
* Translated using Weblate (Spanish) by Juan Ignacio Cherrutti <juancherru@gmail.com>
* Translated using Weblate (Slovak) by Dušan Kazik <prescott66@gmail.com>
* Translated using Weblate (German) by Johannes Weberhofer <jweberhofer@weberhofer.at>
* Translated using Weblate (Czech) by Pavel Borecki <pavel.borecki@gmail.com>

## v1.4.2
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.1...v1.4.2)

* Spelling: Doublespace removed [!2011](https://gitlab.com/Remmina/Remmina/merge_requests/2011) *@kingu*
* Adds ClientBuild settings to RDP plugin to specify client version build number [!2012](https://gitlab.com/Remmina/Remmina/merge_requests/2012) *@mskalski*
* Spelling: Smaller bullet points, RDP plugin, Git [!2014](https://gitlab.com/Remmina/Remmina/merge_requests/2014) *@kingu*
* Allow formatting for SSH session filename [!2015](https://gitlab.com/Remmina/Remmina/merge_requests/2015) *@juarez.rudsatz*
* include juarezr in AUTHORS [!2017](https://gitlab.com/Remmina/Remmina/merge_requests/2017) *@juarez.rudsatz*
* web-browser plugin fixes [!2016](https://gitlab.com/Remmina/Remmina/merge_requests/2016) *@antenore*
* Make duplicate session sensitive only if a profile exists [!2019](https://gitlab.com/Remmina/Remmina/merge_requests/2019) *@antenore*
* Using updated Contribution page from website [!2021](https://gitlab.com/Remmina/Remmina/merge_requests/2021) *@kingu*
* Issue template reworked [!2020](https://gitlab.com/Remmina/Remmina/merge_requests/2020) *@kingu*
* Fix null pointer dereference in remmina_main_quickconnect [!2022](https://gitlab.com/Remmina/Remmina/merge_requests/2022) *@Flow*
* SFTP with tunnel fixes [!2023](https://gitlab.com/Remmina/Remmina/merge_requests/2023) *@giox069*
* Cppcheck and PVS Studio Fixes [!2018](https://gitlab.com/Remmina/Remmina/merge_requests/2018) *@qarmin*
* VNCI fixes [!2025](https://gitlab.com/Remmina/Remmina/merge_requests/2025) *@antenore*
* Flatpak - Updating gnome to 3.36 [!2026](https://gitlab.com/Remmina/Remmina/merge_requests/2026) *@antenore*
* Typo fix in remmina_ssh.c [!2024](https://gitlab.com/Remmina/Remmina/merge_requests/2024) *@gunnarhj*
* Spelling: Select a folder, choose a folder [!2027](https://gitlab.com/Remmina/Remmina/merge_requests/2027) *@kingu*
* Allow enter key in domain textbox of authentication dialog to submit [!2028](https://gitlab.com/Remmina/Remmina/merge_requests/2028) *@piecka*
* rcw event sources cleanup [!2029](https://gitlab.com/Remmina/Remmina/merge_requests/2029) *@giox069*
* Contact file replaces CoC and new README [!2030](https://gitlab.com/Remmina/Remmina/merge_requests/2030) *@kingu*
* THANKS reworked [!2031](https://gitlab.com/Remmina/Remmina/merge_requests/2031) *@kingu*
* Merge weblate translations in master [!2032](https://gitlab.com/Remmina/Remmina/merge_requests/2032) *@antenore*

## v1.4.1
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.0...v1.4.1)

* SSH fixes, should fix #2094 [!2009](https://gitlab.com/Remmina/Remmina/merge_requests/2009) *@giox069*
* Update remmina_filezilla_sftp.sh [!2010](https://gitlab.com/Remmina/Remmina/merge_requests/2010) *@greenfoxua*

## v1.4.0
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.3.10...v1.4.0)

* Rdp clipboard fixes [!2007](https://gitlab.com/Remmina/Remmina/merge_requests/2007) *@giox069*
* Ssh authentication fixes [!2006](https://gitlab.com/Remmina/Remmina/merge_requests/2006) *@giox069*
* Compiler warnings [!1992](https://gitlab.com/Remmina/Remmina/merge_requests/1992) *@antenore*
* flatpak manifest update [!1993](https://gitlab.com/Remmina/Remmina/merge_requests/1993) *@dgcampea*
* Spelling: System proxy settings [!1994](https://gitlab.com/Remmina/Remmina/merge_requests/1994) *@kingu*
* Update ru.po [!1995](https://gitlab.com/Remmina/Remmina/merge_requests/1995) *@hopyres*
* SSH tunnel refactoring [!1996](https://gitlab.com/Remmina/Remmina/merge_requests/1996) *@antenore*
* Parameters passed individually, notation, "Press any key to continue…" [!1997](https://gitlab.com/Remmina/Remmina/merge_requests/1997) *@kingu*
* Replace apt with apt-get [!1999](https://gitlab.com/Remmina/Remmina/merge_requests/1999) *@kingu*
* Updated boolean expressions [!2000](https://gitlab.com/Remmina/Remmina/merge_requests/2000) *@kingu*
* Add end of options escaping [!2002](https://gitlab.com/Remmina/Remmina/merge_requests/2002) *@kingu*
* Remove double-quotes escaping early [!2001](https://gitlab.com/Remmina/Remmina/merge_requests/2001) *@kingu*
* Ending code block whitespace line removed [!1998](https://gitlab.com/Remmina/Remmina/merge_requests/1998) *@kingu*
* Safer removal of build directory [!2003](https://gitlab.com/Remmina/Remmina/merge_requests/2003) *@kingu*
* Spelling: Could not access the RDP server x7 [!2004](https://gitlab.com/Remmina/Remmina/merge_requests/2004) *@kingu*
* Removing double struct declaration. [!2005](https://gitlab.com/Remmina/Remmina/merge_requests/2005) *@antenore*

