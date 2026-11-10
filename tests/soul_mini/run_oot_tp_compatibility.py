#!/usr/bin/env python3
"""Check the actual read-only TP pack's head swaps and native rig compatibility.

This tests the provided packs, not arbitrary future replacements. No archive
content is copied or changed. Native props without a TP replacement are reported.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/soul_mini_models'))
from read_only_archive import ReadOnlyArchive

parser=argparse.ArgumentParser()
parser.add_argument('--mod',type=Path,action='append',required=True)
parser.add_argument('--stormlib',type=Path,required=True)
args=parser.parse_args()
archives=[ReadOnlyArchive(path,args.stormlib) for path in args.mod]
def selected(path):
    key='alt/'+path.removeprefix('__OTR__')
    return next((a for a in reversed(archives) if key in a.names),None),key
with tempfile.TemporaryDirectory(prefix='oot-tp-compatibility-') as directory:
    manifest=Path(directory)/'recipes.json'
    subprocess.run([sys.executable,str(ROOT/'tests/soul_mini/run_oot_catalog_tests.py'),'--dump',str(manifest)],check=True)
    recipes=json.loads(manifest.read_text())
rigs=swaps=0
native_posts=[]
for recipe in recipes:
    if not recipe['skelPath']:continue
    archive,key=selected(recipe['skelPath'])
    if archive is None:continue
    root=ET.fromstring(archive.read(key))
    assert root.tag=='Skeleton',(recipe['id'],root.tag)
    assert int(root.attrib['LimbCount'])+1==recipe['limbCount'],recipe['id']
    assert root.attrib['Type'] in ('Flex','Normal'),recipe['id']
    rigs+=1
    for override in recipe['limbOverrides']:
        if override['dlPath']:
            owner,path=selected(override['dlPath'])
            assert owner is not None,(recipe['id'],'TP skeleton has forced vanilla head',path)
            swaps+=1
        if override['postDlPath'] and selected(override['postDlPath'])[0] is None:
            native_posts.append((recipe['id'],override['postDlPath']))
print('PASS actual TP owner replacement rigs:',rigs,'with native count/layout; matching TP head replacements:',swaps)
for identity,path in native_posts:
    print('Native auxiliary pass retained:',identity,path)
for archive in archives:
    assert not any('/PoseAnimations/' in name or '/NativeNpcV1/' in name or '/Skin/' in name for name in archive.names),archive.path
print('PASS supplied TP packs use canonical replacement rigs; no optional NativeNpcPose/private-skin adapters are required')
for archive in archives:archive.close()
