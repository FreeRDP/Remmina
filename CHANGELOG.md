## v1.4.36
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.35...v1.4.36)

* Fix crash with keyboard-interactive SSH auth [!2576](https://gitlab.com/Remmina/Remmina/merge_requests/2576) *@bhatman1441*
* Prevent empty remmina_file_name in remmina.pref [!2577](https://gitlab.com/Remmina/Remmina/merge_requests/2577) *@bhatman1441*
* rdp/rdp-plugin: Fix faulty color depth check [!2579](https://gitlab.com/Remmina/Remmina/merge_requests/2579) *@pnowack*
* Fix typos [!2578](https://gitlab.com/Remmina/Remmina/merge_requests/2578) *@deining*
* Usbredirect on connect [!2580](https://gitlab.com/Remmina/Remmina/merge_requests/2580) *@hunderteins*
* [REM-3104] Add global RDP options to set FreeRDP auth filter [!2581](https://gitlab.com/Remmina/Remmina/merge_requests/2581) *@myheroyuki*
* Use universal /bin/sh shebang in remmina-rile-wrapper(1) [!2572](https://gitlab.com/Remmina/Remmina/merge_requests/2572) *@klemensn*
* [REM-3104] Added back in line that was accidentally removed before merge [!2582](https://gitlab.com/Remmina/Remmina/merge_requests/2582) *@myheroyuki*
* [REM-3104] Do not initialize rdp_auth_filter [!2583](https://gitlab.com/Remmina/Remmina/merge_requests/2583) *@myheroyuki*
* [REM-3076] Add proxy field for SPICE connections [!2584](https://gitlab.com/Remmina/Remmina/merge_requests/2584) *@myheroyuki*
* add a conditional check for darwin and NetBSD [!2585](https://gitlab.com/Remmina/Remmina/merge_requests/2585) *@gador1*
* [REM-3103] Make more obvious to user that Remmina may not be able to exec... [!2586](https://gitlab.com/Remmina/Remmina/merge_requests/2586) *@myheroyuki*
* [REM-3121] Handle GotFrameBufferUpdate on its own thread to prevent freeze [!2587](https://gitlab.com/Remmina/Remmina/merge_requests/2587) *@myheroyuki*
* Enable horitical scroll on RDP plugin. [!2588](https://gitlab.com/Remmina/Remmina/merge_requests/2588) *@AkiraPenguin*
* [REM-2854] Add timeout option to handle VNC disconnects [!2589](https://gitlab.com/Remmina/Remmina/merge_requests/2589) *@myheroyuki*
* [REM-3134] Fix bug where option to send clipboard as keystrokes did not appear [!2590](https://gitlab.com/Remmina/Remmina/merge_requests/2590) *@myheroyuki*
* Add ability to send a key combination when RDP connection is idle [!2591](https://gitlab.com/Remmina/Remmina/merge_requests/2591) *@dexxter00*
* [REM-3111] Do not prompt user for credentials if they have already been entered [!2592](https://gitlab.com/Remmina/Remmina/merge_requests/2592) *@myheroyuki*
* [REM-3140] Add null check to prevent segfault [!2593](https://gitlab.com/Remmina/Remmina/merge_requests/2593) *@myheroyuki*
* Move the idle timer into rf_context to make it session-specific. [!2594](https://gitlab.com/Remmina/Remmina/merge_requests/2594) *@morganw3*
* [REM-3156] Limit connection name on rcw tab to reasonable length [!2595](https://gitlab.com/Remmina/Remmina/merge_requests/2595) *@myheroyuki*
* Update several dependencies [!2596](https://gitlab.com/Remmina/Remmina/merge_requests/2596) *@myheroyuki*
* [REM-3127] Add user prompt for gateway messages [!2597](https://gitlab.com/Remmina/Remmina/merge_requests/2597) *@myheroyuki*
* [REM-3167] Add reconnect button to rcw [!2598](https://gitlab.com/Remmina/Remmina/merge_requests/2598) *@myheroyuki*
* [REM-3127] Changed to put call to a message accept panel in the RemminaPluginService struct [!2599](https://gitlab.com/Remmina/Remmina/merge_requests/2599) *@myheroyuki*
* Removed unused, commented code [!2600](https://gitlab.com/Remmina/Remmina/merge_requests/2600) *@myheroyuki*
* [REM-2428] Allow floating toolbar to be drag and dropped to more locations on the screen [!2601](https://gitlab.com/Remmina/Remmina/merge_requests/2601) *@myheroyuki*
* [REM-3162] Fixed some compile warnings causing issues with GCC 14.2.1 [!2602](https://gitlab.com/Remmina/Remmina/merge_requests/2602) *@myheroyuki*
* Update ssh error to not show outdated message [!2603](https://gitlab.com/Remmina/Remmina/merge_requests/2603) *@myheroyuki*


## v1.4.35
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.34...v1.4.35)

* Fix build for openssl-1.1 [!2565](https://gitlab.com/Remmina/Remmina/merge_requests/2565) *@bhatman1441*
* Allow running an SSH command when connecting via ssh tunnel [!2566](https://gitlab.com/Remmina/Remmina/merge_requests/2566) *@myheroyuki*
* Check if WINPR_ASSERT is defined [!2567](https://gitlab.com/Remmina/Remmina/merge_requests/2567) *@bhatman1441*
* [REM-3047] fix issue compiling with gcc-14 [!2568](https://gitlab.com/Remmina/Remmina/merge_requests/2568) *@myheroyuki*
* Fix crash caused by enabling disablepasswordstoring [!2569](https://gitlab.com/Remmina/Remmina/merge_requests/2569) *@bhatman1441*
* Revert setting loadbalanceinfo to the old method used before FreeRDP3 [!2570](https://gitlab.com/Remmina/Remmina/merge_requests/2570) *@myheroyuki*
* updated call to set FreeRDP_LoadBalanceInfo [!2571](https://gitlab.com/Remmina/Remmina/merge_requests/2571) *@myheroyuki*

## v1.4.34
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v.1.4.33...v1.4.34)

* [REM-2981] Move ssh-unrelated code outside of HAVE_LIBSSH ifdef [!2540](https://gitlab.com/Remmina/Remmina/merge_requests/2540) *@myheroyuki*
* [REM-2974] only attempt to resolve hostname if initial ssh_connect fails [!2542](https://gitlab.com/Remmina/Remmina/merge_requests/2542) *@myheroyuki*
* [REM-2980] Remove flag that prevented remote audio from working with flatpak version of Remmina [!2543](https://gitlab.com/Remmina/Remmina/merge_requests/2543) *@myheroyuki*
* [REM-2982] Update Flatpak to use GNOME runtime version 44 [!2541](https://gitlab.com/Remmina/Remmina/merge_requests/2541) *@myheroyuki*
* initial port to core22 [!2539](https://gitlab.com/Remmina/Remmina/merge_requests/2539) *@soumyaDghosh*
* [REM-2984] Add null check to prevent freeze when opening an rdp connection [!2545](https://gitlab.com/Remmina/Remmina/merge_requests/2545) *@myheroyuki*
* Fix GTK critical error when editing RDP profile [!2544](https://gitlab.com/Remmina/Remmina/merge_requests/2544) *@bhatman1441*
* Remove snap installs for broken link executables [!2546](https://gitlab.com/Remmina/Remmina/merge_requests/2546) *@bhatman1441*
* SSH private key auth: If username is empty, prompt user to enter it [!2547](https://gitlab.com/Remmina/Remmina/merge_requests/2547) *@bhatman1441*
* Fix Flatpak freeze on connection when importing RDP profile [!2548](https://gitlab.com/Remmina/Remmina/merge_requests/2548) *@bhatman1441*
* Fix of some grammatic issues in German translation [!2550](https://gitlab.com/Remmina/Remmina/merge_requests/2550) *@vhhhl1*
* [REM-3003] Re-add line removed by accident in !2532 [!2551](https://gitlab.com/Remmina/Remmina/merge_requests/2551) *@myheroyuki*
* GtkFileChooserNative support [!2553](https://gitlab.com/Remmina/Remmina/merge_requests/2553) *@robxnano*
* Spice UNIX socket channel initialization enhancement [!2549](https://gitlab.com/Remmina/Remmina/merge_requests/2549) *@BobbyTheBuilder*
* Fix segfault for `remmina -p` [!2555](https://gitlab.com/Remmina/Remmina/merge_requests/2555) *@bhatman1441*
* Add unlocking code to let plugins use passwords [!2556](https://gitlab.com/Remmina/Remmina/merge_requests/2556) *@pnowak433*
* CI Pipeline Updates [!2557](https://gitlab.com/Remmina/Remmina/merge_requests/2557) *@bhatman1441*
* [freerdp] update to build for stable-3.0 [!2554](https://gitlab.com/Remmina/Remmina/merge_requests/2554) *@akallabeth*
* Allow Remmina to be built without FreeRDP [!2558](https://gitlab.com/Remmina/Remmina/merge_requests/2558) *@myheroyuki*
* [REM-3048] Fix crash on multi-monitor with FreeRDP3 [!2559](https://gitlab.com/Remmina/Remmina/merge_requests/2559) *@myheroyuki*
* Added developer_name to meet new flathub requirements [!2561](https://gitlab.com/Remmina/Remmina/merge_requests/2561) *@myheroyuki*
* [New features] Bring back remmina server features with new additions [!2560](https://gitlab.com/Remmina/Remmina/merge_requests/2560) *@bhatman1441*
* [REM-2983] compile ssh with gcrypt for flatpak builds. Also sync flatpak... [!2563](https://gitlab.com/Remmina/Remmina/merge_requests/2563) *@myheroyuki*
* Update copyright info [!2562](https://gitlab.com/Remmina/Remmina/merge_requests/2562) *@bhatman1441*

## v.1.4.33
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.32...v1.4.33)

* [REM-2977] add shortcut for 'send clipboard as keystrokes' [!2535](https://gitlab.com/Remmina/Remmina/merge_requests/2535) *@myheroyuki*
* [REM-2972] Make keeping window open on session disconnect configurable [!2537](https://gitlab.com/Remmina/Remmina/merge_requests/2537) *@myheroyuki*
* [REM-2974] fix crash cause by using freed memory [!2536](https://gitlab.com/Remmina/Remmina/merge_requests/2536) *@myheroyuki*

## v.1.4.32
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.31...v1.4.32)

* [REM2916] Added option to kill async process started by exec plugin on tab close [!2501](https://gitlab.com/Remmina/Remmina/merge_requests/2501) *@myheroyuki*
* Fix search bar toggle behavior [!2503](https://gitlab.com/Remmina/Remmina/merge_requests/2503) *@myheroyuki*
* Fix memory leaks [!2504](https://gitlab.com/Remmina/Remmina/merge_requests/2504) *@bhatman1441*
* [REM-2920] Password visibility can now be toggled on remmina_message_panels [!2505](https://gitlab.com/Remmina/Remmina/merge_requests/2505) *@myheroyuki*
* Remove SSH file paths from remmina file when unchecking box in profile edit [!2506](https://gitlab.com/Remmina/Remmina/merge_requests/2506) *@bhatman1441*
* Solved issue #2910 - Added support for IPv6 with fallback to IPv4 for ssh [!2507](https://gitlab.com/Remmina/Remmina/merge_requests/2507) *@scoulondre*
* Fix UI bugs in Remmina Preferences set new password [!2508](https://gitlab.com/Remmina/Remmina/merge_requests/2508) *@bhatman1441*
* Fix memory leaks and change GTK critical errors to remmina warnings for null icon autostart file [!2509](https://gitlab.com/Remmina/Remmina/merge_requests/2509) *@bhatman1441*
* [REM-2926] Added ability to set a fixed aspect ratio for vnc connections when... [!2511](https://gitlab.com/Remmina/Remmina/merge_requests/2511) *@myheroyuki*
* Fix snap crash [!2512](https://gitlab.com/Remmina/Remmina/merge_requests/2512) *@bhatman1441*
* [REM-2936] Fix crash related to unmap events [!2513](https://gitlab.com/Remmina/Remmina/merge_requests/2513) *@myheroyuki*
* [REM-2938] Improved ordering of tray icon menu items [!2514](https://gitlab.com/Remmina/Remmina/merge_requests/2514) *@myheroyuki*
* Fix memory leaks in remmina_plugin_manager_init with g_free and g_ptr_array_free [!2515](https://gitlab.com/Remmina/Remmina/merge_requests/2515) *@bhatman1441*
* Remote assistance [!2516](https://gitlab.com/Remmina/Remmina/merge_requests/2516) *@myheroyuki*
* Spelling: Various strings for assistance mode [!2517](https://gitlab.com/Remmina/Remmina/merge_requests/2517) *@kingu*
* Fix some compiler warnings [!2518](https://gitlab.com/Remmina/Remmina/merge_requests/2518) *@myheroyuki*
* Ability to SPICE connect to unix domain socket [!2519](https://gitlab.com/Remmina/Remmina/merge_requests/2519) *@BobbyTheBuilder*
* update multi monitor icon [!2520](https://gitlab.com/Remmina/Remmina/merge_requests/2520) *@jtmoree*
* Save toggle options when duplicating connection [!2521](https://gitlab.com/Remmina/Remmina/merge_requests/2521) *@myheroyuki*
* Rem 2954 [!2522](https://gitlab.com/Remmina/Remmina/merge_requests/2522) *@myheroyuki*
* New feature: Allow user to specify use of modifiers when changing key preferences [!2523](https://gitlab.com/Remmina/Remmina/merge_requests/2523) *@bhatman1441*
* [REM-2914] Alert user of unexpected disconnect instead of immediately closing the connection window [!2524](https://gitlab.com/Remmina/Remmina/merge_requests/2524) *@myheroyuki*
* Fix Remmina not using `@REMMINA_BINARY_PATH@` [!2525](https://gitlab.com/Remmina/Remmina/merge_requests/2525) *@Enzime*
* Highlight top bar when `Grab all keyboard events` is enabled [!2526](https://gitlab.com/Remmina/Remmina/merge_requests/2526) *@toliak*
* Snapcraft: Allow remmina preferences file to point to symlinked `current` directory [!2527](https://gitlab.com/Remmina/Remmina/merge_requests/2527) *@bhatman1441*
* [REM-2850] Add ability to automatically move mouse to keep RDP connections alive [!2528](https://gitlab.com/Remmina/Remmina/merge_requests/2528) *@myheroyuki*
* Allow user to set REMMINA_GIT_REVISION [!2529](https://gitlab.com/Remmina/Remmina/merge_requests/2529) *@yurivict*
* Remove unnecessary parameters from remmina_public_get_server_port_wrapper [!2530](https://gitlab.com/Remmina/Remmina/merge_requests/2530) *@bhatman1441*
* Fix freeze that occurs when loading in python modules properly [!2531](https://gitlab.com/Remmina/Remmina/merge_requests/2531) *@myheroyuki*
* [REM-1923] Fix handling of pause break key for RDP connections [!2532](https://gitlab.com/Remmina/Remmina/merge_requests/2532) *@myheroyuki*
* [REM-2971] When quiting Remmina from the system tray the are you sure prompt now functions properly [!2533](https://gitlab.com/Remmina/Remmina/merge_requests/2533) *@myheroyuki*

## v.1.4.31
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.30...v1.4.31)

* [REM-2874] muli password changer search now matches partial strings [!2480](https://gitlab.com/Remmina/Remmina/merge_requests/2480) *@myheroyuki*
* Fix segfault in VNC when using domain socket [!2481](https://gitlab.com/Remmina/Remmina/merge_requests/2481) *@jchw*
* Switched pylist Append to SetItem [!2482](https://gitlab.com/Remmina/Remmina/merge_requests/2482) *@bhatman1441*
* Template texts updated [!2484](https://gitlab.com/Remmina/Remmina/merge_requests/2484) *@kingu*
* Add menu option to delete multiple profiles at the same time [!2483](https://gitlab.com/Remmina/Remmina/merge_requests/2483) *@bhatman1441*
* add 1080p as a default resolution to remmina_pref.c [!2486](https://gitlab.com/Remmina/Remmina/merge_requests/2486) *@Thibaultmol*
* Add ubuntu22.04 dockerfile [!2487](https://gitlab.com/Remmina/Remmina/merge_requests/2487) *@yasinbakhtiar*
* Remove character limit on password length [!2489](https://gitlab.com/Remmina/Remmina/merge_requests/2489) *@bhatman1441*
* Create better postats.html [!2488](https://gitlab.com/Remmina/Remmina/merge_requests/2488) *@yasinbakhtiar*
* Redesign santahat.png & add santahat.svg [!2492](https://gitlab.com/Remmina/Remmina/merge_requests/2492) *@yasinbakhtiar*
* Edit preferences UI [!2491](https://gitlab.com/Remmina/Remmina/merge_requests/2491) *@yasinbakhtiar*
* Change the icon of the new connection button [!2493](https://gitlab.com/Remmina/Remmina/merge_requests/2493) *@yasinbakhtiar*
* Modify the checkbox of the appearance-tab [!2494](https://gitlab.com/Remmina/Remmina/merge_requests/2494) *@yasinbakhtiar*
* Add valign to the switch of the debugging window [!2495](https://gitlab.com/Remmina/Remmina/merge_requests/2495) *@yasinbakhtiar*
* plugins/rdp: Ensure output redirection configuration applies to both dynamic and static rdpsnd [!2498](https://gitlab.com/Remmina/Remmina/merge_requests/2498) *@msizanoen*

## v.1.4.30
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.29...v1.4.30)

* Update snap build to use freerdp 2.9.0 [!2458](https://gitlab.com/Remmina/Remmina/merge_requests/2458) *@myheroyuki*
* Add text mime type formats to RDP clibpoard [!2459](https://gitlab.com/Remmina/Remmina/merge_requests/2459) *@giox069*
* Ensure timer is null after destruction [!2462](https://gitlab.com/Remmina/Remmina/merge_requests/2462) *@myheroyuki*
* Ability to view passwords in the clear using a toggle in the password field [!2460](https://gitlab.com/Remmina/Remmina/merge_requests/2460) *@benoit.lagarde*
* Made icons viewable in lower resulution. [!2463](https://gitlab.com/Remmina/Remmina/merge_requests/2463) *@benoit.lagarde*
* Improve mime file [!2464](https://gitlab.com/Remmina/Remmina/merge_requests/2464) *@yselkowitz1*
* [REM-2809] Appearance preferences now refresh in the main window when the user... [!2466](https://gitlab.com/Remmina/Remmina/merge_requests/2466) *@myheroyuki*
* This should be a message instead of a info print. So that the user can see it by default. [!2461](https://gitlab.com/Remmina/Remmina/merge_requests/2461) *@sork*
* remmina_rdp_monitor_get(): fix maxw, maxh and monitorids calculation [!2467](https://gitlab.com/Remmina/Remmina/merge_requests/2467) *@MaxIhlenfeldt*
* Rem 2864 [!2469](https://gitlab.com/Remmina/Remmina/merge_requests/2469) *@myheroyuki*
* Fix undefined symbol error when importing gi in a python extension [!2470](https://gitlab.com/Remmina/Remmina/merge_requests/2470) *@bhatman1441*
* make it build on macOS [!2471](https://gitlab.com/Remmina/Remmina/merge_requests/2471) *@mvzlb*
* Rem 2864 [!2472](https://gitlab.com/Remmina/Remmina/merge_requests/2472) *@myheroyuki*
* Fix autostart file flatpak exec command [!2474](https://gitlab.com/Remmina/Remmina/merge_requests/2474) *@bhatman1441*
* [REM-1987] allow for dynamic resolution updates for vnc connections [!2476](https://gitlab.com/Remmina/Remmina/merge_requests/2476) *@myheroyuki*
* Fix overlapping text in preferences menu, terminal tab [!2477](https://gitlab.com/Remmina/Remmina/merge_requests/2477) *@bhatman1441*
* Allow could not authenticate banner to go away after successful reauthentication [!2478](https://gitlab.com/Remmina/Remmina/merge_requests/2478) *@bhatman1441*
* Add environments for easy manual testing [!2473](https://gitlab.com/Remmina/Remmina/merge_requests/2473) *@ToolsDevler*

## v1.4.29
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.28...v1.4.29)

* Hiroyuki Tanaka added to README :) [!2451](https://gitlab.com/Remmina/Remmina/merge_requests/2451) *@kingu*
* Initial macOS support [!2453](https://gitlab.com/Remmina/Remmina/merge_requests/2453) *@wegank*
* X2Go error-message consistency [!2452](https://gitlab.com/Remmina/Remmina/merge_requests/2452) *@kingu*
* Avoid crash when closing, fixes issue #2832 [!2454](https://gitlab.com/Remmina/Remmina/merge_requests/2454) *@giox069*
* Update Copyright for 2023 [!2455](https://gitlab.com/Remmina/Remmina/merge_requests/2455) *@myheroyuki*
* Aligning local and downstream jsons [!2456](https://gitlab.com/Remmina/Remmina/merge_requests/2456) *@antenore*

## v1.4.28
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.27...v1.4.28)

* Some minor rdp fixes [!2415](https://gitlab.com/Remmina/Remmina/merge_requests/2415) *@akallabeth*
* Mathias Winterhalter's avatar Fix unix socket support for VNC [!2417](https://gitlab.com/Remmina/Remmina/merge_requests/2417) *@ToolsDevler*
* GVNC: Fixed JPEG quality encoding advertizing [!2418](https://gitlab.com/Remmina/Remmina/merge_requests/2418) *@mdevaev*
* Fix missing null checks causing signal 11 [!2419](https://gitlab.com/Remmina/Remmina/merge_requests/2419) *@ToolsDevler*
* add modified date to sftp file list [!2420](https://gitlab.com/Remmina/Remmina/merge_requests/2420) *@youcefnafa*
* Adding Labels/Tags [!2421](https://gitlab.com/Remmina/Remmina/merge_requests/2421) *@antonio.petricca*
* Spelling: Hostname [!2422](https://gitlab.com/Remmina/Remmina/merge_requests/2422) *@kingu*
* X2Go: Fix annoying default_username bug. [!2423](https://gitlab.com/Remmina/Remmina/merge_requests/2423) *@D0n1elT*
* plugins/x2go/x2go_plugin.c: Fix tip and comment sentence [!2424](https://gitlab.com/Remmina/Remmina/merge_requests/2424) *@kingu*
* Remove webkit_settings_set_enable_frame_flattening() for newer WekbKitGTK, fixes #2780 [!2425](https://gitlab.com/Remmina/Remmina/merge_requests/2425) *@giox069*
* Change password including gateway [!2427](https://gitlab.com/Remmina/Remmina/merge_requests/2427) *@benoit.lagarde*
* Improve rcw close [!2429](https://gitlab.com/Remmina/Remmina/merge_requests/2429) *@giox069*
* Multiple changes to build and run with libsoup 3.0 [!2431](https://gitlab.com/Remmina/Remmina/merge_requests/2431) *@antenore*
* X2Go: Add ssh_passphrase and ssh_privatekey settings. [!2428](https://gitlab.com/Remmina/Remmina/merge_requests/2428) *@D0n1elT*
* RDP: Add option to disable output suppression [!2432](https://gitlab.com/Remmina/Remmina/merge_requests/2432) *@iskunk*
* Spelling: "Passphrase" → "password" [!2433](https://gitlab.com/Remmina/Remmina/merge_requests/2433) *@kingu*
* Fix compile warnings and some spelling corrections [!2434](https://gitlab.com/Remmina/Remmina/merge_requests/2434) *@myheroyuki*
* [Rem-2782] Display protocol name in tooltip for connections in the ... menu... [!2436](https://gitlab.com/Remmina/Remmina/merge_requests/2436) *@myheroyuki*
* [Rem-2782] added protocol icons in dropdown menu [!2438](https://gitlab.com/Remmina/Remmina/merge_requests/2438) *@myheroyuki*
* Fix widget reparenting when entering/exiting fullscreen [!2439](https://gitlab.com/Remmina/Remmina/merge_requests/2439) *@giox069*
* Rem 2760 [!2440](https://gitlab.com/Remmina/Remmina/merge_requests/2440) *@myheroyuki*
* Allow building on a Wayland-only environment - version 4 [!2437](https://gitlab.com/Remmina/Remmina/merge_requests/2437) *@festevam*
* [Rem-2564] Allow for VNC runtime adjustment of color depth [!2442](https://gitlab.com/Remmina/Remmina/merge_requests/2442) *@myheroyuki*
* Fix floating toolbar not disappearing when in fullscreen and keyboard grabbed [!2441](https://gitlab.com/Remmina/Remmina/merge_requests/2441) *@giox069*
* Revert "Merge branch 'Rem-2564' into 'master'" [!2443](https://gitlab.com/Remmina/Remmina/merge_requests/2443) *@myheroyuki*
* [Rem-2654] Allow for runtime adjustment of color depth, both increasing and decreasing [!2444](https://gitlab.com/Remmina/Remmina/merge_requests/2444) *@myheroyuki*
* [Rem-2564] Changed declaration of variables to be compatable with different Ubuntu version [!2445](https://gitlab.com/Remmina/Remmina/merge_requests/2445) *@myheroyuki*
* [Rem-2682] Added view-only mode for RDP [!2447](https://gitlab.com/Remmina/Remmina/merge_requests/2447) *@myheroyuki*
* Removing the News Widget [!2446](https://gitlab.com/Remmina/Remmina/merge_requests/2446) *@antenore*
* Updated flatpak manifest files to match github [!2448](https://gitlab.com/Remmina/Remmina/merge_requests/2448) *@myheroyuki*
* Add missing include for X11/Wayland conditional [!2450](https://gitlab.com/Remmina/Remmina/merge_requests/2450) *@bkohler*

## v1.4.27
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.26...v1.4.27)

* Fix dangling pointer after scrolled_container destruction. [!2409](https://gitlab.com/Remmina/Remmina/merge_requests/2409) *@antenore*
* Strengthen remmina file set string [!2410](https://gitlab.com/Remmina/Remmina/merge_requests/2410) *@giox069*
* Refactoring and minor fixes [!2411](https://gitlab.com/Remmina/Remmina/merge_requests/2411) *@antenore*
* launcher.sh is compatible with xfce4-terminal and gnome-terminal now. [!2412](https://gitlab.com/Remmina/Remmina/merge_requests/2412) *@Lebensgefahr*
* Fix #2473 - revive rcw_focus_out_event to avoid sticky Alt on Alt-TAB [!2413](https://gitlab.com/Remmina/Remmina/merge_requests/2413) *@wolfmanx*

## v1.4.26
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.25...v1.4.26)

* Fix trial for 2577: Closing a VNC connection makes Remmina close all other... [!2391](https://gitlab.com/Remmina/Remmina/merge_requests/2391) *@PHWR*
* Handle after-auth connection errors in VNC properly [!2390](https://gitlab.com/Remmina/Remmina/merge_requests/2390) *@lorenz*
* Using Remmina from command-line for kiosked servers [!2392](https://gitlab.com/Remmina/Remmina/merge_requests/2392) *@marco.fortina*
* Manual page refactoring fixes #2056 [!2393](https://gitlab.com/Remmina/Remmina/merge_requests/2393) *@antenore*
* Add mutex to protect RDP clipboard->srv_data. Fixes #2666 [!2395](https://gitlab.com/Remmina/Remmina/merge_requests/2395) *@giox069*
* SSH Forward X11 [!2397](https://gitlab.com/Remmina/Remmina/merge_requests/2397) *@antenore*
* Add '--no-tray-icon' command-line option [!2398](https://gitlab.com/Remmina/Remmina/merge_requests/2398) *@marco.fortina*
* Python plugins [!2399](https://gitlab.com/Remmina/Remmina/merge_requests/2399) *@ToolsDevler*
* Apply plugin api changes from Python plugin change [!2400](https://gitlab.com/Remmina/Remmina/merge_requests/2400) *@ToolsDevler*
* Extract python plugin [!2401](https://gitlab.com/Remmina/Remmina/merge_requests/2401) *@ToolsDevler*
* Make FreeRDPs TLS Security Level setting accessible in the advanced settings view [!2402](https://gitlab.com/Remmina/Remmina/merge_requests/2402) *@antenore*
* Disable grabs for SSH and SFTP, #closes #2728 [!2403](https://gitlab.com/Remmina/Remmina/merge_requests/2403) *@antenore*
* Cannot disable shared folder [!2404](https://gitlab.com/Remmina/Remmina/merge_requests/2404) *@antenore*
* Use PyInitializeEx in order to skip signal handler registration [!2405](https://gitlab.com/Remmina/Remmina/merge_requests/2405) *@ToolsDevler*
* Ignore add new connection button in kiosk mode [!2406](https://gitlab.com/Remmina/Remmina/merge_requests/2406) *@antenore*
* WWW plugin refactoring [!2407](https://gitlab.com/Remmina/Remmina/merge_requests/2407) *@antenore*

## v1.4.25
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.24...v1.4.25)

* kiosk: Drop GNOME MediaKeys plugin [!2377](https://gitlab.com/Remmina/Remmina/merge_requests/2377) *@jbicha*
* Honour soft links target in SFTP [!2379](https://gitlab.com/Remmina/Remmina/merge_requests/2379) *@antenore*
* Optional close confirmation [!2380](https://gitlab.com/Remmina/Remmina/merge_requests/2380) *@antenore*
* Fix some build warnings [!2382](https://gitlab.com/Remmina/Remmina/merge_requests/2382) *@donoban*
* Fix manpages [!2378](https://gitlab.com/Remmina/Remmina/merge_requests/2378) *@Fantu*
* Snap cleanup + kwallet support [!2381](https://gitlab.com/Remmina/Remmina/merge_requests/2381) *@antenore*
* Deprecations and amend g_date_time_format_iso8601 [!2383](https://gitlab.com/Remmina/Remmina/merge_requests/2383) *@antenore*
* Fixes to snap build [!2384](https://gitlab.com/Remmina/Remmina/merge_requests/2384) *@giox069*
* Removing dependencies that are available as extensions [!2385](https://gitlab.com/Remmina/Remmina/merge_requests/2385) *@antenore*
* FreeRDP_OffscreenSupportLevel is of type UINT32 [!2386](https://gitlab.com/Remmina/Remmina/merge_requests/2386) *@akallabeth*
* Minor fixes [!2387](https://gitlab.com/Remmina/Remmina/merge_requests/2387) *@antenore*
* Get the right value for FreeRDP_AutoReconnectMaxRetries [!2388](https://gitlab.com/Remmina/Remmina/merge_requests/2388) *@antenore*

## v1.4.24
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.23...v1.4.24)

* Contribution section added to issue template [!2365](https://gitlab.com/Remmina/Remmina/merge_requests/2365) *@kingu*
* Language of VNC encoding cleaned up [!2367](https://gitlab.com/Remmina/Remmina/merge_requests/2367) *@kingu*
* Remmina Hardening and Compliance [!2366](https://gitlab.com/Remmina/Remmina/merge_requests/2366) *@antenore*
* Remmina_preferences language reworked [!2368](https://gitlab.com/Remmina/Remmina/merge_requests/2368) *@kingu*
* Thanks 2021 [!2371](https://gitlab.com/Remmina/Remmina/merge_requests/2371) *@kingu*
* Resolve "Follow-up from "Remmina_preferences language reworked"" [!2369](https://gitlab.com/Remmina/Remmina/merge_requests/2369) *@antenore*
* Encryption level language reworked [!2372](https://gitlab.com/Remmina/Remmina/merge_requests/2372) *@kingu*
* Issue 2122 : Confirm on close of window [!2374](https://gitlab.com/Remmina/Remmina/merge_requests/2374) *@emmguyot*
* Adding flush and cairo clean up [!2375](https://gitlab.com/Remmina/Remmina/merge_requests/2375) *@antenore*

## v1.4.23
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.22...v1.4.23)

* Patch for a Remmina segfault and stats code cleaning [!2358](https://gitlab.com/Remmina/Remmina/merge_requests/2358) *@antenore*
* Make Appindicator optional [!2359](https://gitlab.com/Remmina/Remmina/merge_requests/2359) *@antenore*
* Added check-box to force tight encoding for VNC connections [!2360](https://gitlab.com/Remmina/Remmina/merge_requests/2360) *@antenore*
* remote resolution: use multiple of four [!2353](https://gitlab.com/Remmina/Remmina/merge_requests/2353) *@eworm-de*
* Add Keyboard mapping per client RDP [!2361](https://gitlab.com/Remmina/Remmina/merge_requests/2361) *@headkaze*
* Improve TLS error message, fixes #2364 [!2362](https://gitlab.com/Remmina/Remmina/merge_requests/2362) *@antenore*
* Triage policy language reworked [!2363](https://gitlab.com/Remmina/Remmina/merge_requests/2363) *@kingu*

## v1.4.22
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.21...v1.4.22)

* Fix crash if main window is closed #1692 [!2330](https://gitlab.com/Remmina/Remmina/merge_requests/2330) *@broth-itk*
* Main window position reset after opening a connection (issue 2587) [!2331](https://gitlab.com/Remmina/Remmina/merge_requests/2331) *@broth-itk*
* File Interface refactoring [!2332](https://gitlab.com/Remmina/Remmina/merge_requests/2332) *@antenore*
* CMake refactoring and build time warnings [!2333](https://gitlab.com/Remmina/Remmina/merge_requests/2333) *@antenore*
* Add Croatian language to desktop shortcuts and infos [!2334](https://gitlab.com/Remmina/Remmina/merge_requests/2334) *@muzena*
* Appdata corrections and renewal [!2336](https://gitlab.com/Remmina/Remmina/merge_requests/2336) *@kingu*
* Fixes for freerdp3 compatibility. [!2337](https://gitlab.com/Remmina/Remmina/merge_requests/2337) *@giox069*
* X2Go: Rewrite dialog-system; Ask users which session to resume... [!2328](https://gitlab.com/Remmina/Remmina/merge_requests/2328) *@D0n1elT*
* int main(): print instructions how to enable a more verbose output of remmina [!2338](https://gitlab.com/Remmina/Remmina/merge_requests/2338) *@D0n1elT*
* Mitigations for #2635 (default printer) with freerdp < 3 [!2343](https://gitlab.com/Remmina/Remmina/merge_requests/2343) *@giox069*
* X2Go: Add a session-terminate button into the session resuming selection menu [!2339](https://gitlab.com/Remmina/Remmina/merge_requests/2339) *@D0n1elT*
* Properly warn users when using a plugin which requires GtkSocket [!2340](https://gitlab.com/Remmina/Remmina/merge_requests/2340) *@D0n1elT*
* x2go_plugin.c: Make changes to source strings for translations. [!2344](https://gitlab.com/Remmina/Remmina/merge_requests/2344) *@D0n1elT*
* Fix ubuntu-impish-amd64 build errors. [!2345](https://gitlab.com/Remmina/Remmina/merge_requests/2345) *@D0n1elT*
* Hopefully fix Ubuntu Impish Build [!2347](https://gitlab.com/Remmina/Remmina/merge_requests/2347) *@D0n1elT*
* Fix string format [!2348](https://gitlab.com/Remmina/Remmina/merge_requests/2348) *@antenore*
* 2634-ssh-opening-command [!2342](https://gitlab.com/Remmina/Remmina/merge_requests/2342) *@merarischreoder*
* New debug strings reworked [!2341](https://gitlab.com/Remmina/Remmina/merge_requests/2341) *@kingu*
* Deduplicated "Started PyHoca" string [!2346](https://gitlab.com/Remmina/Remmina/merge_requests/2346) *@kingu*
* Removing the Remmina stats sender and repurposing Remmina stats [!2350](https://gitlab.com/Remmina/Remmina/merge_requests/2350) *@antenore*
* X2Go: Major rewrite of session-terminating system. [!2349](https://gitlab.com/Remmina/Remmina/merge_requests/2349) *@D0n1elT*
* Message about debugging info reworked [!2351](https://gitlab.com/Remmina/Remmina/merge_requests/2351) *@kingu*
* Strings in rcw.c reworked [!2352](https://gitlab.com/Remmina/Remmina/merge_requests/2352) *@kingu*
* VNC custom encodings to avoid corrupted frames. [!2354](https://gitlab.com/Remmina/Remmina/merge_requests/2354) *@antenore*
* X2Go format string bugs [!2355](https://gitlab.com/Remmina/Remmina/merge_requests/2355) *@antenore*

## v1.4.21
[full changelog](https://gitlab.com/Remmina/Remmina/compare/v1.4.20...v1.4.21)

* Nullify host if qucikconnect isn't a valid address [!2298](https://gitlab.com/Remmina/Remmina/merge_requests/2298) *@antenore*
* rdp_plugin.c: Fix dereferencing of NULL variable when profile name is empty [!2299](https://gitlab.com/Remmina/Remmina/merge_requests/2299) *@agunnerson-ibm*
* Resolve label spacing in preferences window [!2300](https://gitlab.com/Remmina/Remmina/merge_requests/2300) *@antenore*
* GNOME 40 runtime and other updates [!2302](https://gitlab.com/Remmina/Remmina/merge_requests/2302) *@antenore*
* Add support for ESX web console login [!2303](https://gitlab.com/Remmina/Remmina/merge_requests/2303) *@antenore*
* Added RDP general option to remap scancodes [!2304](https://gitlab.com/Remmina/Remmina/merge_requests/2304) *@antenore*
* Back compatibility with WebKit < 2.32.0 [!2305](https://gitlab.com/Remmina/Remmina/merge_requests/2305) *@antenore*
* Make screenshot file names iso8601 compliant [!2306](https://gitlab.com/Remmina/Remmina/merge_requests/2306) *@antenore*
* Typo in bug-report template [!2308](https://gitlab.com/Remmina/Remmina/merge_requests/2308) *@kingu*
* Typo in merge-request template [!2309](https://gitlab.com/Remmina/Remmina/merge_requests/2309) *@kingu*
* Implementing restricted-mode and password hash [!2307](https://gitlab.com/Remmina/Remmina/merge_requests/2307) *@antenore*
* Unitialized var by @qarmin (Rafał Mikrut ). Closes #2594 [!2310](https://gitlab.com/Remmina/Remmina/merge_requests/2310) *@antenore*
* Include X2Go plugin [!2301](https://gitlab.com/Remmina/Remmina/merge_requests/2301) *@D0n1elT*
* Add integer-only input field for plugin settings in Remmina Editor [!2311](https://gitlab.com/Remmina/Remmina/merge_requests/2311) *@D0n1elT*
* Adding VNCI Listen port field tooltip [!2314](https://gitlab.com/Remmina/Remmina/merge_requests/2314) *@antenore*
* Using core20 for snap [!2315](https://gitlab.com/Remmina/Remmina/merge_requests/2315) *@antenore*
* Fix the translation problem of "tooltip" in ssh window [!2317](https://gitlab.com/Remmina/Remmina/merge_requests/2317) *@HeroesLoveToRoujiamo*
* X2Go plugin language reworked [!2313](https://gitlab.com/Remmina/Remmina/merge_requests/2313) *@kingu*
* Rework x2go_plugin.c to comply with remmina coding style. [!2319](https://gitlab.com/Remmina/Remmina/merge_requests/2319) *@D0n1elT*
* Add validation system for Remmina Editor [!2312](https://gitlab.com/Remmina/Remmina/merge_requests/2312) *@D0n1elT*
* Small changes to README, AUTHORS and error message adjusting in remmina_file_editor.c [!2320](https://gitlab.com/Remmina/Remmina/merge_requests/2320) *@D0n1elT*
* remmina_file_editor.c: Readd by mistake removed '!' [!2322](https://gitlab.com/Remmina/Remmina/merge_requests/2322) *@D0n1elT*
* Mitigation for #1951 and extra RDP clibpoard debug messages [!2323](https://gitlab.com/Remmina/Remmina/merge_requests/2323) *@giox069*
* X2Go plugin language reworked 2 [!2321](https://gitlab.com/Remmina/Remmina/merge_requests/2321) *@kingu*
* Rollback Let's Encrypt SSL workaround [!2324](https://gitlab.com/Remmina/Remmina/merge_requests/2324) *@antenore*
* Add RDP clipboard support for Microsoft HTML Clipboard Format [!2325](https://gitlab.com/Remmina/Remmina/merge_requests/2325) *@giox069*
* Building FreeRDP with icu support [!2326](https://gitlab.com/Remmina/Remmina/merge_requests/2326) *@antenore*
* Resource renaming to comply with the Freedesktop rules [!2327](https://gitlab.com/Remmina/Remmina/merge_requests/2327) *@antenore*

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

*This Change Log was automatically generated by [gitlab_awesome_release](https://gitlab.com/sue445/gitlab_awesome_release)*
