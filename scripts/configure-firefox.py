#!/usr/bin/env python3
"""Enable native left-side tabs in an explicitly selected Firefox profile."""
import argparse
import json
import os
from pathlib import Path
import shutil
import tempfile

START = '// Linux Rond: vertical tabs\n'
END = '// End Linux Rond vertical tabs\n'
PREFS = {'sidebar.revamp': True, 'sidebar.verticalTabs': True,
         'sidebar.position_start': True, 'sidebar.visibility': 'always-show',
         'sidebar.expandOnHover': False}


def configure(profile):
    profile = Path(profile)
    if not profile.is_dir():
        raise ValueError('Firefox profile directory not found')
    target = profile / 'user.js'
    previous = target.read_text() if target.exists() else ''
    if START in previous:
        begin = previous.index(START)
        finish = previous.find(END, begin)
        if finish < 0:
            raise ValueError('Incomplete Linux Rond preference block; file preserved')
        previous = previous[:begin] + previous[finish + len(END):]
    block = START + ''.join(f'user_pref({json.dumps(k)}, {json.dumps(v)});\n' for k,v in PREFS.items()) + END
    updated = previous.rstrip('\n') + ('\n\n' if previous.strip() else '') + block
    if target.exists() and target.read_text() == updated:
        return None
    backup = None
    if target.exists():
        fd, name = tempfile.mkstemp(prefix='user.js.before-linux-rond-', dir=profile)
        os.close(fd); backup = Path(name); shutil.copyfile(target, backup)
    fd, temporary = tempfile.mkstemp(prefix='.linux-rond-user-', dir=profile)
    try:
        with os.fdopen(fd, 'w') as file:
            file.write(updated)
        os.replace(temporary, target)
    finally:
        Path(temporary).unlink(missing_ok=True)
    return backup


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', required=True, type=Path)
    args = parser.parse_args()
    backup = configure(args.profile)
    print('Vertical tabs configured. Firefox reads user.js at startup.')
    if backup: print(f'Previous preferences: {backup}')
