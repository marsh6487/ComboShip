#!/usr/bin/env python3
"""Exercise real MM actor-owned receipt orchestration with native engine types."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'mm/2s2h/Enhancements/Gameplay/ComboVanillaRewards.cpp').read_text()


def function(signature):
    start=source.index(signature);brace=source.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


state=source[source.index('struct Context {'):source.index('bool ContextMatches')]
functions='void GiveReceipt(Actor*,PlayState*);\nvoid DrawReceipt(Actor*,PlayState*);\n'
for signature in ('bool ContextMatches(', 'void ReleaseReceiptMarker(', 'void CancelReceipt(', 'void Reset()',
                  'void ReceiptDestroyed(', 'bool ReceiptMatches(',
                  'void GiveReceipt(Actor* actor, PlayState* play) {',
                  'void DrawReceipt(Actor* actor, PlayState* play) {',
                  'bool OfferReceipt(', 'void PickupDestroyed(', 'void PickupUpdate(', 'void OpenText('):
    functions+=function(signature)+'\n'
fixture=(ROOT/'tests/combo_vanilla/mm_rewards_receipt_test.cpp').read_text()
fixture=fixture.replace('/* STATE */',state).replace('/* FUNCTIONS */',functions)
fixture=fixture.replace('/* ACTIVE_SAFE */',function('bool Active(')+'\n'+function('bool Safe('))
with tempfile.TemporaryDirectory(prefix='vanilla-mm-receipt-') as tmp:
    cpp=Path(tmp)/'test.cpp';cpp.write_text(fixture)
    flags=['-std=c++20','-w','-DF3DEX_GBI_2']
    for path in ('','mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
        flags+=['-I'+str(ROOT/path)]
    if '--sanitize' in sys.argv:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    binary=Path(tmp)/'test'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
