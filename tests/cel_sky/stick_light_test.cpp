// Execute the real stick updater using the expanded native light contract.
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <vector>
#include "soh/soh/Enhancements/savestate_serialize.h"
using u8 = uint8_t; using s16 = int16_t; using f32 = float;
struct Vec3f { float x, y, z; };
struct LightInfo { struct { struct { f32 x, y, z; } point; } params; };
struct LightNode {};
struct Player { int heldItemAction; int unk_860; struct { Vec3f tip; } meleeWeaponInfo[1]; };
struct PlayState { int lightCtx; Player* player; };
PlayState* gPlayState;
#define GET_PLAYER(p) ((p)->player)
constexpr int PLAYER_IA_DEKU_STICK = 1;
static bool sInserted = false;
static LightNode node;
static LightNode* sStickNode = nullptr;
static LightInfo sStickLight;
static u8 sBaseR = 255, sBaseG = 150, sBaseB = 0;
constexpr s16 kStickLightRadius = 200;
void ResolveBaseColor() {}
float Rand_ZeroOne() { return .5f; }
void LightContext_RemoveLight(PlayState*, int*, LightNode*) {}
LightNode* LightContext_InsertLight(PlayState*, int*, LightInfo*) { return &node; }
void Lights_PointNoGlowSetInfo(LightInfo* info, f32 x, f32 y, f32 z, u8, u8, u8, s16) { info->params.point = {x,y,z}; }
void Lights_PointSetColorAndRadius(LightInfo*, u8, u8, u8, s16) {}
/* PRODUCTION_STICK_REMOVE */
/* PRODUCTION_STICK_UPDATE */
/* PRODUCTION_STICK_SAVESTATE */
static std::vector<unsigned char> CaptureStick() {
    SaveStateCtx measure{nullptr, 0, SHIP_SAVESTATE_MEASURE};
    DekuStickLight_SaveState(&measure);
    std::vector<unsigned char> data(measure.offset);
    SaveStateCtx save{data.data(), 0, SHIP_SAVESTATE_SAVE};
    DekuStickLight_SaveState(&save);
    return data;
}
static void RestoreStick(std::vector<unsigned char>& data) {
    SaveStateCtx load{data.data(), 0, SHIP_SAVESTATE_LOAD};
    DekuStickLight_SaveState(&load);
}
int main() {
    Player player{PLAYER_IA_DEKU_STICK, 20, {{{50000.5f, -40000.25f, 75000.75f}}}};
    PlayState play{0, &player}; gPlayState = &play;
    auto unlitState = CaptureStick();
    UpdateDekuStickLight();
    if (!sInserted || sStickLight.params.point.x != 50000.5f || sStickLight.params.point.y != -40000.25f || sStickLight.params.point.z != 75000.75f) {
        std::fputs("FAIL: held-stick light narrows expanded world position\n",stderr); return 1;
    }
    auto litState = CaptureStick();
    RestoreStick(unlitState);
    assert(!sInserted && sStickNode == nullptr);
    RestoreStick(litState);
    assert(sInserted && sStickNode == &node && sStickLight.params.point.y == -40000.25f);
    player.meleeWeaponInfo[0].tip = {-60000.5f, 45000.25f, 90000.75f}; UpdateDekuStickLight();
    if (sStickLight.params.point.x != -60000.5f || sStickLight.params.point.y != 45000.25f || sStickLight.params.point.z != 90000.75f) {
        std::fputs("FAIL: held-stick light movement narrows expanded world position\n",stderr); return 1;
    }
    player.unk_860 = 0; UpdateDekuStickLight();
    if (sInserted || sStickNode != nullptr) { std::fputs("FAIL: extinguished stick retains its light\n",stderr); return 1; }
    RestoreStick(litState);
    assert(sInserted && sStickNode == &node && sStickLight.params.point.y == -40000.25f);
    UpdateDekuStickLight();
    assert(!sInserted && sStickNode == nullptr);
    std::puts("cel held-stick wide light and savestate ownership: PASS");
}
