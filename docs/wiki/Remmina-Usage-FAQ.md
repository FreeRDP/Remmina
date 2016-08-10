# Remmina Usage FAQ

1. **Where are all my connections and preferences stored ?**  
**A:** Remmina stores its configuration file in directories identified according the [XDG Base Directory Specification](http://www.freedesktop.org/Standards/basedir-spec).
A main configuration file `remmina.pref` is stored into `$XDG_CONFIG_HOME/remmina/` directory, which defaults to `$HOME/.config/remmina/`.
Every single connection profile is stored into a `XXXXXXXXXXXXX.remmina` file under `$XDG_DATA_HOME/remmina` directory, which defaults to `$HOME/.local/share/remmina`.
Older versions of remmina used to store all `*.remmina` and `remmina.pref` files under `$HOME/.remmina`. If a newer version of remmina finds an existing `$HOME/remmina/` legacy directory, then remmina will revert back to use it.

2. **Can I start a connection using a shell command ?**  
**A:** Yes, use `remmina -c filename`.

3. **When I'm in fullscreen mode, I cannot see the "floating toolbar" (the bar at the top of the screen), or I can see part of it but it does not scroll down or receive my input.**  
**A:** You are using Gnome Shell and an older version of remmina. This has been fixed under remmina 1.2.0-rcgit.3 compiled with GTK+ 3.10 or newer. Please restart your system after upgrading remmina.

4. **The "Quality settings" in Remmina preferences -> RDP is not saving the default quality setting.**  
**A:** This is a quality profile editor, not a quality chooser. See issue #521

5. **I'm unable to escape from VMware vSphere client console pressing Ctrl+Alt.**  
**A:** You are probably pushing the Ctrl key on the right side of the keyboard. That key is reserved as Remmina Host key. Use the left Ctrl key or change the host key in the Remmina global settings.

6. **Under Gnome Shell the Remmina systray menu is hidden in the hidden notification area**  
**A:** Install this [AppIndicator Support Gnome Shell Extension](https://extensions.gnome.org/extension/615/appindicator-support/) to see the systray menu in a more confortable place.

7. **After a Remmina upgrade, I'm unable to connect to some servers.**  
**A:** Try to remove the file `~/.config/freerdp/known_hosts`.

8. **After a Remmina upgrade, I have to reenter all passwords.**  
**A:** Yes, with commit 57ec85d8e9bf773b8a08c17a0218b6cd643c828b we switched to libsecret, and passwords cannot be imported automatically.
