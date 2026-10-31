import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/workspace-backup.py"

class WorkspaceBackup(unittest.TestCase):
    def test_upgrade_rollback_preserves_original_packages(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            backup, data, release = (root / name for name in ("backup", "data", "release"))
            installed = data / "plasma/plasmoids/org.softyoda.arcpanel"
            installed.mkdir(parents=True)
            (installed / "main.qml").write_text("original")
            for name in ("org.softyoda.arcpanel", "round-workspace-controls"):
                path = release / "plasma" / name
                path.mkdir(parents=True)
                (path / "main.qml").write_text("new")
            def run(mode, *args, check=True):
                return subprocess.run(["python3", str(SCRIPT), mode, str(backup), str(data), *map(str,args)],
                                      check=check, capture_output=True, text=True)
            run("save", release, "true", "true")
            shutil.copy2(release / "plasma/org.softyoda.arcpanel/main.qml", installed / "main.qml")
            controls = data / "plasma/plasmoids/org.softyoda.roundworkspacecontrols"
            shutil.copytree(release / "plasma/round-workspace-controls", controls)
            (release / "plasma/org.softyoda.arcpanel/main.qml").write_text("upgrade")
            run("save", release, "false", "false")
            self.assertEqual(json.loads((backup / "state.json").read_text())["safeAreaValue"], "true")
            shutil.copy2(release / "plasma/org.softyoda.arcpanel/main.qml", installed / "main.qml")
            (installed / "main.qml").write_text("user edit")
            self.assertNotEqual(run("restore", check=False).returncode, 0)
            self.assertEqual((installed / "main.qml").read_text(), "user edit")
            self.assertNotEqual(run("save", release, "false", "false", check=False).returncode, 0)
            (installed / "main.qml").write_text("upgrade")
            run("restore")
            self.assertEqual((installed / "main.qml").read_text(), "original")
            self.assertFalse(controls.exists())

if __name__ == "__main__":
    unittest.main()
