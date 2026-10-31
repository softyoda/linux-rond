#!/usr/bin/env python3
"""Create an isolated Firefox demo profile without launching the browser."""
import argparse
import html
import json
from pathlib import Path
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--root", type=Path, help="Parent directory for a fresh demo profile")
args = parser.parse_args()
if args.root:
    args.root.mkdir(parents=True, exist_ok=True, mode=0o700)
profile = Path(tempfile.mkdtemp(prefix="linux-rond-firefox-", dir=args.root))
pages = profile / "demo-pages"
pages.mkdir()
urls = []
for name, title, subtitle in [
    ("home", "Linux Rond", "Firefox with native vertical tabs"),
    ("reading", "Reading", "Check text clarity in square and circular modes"),
    ("interaction", "Interaction", "Try a link, a button, and a text field"),
]:
    page = pages / (name + ".html")
    body = ("<button onclick=\"this.textContent='Clicked'\">Touch or click</button> "
            "<input aria-label=\"Test text\" placeholder=\"Type here\">") if name == "interaction" else (
        "<p>These pages are stored locally in this temporary profile. "
        "Use the left tab strip to switch pages and compare the projection.</p>" * 4)
    page.write_text("<!doctype html><html lang=en><meta charset=utf-8><title>" + html.escape(title) +
                   "</title><style>body{margin:24px;background:#172331;color:#e8edf5;font:18px/1.6 sans-serif}"
                   "h1{color:#81c9fa;font-size:30px}a{color:#a5d9ff}button,input{font:inherit;padding:10px;max-width:90%}"
                   "nav{display:flex;gap:14px;flex-wrap:wrap}</style><nav><a href=home.html>Home</a>"
                   "<a href=reading.html>Reading</a><a href=interaction.html>Interaction</a></nav><h1>" +
                   html.escape(title) + "</h1><p>" + html.escape(subtitle) + "</p>" + body + "</html>\n")
    urls.append(page.as_uri())
prefs = {
    "sidebar.revamp": True,
    "sidebar.verticalTabs": True,
    "sidebar.position_start": True,
    "sidebar.visibility": "always-show",
    "sidebar.expandOnHover": False,
    "browser.aboutwelcome.enabled": False,
    "browser.shell.checkDefaultBrowser": False,
    "browser.startup.firstrunSkipsHomepage": False,
    "browser.startup.homepage_override.mstone": "ignore",
    "startup.homepage_welcome_url": "",
    "startup.homepage_welcome_url.additional": "",
    "browser.startup.page": 1,
    "browser.startup.homepage": "|".join(urls),
}
(profile / "user.js").write_text("// Linux Rond isolated demo profile.\n" + "".join(
    f"user_pref({json.dumps(key)}, {json.dumps(value)});\n" for key, value in prefs.items()))
manifest = {
    "profile": str(profile),
    "verifiedBrowser": "Mozilla Firefox 158.0.1",
    "urls": urls,
    "command": ["firefox", "--new-instance", "--profile", str(profile)],
    "environment": {"MOZ_ENABLE_WAYLAND": "1"},
    "note": "Use the parent-provided isolated session environment. No browser has been launched.",
}
(profile / "linux-rond-demo.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
