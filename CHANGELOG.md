# Changelog

Current Remmina changelog, see CHANGELOG.archive.md for old releases.

## [v1.3.0](https://gitlab.com/Remmina/Remmina/tags/v1.3.0) (2019-01-18)

### Most notable

- Use window resolution #1 (by Giovanni Panozzo)
- rcw_preopen complete (by Giovanni Panozzo)
- RDP: new global parameter rdp_map_keycode (by Giovanni Panozzo)
- Use decimal instead of hex on rdp keycode map (by Giovanni Panozzo)

### Translations

- Adding basic script to update the po files (by Antenore Gatta)
- Added remmina.pot (by Antenore Gatta)
- Change es_VE.po header (Language-Team and Language) (by Antenore Gatta)
- Fixed some Italian translations (by Antenore Gatta)
- Fixing es_VE Spanish #1797 (by Antenore Gatta)
- French translation update for v1.3.0 (by Davy Defaud)
- Improving i18n subsystem (by Antenore Gatta)
- Russian translation updated (fully translated) (by TreefeedXavier)
- Translations - Fixing errors reported in the issue #1797 (by Antenore Gatta)
- Update Hungarian translation (by Meskó Balázs)
- Update de.po (by Ozzie Isaacs)
- Update tr.po (by Serdar Sağlam)
- Updated german translation (by Ozzie Isaacs)
- Updated ru.po (by TreefeedXavier)
- Updated translators list ru.po (by TreefeedXavier)
- Updating da.po (by Antenore Gatta)
- Updating po translation files (by Antenore Gatta)
- Updating translation credits (by Antenore Gatta)

### Other enhancements and fixes

- Added xrdp friendly options Closes #1742 (by Antenore Gatta)
- Adding language detection (by Antenore Gatta)
- Auth panel widget placement (by Antenore Gatta)
- CSS modifications to adapt to stock Gnome and Gtk themes (by Antenore Gatta)
- Correctly set focus after rcw_preopen (by Giovanni Panozzo)
- Deprecates dynamic_resolution_width and height cfg params (by Giovanni Panozzo)
- Disable glyph cache by default (by Antenore Gatta)
- Fix crash when showing password panel (by Giovanni Panozzo)
- Fix gtk_window_present_with_time() time (by Giovanni Panozzo)
- Fixed build for the flatpak (by Antenore Gatta)
- Fixed missing icons (by Antenore Gatta)
- Gtk deprecation and CSS restzling (by Antenore Gatta)
- Gtk icon cache update during install phase (by Antenore Gatta)
- Icons and gtk fixes for rcw_reopen (by Antenore Gatta)
- Make menu items paintable by the application (by Antenore Gatta)
- Open connection window before connecting (by Giovanni Panozzo)
- Prevent toolbar signals while reconfiguring toolbar (by Giovanni Panozzo)
- RDP fixes: remove redundant rfi->width/rfi->height and more (by Giovanni Panozzo)
- RDP: correctly destroy rfi->surface during a desktop resize (by Giovanni Panozzo)
- RDP: move gdi_resize() to a better place (by Giovanni Panozzo)
- RDP: remove unneeded OrderSupport struct init (by Giovanni Panozzo)
- Remove deprecated floating toolbar toplevel window (by Giovanni Panozzo)
- Search box clear icon (by Giovanni Panozzo)
- Update CONTRIBUTING.md (by Antenore Gatta)
- Update README.md (by Antenore Gatta)
- Update toolbar button handling (by Giovanni Panozzo)
- Updated CSS to have black background in fullscreen (by Antenore Gatta)
- Updated sponsor list (by Antenore Gatta)
- Updated wiki URLs (by Antenore Gatta)
- Updating Doxygen config (by Antenore Gatta)
- Updating coyright for year 2019 (by Antenore Gatta)
- VNC: Fix possible crash during connection (by Giovanni Panozzo)
- allow closing tab after error message panel is shown (by Giovanni Panozzo)
- flatpak: update freerdp from 2.0.0-rc3 to 2.0.0-rc4 (by Denis Ollier)
- flatpak: update gnome sdk from 3.28 to 3.30 (by Denis Ollier)
- flatpak: update libssh from 0.8.5 to 0.8.6 (by Denis Ollier)
- flatpak: update libvncserver from 0.9.11 to 0.9.12 (by Denis Ollier)
- flatpak: update nx-libs from 3.5.99.16 to 3.5.99.17 (by Denis Ollier)
- flatpak: update six from 1.11.0 to 1.12.0 (by Denis Ollier)

## [v1.2.32.1](https://gitlab.com/Remmina/Remmina/tags/v1.2.32.1) (2018-11-14)

