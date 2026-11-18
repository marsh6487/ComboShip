#!/usr/bin/env python3
"""Compile actual MM horse spawn/reset bodies; engine services are narrow counters.

The regression recreates a still-resident mounted marker after Great Bay -> OoT
and returns to Pirates' Fortress. It also covers native supported scenes, scene
layers, form changes, failed actor allocation and saved parked-horse preservation.
This is native-source component proof, not recorded game playback.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--ref', help='Read-only regression control at a Git revision')
args = parser.parse_args()


def read(path):
    if args.ref:
        return subprocess.check_output(['git', 'show', args.ref + ':' + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def block(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


horse = read('mm/src/code/z_horse.c')
resume = block(read('mm/2s2h/BenPort.cpp'), 'extern "C" COMBO_EXPORT void MM_ResumeGame(')
scenes = sorted(set(re.findall(r'\bSCENE_\w+', horse)) | {'SCENE_KAIZOKU', 'SCENE_PIRATE', 'SCENE_NAKANIWA'})
production = '\n'.join([
    block(horse, 'HorseValidScene sHorseValidScenes[]') + ';',
    block(horse, 's32 Horse_IsValidSpawn('),
    block(horse, 'void Horse_ResetHorseData('),
    block(horse, 'HorseValidScene sHorseValidSceneLayers[]') + ';',
    block(horse, 's32 Horse_IsValidSceneLayer('),
    block(horse, 'void Horse_SpawnOverworld('),
    block(horse, 'void Horse_ResetMountState(') if 'void Horse_ResetMountState(' in horse else
    'void Horse_ResetMountState() {}',
])
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
using s16=int16_t; using s32=int32_t; using f32=float;
''' + 'enum { ' + ', '.join(scenes) + ' };\n' + r'''
enum { PLAYER_FORM_HUMAN, PLAYER_FORM_DEKU, PLAYER_FORM_GORON, PLAYER_FORM_ZORA, PLAYER_FORM_FIERCE_DEITY };
enum { QUEST_SONG_EPONA, ACTOR_EN_HORSE, ENHORSE_PARAM_4000, ENHORSE_1, ENHORSE_2, ENHORSE_11 };
constexpr float BGCHECK_Y_MIN=-32000;
struct Vec3f { float x{},y{},z{}; };
struct Vec3s { s16 x{},y{},z{}; };
struct Actor { struct { Vec3f pos; Vec3s rot; } world; struct { Vec3s rot; } shape; };
struct CollisionPoly {};
struct Player { Actor actor; int transformation=PLAYER_FORM_HUMAN; Actor* rideActor=nullptr; };
struct PlayState { s16 sceneId; int actorCtx; int colCtx; };
struct HorseValidScene { s16 sceneId; s16 sceneLayerMinusOne; };
struct HorseData { s16 sceneId; Vec3s pos; s16 yaw; };
struct SaveContext { int sceneLayer{}; struct { struct { HorseData horseData; } saveInfo; } save; };
SaveContext gSaveContext{};
s32 gHorseIsMounted=false,D_801BDAA0=false,gHorsePlayedEponasSong=false;
bool ownsSong=true,allocationFails=false; int spawns=0,mounts=0,cameras=0;
Actor horseActor{};
#define ARRAY_COUNT(x) (sizeof(x)/sizeof((x)[0]))
#define CHECK_QUEST_ITEM(x) ownsSong
#define ENHORSE_PARAMS(...) 0
float BgCheck_EntityRaycastFloor1(int*,CollisionPoly**,Vec3f*) { return 10; }
Actor* Actor_Spawn(int*,PlayState*,int,float,float,float,s16,s16,s16,int) {
    ++spawns; return allocationFails? nullptr : &horseActor;
}
void Player_MountHorse(PlayState*,Player*,Actor* actor) { assert(actor); ++mounts; }
void Player_SetCameraHorseSetting(PlayState*,Player*) { ++cameras; }
void Check(bool ok,const char* reason) { if(!ok) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
''' + production + r'''
int main() {
    Player player{};
    PlayState play{SCENE_KAIZOKU,0,0};
    gSaveContext.save.saveInfo.horseData={SCENE_30GYOSON,{12,34,56},78};
    const auto parked=gSaveContext.save.saveInfo.horseData;
    // The exact reported stale marker previously mounted in this unsupported
    // exterior because only layer zero was checked, not the native scene table.
    for (int unsupported : {SCENE_KAIZOKU,SCENE_PIRATE,SCENE_NAKANIWA}) {
        play.sceneId=unsupported; gHorseIsMounted=true; spawns=mounts=cameras=0;
        Horse_SpawnOverworld(&play,&player);
        Check(!gHorseIsMounted && !spawns && !mounts && !cameras,
              "unsupported shuffled destination inherited the suspended MM mount");
    }
    for (const auto& scene:sHorseValidScenes) {
        play.sceneId=scene.sceneId;gSaveContext.sceneLayer=0;
        player={};gHorseIsMounted=true;spawns=mounts=cameras=0;
        Horse_SpawnOverworld(&play,&player);
        Check(gHorseIsMounted && spawns==1 && mounts==1 && cameras==1,
              "ordinary native mounted travel lost its supported scene");
    }
    play.sceneId=SCENE_30GYOSON;gSaveContext.sceneLayer=1;
    player={};gHorseIsMounted=true;spawns=mounts=cameras=0;
    Horse_SpawnOverworld(&play,&player);
    Check(mounts==1,"native post-Gyorg Great Bay layer must retain Epona");
    gSaveContext.sceneLayer=2;gHorseIsMounted=true;spawns=mounts=cameras=0;
    Horse_SpawnOverworld(&play,&player);
    Check(!gHorseIsMounted && !spawns,"unsupported scene layer retained stale mount");
    gSaveContext.sceneLayer=0;
    for(int form:{PLAYER_FORM_DEKU,PLAYER_FORM_GORON,PLAYER_FORM_ZORA,PLAYER_FORM_FIERCE_DEITY}) {
        player={};player.transformation=form;gHorseIsMounted=true;spawns=mounts=cameras=0;
        Horse_SpawnOverworld(&play,&player);
        Check(!gHorseIsMounted && !mounts,"nonhuman form auto-mounted Epona");
    }
    player={};gHorseIsMounted=true;allocationFails=true;spawns=mounts=cameras=0;
    Horse_SpawnOverworld(&play,&player);
    Check(!gHorseIsMounted && !player.rideActor && !mounts && !cameras,
          "failed horse spawn reached native null dereference");
    allocationFails=false;ownsSong=false;gHorseIsMounted=true;spawns=mounts=cameras=0;
    Horse_SpawnOverworld(&play,&player);
    Check(!gHorseIsMounted && !mounts,"missing Epona song retained stale mount");
    gHorseIsMounted=D_801BDAA0=gHorsePlayedEponasSong=true;
    Horse_ResetMountState();
    Check(!gHorseIsMounted && !D_801BDAA0 && !gHorsePlayedEponasSong,
          "foreign engine resume retained transient horse action state");
    const auto& kept=gSaveContext.save.saveInfo.horseData;
    Check(kept.sceneId==parked.sceneId && kept.pos.x==parked.pos.x && kept.pos.y==parked.pos.y &&
          kept.pos.z==parked.pos.z && kept.yaw==parked.yaw,
          "mount reset changed Epona's saved parked position");
    std::puts("PASS native horse scene/layer/form gates, suspended mount reset, allocation failure and parked-save preservation");
}
'''
# initializer_list is required by the fixture's matrix loops.
fixture = '#include <initializer_list>\n' + fixture
with tempfile.TemporaryDirectory(prefix='horse-transition-') as tmp:
    path=Path(tmp)/'horse.cpp'; binary=Path(tmp)/'horse'
    path.write_text(fixture)
    command=['c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Wno-unused-parameter']
    if args.sanitize:
        command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    subprocess.run(command+[str(path),'-o',str(binary)],check=True)
    env=dict(os.environ);env['ASAN_OPTIONS']='detect_leaks=0'
    subprocess.run([str(binary)],check=True,env=env)

assert resume.count('Horse_ResetMountState();') == 1, 'engine resume must clear its resident mount marker'
assert resume.index('Horse_ResetMountState();') < resume.index('MM_RunGameLoop();')
print('PASS MM foreign resume clears mount state before player reconstruction')
