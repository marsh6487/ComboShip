#!/usr/bin/env python3
"""Filesystem/config/validation controls only; no native graph/seed claim."""
import copy
import json
import os
from pathlib import Path
import sys
import subprocess
import hashlib
import tempfile
from native_seed_config import cases,native_junk_names
from native_seed_regression import prepare_runtime,require_native_junk,run,module_hashes

def rejects(action,expected):
    try:action()
    except AssertionError as error:
        assert expected in str(error),(expected,str(error))
    else:raise AssertionError('negative control accepted: '+expected)

with tempfile.TemporaryDirectory(prefix='firstdrops-native-scaffold-') as temporary:
    root=Path(temporary);native=root/'source';native.mkdir()
    # This is filesystem staging data, never loaded as a native game module.
    library=native/'staging-fixture.so';library.write_text('first-build')
    dependency=native/'staging-fixture.so.1';dependency.write_text('versioned-native-staging-data')
    asset=native/'staging-fixture.o2r';asset.write_text('real-input-placeholder-for-filesystem-test-only')
    work=prepare_runtime(native,root/'evidence')
    assert (work/library.name).resolve()==library
    assert (work/asset.name).resolve()==asset
    asset.unlink();library.write_text('second-build')
    rejects(lambda:prepare_runtime(native,root/'evidence'),'fresh path')
    # Refusal preserves prior evidence and cannot leave a reused stale manifest.
    assert (work/asset.name).is_symlink()
    fresh=prepare_runtime(native,root/'fresh-evidence')
    assert not (fresh/asset.name).exists() and not (fresh/asset.name).is_symlink()
    assert (fresh/library.name).read_text()=='second-build'
    dangling=root/'dangling';dangling.symlink_to(root/'absent')
    rejects(lambda:prepare_runtime(native,dangling),'fresh path')
    # Exercise real argparse/main normalization, not only helper calls. Existing
    # paths must be rejected before any module validation/loading or directory creation.
    for destination in (root/'evidence',dangling):
        result=subprocess.run([sys.executable,str(Path(__file__).with_name('native_seed_regression.py')),
                               '--runtime-dir',str(native),'--driver',sys.executable,
                               '--output',str(destination)],capture_output=True,text=True)
        assert result.returncode!=0 and '--output must be a fresh path' in result.stderr,result
    assert dangling.is_symlink() and not (root/'absent').exists()
    assert module_hashes(fresh)=={path.name:hashlib.sha256(path.read_bytes()).hexdigest()
                                 for path in (library,dependency)}
    os.environ['SHIP_HOME']=str(root/'wrong-inherited-home')
    log=root/'child.log'
    run([sys.executable,'-c','import os,json;print(json.dumps({"home":os.environ["SHIP_HOME"],"cwd":os.getcwd()}))'],fresh,log,10)
    child=json.loads(log.read_text())
    assert child=={'home':str(fresh.resolve()),'cwd':str(fresh.resolve())}
    assert os.environ['SHIP_HOME']==str(root/'wrong-inherited-home')

identities=native_junk_names()
assert len(identities)==13
assert identities['Recovery Heart']=='RG_RECOVERY_HEART' # native junk, unlike health upgrades
assert 'Heart Container' not in identities and 'Piece of Heart' not in identities and 'Ice Trap' not in identities
# These are explicit schema validator inputs, never presented as native outputs.
dump={'items':[{'name':'Blue Rupee','advancement':False,'trap':False},
               {'name':'Heart Container','advancement':False,'trap':False},
               {'name':'Piece of Heart','advancement':False,'trap':False},
               {'name':'Ice Trap','advancement':False,'trap':True}],
      'pool':[{'name':'Blue Rupee','category':'junk'}]}
assert require_native_junk('Blue Rupee',dump,'positive')=='RG_BLUE_RUPEE'
assert require_native_junk('Blue Rupee (OOT)',dump,'positive')=='RG_BLUE_RUPEE'
for bad in ('Heart Container','Piece of Heart','Ice Trap','Blue Rupee (MM)'):
    rejects(lambda:require_native_junk(bad,dump,'negative'),'exact native GetJunkItem')
bad=copy.deepcopy(dump);bad['pool'][0]['category']='health'
rejects(lambda:require_native_junk('Blue Rupee',bad,'negative'),'pool category')
bad=copy.deepcopy(dump);bad['items'][0]['trap']=True
rejects(lambda:require_native_junk('Blue Rupee',bad,'negative'),'metadata contradicts')
for case in cases():
    for key in ('BaseIceTraps','AdditionalIceTraps','IceTrapPercent','BombchuBag','EnableBombchuDrops'):
        assert case['settings']['gRandoSettings.'+key]==0
print('PASS integration scaffolding: fresh-only evidence, no stale assets, pinned child SHIP_HOME, exact native junk identities/category and health/trap negatives')
