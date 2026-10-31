# Firefox vertical-tab demo

Prepare a fresh profile; this command does not launch Firefox or touch existing profiles:

```sh
python3 tests/integration/prepare-firefox-demo.py --root /tmp/linux-rond-engine-dev/firefox-demo-profiles
```

The printed manifest contains the profile path and launch arguments. It creates three local
pages and a `user.js` enabling native vertical tabs on the left in left-to-right locales:

```js
user_pref("sidebar.revamp", true);
user_pref("sidebar.verticalTabs", true);
user_pref("sidebar.position_start", true);
user_pref("sidebar.visibility", "always-show");
user_pref("sidebar.expandOnHover", false);
```

These names and the automatic initial expansion were checked in the installed Firefox 158.0.1:

```sh
unzip -p /usr/lib/firefox/browser/omni.ja defaults/preferences/firefox.js
unzip -p /usr/lib/firefox/omni.ja moz-src/browser/components/sidebar/SidebarState.sys.mjs
```

`unzip -p` handles these optimized archives even when Python's `zipfile` rejects them.
No extension, CSS override, browser account, or normal profile configuration is required.

Launch only after selecting the intended isolated compositor environment:

```sh
MOZ_ENABLE_WAYLAND=1 firefox --new-instance --profile /path/from/manifest
```

Retain the demo session's `XDG_RUNTIME_DIR`, `WAYLAND_DISPLAY`, and `DBUS_SESSION_BUS_ADDRESS`.
`--new-instance` and `--profile` are advertised by this Firefox's `--help` and prevent forwarding
this launch to an existing browser instance. `--no-remote` is not advertised by this version;
the prepared command does not depend on it.

Close that Firefox instance before removing its generated directory. The helper never edits
`profiles.ini`, `installs.ini`, or another profile. Visual validation remains a separate step.
