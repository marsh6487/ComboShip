#!/usr/bin/env python3
"""Reconstructed shared-soul production graph and native bridge regressions."""
from pathlib import Path
import argparse, os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
# Source catalog audit uses native grant identities, not display-name inference at runtime.
rows=re.findall(r'SHARED_SOUL\((\w+), (\w+), (\w+), "([^"]+)", "([^"]+)"\)',(ROOT/'combo/rando/SharedSoulCatalog.inc').read_text())
assert len(rows)==68
assert {family:sum(r[0]==family for r in rows) for family in ('ENEMY','NPC','ANIMAL','MISC')}=={'ENEMY':26,'NPC':36,'ANIMAL':4,'MISC':2}
native=(ROOT/'combo/rando/SoulShuffleCatalog.inc').read_text()
mm=(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
for family,oot,peer,oname,mname in rows:
    assert f'SOUL(OOT, {family}, {oot}, "{oname}")' in native
    if family=='ENEMY':
        assert re.search(r'RI\(RI_SOUL_ENEMY_'+peer+r',\s*"the",\s*"'+re.escape(mname)+'"',mm)
    else:
        assert f'SOUL(MM, {family}, {peer}, "{mname}")' in native
assert len(re.findall(r'^SOUL_LEGACY', (ROOT/'soh/soh/Enhancements/randomizer/SoulShuffleLegacy.inc').read_text(),re.M))==24
give=(ROOT/'mm/2s2h/Rando/GiveItem.cpp').read_text()
receipt=give.index('FleetShared_OnNativeObtained((int)recordId)')
assert give.index('nei->comboObtainedFc[fc]++')<receipt
assert give.index('nei->comboAppliedFc[fc]++')<receipt
p=argparse.ArgumentParser();p.add_argument('--runtime-only',action='store_true');a=p.parse_args()
def body(source, signature):
    a=source.index(signature);b=source.index('{',a);depth=1;e=b+1
    while depth:
        depth+=(source[e]=='{')-(source[e]=='}');e+=1
    return source[a:e]
with tempfile.TemporaryDirectory(prefix='shared-souls-') as tmp:
    callback=(ROOT/'soh/soh/FleetShipCombo/FleetSharedItems.cpp').read_text()
    (Path(tmp)/'shared_receipt.inc').write_text(body(callback,'extern "C" void FleetShared_OnNativeObtained(int nativeId)'))
    callback=(ROOT/'mm/2s2h/FleetShipCombo/FleetSharedItems.cpp').read_text()
    (Path(tmp)/'shared_receipt_mm.inc').write_text(body(callback,'extern "C" void FleetShared_OnNativeObtained(int nativeId)'))
    if not a.runtime_only:
        for test in ('shared_graph_test','shared_fill_test'):
            for mode in ('original','combo'):
                exe=Path(tmp)/(test+mode)
                subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O0','-I'+str(ROOT/'combo'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(ROOT/'tests/shuffle_parity/souls'/(test+'.cpp')),'-o',str(exe)],check=True)
                subprocess.run([str(exe)],check=True,cwd=tmp)
    for game in ('oot','mm'):
        folders=('soh','soh/include','soh/src','soh/assets','libultraship/include','combo') if game=='oot' else ('mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo')
        for mode in ('original','combo'):
            exe=Path(tmp)/(game+mode)
            command=[os.environ.get('CXX','c++'),'-std=c++20','-fmax-errors=5','-O0','-I'+tmp,'-rdynamic','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',*(['-DTEST_MM'] if game=='mm' else []),*(['-DCOMBO_BUILD'] if mode=='combo' else []),*['-I'+str(ROOT/f) for f in folders],str(ROOT/'tests/shuffle_parity/souls/shared_native_test.cpp'),'-ldl','-o',str(exe)]
            subprocess.run(command,check=True);subprocess.run([str(exe)],check=True)
