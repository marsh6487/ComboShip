#!/usr/bin/env python3
"""Execute production native draw/light/fairy integration and optional compile probes."""
import argparse
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize',action='store_true')
parser.add_argument('--case',choices=('native_actor','native_sky','native_fairy','native_light','native_teardown'))
args=parser.parse_args()
source=(ROOT/'soh/src/code/z_actor.c').read_text()
actor=block_from(source,source.index('void Actor_DrawAll('))
play=(ROOT/'soh/src/code/z_play.c').read_text()
destroy=block_from(play,play.index('void Play_Destroy('))
teardown=destroy[destroy.index('{')+1:destroy.index('    play->state.gfxCtx->callback = NULL;')]
play=block_from(play,play.index('void Play_Draw('))
sky=play[play.index('        if ((HREG(80) != 10) || (HREG(83) != 0))'):play.index('        FrameTiming_EndNamedSpan("play_draw", "sun_moon", drawPhase);')]
fairy=(ROOT/'soh/src/overlays/actors/ovl_En_Elf/z_en_elf.c').read_text()
fairy_lights=block_from(fairy,fairy.index('void EnElf_UpdateLights('))
fairy_lights=re.sub(r'\bthis\b','self',fairy_lights)
fairy_update=block_from(fairy,fairy.index('void EnElf_Update(Actor* thisx, PlayState* play) {'))
fairy_update=re.sub(r'\bthis\b','self',fairy_update)
lights=(ROOT/'soh/src/code/z_lights.c').read_text()
setter=block_from(lights,lights.index('void Lights_PointSetColorAndRadius('))
flags=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if args.sanitize else []
with tempfile.TemporaryDirectory(prefix='cel-native-') as temporary:
 build=Path(temporary)
 for name,marker,production in [('native_actor','NATIVE_ACTOR_DRAW_ALL',actor),('native_sky','NATIVE_SKY_PHASE',sky),('native_fairy','NATIVE_FAIRY_LIGHTS',fairy_lights),('native_light','NATIVE_LIGHT_SETTER',setter),('native_teardown','NATIVE_TEARDOWN_PHASE',teardown)]:
  if args.case and args.case!=name: continue
  cpp=build/(name+'.cpp')
  cpp.write_text((ROOT/'tests/cel_sky'/(name+'_test.cpp')).read_text().replace('/* '+marker+' */',production).replace('/* NATIVE_FAIRY_UPDATE */',fairy_update))
  binary=build/name
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-Wno-unused-variable',*flags,str(cpp),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
