#!/usr/bin/env python3
"""Compile the production trade wire policy against both ports' real FC item tables."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--red-probe', action='store_true')
parser.add_argument('--source-ref')
args = parser.parse_args()

with tempfile.TemporaryDirectory(prefix='trade-sync-') as td:
    out = Path(td)
    if args.red_probe:
        # Execute the actual previous wire assignment/merge: the consumer's
        # state never travels, so the recipient still treats that item as held.
        path = 'soh/soh/FleetShipCombo/FleetSync.cpp'
        source = (subprocess.check_output(['git','show',args.source_ref+':'+path],cwd=ROOT,text=True)
                  if args.source_ref else (ROOT/path).read_text())
        write = re.search(r'sh\["tradeAdultOwned"\] = nei->tradeAdultOwned;', source)
        merge = re.search(r'nei->tradeAdultOwned \|= sh\["tradeAdultOwned"\]\.get<uint32_t>\(\);', source)
        assert write and merge, 'Run the red probe against trade-implementation-base'
        code = r'''
#include <cassert>
#include <cstdint>
#include <nlohmann/json.hpp>
struct State { uint32_t tradeAdultOwned=0; uint32_t tradeAdultRevision[23]{}; };
int main(){ State source{}, peer{}; source.tradeAdultOwned=1u<<17; source.tradeAdultRevision[17]=1;
auto* nei=&source; nlohmann::json sh;
''' + write.group() + '\nnei=&peer;\n' + merge.group() + r'''
assert(peer.tradeAdultRevision[17]==1 && "hand-in must survive source-to-peer snapshot"); }
'''
        (out/'red.cpp').write_text(code)
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17',str(out/'red.cpp'),'-o',str(out/'red')],check=True)
        subprocess.run([str(out/'red')],check=True)
    else:
        for game, inc in [('oot','soh/soh'), ('mm','mm/2s2h')]:
            flags = ['-std=c++17','-Wall','-Wextra','-Werror']
            if args.sanitize:
                flags += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
            binary = out/game
            subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(ROOT),'-I'+str(ROOT/inc),
                            str(ROOT/'tests/trade_consumption/sync_test.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
        for path in ['soh/soh/FleetShipCombo/FleetSync.cpp','mm/2s2h/FleetShipCombo/FleetSync.cpp']:
            source = (ROOT/path).read_text()
            assert 'ComboTrade::WriteShared(*nei, sh);' in source
            assert 'ComboTrade::ReadShared(*nei, sh);\n    TradeAdult_ReconcileConsumed();' in source
            assert 'ComboTrade::AcknowledgeConsumedGrant(*nei, fcId)' in source
            assert 'snapshot.Array("nei.tradeAdultRevision", nei->tradeAdultRevision);' in source
        print('PASS trade sync: both ports publish, merge, reconcile, suppress stale grants, and audit revisions')
