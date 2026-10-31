#!/usr/bin/env python3
"""Exercise actual client events in the isolated compositor."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time

BUILD, RUN = map(Path, sys.argv[1:3])
ROOT = Path(__file__).resolve().parents[2]
assert os.environ.get('LINUX_ROND_TEST_SESSION') == '1'
assert os.environ.get('WAYLAND_DISPLAY', '').startswith('linux-rond-test-')
children = []
checks = []


def call(method, *args):
    return subprocess.check_output(['qdbus6', 'org.kde.KWin', '/LinuxRond',
        'org.softyoda.LinuxRond.'+method, *map(str,args)], text=True).strip()


def label(window):
    prefix='org.softyoda.LinuxRondProbe.'
    app=window['appId']
    if app.startswith(prefix): return app[len(prefix):]
    return {'Terminal':'A','File manager':'B','Web browser':'C','Legacy application':'XW'}.get(window['title'],window['title'])


def state():
    return json.loads(call('StateJson'))


def wait_for(predicate):
    end = time.monotonic()+5
    while time.monotonic()<end:
        value=state()
        if predicate(value): return value
        time.sleep(.1)
    raise AssertionError(value)


def events():
    return [json.loads(line) for line in (RUN/'events.jsonl').read_text().splitlines()]


def input_event(action, *coords):
    subprocess.run([str(BUILD/'round_input_driver'),action,*map(str,coords)],check=True)
    time.sleep(.08)


def map_point(x, y, shape=1, window='A', global_source=False):
    s=state();w=next(w for w in s['windows'] if label(w)==window)
    source=w['sourceGeometry'];dest=s['content']
    if not global_source:
        x+=w['clientGeometry']['x'];y+=w['clientGeometry']['y']
    nx=2*(x-source['x'])/source['width']-1
    ny=2*(y-source['y'])/source['height']-1
    result=subprocess.check_output([str(BUILD/'round_map_point'),str(ROOT/'projection/approved-mesh.json'),
        str(shape),str(nx),str(ny),*[str(dest[k]) for k in ['x','y','width','height']]],text=True)
    return tuple(map(float,result.split()))


def click_source(x,y,shape=1,action='click',window='A'):
    before=len(events());input_event(action,*map_point(x,y,shape,window))
    kind='mouse-up' if action=='click' else 'touch-196'
    matches=[e for e in events()[before:] if e['event']==kind and e['window']==window]
    assert matches,(kind,window,events()[before:])
    e=matches[-1]
    assert abs(e['x']-x)<1 and abs(e['y']-y)<1,(x,y,e)
    checks.append(f'{action} {window} ({x},{y})')


def capture(name):
    result=subprocess.run(['spectacle','-b','-n','-f','-o',str(RUN/(name+'.png'))],
        stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=15)
    assert result.returncode==0


try:
    for x,y in [(360,360),(30,150),(690,150),(30,675),(690,675),(200,40)]:
        click_source(x,y)
        if (x,y)==(690,150):capture("input-pointer")
        click_source(x,y,action='touch')
        if (x,y)==(690,150):capture("input-touch")
    before=len(events())
    input_event('two-touch',*map_point(220,300),*map_point(500,420))
    assert len({e['id'] for e in events()[before:] if e['event'].startswith('touch-')})==2
    checks.append('two simultaneous contacts')

    geometry=next(w['sourceGeometry'] for w in state()['windows'] if label(w)=='A')
    before=len(events());origin=map_point(360,660)
    input_event('drag',*origin,origin[0]+30,origin[1]-30)
    assert any(e['event']=='system-move-request' and e['id']==1 for e in events()[before:])
    assert next(w['sourceGeometry'] for w in state()['windows'] if label(w)=='A')==geometry
    checks.append('native window move request is cancelled')

    for action,kind in [('drag','mouse-up'),('touch-drag','touch-196')]:
        before=len(events());input_event(action,*map_point(360,360),15,360)
        got=[e for e in events()[before:] if e['event']==kind]
        assert got and abs(got[-1]['x'])<1 and abs(got[-1]['y']-360)<1,events()[before:]
        checks.append(action+' outside content keeps capture')

    click_source(40,60)
    menu=wait_for(lambda s:any(t['popup'] for t in s['transients']))
    capture('popup-menu')
    rect=next(t['bufferGeometry'] for t in menu['transients'] if t['popup'])
    before=len(events())
    input_event('click',*map_point(rect['x']+rect['width']/2,rect['y']+rect['height']/2,global_source=True))
    assert any(e['event']=='menu-action' for e in events()[before:]),events()[before:]
    checks.append('popup menu action')

    a=next(w['id'] for w in state()['windows'] if label(w)=='A')
    for name in ['B','C']:
        children.append(subprocess.Popen([str(BUILD/'round_probe'),name,str(RUN/'events.jsonl')],
            stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL))
    s=wait_for(lambda s:len(s['windows'])==3 and all(w['canvasAccepted'] for w in s['windows']))
    ids={label(w):w['id'] for w in s['windows']}
    call('Minimize',ids['C']);call('Activate',a);call('ToggleOverview')
    wait_for(lambda s:s['overview'] and len(s['placements'])==3)
    capture('overview')
    item=next(p for p in state()['placements'] if p['id']==ids['B'])['bounds']
    input_event('click',item['x']+item['width']/2,item['y']+item['height']/2)
    wait_for(lambda s:not s['overview'] and s['activeId']==ids['B'])
    checks.append('overview activates clicked window')

    input_event('key-overview',0,0);wait_for(lambda s:s['overview'])
    input_event('key-overview',0,0);wait_for(lambda s:not s['overview'])
    checks.append('Super+Tab toggles overview with clean key releases')

    call('ToggleOverview')
    item=next(p for p in state()['placements'] if p['id']==a)['bounds']
    content=state()['content']
    input_event('drag',item['x']+item['width']/2,item['y']+item['height']/2,
        content['x']+content['width']*.1,content['y']+content['height']*.5)
    wait_for(lambda s:s['selectingPartner'] and s['splitLeftId']==a)
    capture('choose-partner')
    item=next(p for p in state()['placements'] if p['id']==ids['B'])['bounds']
    input_event('touch',item['x']+item['width']/2,item['y']+item['height']/2)
    wait_for(lambda s:not s['overview'] and s['splitLeftId']==a and s['splitRightId']==ids['B'])
    capture('split')
    wait_for(lambda s:all(w['canvasAccepted'] for w in s['windows'] if label(w) in ['A','B']))
    for name,shape,fraction in [('A',2,.25),('B',3,.75)]:
        rect=next(w['clientGeometry'] for w in state()['windows'] if label(w)==name)
        assert abs(rect['width']*2-rect['height'])<1,rect
        click_source(rect['width']/2,rect['height']/2,shape=shape,window=name)
        click_source(rect['width']*fraction,rect['height']*.58,shape=shape,window=name,action='touch')
    checks.append('drag to split and touch partner selection')

    call('ExitSplitFor',a)
    s=wait_for(lambda s:not s['splitLeftId'] and not s['splitRightId'])
    modes={label(w):w['circular'] for w in s['windows']}
    assert modes['A'] and not modes['B'],modes
    checks.append('split preserves per-app mode')
    call('Close',ids['C']);wait_for(lambda s:len(s['windows'])==2)
    call('Minimize',ids['B']);wait_for(lambda s:next(w for w in s['windows'] if w['id']==ids['B'])['minimized'])
    checks.append('targeted minimize and close')

    call('Activate',a)
    wait_for(lambda s:next(w for w in s['windows'] if w['id']==a)['canvasAccepted'])
    for action,kind in [('hold-click','mouse-up'),('hold-touch','touch-196')]:
        before=len(events());point=map_point(360,360)
        held=subprocess.Popen([str(BUILD/'round_input_driver'),action,*map(str,point)],stdout=subprocess.PIPE,text=True)
        assert held.stdout.readline().strip()=='pressed'
        wait_for(lambda s:s['captureActive'])
        call('ToggleMode',a)
        s=state();assert s['captureActive'] and s['pendingActions']==1,s
        assert next(w for w in s['windows'] if w['id']==a)['circular']
        assert not any(e['event']==kind for e in events()[before:])
        assert held.wait(timeout=5)==0
        wait_for(lambda s:not s['captureActive'] and s['pendingActions']==0 and not next(w for w in s['windows'] if w['id']==a)['circular'])
        assert len([e for e in events()[before:] if e['event']==kind])==1
        call('ToggleMode',a)
        checks.append(action+' defers mode change until physical release')

    call('Close',a);wait_for(lambda s:not any(label(w)=='A' for w in s['windows']))
    children.append(subprocess.Popen([str(BUILD/'round_probe'),'A',str(RUN/'events.jsonl')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL))
    s=wait_for(lambda s:any(label(w)=='A' and w['canvasAccepted'] for w in s['windows']))
    assert next(w for w in s['windows'] if label(w)=='A')['circular']
    capture('saved-mode-restored')
    checks.append('per-app mode restored for a new process')

    if os.environ.get('LINUX_ROND_XWAYLAND')=='1':
        environment=os.environ.copy()
        environment.update(json.loads((RUN/'x11-env.json').read_text()))
        environment['QT_QPA_PLATFORM']='xcb'
        assert environment.get('DISPLAY')
        children.append(subprocess.Popen([str(BUILD/'round_probe'),'XW',str(RUN/'events.jsonl')],
            env=environment,stdout=open(RUN/'xwayland-probe.log','w'),stderr=subprocess.STDOUT))
        s=wait_for(lambda s:any(label(w)=='XW' and w['canvasAccepted'] for w in s['windows']))
        xw=next(w for w in s['windows'] if label(w)=='XW')
        call('Activate',xw['id'])
        if not xw['circular']:call('ToggleMode',xw['id'])
        time.sleep(.7)
        capture('xwayland')
        click_source(360,360,window='XW')
        click_source(70,180,window='XW')
        click_source(360,360,window='XW',action='touch')
        checks.append('XWayland pointer and touch')
finally:
    (RUN/'checks.json').write_text(json.dumps(checks,indent=2))
    for child in children:
        child.terminate()
        try:child.wait(timeout=3)
        except subprocess.TimeoutExpired:child.kill()
print(f'{len(checks)} integrated checks passed.')
