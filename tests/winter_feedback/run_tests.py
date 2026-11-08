#!/usr/bin/env python3
"""Exercise production shovel and winter feedback bodies with native engine types."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/ui_visibility'))
from run_control_input_tests import compiler_flags, function


def body(host, path, name):
    return function((ROOT / host / path).read_text(), name)


def compile_run(host, case, source):
    with tempfile.TemporaryDirectory(prefix='winter-feedback-') as directory:
        path = Path(directory) / 'test.c'
        path.write_text(source)
        for combo in (False, True):
            binary = Path(directory) / ('combo' if combo else 'original')
            flags = compiler_flags(host, combo)
            # Native headers use signed (1 << 31) flag constants; retain all
            # other ASan/UBSan checks without testing that pre-existing spelling.
            flags += ['-fno-sanitize=shift-base']
            if host == 'mm':
                flags += ['-DWINTER_TEST_MM', '-include', str(ROOT / 'mm/include/z64.h'),
                          '-DPLAYER_STATE1_LOADING=PLAYER_STATE1_200', '-DPLAYER_STATE1_IN_ITEM_CS=0u',
                          '-DPLAYER_STATE1_GETTING_ITEM=PLAYER_STATE1_400',
                          '-DPLAYER_STATE1_DAMAGED=PLAYER_STATE1_4000000',
                          '-DPLAYER_STATE1_HANGING_OFF_LEDGE=0u',
                          '-DPLAYER_STATE1_CLIMBING_LEDGE=PLAYER_STATE1_4',
                          '-include', str(ROOT / 'mm/mods/nei_oot_compat.h'),
                          '-include', str(ROOT / 'mm/soh/_nei_compat_core.h')]
            subprocess.run([os.environ.get('CC', 'cc'), *flags, str(path), '-lm', '-o', str(binary)],
                           check=True, cwd=ROOT)
            subprocess.run([str(binary)], check=True, cwd=ROOT,
                           env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
            print(f'PASS {host} {case} {"Combo" if combo else "original"}', flush=True)


def shovel(host):
    prefix = r'''
#include "global.h"
#include "mods/items/logic/item_shovel.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/extended_player.h"
#include <assert.h>
#include <stdio.h>
#ifdef WINTER_TEST_MM
#define upperSkelAnime skelAnimeUpper
#define linearVelocity speedXZ
#endif
CustomItemState gCustomItemState;
static ItemInputState nextInput;
static s8 sShovelPrevInvinc;
static LinkAnimationHeader digAnimation, equipAnimation;
static int assetAvailable = 1, impacts, starts;
#define NEI_ANIM_DAMPE_DIG "dig"
LinkAnimationHeader* NeiAnim_Load(const char* name) {return assetAvailable ? &digAnimation : NULL;}
void LinkAnimation_PlayOnce(PlayState* play, SkelAnime* skel, LinkAnimationHeader* animation) {
    ++starts;
    skel->animation = animation;
    skel->curFrame = 0;
    skel->endFrame = 39;
}
s32 LinkAnimation_Update(PlayState* play, SkelAnime* skel) {
    if (skel->curFrame >= skel->endFrame) return 1;
    skel->curFrame += 1.5f;
    return 0;
}
void ItemEquip_PlayEquipSFXForAction(PlayState* play, Player* p, s32 action) {}
void ItemEquip_PlayUnequipSFXForAction(PlayState* play, Player* p, s32 action) {}
void PerformDig(Player* p, PlayState* play) {++impacts;}
void ItemInput_Update(ItemInputState* out, u8 item, Player* p, PlayState* play) {*out = nextInput;}
'''
    functions = [body(host, 'mods/items/helpers/equip_helper.c', name)
                 for name in ('ItemInput_CheckDamage', 'ItemInput_IsBlockedEx', 'ItemInput_IsBlocked')]
    functions += [body(host, 'mods/items/logic/item_shovel.c', name)
                  for name in ('Shovel_Stop', 'Shovel_Start', 'Shovel_UpdateAnimation',
                               'Handle_Shovel', 'Player_UpperAction_Shovel')]
    checks = r'''
int main(void) {
    Player p = {0};
    PlayState play = {0};
    p.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    nextInput.wasEquipped = 1;
    nextInput.isPressed = 1;
    Handle_Shovel(&p, &play);
    assert(shActive);
    // The native equip transition owns this skeleton before the first shovel callback.
    p.upperSkelAnime.animation = &equipAnimation;
    p.upperSkelAnime.curFrame = p.upperSkelAnime.endFrame = 10;
    nextInput.isPressed = 0; // release; never send a second press
    for (int frame = 0; frame < 60 && shActive; ++frame) {
        Player_UpperAction_Shovel(&p, &play);
        Handle_Shovel(&p, &play);
    }
    assert(impacts == 1 && !shActive && !shAnimating);
    assert(p.upperSkelAnime.animation == &digAnimation);
    assert(!(p.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    // Already equipped: one more press produces exactly one more impact.
    nextInput.isPressed = 1;
    Handle_Shovel(&p, &play);
    nextInput.isPressed = 0;
    for (int frame = 0; frame < 60 && shActive; ++frame) {
        Player_UpperAction_Shovel(&p, &play);
        Handle_Shovel(&p, &play);
    }
    assert(impacts == 2 && !shActive);
    assetAvailable = 0;
    nextInput.isPressed = 1;
    Handle_Shovel(&p, &play);
    assert(!shActive && impacts == 2);
    assetAvailable = 1;
    Handle_Shovel(&p, &play);
    nextInput.wasEquipped = 0;
    Handle_Shovel(&p, &play);
    assert(!shActive && !shAnimating);
    puts("PASS first shovel press survives native equip handoff; repeat, missing asset, removal");
}
'''
    compile_run(host, 'shovel', prefix + '\n'.join(functions) + checks)


def surface(host):
    player = (ROOT / host / 'src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include <assert.h>
#include <stdio.h>
static PlayState testPlay;
PlayState* gPlayState = &testPlay;
SaveContext gSaveContext;
static int winter = 1, onWater, frozen, foreignAudio = 1, mmCalls, nativeCalls, transformed;
static int lastSfx;
int32_t SeasonWeather_ParticleSeasonForPlay(const PlayState* play) {return winter ? SEASON_WINTER : -1;}
int MMWeather_SeasonForPlay(const PlayState* play) {return winter ? SEASON_WINTER : -1;}
u8 RocBoots_OnWater(void) {return onWater;}
u8 RocBoots_OnFrozenWater(void) {return frozen;}
s32 MmSfx_PlayAtPos(u16 sfx, Vec3f* pos) {++mmCalls; lastSfx = sfx; return foreignAudio;}
u8 TransformMasks_TryPlayMmStepSfx(u16 sfx, Vec3f* pos) {
    if (transformed) {lastSfx = sfx + 0x120; ++mmCalls;}
    return transformed;
}
void func_800F4010(Vec3f* pos, u16 sfx, f32 pitch) {++nativeCalls; lastSfx = sfx;}
bool WolfLinkHost_PlayFootstep(Player* p) {return false;}
static int iron;
u8 VanillaTB_IsIronBoots(void) {return iron;}
void Audio_PlaySfx_AtPosForMetalEffectsWithSyncedFreqAndVolume(Vec3f* pos, u16 sfx, f32 lerp) {
    ++nativeCalls; lastSfx = sfx;
}
'''
    step = 'Player_AnimSfx_PlayFloorWalk' if host == 'mm' else 'Player_PlaySteppingSfx'
    funcs = [function(player, 'Player_GetStepSfxOffset'), function(player, step)]
    checks = r'''
int main(void) {
    Player p = {0};
#ifdef WINTER_TEST_MM
    PlayerAgeProperties age = {0};
#else
    PlayerAgeProperties age = {0};
#endif
    p.ageProperties = &age;
    p.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    p.actor.floorBgId = BGCHECK_SCENE;
    const int dry[] = {0, 1, 2, 3, 8};
    for (unsigned i = 0; i < sizeof(dry)/sizeof(dry[0]); ++i) {
        p.floorSfxOffset = dry[i];
        assert(Player_GetStepSfxOffset(&p) == 14);
        STEP(&p, 0.0f);
        assert(lastSfx == 0x80E);
        assert(p.floorSfxOffset == dry[i]); // audio selection cannot change floor physics/effects
    }
    const int protected[] = {4, 5, 6, 7, 9, 10, 11, 12, 13, 15};
    for (unsigned i = 0; i < sizeof(protected)/sizeof(protected[0]); ++i) {
        p.floorSfxOffset = protected[i];
        assert(Player_GetStepSfxOffset(&p) == protected[i]);
    }
    p.floorSfxOffset = 0;
    p.actor.bgCheckFlags |= BGCHECKFLAG_WATER;
    assert(Player_GetStepSfxOffset(&p) == 0);
    p.actor.bgCheckFlags = 0;
    assert(Player_GetStepSfxOffset(&p) == 0);
    p.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    onWater = 1;
    assert(Player_GetStepSfxOffset(&p) == 0);
    onWater = 0; winter = 0; // other seasons, interiors, Native
    assert(Player_GetStepSfxOffset(&p) == 0);
    winter = 1; p.actor.floorBgId = 0;
    assert(Player_GetStepSfxOffset(&p) == 0); // moving props are not snowy scene terrain
    p.actor.floorBgId = BGCHECK_SCENE;
#ifdef WINTER_TEST_MM
    iron = 1;
    STEP(&p, 0.0f);
    assert(lastSfx == NA_SE_PL_WALK_METAL1);
    iron = 0;
    age.surfaceSfxIdOffset = 0x120;
    STEP(&p, 0.0f);
    assert(lastSfx == 0x92E);
#else
    p.currentBoots = PLAYER_BOOTS_IRON;
    STEP(&p, 0.0f);
    assert(lastSfx == NA_SE_PL_WALK_HEAVYBOOTS);
    p.currentBoots = PLAYER_BOOTS_KOKIRI;
    transformed = 1;
    STEP(&p, 0.0f);
    assert(lastSfx == 0x92E);
    transformed = 0; foreignAudio = 0;
    STEP(&p, 0.0f);
    assert(lastSfx == NA_SE_PL_WALK_SAND); // standalone without optional MM audio
#endif
    puts("PASS snow terrain selection, ice/water priority, boot/form offsets and native fallback");
}
'''.replace('STEP(', step + '(')
    compile_run(host, 'surface', prefix + '\n'.join(funcs) + checks)


def block_at(source, opening):
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[opening:end]


def collision(host):
    player = (ROOT / host / 'src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    proc = function(player, 'Player_ProcessSceneCollision')
    before = proc[:proc.index('    Actor_UpdateBgCheckInfo(')]
    call = proc[proc.index('    Actor_UpdateBgCheckInfo('):]
    call = call[:call.index(';') + 1]
    before = before.replace('Player_ProcessSceneCollision', 'CheckRippleFlags') + call + '\n}'
    start = proc.index('// Roc\'s Boots: WALK ON WATER')
    pin = block_at(proc, proc.index('{', start))
    suffix = ''
    if host == 'mm':
        offset = proc.index(pin) + len(pin)
        suffix = proc[offset:proc.rfind('}')]
    prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include <assert.h>
#include <stdio.h>
#ifdef WINTER_TEST_MM
#include "2s2h/GameInteractor/GameInteractor.h"
#define yDistToWater depthInWater
#define WATER_SWIM PLAYER_STATE1_8000000
#else
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#define WATER_SWIM PLAYER_STATE1_IN_WATER
#endif
static int winter = 1, boots;
static u8 sRocOnWater;
static s32 sPrevFloorProperty, sFloorType, sPlayerFloorType, sPlayerFloorEffect;
static f32 sYDistToFloor, sPlayerYDistToFloor;
static s16 sFloorShapePitch, sFloorPitchShape;
static u32 receivedFlags;
u8 Seasons_WalksOnWater(void) {return winter;}
u8 RocBoots_IsWorn(void) {return boots;}
u8 GaroForm_WalksOnWater(void) {return 0;}
s32 MinishTiny_CrawlspacePassthrough(Player* p, PlayState* play) {return 0;}
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {return result;}
bool func_801242B4(Player* p) {return false;}
void Player_Action_35(Player* p, PlayState* play) {}
void Player_Action_36(Player* p, PlayState* play) {}
void Player_Action_93(Player* p, PlayState* play) {}
void Player_Action_80845EF8(Player* p, PlayState* play) {}
void Player_Action_80845CA4(Player* p, PlayState* play) {}
#ifdef WINTER_TEST_MM
void Actor_UpdateBgCheckInfo(PlayState* play, Actor* actor, f32 h, f32 r, f32 c, u32 flags) {receivedFlags = flags;}
u16 SurfaceType_GetSfxOffset(CollisionContext* ctx, CollisionPoly* poly, s32 id) {return 0;}
#else
void Actor_UpdateBgCheckInfo(PlayState* play, Actor* actor, f32 h, f32 r, f32 c, s32 flags) {receivedFlags = flags;}
void Math_Vec3f_Copy(Vec3f* dst, Vec3f* src) {*dst = *src;}
#endif
'''
    boots_source = 'mods/equipment/behaviors/equip_roc_boots.c'
    funcs = [body(host, boots_source, 'RocBoots_WalksOnWater')]
    if 'RocBoots_OnFrozenWater' in (ROOT / host / boots_source).read_text():
        funcs.append(body(host, boots_source, 'RocBoots_OnFrozenWater'))
    else:
        funcs.append('u8 RocBoots_OnFrozenWater(void) {return 0;}')
    pin = 'void CheckSurfacePin(PlayState* play, Player* this) {CollisionPoly* floorPoly = this->actor.floorPoly; s32 spAC = 0;\n' + pin + suffix + '\n}'
    checks = r'''
int main(void) {
    Player p = {0};
    PlayState play = {0};
    PlayerAgeProperties age = {0};
    p.ageProperties = &age;
#ifdef WINTER_TEST_MM
    p.transformation = PLAYER_FORM_HUMAN;
#endif
    CheckRippleFlags(&play, &p);
    assert(receivedFlags & 0x40); // native Actor_UpdateBgCheckInfo's no-entry-ripple option
    p.stateFlags1 = WATER_SWIM;
    CheckRippleFlags(&play, &p);
    assert(!(receivedFlags & 0x40)); // submerged swimming retains normal water effects
    p.stateFlags1 = 0;
    winter = 0;
    CheckRippleFlags(&play, &p);
    assert(!(receivedFlags & 0x40)); // water walking boots outside winter remain watery
    winter = 1;
    p.actor.yDistToWater = 0.5f;
    p.actor.world.pos.y = 100;
    p.actor.velocity.y = -1;
    p.actor.bgCheckFlags = BGCHECKFLAG_WATER | BGCHECKFLAG_WATER_TOUCH;
    p.floorSfxOffset = 4;
    CheckSurfacePin(&play, &p);
    assert(p.actor.world.pos.y == 100.5f && p.actor.floorHeight == 100.5f);
    assert(p.actor.bgCheckFlags & BGCHECKFLAG_GROUND);
    assert(!(p.actor.bgCheckFlags & (BGCHECKFLAG_WATER | BGCHECKFLAG_WATER_TOUCH)));
    assert(p.floorSfxOffset == 15); // no lake-bottom polygon needed for ice audio
    assert(p.actor.velocity.y == 0 && RocBoots_OnFrozenWater());
    p.stateFlags1 = WATER_SWIM;
    p.actor.yDistToWater = 30;
    CheckSurfacePin(&play, &p);
    assert(!RocBoots_OnFrozenWater() && p.actor.world.pos.y == 100.5f);
    p.stateFlags1 = 0;
    p.actor.velocity.y = 1;
    CheckSurfacePin(&play, &p);
    assert(!RocBoots_OnFrozenWater());
    p.actor.velocity.y = 0;
    winter = 0; boots = 1;
    p.floorSfxOffset = 4;
    CheckSurfacePin(&play, &p);
    assert(p.floorSfxOffset == 4 && !RocBoots_OnFrozenWater());
    winter = 1; boots = 0;
    p.actor.yDistToWater = -32000;
    CheckSurfacePin(&play, &p);
    assert(!RocBoots_OnFrozenWater()); // shoreline: no stale surface pin
    puts("PASS frozen pin, null bottom, ripple flags, swimming, boots, jumping and shoreline");
}
'''
    compile_run(host, 'collision', prefix + '\n'.join(funcs) + before + pin + checks)


def grotto(host):
    prefix = r'''
#include "global.h"
#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"
#include "mods/items/helpers/fx_helper.h"
#include <assert.h>
#include <stdio.h>
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;
static int colliderDestroyed, chimes;
s32 Collider_DestroyCylinder(PlayState* play, ColliderCylinder* collider) {++colliderDestroyed; return 1;}
void DoorAna_WaitOpen(DoorAna* door, PlayState* play) {}
void Audio_PlaySoundGeneral(u16 id, Vec3f* pos, u8 token, f32* freq, f32* volume, s8* reverb) {++chimes;}
void FX_SpawnParticles(PlayState* play, Vec3f* pos, FX_Type type, u8 count) {}
'''
    func = body(host, 'mods/items/logic/item_shovel.c', 'RevealGrotto')
    checks = r'''
int main(void) {
    PlayState play = {0};
    for (int type = 0x100; type <= 0x200; type += 0x100) {
        DoorAna grotto = {0};
        grotto.actor.params = 0x504A | type;
        int before = colliderDestroyed;
        RevealGrotto(&play, &grotto.actor);
        assert(grotto.actor.params == 0x504A && grotto.actionFunc == DoorAna_WaitOpen);
        assert(colliderDestroyed == before + (type == 0x200));
        before = chimes;
        RevealGrotto(&play, &grotto.actor);
        assert(chimes == before); // repeated digs cannot double-destroy a collider
    }
#ifdef WINTER_TEST_MM
    DoorAna sceneExit = {0};
    sceneExit.actor.params = 0x634A;
    int before = colliderDestroyed;
    RevealGrotto(&play, &sceneExit.actor);
    assert(sceneExit.actor.params == 0x634A && !sceneExit.actionFunc && colliderDestroyed == before);
#endif
    puts("PASS first dig reveals hidden grottos once; destination bits and visible scene exits preserved");
}
'''
    compile_run(host, 'grotto', prefix + func + checks)


def freeze(host):
    prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include <assert.h>
#include <stdio.h>
static PlayState testPlay;
PlayState* gPlayState = &testPlay;
// Model world state changing between frames, outside the production getter.
static volatile int owned = 1, thawed, presentation = -1;
static volatile u8 saved = SEASON_WINTER;
u8 Seasons_SeasonCount(void) {return owned;}
u8 Seasons_GetSeason(void) {return saved;}
bool ZorasDomain_IsAlwaysThawed(PlayState* play) {return thawed;}
int MMWeather_Season(void) {return presentation;}
'''
    func = body(host, 'mods/items/logic/item_rod_of_seasons.c', 'Seasons_WalksOnWater')
    checks = r'''
int main(void) {
    assert(Seasons_WalksOnWater()); // temple: weather is ineligible, saved Winter remains active
    saved = SEASON_SUMMER; presentation = SEASON_WINTER;
    assert(!Seasons_WalksOnWater()); // manual preview cannot grant freeze collision
    saved = SEASON_WINTER; owned = 0;
    assert(!Seasons_WalksOnWater());
#ifndef WINTER_TEST_MM
    owned = 1; thawed = 1;
    assert(!Seasons_WalksOnWater()); // preserve the explicit Zora's Domain thaw exception
#endif
    puts("PASS owned saved Winter freezes temple pools; previews and unowned seasons do not");
}
'''
    compile_run(host, 'freeze', prefix + func + checks)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('case', choices=('shovel', 'surface', 'collision', 'grotto', 'freeze', 'all'), nargs='?', default='all')
    args = parser.parse_args()
    for case in ('shovel', 'surface', 'collision', 'grotto', 'freeze') if args.case == 'all' else (args.case,):
        for host in ('soh', 'mm'):
            globals()[case](host)
