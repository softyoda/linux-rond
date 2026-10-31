from pathlib import Path
import subprocess,tempfile,unittest
SCRIPT=Path(__file__).resolve().parents[1]/'scripts/configure-tray.py'
class Tray(unittest.TestCase):
    def test_only_arc_panel_and_idempotent(self):
        other='[Containments][3]\nplugin=org.kde.panel\n[Containments][3][Applets][4]\nplugin=org.kde.plasma.systemtray\n[Containments][3][Applets][4][General]\nextraItems=untouched\n'
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'plasma.ini'
            p.write_text('[Containments][1]\nplugin=org.softyoda.arcpanel\n[Containments][1][Applets][2]\nplugin=org.kde.plasma.systemtray\n[Containments][1][Applets][2][General]\nextraItems=old\n'+other)
            def run():subprocess.run(['python3',str(SCRIPT),str(p)],check=True,capture_output=True)
            run();first=p.read_text();run()
            self.assertEqual(first,p.read_text())
            self.assertIn(other,first)
            self.assertIn('scaleIconsToFit=true',first)
if __name__=='__main__':unittest.main()
