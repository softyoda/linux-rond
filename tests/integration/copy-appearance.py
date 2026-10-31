#!/usr/bin/env python3
"""Copy only appearance keys into an isolated demo configuration."""
import configparser
import json
import re
from pathlib import Path
import sys

source, destination = map(Path, sys.argv[1:3])
config = configparser.ConfigParser(interpolation=None, strict=False)
config.optionxform = str
config.read(source/'kdeglobals')
result = configparser.ConfigParser(interpolation=None)
result.optionxform = str
allowed = {
    'General': {'ColorScheme','font','fixed','smallestReadableFont','menuFont','toolBarFont','taskbarFont'},
    'Icons': {'Theme'},
    'KDE': {'widgetStyle'},
}
for section in config.sections():
    if section.startswith(('Colors:', 'ColorEffects:')):
        result[section] = dict(config[section])
    elif section in allowed:
        values = {key:value for key,value in config[section].items() if key in allowed[section]}
        if values: result[section] = values
if 'General' not in result: result['General'] = {}
if not result['General'].get('ColorScheme'): result['General']['ColorScheme'] = 'BreezeDark'
destination.mkdir(parents=True,exist_ok=True)
with (destination/'kdeglobals').open('w') as file: result.write(file,space_around_delimiters=False)

panels=configparser.ConfigParser(interpolation=None,strict=False)
panels.optionxform=str
panels.read(source/'plasma-org.kde.plasma.desktop-appletsrc')
for section in panels.sections():
    match=re.fullmatch(r'(Containments\]\[\d+)\]\[Applets\]\[\d+',section)
    if not match or panels.get(section,'plugin',fallback='') not in ['org.kde.plasma.kickoff','org.kde.plasma.kicker']: continue
    if panels.get(match.group(1),'plugin',fallback='')!='org.softyoda.arcpanel': continue
    icon=panels.get(section+'][Configuration][General','icon',fallback='')
    if icon:
        (destination/'reference-launcher.json').write_text(json.dumps(icon))
        break
