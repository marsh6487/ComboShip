#!/usr/bin/env python3
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
assert (ROOT/'mm/2s2h/Rando/SoulShuffleRuntime.cpp').exists(), 'native MM optional soul runtime absent'
def body(source,signature):
    a=source.index(signature);b=source.index('{',a);e=b+1;depth=1
    while depth:
        depth+=(source[e]=='{')-(source[e]=='}');e+=1
    return source[a:e]
def prepare_mm(tmp):
    source=(ROOT/'mm/src/code/z_actor.c').read_text()
    a=source.index('Actor* Actor_SpawnAsChildAndCutscene(');b=source.index('    profile = Actor_LoadOverlay',a)
    (Path(tmp)/'mm_spawn.inc').write_text(source[a:b]+'    ++spawnReachesAllocation;return nullptr;\n}\n')
    chicks=(ROOT/'mm/src/overlays/actors/ovl_En_Nwc/z_en_nwc.c').read_text()
    a=chicks.index('typedef enum EnNwcState');b=chicks.index('} EnNwcState;',a)+len('} EnNwcState;')
    extracted=chicks[a:b]+'\n'
    for signature in ('EnHs* EnNwc_FindGrog(PlayState* play) {','s32 EnNwc_IsFound(','void EnNwc_CheckFound('):
        extracted+=body(chicks,signature)+'\n'
    (Path(tmp)/'mm_chicks.inc').write_text(re.sub(r'\bthis\b','self',extracted))
    player=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    a=player.index('void Player_Action_70(Player* this, PlayState* play) {')
    a=player.index('            if (MM_SoulCanSpawnNonEnemy(',a);e=player.index('{',a)+1;d=1
    while d:d+=(player[e]=='{')-(player[e]=='}');e+=1
    (Path(tmp)/'mm_bottle.inc').write_text(re.sub(r'\bthis\b','self',player[a:e]))
