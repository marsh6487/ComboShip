// The renderer fixture already owns OoT's native Player type. Isolate the MM
// pose inputs here instead of reinterpreting that incompatible engine layout.
// Full MM Player/CustomItem/Actor_Draw coverage lives in adult_mm_item_presentation.
namespace MmReceiptFixture {
#include "nei_mm_receipt_enums.inc"
struct Player {
    Actor actor{};
    int transformation = PLAYER_FORM_HUMAN;
    Vec3f bodyPartsPos[PLAYER_BODYPART_MAX]{};
    PosRot leftHandWorld{};
};
Player player{};
struct LocalPlay {
    Player* localPlayer = &player;
} localPlay;
LocalPlay* gPlayState = &localPlay;
int skeleton;
int* sSkel = &skeleton;
u8 sReady = 1, sIsChildRig = 0;
bool adultMode = false, mario = false, wolf = false;
int activeForm = CUSTOM_FORM_NONE;
Actor receipt{};
s32 AdultLink_IsActive() { return adultMode; }
s32 CustomForms_ActiveForm() { return activeForm; }
u8 Sm64Mario_ShouldHideLink() { return mario; }
u8 WolfLinkForm_IsSelected() { return wolf; }
void FixtureActorScale(Actor* actor, float scale) { actor->scale = {scale, scale, scale}; }
void ResetEligibility() {
    player = {};
    player.leftHandWorld.pos = {7, 171, 23};
    player.bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND] = {13, 175, 27};
    localPlay.localPlayer = &player;
    gPlayState = &localPlay;
    sSkel = &skeleton;
    sReady = 1;
    sIsChildRig = 0;
    adultMode = mario = wolf = false;
    activeForm = CUSTOM_FORM_NONE;
}
// Resource readiness, local-player identity and replacement renderer selection
// are controlled boundaries. Eligibility and the receipt pose run verbatim.
#pragma push_macro("GET_PLAYER")
#undef GET_PLAYER
#define GET_PLAYER(play) ((play)->localPlayer)
#include "nei_mm_receipt_production.inc"
#pragma pop_macro("GET_PLAYER")
} // namespace MmReceiptFixture
