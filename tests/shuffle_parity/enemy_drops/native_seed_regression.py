#!/usr/bin/env python3
"""Run actual built native DLL graph oracles and comborando fill/replay.

No replacement graph, combat predicates, fabricated DLLs, or synthesized output.
Missing modules/assets/exports fail setup. Separate processes isolate native state.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from native_seed_config import catalogue, cases, native_junk_names

PREFIX = 'Global Enemy Drop: '

def require(value, message):
    if not value:
        raise AssertionError(message)

def write(path, value):
    path.write_text(json.dumps(value, indent=2)+'\n')

def module_name():
    return 'soh.dll' if sys.platform=='win32' else 'libsoh.dylib' if sys.platform=='darwin' else 'libsoh.so'

def probe(config, mode, output):
    """This process loads the actual game module and queries its real graph."""
    lib = ctypes.CDLL(str(Path(module_name()).resolve()))
    def bind(name, result=None, args=()):
        fn = getattr(lib,name);fn.restype=result;fn.argtypes=list(args);return fn
    init=bind('SOH_InitRandoHeadless')
    setmode=bind('SOH_SetGameMode',args=(ctypes.c_int,))
    restore=bind('SOH_RestoreRandoSettings',args=(ctypes.c_char_p,))
    dump=bind('SOH_DumpRandoStaticData',ctypes.c_char_p)
    settings=bind('SOH_DumpRandoSettings',ctypes.c_char_p)
    layout=bind('SOH_SetDungeonLayoutsForCombo',ctypes.c_int,(ctypes.c_char_p,))
    dump_layout=bind('SOH_DumpDungeonLayouts',ctypes.c_char_p)
    seed=bind('SOH_SetComboRandoSeed',args=(ctypes.c_uint64,))
    tricks=bind('SOH_SetEnabledTricks',args=(ctypes.c_char_p,))
    reset=bind('Combo_SOH_Rando_Reset')
    owned=bind('Combo_SOH_Rando_SetOwnedItems',args=(ctypes.c_char_p,))
    reachable=bind('Combo_SOH_Rando_GetReachableChecks',ctypes.c_char_p)
    setmode(1 if mode=='solo-graph' else 0)
    init();restore(json.dumps(config['settings']).encode());tricks(b'[]');seed(20261109)
    assets = (1 if Path('oot.o2r').is_file() else 0) | (2 if Path('oot-mq.o2r').is_file() else 0)
    # Match the ROM-free headless ctor's Vanilla fallback. MQ coverage requires
    # the real configured asset file; never manufacture an availability marker.
    assets = assets or 1
    require(not config['mq'] or assets & 2, 'selected MQ cases require the real oot-mq.o2r in --runtime-dir')
    require(len(config['mq'])==12 or assets & 1, 'selected Vanilla cases require original quest availability')
    require(layout(json.dumps({'masterQuestDungeons':config['mq'],'dungeonLayoutAssets':assets}).encode())==1,
            'selected V/MQ layouts rejected: native quest assets are required; do not create placeholder assets')
    data=json.loads(dump())
    actual_settings=json.loads(settings())
    for key,value in config['settings'].items():
        require(actual_settings.get(key)==value, 'native setting did not apply: '+key)
    resolved=json.loads(dump_layout())
    require(set(resolved['masterQuestDungeons'])==set(config['mq']), 'native resolved layouts differ')
    require(not data['accessibility']['noLogic'], 'native dump is No Logic')
    require(not data.get('startingSouls'), 'starting souls invalidate missing-soul negative controls')
    fixed={row['check']:row for row in data['fixed']}
    checks={row['name'] for row in data['checks']}|set(fixed)
    actual={name for name in checks if name.startswith(PREFIX)}
    require(actual==set(config['expected']), 'native pool/source profile mismatch: '+repr(actual^set(config['expected'])))
    if config['excluded']:
        for name in actual:
            require(name in fixed, 'excluded native check is not fixed: '+name)
            require_native_junk(fixed[name]['item'], data, 'excluded native check '+name)
            require(not fixed[name]['advancement'], 'excluded fixed native junk marked advancement: '+name)
    # Real inventory operations and actual ReachabilitySearch. Generous copies
    # cover native small-key/token counts; every listed item is native metadata.
    inventory=[row['name'] for row in data['items']]*100
    def search(items):
        reset();owned(json.dumps(items).encode());return set(json.loads(reachable()))
    full=search(inventory)
    require(actual<=full, 'native graph cannot reach fully equipped source checks: '+repr(actual-full))
    controls=[]
    if config['settings']['gRandoSettings.ShuffleEnemyDrops']:
        for row in catalogue():
            if row['check'] not in actual:continue
            require(row['soul'] in inventory, 'source soul missing from native item metadata: '+row['soul'])
            without=search([item for item in inventory if item!=row['soul']])
            require(row['check'] not in without, 'actual native graph bypasses own source soul: '+row['check'])
            controls.append(row['check'])
    write(output,{'mode':mode,'case':config['name'],'settings':actual_settings,'layouts':resolved,
                  'dump':data,'fullInventoryReachable':sorted(full),'ownSoulNegatives':controls})

def require_native_junk(item, dump, context):
    require(isinstance(item,str), context+': unexpected native placement shape')
    name=item.removesuffix(' (OOT)')
    identities=native_junk_names()
    require(name in identities, context+': not an exact native GetJunkItem ID/category: '+item)
    metadata=[row for row in dump['items'] if row['name']==name]
    require(len(metadata)==1 and not metadata[0]['advancement'] and not metadata[0]['trap'],
            context+': native item metadata contradicts junk identity: '+item)
    # Pool[] has the actual native category; Fixed[]/Items[] omit category.
    for row in dump['pool']:
        if row['name']==name:
            require(row['category']=='junk', context+': native pool category is not junk: '+item)
    return identities[name]


def is_native_module(path):
    return path.is_file() and ('.so' in path.name or path.suffix in ('.dll','.dylib'))


def module_hashes(work):
    return {path.name:hashlib.sha256(path.read_bytes()).hexdigest()
            for path in work.iterdir() if is_native_module(path)}


def require_fresh_output(out):
    require(not out.exists() and not out.is_symlink(),
            '--output must be a fresh path; existing evidence is retained: '+str(out))


def prepare_runtime(runtime, out):
    # Refuse every existing destination, including stale/dangling symlinks. Never
    # delete or reuse an earlier run's modules, assets, logs or successful manifest.
    require_fresh_output(out)
    out.mkdir(parents=True)
    work=out/'runtime';work.mkdir()
    for path in runtime.iterdir():
        if is_native_module(path) or (path.is_file() and path.suffix=='.o2r'):
            (work/path.name).symlink_to(path.resolve())
    return work


def run(command, cwd, log, timeout):
    with log.open('w') as stream:
        child_env=os.environ.copy();child_env['SHIP_HOME']=str(cwd.resolve())
        result=subprocess.run(command,cwd=cwd,env=child_env,stdout=stream,stderr=subprocess.STDOUT,timeout=timeout)
    require(result.returncode==0, 'native command failed ('+str(result.returncode)+'): '+str(log))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-dir',type=Path)
    parser.add_argument('--driver',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--timeout',type=int,default=1800)
    parser.add_argument('--case',action='append',help='Limit named cases; default all12')
    parser.add_argument('--probe',type=Path,help=argparse.SUPPRESS)
    parser.add_argument('--probe-mode',choices=('solo-graph','combined-graph'),help=argparse.SUPPRESS)
    args=parser.parse_args()
    if args.probe:
        probe(json.loads(args.probe.read_text()),args.probe_mode,args.output);return
    require(args.runtime_dir and args.driver and args.output,'--runtime-dir --driver --output required')
    runtime=args.runtime_dir.resolve();driver=args.driver.resolve();out=args.output.expanduser().absolute()
    require_fresh_output(out)
    require(driver.is_file(),'native comborando executable missing: '+str(driver))
    require((runtime/module_name()).is_file(),'native SoH module missing: '+str(runtime/module_name()))
    # Isolate config and fixed seed output; fresh evidence prevents stale modules.
    work=prepare_runtime(runtime,out)
    selected=[case for case in cases() if not args.case or case['name'] in args.case]
    require(selected,'no case selected')
    manifest={'driver':str(driver),'driverSha256':hashlib.sha256(driver.read_bytes()).hexdigest(),
              'soloFill':'UNSUPPORTED: current CLI only exports combined fill; solo graph is tested separately',
              'moduleSha256':module_hashes(work),
              'cases':[]}
    for case in selected:
        folder=out/case['name'];folder.mkdir(exist_ok=True);config=folder/'config.json';write(config,case)
        probes={}
        for mode in ('solo-graph','combined-graph'):
            result=folder/(mode+'.json')
            run([sys.executable,str(Path(__file__).resolve()),'--probe',str(config),'--probe-mode',mode,'--output',str(result)],work,folder/(mode+'.log'),args.timeout)
            probes[mode]=json.loads(result.read_text())
        # Feed actual effective native settings into the real CLI, not display
        # labels or guessed option enums. Both modes' oracle reports remain saved.
        settings_file=folder/'consolidated-settings.json'
        write(settings_file,{'oot':{'settings':probes['combined-graph']['settings'],
                                     'enabledTricks':[],**probes['combined-graph']['layouts']},
                             'mm':{'settings':{}},'sharedItems':[]})
        native_output=work/'saves/combo/comborando.spoiler.json'
        if native_output.exists():native_output.unlink()
        cross_flags=['--cross-dungeon-entrances','--cross-boss-entrances','--expanded-dungeon-pool',
                     '--cross-interior-entrances','--cross-grotto-entrances','--cross-shop-entrances',
                     '--cross-door-interior-entrances']
        command=[str(driver),'--settings',str(settings_file),'--master-seed','20261109','--count','1',
                 '--starting-game','oot','--shared-items','none','--triforce-required','0']
        for flag in cross_flags:command += [flag,'0']
        run(command,work,folder/'generate.log',args.timeout)
        require(native_output.is_file(),'native driver did not write consolidated spoiler')
        saved=folder/'spoiler.json';shutil.copy2(native_output,saved);spoiler=json.loads(saved.read_text())
        actual={name for name in spoiler['oot']['placements'] if name.startswith(PREFIX)}
        require(actual==set(case['expected']),'real combined seed missing/extra first-type check: '+repr(actual^set(case['expected'])))
        require(set(spoiler['oot']['masterQuestDungeons'])==set(case['mq']),'combined seed layout changed')
        require(not spoiler['oot']['enabledTricks'],'combined seed silently enabled tricks')
        if case['excluded']:
            for name in actual:
                item=spoiler['oot']['placements'][name]
                require_native_junk(item,probes['combined-graph']['dump'],'excluded combined check '+name)
        run([str(driver),'--playthrough',str(saved)],work,folder/'playthrough.log',args.timeout)
        manifest['cases'].append({'name':case['name'],'checks':len(actual),'combinedFillReplay':'PASS',
                                  'soloNativeGraph':'PASS','sourceSoulNegativeCount':len(probes['solo-graph']['ownSoulNegatives'])})
        write(out/'manifest.json',manifest)
    print('PASS actual native graph/pool + combined fill/replay:',len(selected),'cases; standalone solo fill remains unsupported')

if __name__=='__main__':
    main()
