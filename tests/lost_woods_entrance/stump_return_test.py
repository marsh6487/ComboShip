#!/usr/bin/env python3
"""Exercise the actual stump return bounds and legacy-pack compatibility."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
source = r'''
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
SaveContext gSaveContext{};
static void require(bool condition, const char* reason) {
    if (!condition) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); }
}
int main() {
    PlayState play{}; play.sceneId=SCENE_LOST_WOODS; play.roomCtx.curRoom.num=0;
    ActorEntry spawn{}; spawn.pos={-417,0,-122}; spawn.rot.y=11520;
    play.linkActorEntry=&spawn;
    RoomShape shape{}; RoomShapeDListsEntry entry{};
    shape.normal.base.type=ROOM_SHAPE_TYPE_NORMAL; shape.normal.numEntries=1;
    shape.normal.entries=&entry; play.roomCtx.curRoom.roomShape=&shape;
    alignas(Gfx) static const char current[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_stump_v2";
    alignas(Gfx) static const char legacy[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_v1";
    alignas(Gfx) static const char native[]="__OTR__scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00DL_005788";
    gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=0;
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
    Vec3f stump={120,0,240}, approach={72,0,240}, oldArrival={-417,0,-122};
    entry.opa=(Gfx*)current;
    require(ComboOpeningForest_AtReturn(&play,&stump),"the current doorway must return at the large corner stump");
    require(ComboOpeningForest_UsesPackTunnel(&play),"the stump pack must suppress the temporary arch");
    require(!ComboOpeningForest_AtReturn(&play,&approach),"walking beside the stump must not teleport");
    require(!ComboOpeningForest_AtReturn(&play,&oldArrival),"the retained native arrival must not bounce");
    const auto original=gSaveContext.save;
    for (int day=1;day<=3;++day) for(int form=0;form<PLAYER_FORM_MAX;++form) {
        gSaveContext.save.day=day; gSaveContext.save.playerForm=form;
        require(ComboOpeningForest_AtReturn(&play,&stump),"the return must remain available on every day/form");
        for(auto point : {Vec3f{120,150,240},Vec3f{120,-50,240},Vec3f{120,0,310},Vec3f{192,0,240}})
            require(!ComboOpeningForest_AtReturn(&play,&point),"stump bounds must exclude its wall, roof and ground below");
    }
    gSaveContext.save=original;
    entry.opa=(Gfx*)legacy;
    require(ComboOpeningForest_UsesPackTunnel(&play) && ComboOpeningForest_AtReturn(&play,&oldArrival),
            "an existing v1 pack must retain its visible return location");
    require(!ComboOpeningForest_AtReturn(&play,&stump),"a legacy visible doorway must not leave an invisible stump trigger");
    entry.opa=(Gfx*)native;
    require(!ComboOpeningForest_UsesPackTunnel(&play) && ComboOpeningForest_AtReturn(&play,&stump),
            "native assets must use the temporary arch at the same stump");
    play.roomCtx.curRoom.num=1;
    require(!ComboOpeningForest_AtReturn(&play,&stump),"the second room must retain its native onward route");
    play.roomCtx.curRoom.num=0; gSaveContext.sceneLayer=1;
    require(!ComboOpeningForest_AtReturn(&play,&stump),"flashbacks must not receive a return trigger");
    require(std::memcmp(&original,&gSaveContext.save,sizeof(original))==0,"checking the return cannot change save progress");
    std::puts("PASS stump return: active mesh, exact bounds, native fallback, v1 compatibility, every day/form, progress preserved");
}
'''
with tempfile.TemporaryDirectory(prefix='forest-stump-') as temporary:
    path=Path(temporary); cpp=path/'test.cpp'; binary=path/'test'; cpp.write_text(source)
    includes=['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']
    command=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
             *['-I'+str(ROOT/p) for p in includes]]
    if args.sanitize: command+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    subprocess.run([*command,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
