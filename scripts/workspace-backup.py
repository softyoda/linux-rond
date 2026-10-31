#!/usr/bin/env python3
"""Snapshot and restore only packages owned by workspace activation."""
import hashlib
import json
import shutil
import sys
from pathlib import Path

PACKAGES = {
    "org.softyoda.arcpanel": "plasma/org.softyoda.arcpanel",
    "org.softyoda.roundworkspacecontrols": "plasma/round-workspace-controls",
}

def digest(path):
    if not path.exists():
        return None
    result = hashlib.sha256()
    for entry in sorted(path.rglob("*")):
        if entry.is_file():
            result.update(str(entry.relative_to(path)).encode())
            result.update(entry.read_bytes())
    return result.hexdigest()

mode, backup_arg, data_arg, *args = sys.argv[1:]
backup, data = Path(backup_arg), Path(data_arg)
if mode == "save":
    release, safe_value, safe_loaded = args
    if (backup / "state.json").exists():
        state = json.loads((backup / "state.json").read_text())
        for name, source in PACKAGES.items():
            record = state["packages"][name]
            expected = record["expected"]
            if not isinstance(expected, list):
                expected = [expected]
            current = digest(data / "plasma/plasmoids" / name)
            if current not in [None, record["original"], *expected]:
                raise SystemExit(f"Package changed locally; activation stopped: {name}")
            new = digest(Path(release) / source)
            record["expected"] = list(dict.fromkeys([*expected, new]))
        (backup / "state.json").write_text(json.dumps(state, indent=2) + "\n")
        sys.exit(0)
    backup.mkdir(parents=True, exist_ok=True, mode=0o700)
    records = {}
    for name, source in PACKAGES.items():
        installed = data / "plasma/plasmoids" / name
        records[name] = {"original": digest(installed), "expected": [digest(Path(release) / source)]}
        if installed.exists():
            shutil.copytree(installed, backup / name, symlinks=True, dirs_exist_ok=True)
    state = {"safeAreaValue": safe_value, "safeAreaLoaded": safe_loaded == "true", "packages": records}
    (backup / "state.json").write_text(json.dumps(state, indent=2) + "\n")
elif mode == "restore":
    state = json.loads((backup / "state.json").read_text())
    for name, record in state["packages"].items():
        installed = data / "plasma/plasmoids" / name
        current = digest(installed)
        if current == record["original"]:
            continue
        expected = record["expected"] if isinstance(record["expected"], list) else [record["expected"]]
        if current not in [None, *expected]:
            raise SystemExit(f"Package changed locally; preserved: {installed}. Backup: {backup / name}")
        if installed.exists():
            shutil.rmtree(installed)
        if record["original"] is not None:
            shutil.copytree(backup / name, installed, symlinks=True)
else:
    raise SystemExit("Expected save or restore")
