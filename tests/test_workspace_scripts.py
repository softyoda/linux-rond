import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MOCK = r'''#!/usr/bin/env python3
import json,os,shutil,sys
from pathlib import Path
command=Path(sys.argv[0]).name
args=sys.argv[1:]
root=Path(os.environ['MOCK_ROOT'])
state_file=root/'state.json'
state=json.loads(state_file.read_text())
with (root/'trace.jsonl').open('a') as f: f.write(json.dumps([command,*args])+'\n')
def save(): state_file.write_text(json.dumps(state))
if command=='kwin_wayland': print('kwin 6.8.1')
elif command=='cmake' and '--build' in args:
    build=Path(args[args.index('--build')+1]);build.mkdir(exist_ok=True)
    for name in ['linux_rond_workspace.so','linux_rond_workspace_loader.so']:(build/name).write_text('test binary')
elif command=='kreadconfig6': print(state['safeValue'])
elif command=='kwriteconfig6':
    if 'kwinrc' in args: state['safeValue']=args[-1];save()
elif command=='kpackagetool6':
    source=Path(args[-1]);name=json.loads((source/'metadata.json').read_text())['KPlugin']['Id']
    target=Path(os.environ['XDG_DATA_HOME'])/'plasma/plasmoids'/name
    if target.exists():shutil.rmtree(target)
    shutil.copytree(source,target)
elif command=='qdbus6':
    method=args[2]
    if method.endswith('StateJson'):
        if not state['loaded']:sys.exit(1)
        print(json.dumps({'ready':True,'enabled':state['enabled'],'captureActive':False,'output':{'width':720}}))
    elif method.endswith('SetEnabled'):state['enabled']=args[-1]=='true';save()
    elif method.endswith('.loadEffect'):
        if os.environ.get('MOCK_LOAD_FAIL'):print('false')
        else:state['loaded']=True;save();print('true')
    elif method.endswith('.unloadEffect'):state['loaded']=False;save()
    elif method.endswith('.LoadPlugin'):print('true')
    elif method.endswith('.isScriptLoaded'):print(str(state['safeLoaded']).lower())
    elif method.endswith('.unloadScript'):state['safeLoaded']=False;save();print('true')
    elif method.endswith('.loadScript'):state['safeLoaded']=True;save();print('1')
'''

class WorkspaceScripts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        binary = self.root / 'bin'
        binary.mkdir()
        for name in ['cmake','ctest','kwin_wayland','qdbus6','kreadconfig6','kwriteconfig6','systemctl','kpackagetool6']:
            path=binary/name;path.write_text(MOCK);path.chmod(0o755)
        (self.root/'state.json').write_text(json.dumps(dict(loaded=False,enabled=False,safeValue='true',safeLoaded=True)))
        self.env=dict(os.environ, MOCK_ROOT=str(self.root),XDG_DATA_HOME=str(self.root/'data'),
                      XDG_CONFIG_HOME=str(self.root/'config'),PATH=str(binary)+os.pathsep+os.environ['PATH'])
        self.workspace=self.root/'data/linux-rond/workspace'
        original=self.root/'data/plasma/plasmoids/org.softyoda.arcpanel'
        original.mkdir(parents=True)
        (original/'original').write_text('preserve')
    def run_script(self,name,*args,check=True,**env):
        return subprocess.run(['bash',str(ROOT/'scripts'/name),*args],env={**self.env,**env},
                              check=check,capture_output=True,text=True)
    def trace(self):return [json.loads(line) for line in (self.root/'trace.jsonl').read_text().splitlines()]
    def test_prepare_does_not_activate(self):
        self.run_script('install-workspace.sh')
        self.assertTrue((self.workspace/'prepared/plugins/kwin/effects/plugins/linux_rond_workspace.so').exists())
        self.assertFalse((self.workspace/'current').exists())
        self.assertFalse(any(entry[0] in ['qdbus6','kwriteconfig6','kpackagetool6','systemctl'] for entry in self.trace()))
    def test_enable_then_targeted_restore(self):
        self.run_script('install-workspace.sh','--enable')
        entries=self.trace()
        off=next(i for i,e in enumerate(entries) if e[0]=='qdbus6' and e[3].endswith('.unloadScript'))
        on=next(i for i,e in enumerate(entries) if e[0]=='qdbus6' and e[3].endswith('.SetEnabled') and e[-1]=='true')
        self.assertLess(off,on)
        self.assertTrue((self.root/'config/autostart/linux-rond-workspace.desktop').exists())
        self.run_script('disable-workspace.sh')
        self.assertFalse((self.root/'config/autostart/linux-rond-workspace.desktop').exists())
        self.assertEqual((self.root/'data/plasma/plasmoids/org.softyoda.arcpanel/original').read_text(),'preserve')
        self.assertFalse((self.root/'data/plasma/plasmoids/org.softyoda.roundworkspacecontrols').exists())
        state=json.loads((self.root/'state.json').read_text())
        self.assertEqual(state['safeValue'],'true');self.assertTrue(state['safeLoaded']);self.assertFalse(state['loaded'])
    def test_failed_load_restores_previous_state(self):
        result=self.run_script('install-workspace.sh','--enable',check=False,MOCK_LOAD_FAIL='1')
        self.assertNotEqual(result.returncode,0)
        state=json.loads((self.root/'state.json').read_text())
        self.assertEqual(state['safeValue'],'true');self.assertTrue(state['safeLoaded'])
        self.assertTrue((self.root/'data/plasma/plasmoids/org.softyoda.arcpanel/original').exists())
        self.assertFalse((self.root/'config/autostart/linux-rond-workspace.desktop').exists())

if __name__=='__main__':unittest.main()