- Add desktop-gnome-platform and fix themes in SNAP, fixes issue #1730 and fixes missing SNAP localization (Giovanni Panozzo)
- Fix SNAP icon (Giovanni Panozzo)
- flatpak: update libssh from 0.8.2 to 0.8.3 (Denis Ollier)
- flatpak: update libssh from 0.8.3 to 0.8.4 (Denis Ollier)
- flatpak: update libssh from 0.8.4 to 0.8.5 (Denis Ollier)
- flatpak: update lz4 from 1.8.2 to 1.8.3 (Denis Ollier)
- flatpak: update pyparsing from 2.2.0 to 2.2.1 (Denis Ollier)
- flatpak: update pyparsing from 2.2.1 to 2.2.2 (Denis Ollier)
- flatpak: update pyparsing from 2.2.2 to 2.3.0 (Denis Ollier)
- Implement smartcard name setting. Should fix #1737 (Antenore Gatta)
- man+help: elaborate on file types of -connect and -edit cmd line options (Mikkel Kirkgaard Nielsen)
- RDP: add FREERDP_ERROR_SERVER_DENIED_CONNECTION message (Giovanni Panozzo)
- Removing X11Forwarding code as it is wrong and causing issues (Antenore Gatta)
- Update fix tr.po (Serdar Sağlam)

## [v1.2.32](https://gitlab.com/Remmina/Remmina/tags/v1.2.32) (2018-10-06)

- Avoid to save last_success property if stats are not enabled. @antenore
- GW auth data was saved in the server auth data. @antenore
- Do not register socket plugins when X11 is not available. @antenore
- Screenshot enhancements. @antenore
- RDP GW authentication. @antenore
- Adding global preference for search bar visibility. @antenore
- Allow wayland backend again when GTK >= 3.22.27. @giox069
- tr.po @TeknoMobil
- Add option to honour https_proxy and http_proxy environment variable. @antenore
- Force program name to app id. @antenore
- Printing builds flags with remmina --full-version command option. @antenore
- New plugin Simple Terminal. @antenore
- Fix KB grabbing when switching workspace. @giox069
- Dealing correcthly with some deprecations, getting rid of most of G_GNUC_BEGIN_IGNORE_DEPRECATIONS. @antenore
- Improving file type hadling. @antenore
- Adding error check on remmina_pref_save. @antenore
- Many bug fixing as usual.

## [v1.2.31.4](https://gitlab.com/Remmina/Remmina/tags/v1.2.31.4) (2018-08-24)

This is a bug fixing release.

**Fixed bugs:**

- Fix KB grabbing when switching workspace.
- Fix some possible crashes when reading a remminafile.
- Fixes a crash deleting XDMCP profile.
- Fixing libssh deprecations.

**Implemented enhancements:**

- Improving file type hadling.
- flatpak: update libssh from 0.7.5 to 0.8.1
- flatpak: update freerdp from 2.0.0-rc2 to 2.0.0-rc3
- Snap: update to libssh 0.8.0.

## [v1.2.31.3](https://gitlab.com/Remmina/Remmina/tags/v1.2.31.3) (2018-08-14)

This is a bug fixing release.

**Fixed bugs:**

- Do not send stats if the remmina.pref file is read-only.
- cmake: include libssh_threads only when available.
- Set program class to REMMINA_APP_ID, fixes #1706.

**Implemented enhancements:**

- Implement send ctrl+alt+fn keys. Closes #1707.

## [v1.2.31.1](https://gitlab.com/Remmina/Remmina/tags/v1.2.31.1) (2018-07-31)

This is a bug fixing release.

**Fixed bugs:**

- Cannot minimize in fullscreen mode.
- Crash with the RDP plugin.
- Missing manual pages for Debian.

**Closed issues:**

- Minimize window doesn't minimize the window [\#1696](https://gitlab.com/Remmina/Remmina/issues/1696)
- Minimize window button is disable, after connecting to RDS [\#1700](https://gitlab.com/Remmina/Remmina/issues/1700)

## [v1.2.31](https://gitlab.com/Remmina/Remmina/tags/v1.2.31) (2018-07-28)

This is the first release on GitLab and we are still moving and adapting tools and
integrations, therefore this changelog is quite short than usually.

Notables changes since the last release are:

- Custom color schemes per profile for the SSH plugin (@denk_mal).
- Flatpak updates and fixes (@larchunix)
- Kiosk mode with integration in the login manager (@antenore).
- New Icons (@antenore and @larchunix).
- SFTP tool password fixes (@Feishi).
- Several fixes around RDP and compilations issues (@giox069 and @larchunix).

