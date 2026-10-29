#include "global.h"
#include "mods/actors/trutefel/actors/z_en_hammergeist.h"
#include "overlays/effects/ovl_Effect_Ss_En_Ice/z_eff_ss_en_ice.h"
#include "overlays/effects/ovl_Effect_Ss_En_Fire/z_eff_ss_en_fire.h"
#include <cassert>
#include <cstdio>
#include <vector>

static PlayState play;
static EnHammergeist enemy;
static std::vector<EffectSsEnIceInitParams> ice;
static std::vector<EffectSsEnFireInitParams> fire;
static std::vector<u16> sounds;
static bool animationDone;
static int animationUpdates, idleTransitions;

extern "C" void Audio_PlayActorSound2(Actor* actor, u16 sound) {
    assert(actor == &enemy.actor);
    assert(sound == NA_SE_PL_FREEZE_S || sound == NA_SE_EV_FLAME_IGNITION);
    sounds.push_back(sound);
}
extern "C" void EffectSs_Spawn(PlayState* state, s32 type, s32 priority, void* parameters) {
    assert(state == &play);
    if (type == EFFECT_SS_EN_ICE) {
        assert(priority == 80);
        const auto& effect = *static_cast<EffectSsEnIceInitParams*>(parameters);
        assert(effect.actor == &enemy.actor && effect.type == 0 && effect.scale == 4);
        assert(effect.primColor.r == 150 && effect.primColor.g == 150 && effect.primColor.b == 150 &&
               effect.primColor.a == 250 && effect.envColor.r == 235 && effect.envColor.g == 245 &&
               effect.envColor.b == 255);
        ice.push_back(effect);
    } else {
        assert(type == EFFECT_SS_EN_FIRE && priority == 128);
        const auto& effect = *static_cast<EffectSsEnFireInitParams*>(parameters);
        assert(effect.actor == &enemy.actor && effect.scale == 400 && effect.unk_12 == 0 &&
               effect.flags == 0 && effect.bodyPart == -1);
        fire.push_back(effect);
    }
}
extern "C" s32 SkelAnime_Update(SkelAnime* animation) {
    assert(animation == &enemy.skelAnime);
    ++animationUpdates;
    return animationDone;
}
extern "C" void EnHammergeist_SetupDoNothing(EnHammergeist* actor, PlayState* state) {
    assert(actor == &enemy && state == &play);
    ++idleTransitions;
}
extern "C" {
#include "hammergeist_adapters.inc"
}

static void Frame(float frame) {
    enemy.skelAnime.curFrame = frame;
    EnHammergeist_Infuse(&enemy, &play);
}

static void Position(const Vec3f& actual, const Vec3f& expected, float yOffset = 0) {
    assert(actual.x == expected.x && actual.y == expected.y + yOffset && actual.z == expected.z);
}

int main() {
    const Vec3f left{ 180000.25f, 90000.5f, -123456.75f };
    const Vec3f right{ -234567.5f, 120000.25f, 180000.75f };
    enemy.hammerLeftCollider.dim.pos = left;
    enemy.hammerRightCollider.dim.pos = right;
    enemy.leftHammerInfused = enemy.rightHammerInfused = true;
    EnHammergeist_DefuseLeftHammer(&enemy, &play);
    assert(!enemy.leftHammerInfused && enemy.rightHammerInfused && ice.size() == 8 && fire.empty());
    for (const auto& effect : ice)
        Position(effect.pos, left);
    EnHammergeist_DefuseRightHammer(&enemy, &play);
    assert(!enemy.rightHammerInfused && fire.size() == 8);
    for (const auto& effect : fire)
        Position(effect.pos, right);
    Frame(8);
    assert(ice.size() == 8 && fire.size() == 8);
    Frame(9);
    assert(enemy.faceIndex == HAMMERGEIST_FACE_MOUTH_OPEN);
    Frame(14);
    assert(!enemy.rightHammerInfused && fire.size() == 8);
    Frame(15);
    assert(enemy.rightHammerInfused && fire.size() == 16 && enemy.faceIndex == HAMMERGEIST_FACE_NORMAL);
    Frame(31);
    assert(enemy.faceIndex == HAMMERGEIST_FACE_MOUTH_OPEN && !enemy.leftHammerInfused);
    Frame(37);
    assert(enemy.leftHammerInfused && ice.size() == 16 && enemy.faceIndex == HAMMERGEIST_FACE_LAUGH);
    for (size_t i = 8; i < ice.size(); ++i)
        Position(ice[i].pos, left, 70);
    for (const auto& effect : fire)
        Position(effect.pos, right);
    animationDone = true;
    Frame(38);
    assert(animationUpdates == 7 && idleTransitions == 1 && ice.size() == 16 && fire.size() == 16);
    assert(sounds.size() == 32);
    Position(enemy.hammerLeftCollider.dim.pos, left);
    Position(enemy.hammerRightCollider.dim.pos, right);
    std::puts("PASS Hammergeist: native float effect positions, original eight-particle bursts, frame guards, colors/scales and offset");
}
