#!/usr/bin/env python3
"""Pinned environmental ownership/identity and real persisted-ID audit; not gameplay proof."""
from pathlib import Path
import collections,json,re,tempfile,subprocess,os
R=Path(__file__).resolve().parents[3];m=json.loads(Path(__file__).with_name('donor_manifest.json').read_text());rows=m['checks'];aliases=m['placements']
assert m['pin']=='0c0f8a8e1412f3bc979e77a2563293770d58a439'
assert len(rows)==len({x['id'] for x in rows})==211 and len(aliases)==279
assert collections.Counter(x['family'] for x in rows)=={'rock':127,'boulder':23,'boulder-red':7,'icicle':25,'soil':29}
lookup=collections.defaultdict(set)
for x in aliases:lookup[tuple(x[k] for k in ['scene','setup','room','index','slice'])].add(x['id'])
assert all(len(v)==1 for v in lookup.values())
assert len(re.findall(r'^MM_ENVIRONMENT_CHECK\(', (R/'mm/2s2h/Rando/MMEnvironmentChecks.inc').read_text(),re.M))==204
actual=collections.defaultdict(list)
for p in (R/'mm/2s2h/Rando/Logic/Regions').glob('*.cpp'):
 reg=None
 for line in p.read_text().splitlines():
  r=re.search(r'Regions\[(RR_\w+)\] =',line)
  if r:reg=r[1]
  c=re.search(r'CHECK\((RC_\w+), (.*)\),',line)
  if c:actual[c[1]].append((reg,c[2]))
for x in rows:
 assert actual[x['id']]==[(x['region'],x['condition'])],x['id']
 assert x['condition']!='true'
assert len({x['region'] for x in rows})==36
with tempfile.TemporaryDirectory() as d:
 d=Path(d);(d/'ids.cpp').write_text('#include <cassert>\n#include "mm/2s2h/Rando/Types.h"\nint main(){\n#include "tests/shuffle_parity/legacy_ids.inc"\n}\n')
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(R),str(d/'ids.cpp'),'-o',str(d/'ids')],check=True);subprocess.run([str(d/'ids')],check=True)
print('PASS211 checks,279 unambiguous aliases,204 appended IDs,36 native regions and old persisted IDs')
