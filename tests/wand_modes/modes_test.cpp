// Real mode/input/magic bodies; heap, object, collider registration and drawing are boundaries.
#include "global.h"
#include "mods/items/custom_items.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/items/helpers/target_select_helper.h"
#include "mods/items/helpers/combat_helper.h"
#include "mods/items/helpers/fx_helper.h"
#include "mods/items/objects/object_tornado.h"
#include "mods/extended_player.h"
#include "mods/extended_equipment.h"
#include "mods/extended_inventory.h"
#include "mods/oot_asset_loader/oot_asset_loader.h"
#include "GameInteractor/GameInteractor.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

SaveContext gSaveContext{};
PlayState* gPlayState;
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;
MtxF matrixStack[32]{};
MtxF* sCurrentMatrix = matrixStack;
RegEditor editor{}; RegEditor* gRegEditor = &editor;
static Input* sPlayerControlInput;
static s32 sPlayerUseHeldItem, sPlayerHeldItemButtonIsHeldDown;
static u16 sPlayerItemButtons[] = {BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT};
static u16 sDpadItemButtons[] = {BTN_DRIGHT, BTN_DLEFT, BTN_DDOWN, BTN_DUP};
u8 gItemSlots[77]{};
PlayerAgeProperties sPlayerAgeProperties[PLAYER_FORM_MAX]{};
typedef struct {u8 itemId; s16 actorId;} ExplosiveInfo;
ExplosiveInfo sPlayerExplosiveInfo[PLAYER_EXPLOSIVE_MAX]{};
bool capeOwned = false, spawnFails = false, onSlab = false, modelPresent = false;
int ownershipRule = WAND_RANDO_MEDALLIONS, requestedObjects = 0, menuOpened = 0, hits = 0;
Collider* registeredCollider;
std::vector<Actor*> actors;
CollisionHeader slabCollision{};
typedef struct { const char* iconPath; u8 iconSize; u8 enabled; } BoxMenuEntry;
typedef void (*BoxMenuConfirmFn)(s32);
bool menuOpen = false;
BoxMenuConfirmFn confirmWheel;
int wheelCount, wheelIndex;
#define CANE_TYPE_ULTRAHAND 3
#define TARGETSEL_LIST_HEAD(list) ((list).first)
const u8 gTargetSelectDefaultCats[4] = { ACTORCAT_ENEMY, ACTORCAT_PROP, ACTORCAT_CHEST, ACTORCAT_NPC };
// C's enum loop/cross-enum conversions, retained for native C bodies in this C++ fixture.
EquipSlot operator++(EquipSlot& slot, int) { auto old = slot; slot = (EquipSlot)((int)slot + 1); return old; }
DpadEquipSlot operator++(DpadEquipSlot& slot, int) { auto old = slot; slot = (DpadEquipSlot)((int)slot + 1); return old; }
ItemId Player_Dpad_GetItemOnButton(PlayState* play, Player* player, EquipSlot slot) {
    return Player_Dpad_GetItemOnButton(play, player, (DpadEquipSlot)slot);
}

s32 CVarGetInteger(const char* name, s32 fallback) {
    return std::strcmp(name, "gRando.Options.RO_ELEMENTAL_WAND_SHUFFLE") == 0 ? ownershipRule :
        std::strcmp(name, "gEnhancements.Dpad.DpadEquips") == 0 ? 1 : fallback;
}
NeiSaveData* Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
u8 ExtEquip_CapeOwned() { return capeOwned; }
u8 Pacci_UltrahandModeActive() { return 0; }
u8 Sw97_IsBowItem(u8) { return 0; }
u8 Sw97_IsSlingItem(u8) { return 0; }
u8 Sw97_EffectiveElement(u8) { return 0; }
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
void GameInteractor_ExecuteOnSceneFlagSet(s16, FlagType, u32) {}
void GameInteractor_ExecuteOnSceneFlagUnset(s16, FlagType, u32) {}
void Audio_PlaySoundGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) {}
void ItemEquip_PlayEquipSFX(PlayState*, Player*) {}
void ItemEquip_PlayUnequipSFX(PlayState*, Player*) {}
void Actor_PlaySfx_Flagged(Actor*, u16) {}
void Actor_SetColorFilter(Actor* actor, u16, u16, u16, u16 frames) { actor->colorFilterTimer = frames; }
void SoundSource_PlaySfxAtFixedWorldPos(PlayState*, Vec3f*, s32, u16) {}
void FX_SpawnRadialDust(PlayState*, Vec3f*, f32, f32, u8, FX_Color*) {}
f32 Math_SinS(s16 yaw) { return std::sin(yaw * M_PI / 32768.0); }
f32 Math_CosS(s16 yaw) { return std::cos(yaw * M_PI / 32768.0); }
s16 Math_Vec3f_Yaw(Vec3f* from, Vec3f* to) { return std::atan2(to->x - from->x, to->z - from->z) * 32768.0 / M_PI; }
s16 Math_Vec3f_Pitch(Vec3f* from, Vec3f* to) {
    float dx = to->x - from->x, dz = to->z - from->z;
    return std::atan2(from->y - to->y, std::sqrt(dx * dx + dz * dz)) * 32768.0 / M_PI;
}
f32 Math_SmoothStepToF(f32* value, f32 target, f32 scale, f32 maxStep, f32 minStep) {
    float step = (target - *value) * scale;
    if (step > maxStep) step = maxStep;
    if (step < -maxStep) step = -maxStep;
    if (std::fabs(step) < minStep) step = target - *value;
    *value += step;
    return target - *value;
}
void FrameInterpolation_RecordMatrixPush() {}
void FrameInterpolation_RecordMatrixPop() {}
void FrameInterpolation_RecordMatrixRotate1Coord(u32, f32, u8) {}

// Unrelated native actions/animation/camera are boundaries; native wand dispatch
// itself includes the actual item-action lookup, UseItem and button processors.
u8 Cane_GetType() { return 0; }
u8 Pacci_IsHoldingUltrahand() { return 0; }
bool func_801240DC(Player*) { return false; }
s32 func_8082DA90(PlayState*) { return 0; }
s32 func_8082FD0C(Player*, PlayerItemAction) { return EQUIP_SLOT_NONE; }
DpadEquipSlot func_Dpad_8082FD0C(Player*, PlayerItemAction) { return EQUIP_SLOT_D_NONE; }
u8 Player_MaskIdToItemId(s32) { return ITEM_NONE; }
void func_80838A20(PlayState*, Player*) {}
s32 Inventory_GetBtnBItem(PlayState*) { return ITEM_NONE; }
PlayerBButtonSword Player_GetHeldBButtonSword(Player*) { return PLAYER_B_SWORD_NONE; }
PlayerBButtonSword Player_BButtonSwordFromIA(Player*, PlayerItemAction) { return PLAYER_B_SWORD_NONE; }
PlayerMeleeWeapon Player_MeleeWeaponFromIA(PlayerItemAction) { return PLAYER_MELEEWEAPON_NONE; }
PlayerExplosive Player_ExplosiveFromIA(Player*, PlayerItemAction) { return PLAYER_EXPLOSIVE_NONE; }
void func_80839978(PlayState*, Player*) {}
void func_80839A10(PlayState*, Player*) {}
s32 RocsFeatherVanilla_TryUse(PlayState*, Player*, s32) { return 0; }
u8 CustomForms_UseItem(Player*, ItemId) { return 0; }
void WolfLinkHost_OnUseItem(PlayState*, Player*, s32) {}
void CustomItems_PutAwayHeldItems(Player*, PlayState*) {}
void ItemEquip_BeginItemChangeSound(PlayState*, Player*, s32) {}
s32 OotSpells_TryUseItem(PlayState*, Player*, PlayerItemAction, ItemId) { return 0; }
void func_808318C0(PlayState*) {}
s32 func_80831814(Player*, PlayState*, s32) { return 0; }
void func_8083A658(PlayState*, Player*) {}
void func_8082E1F0(Player*, u16) {}
void Player_DestroyHookshot(Player*) {}
void Player_DetachHeldActor(PlayState*, Player*) {}
void Player_InitItemActionWithAnim(PlayState*, Player* player, PlayerItemAction action) {
    player->heldItemAction = player->itemAction = action; player->modelGroup = player->nextModelGroup;
}
void Player_StartChangingHeldItem(Player* player, PlayState*) { player->stateFlags3 &= ~PLAYER_STATE3_START_CHANGING_HELD_ITEM; }
void Audio_PlaySfx(u16) {}
void lusprintf(const char*, int, int, const char*, ...) {}
bool Player_IsGoronOrDeku(Player* player) { return player->transformation == PLAYER_FORM_GORON || player->transformation == PLAYER_FORM_DEKU; }
s32 BgCheck_EntityCheckCeiling(CollisionContext*, f32*, Vec3f*, f32, CollisionPoly**, s32*, Actor*) { return 0; }
u8 Message_GetState(MessageContext*) { return TEXT_STATE_NONE; }
void Message_StartTextbox(PlayState*, u16, Actor*) {}
u8 MasterCycle_IsRiding() { return 0; }
s32 CustomItems_BlocksMovement(Player*) { return 0; }
void GameInteractor_ExecuteOnPassPlayerInputs(Input*) {}
void WolfLinkHost_RestorePlayerInput(Player*, Input*) {}

u8 BoxMenu_IsOpen() { return menuOpen; }
u8 BoxMenu_Open(PlayState*, const BoxMenuEntry*, s32 count, s32 selected, u16 button, BoxMenuConfirmFn fn) {
    assert(button == BTN_L); ++menuOpened; wheelCount = count; wheelIndex = selected; confirmWheel = fn; return 1;
}
void* Wand_ModeIcon(u8) { static const char icon[] = "icon"; return (void*)icon; }
void ExtInv_RefreshButtonIconsForItem(PlayState*, u16 item) { assert(item == ITEM_ELEMENTAL_WAND); }
s32 func_8083485C(Player*, PlayState*) { return 0; }
s16 Animation_GetLastFrame(void*) { return 12; }
void PlayerAnimation_Change(PlayState*, SkelAnime*, PlayerAnimationHeader*, f32, f32, f32, u8, f32) {}
s32 PlayerAnimation_Update(PlayState*, SkelAnime*) { return 1; }
static void WandSand_SlabDraw(Actor*, PlayState*) {}
static void WandWater_GeyserDraw(Actor*, PlayState*) {}
static void WandMeteor_TintDraw(Actor*, PlayState*) {}
void LiveActor(Actor*, PlayState*) {}
void NativeBombUpdate(Actor* actor, PlayState*) {
    actor->world.pos.x += Math_SinS(actor->world.rot.y) * actor->speed;
    actor->world.pos.z += Math_CosS(actor->world.rot.y) * actor->speed;
    actor->bgCheckFlags &= ~BGCHECKFLAG_WALL;
    if (((EnBom*)actor)->timer == 0) actor->params = BOMB_TYPE_EXPLOSION;
}
void Actor_Kill(Actor* actor) { actor->update = nullptr; actor->draw = nullptr; }
s32 DynaPolyActor_IsPlayerOnTop(DynaPolyActor*) { return onSlab; }
s32 Object_SpawnPersistent(ObjectContext* ctx, s16 id) {
    ++requestedObjects; ctx->slots[ctx->numEntries].id = id; return ctx->numEntries++;
}
Actor* Actor_Spawn(ActorContext*, PlayState* play, s16 id, f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz, s32 params) {
    if (spawnFails) return nullptr;
    Actor* actor = (Actor*)std::calloc(1, std::max({sizeof(EnBom), sizeof(DynaPolyActor), sizeof(Actor)}));
    actors.push_back(actor);
    actor->id = id; actor->world.pos = {x, y, z}; actor->shape.rot = {rx, ry, rz};
    actor->world.rot = actor->shape.rot; actor->params = params; actor->update = LiveActor;
    actor->scale = {0.1f, 5.0f / 90.0f, 0.1f};
    if (id == ACTOR_OBJ_LIFT) {
        // The separate native Storm test executes full native spawn. Here the
        // ObjLift boundary supplies its registered native collision and flag gate.
        if (Flags_GetSwitch(play, 0)) Actor_Kill(actor);
        ((DynaPolyActor*)actor)->bgId = 0;
        play->colCtx.dyna.bgActors[0].colHeader = &slabCollision;
    } else if (id == ACTOR_EN_BOM) {
        actor->update = NativeBombUpdate; actor->draw = LiveActor;
    } else {
        assert(id == ACTOR_EN_OKARINA_EFFECT);
        if (play->envCtx.precipitation[PRECIP_RAIN_CUR]) Actor_Kill(actor);
    }
    return actor;
}
s32 Collider_InitCylinder(PlayState*, ColliderCylinder*) { return 1; }
s32 Collider_SetCylinder(PlayState*, ColliderCylinder* col, Actor* owner, ColliderCylinderInit* init) {
    col->base.actor = owner; col->base.atFlags = init->base.atFlags;
    col->elem.atDmgInfo.dmgFlags = init->elem.atDmgInfo.dmgFlags;
    col->elem.atDmgInfo.effect = init->elem.atDmgInfo.effect;
    col->elem.atDmgInfo.damage = init->elem.atDmgInfo.damage;
    col->dim = init->dim; return 1;
}
s32 CollisionCheck_SetAT(PlayState*, CollisionCheckContext*, Collider* col) { registeredCollider = col; ++hits; return 1; }
void* OotAssets_LoadGfx(const char* path) {
    assert(std::strstr(path, "objects/object_bv/gBarinadeDL_") != nullptr);
    static Gfx dl[1]{}; return modelPresent ? dl : nullptr;
}

#include "modes.inc"

void ResetWorld(Player& player, PlayState& play) {
    for (Actor* actor : actors) std::free(actor);
    actors.clear(); player = {}; play = {}; gSaveContext = {};
    gPlayState = &play; play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.heldItemAction = (PlayerItemAction)PLAYER_IA_ELEMENTAL_WAND;
    player.itemAction = player.heldItemAction; player.heldItemId = ITEM_ELEMENTAL_WAND;
    player.actor.id = ACTOR_PLAYER;
    player.transformation = PLAYER_FORM_HUMAN;
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    gSaveContext.magicCapacity = 48; gSaveContext.save.saveInfo.playerData.magic = 48;
    Nei_Save()->ootQuestItems = 0x3f; Nei_Save()->wandRodsOwned = 0x3f;
    std::memset(gSaveContext.save.saveInfo.equips.buttonItems, ITEM_NONE, sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
    std::memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems, ITEM_NONE, sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems));
    play.objectCtx.numEntries = 1; play.objectCtx.slots[0].id = OBJECT_D_LIFT;
    slabCollision.minBounds = {-100, -60, -50}; slabCollision.maxBounds = {100, 20, 50};
    WandSand_Forget(); WandWater_Forget(); WandShadow_Forget(); WandStorm_Forget();
    sWindOn = 0; sWindDrainTimer = 0; sWindPrevInvinc = 0;
    sSandMeasured = 0; sSandTopOffset = sSandReach = 0;
    sMeteorBombUpdate = nullptr; sMeteorBombDraw = nullptr;
    Player_InitElementalWandIA(&play, &player);
    sStormRayColBuilt = 0; sStormRayCol = {};
    spawnFails = onSlab = capeOwned = false; requestedObjects = 0; menuOpen = false;
    ownershipRule = WAND_RANDO_MEDALLIONS; registeredCollider = nullptr;
}

void CheckSixModeDispatchAndMagic() {
    Player player{}; PlayState play{};
    const int costs[] = {2, 0, 2, 3, 6, 3};
    for (u8 mode = 0; mode < 6; ++mode) {
        ResetWorld(player, play);
        assert(Wand_Cast(&player, &play, mode));
        assert(gSaveContext.save.saveInfo.playerData.magic == 48 - costs[mode]);
        assert(mode == 0 ? sSandSlabs[0] != nullptr : mode == 1 ? sWindOn : mode == 2 ? sWaterGeyser != nullptr :
            mode == 3 ? actors.back()->id == ACTOR_EN_BOM : mode == 4 ? actors.back()->id == ACTOR_EN_OKARINA_EFFECT : sShadowBolt.active);
        ResetWorld(player, play); gSaveContext.magicCapacity = 0;
        assert(!Wand_Cast(&player, &play, mode) && actors.empty());
        ResetWorld(player, play); gSaveContext.save.saveInfo.playerData.magic = 0;
        assert(!Wand_Cast(&player, &play, mode) && actors.empty());
        ResetWorld(player, play); capeOwned = true;
        assert(Wand_Cast(&player, &play, mode));
        assert(gSaveContext.save.saveInfo.playerData.magic == 48 - costs[mode] / 2);
    }
    ResetWorld(player, play);
    assert(!Wand_Cast(&player, &play, 6) && actors.empty());
    for (u8 mode : {0, 2, 3, 4}) {
        ResetWorld(player, play); spawnFails = true;
        assert(!Wand_Cast(&player, &play, mode));
        assert(gSaveContext.save.saveInfo.playerData.magic == 48);
    }
    ResetWorld(player, play); play.envCtx.precipitation[PRECIP_RAIN_CUR] = 1;
    assert(!Wand_Cast(&player, &play, 4) && gSaveContext.save.saveInfo.playerData.magic == 48);
}

void CheckSandWaterAndMeteor() {
    Player player{}; PlayState play{};
    for (u8 mode : {0, 2}) {
        ResetWorld(player, play); play.objectCtx.numEntries = 0;
        assert(!Wand_Cast(&player, &play, mode));
        assert(requestedObjects == 1 && actors.empty() && gSaveContext.save.saveInfo.playerData.magic == 48);
        Flags_SetSwitch(&play, 0); assert(Wand_Cast(&player, &play, mode));
        assert(Flags_GetSwitch(&play, 0));
    }
    ResetWorld(player, play); assert(Wand_Cast(&player, &play, 0));
    Actor* slab = sSandSlabs[0];
    assert(std::fabs(slab->world.pos.y + 20) < 0.01f && slab->scale.x == 1 && slab->room == -1);
    assert(!WandSand_HoldElapsed(&player, 1));
    onSlab = true; for (int frame = 0; frame < 23; ++frame) slab->update(slab, &play);
    assert(slab->update != nullptr && slab->scale.x == 1);
    slab->update(slab, &play); assert(slab->update == nullptr && sSandSlabs[0] == nullptr);

    ResetWorld(player, play); assert(Wand_Cast(&player, &play, 2));
    Actor* water = sWaterGeyser; water->update(water, &play);
    assert(water->world.pos.y > 0 && water->world.pos.y <= 10);
    assert(Wand_Cast(&player, &play, 2) && sWaterGeyser == water);
    for (int frame = 0; frame < 90; ++frame) water->update(water, &play);
    assert(water->world.pos.y > 239 && water->world.pos.y <= 240);
    assert(Wand_Cast(&player, &play, 2));
    for (int frame = 0; frame < 90; ++frame) water->update(water, &play);
    assert(water->world.pos.y >= 10 && water->world.pos.y < 11);

    ResetWorld(player, play); assert(Wand_Cast(&player, &play, 3));
    Actor* meteor = actors.back(); assert(meteor->speed > 0 && meteor->scale.x > 0);
    meteor->floorHeight = 0;
    for (int frame = 0; frame < 8; ++frame) meteor->update(meteor, &play);
    assert(meteor->world.pos.z > 70 && meteor->world.pos.y > 40 && meteor->velocity.y == 0);
    meteor->bgCheckFlags |= BGCHECKFLAG_WALL; meteor->update(meteor, &play);
    assert(meteor->params == BOMB_TYPE_BODY);
    METEOR_HOP_TIME(meteor) = 64; meteor->bgCheckFlags |= BGCHECKFLAG_WALL;
    meteor->update(meteor, &play); assert(meteor->params == BOMB_TYPE_EXPLOSION);
}

void CheckWindStormAndShadow() {
    Player player{}; PlayState play{};
    ResetWorld(player, play); assert(Wand_Cast(&player, &play, 1));
    player.actor.velocity.y = 8; WandWind_Boost(&player); assert(player.actor.velocity.y > 12);
    player.actor.velocity.y = -8; WandWind_TickHover(&player, 1); assert(player.actor.velocity.y == 4);
    for (int frame = 0; frame < 20; ++frame) WandWind_Tick(&play, &player);
    assert(gSaveContext.save.saveInfo.playerData.magic == 47 && sWindOn);
    assert(Wand_Cast(&player, &play, 1) && !sWindOn);
    assert(Wand_Cast(&player, &play, 1)); player.invincibilityTimer = 1;
    WandWind_Tick(&play, &player); assert(!sWindOn);

    ResetWorld(player, play); Actor enemy{}; enemy.update = LiveActor; enemy.focus.pos = {0, 30, 200};
    player.focusActor = &enemy; player.stateFlags3 |= PLAYER_STATE3_HOSTILE_LOCK_ON;
    assert(Wand_Cast(&player, &play, 4) && actors.empty() && sStormRay.active);
    assert(!Wand_Cast(&player, &play, 4) && gSaveContext.save.saveInfo.playerData.magic == 42);
    WandStorm_Tick(&play, &player);
    assert(sStormRay.pos.z > 17 && registeredCollider && sStormRayCol.elem.atDmgInfo.damage == 2);
    modelPresent = false; sStormBeamTried = 0;
    assert(!WandStorm_LoadBeam() && sStormRay.active);
    sStormRayCol.base.atFlags |= AT_HIT; WandStorm_Tick(&play, &player); assert(!sStormRay.active);

    ResetWorld(player, play); assert(Wand_Cast(&player, &play, 5));
    assert(!Wand_Cast(&player, &play, 5) && gSaveContext.save.saveInfo.playerData.magic == 45);
    for (int frame = 0; frame < 90; ++frame) WandShadow_Tick(&play);
    assert(!sShadowBolt.active);
}

void CheckOwnedModesAndInput() {
    Player player{}; PlayState play{};
    for (u16 button : {BTN_CLEFT, BTN_DUP}) {
        for (u8 mode = 0; mode < 6; ++mode) {
            ResetWorld(player, play); Nei_Save()->wandMode = mode;
            if (button == BTN_CLEFT) BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_ELEMENTAL_WAND;
            else DPAD_BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_D_UP) = ITEM_ELEMENTAL_WAND;
            player.heldItemAction = PLAYER_IA_NONE;
            play.gameplayFrames = 1; Wand_TickInput(&play, &player);
            player.heldItemAction = (PlayerItemAction)PLAYER_IA_ELEMENTAL_WAND;
            play.state.input[0].press.button = play.state.input[0].cur.button = button;
            ++play.gameplayFrames; Wand_TickInput(&play, &player);
            assert(actors.empty() && !sWindOn && !sShadowBolt.active);
            ++play.gameplayFrames; Wand_TickInput(&play, &player);
            assert(mode == 0 ? sSandSlabs[0] != nullptr : mode == 1 ? sWindOn : mode == 2 ? sWaterGeyser != nullptr :
                mode == 3 ? actors.back()->id == ACTOR_EN_BOM : mode == 4 ? actors.back()->id == ACTOR_EN_OKARINA_EFFECT : sShadowBolt.active);
            const int costs[] = {2, 0, 2, 3, 6, 3};
            assert(gSaveContext.save.saveInfo.playerData.magic == 48 - costs[mode]);
        }
    }
    ResetWorld(player, play); ownershipRule = WAND_RANDO_ELEMENTAL;
    Nei_Save()->wandRodsOwned = (1 << WAND_MODE_WATER) | (1 << WAND_MODE_SCEPTER);
    Nei_Save()->wandMode = WAND_MODE_METEOR;
    assert(Wand_ModeCount() == 2 && Wand_GetMode() == WAND_MODE_WATER);
    Wand_SetMode(WAND_MODE_SCEPTER); assert(Wand_GetMode() == WAND_MODE_SCEPTER);
    for (int frame = 0; frame < 8; ++frame) {
        ++play.gameplayFrames; play.state.input[0].cur.button = BTN_L; Wand_TickInput(&play, &player);
    }
    assert(wheelCount == 2 && wheelIndex == 1);
    confirmWheel(0); assert(Wand_GetMode() == WAND_MODE_WATER);

    ResetWorld(player, play); Nei_Save()->wandMode = WAND_MODE_METEOR;
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_ELEMENTAL_WAND;
    play.gameplayFrames = 1; Wand_TickInput(&play, &player);
    play.state.input[0].press.button = BTN_CLEFT;
    for (u32 flags : {PLAYER_STATE1_DAMAGED, PLAYER_STATE1_SHIELDING}) {
        player.stateFlags1 = flags; ++play.gameplayFrames; Wand_TickInput(&play, &player);
        assert(actors.empty() && gSaveContext.save.saveInfo.playerData.magic == 48);
    }
    player.stateFlags1 = 0; player.stateFlags3 = PLAYER_STATE3_START_CHANGING_HELD_ITEM;
    ++play.gameplayFrames; Wand_TickInput(&play, &player); assert(actors.empty());
    player.stateFlags3 = 0; menuOpen = true;
    ++play.gameplayFrames; Wand_TickInput(&play, &player); assert(actors.empty());
    menuOpen = false; ++play.gameplayFrames; Wand_TickInput(&play, &player);
    assert(actors.back()->id == ACTOR_EN_BOM);
    ++play.sceneId; ++play.gameplayFrames; play.state.input[0].press.button = 0;
    sShadowBolt.active = sStormRay.active = 1; Wand_TickInput(&play, &player);
    assert(!sShadowBolt.active && !sStormRay.active);
}

void CheckHeldSandBlocked() {
    Player player{}; PlayState play{};
    ResetWorld(player, play); Nei_Save()->wandMode = WAND_MODE_SAND;
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_ELEMENTAL_WAND;
    ++play.gameplayFrames; Wand_TickInput(&play, &player);
    player.stateFlags1 = PLAYER_STATE1_SHIELDING;
    play.state.input[0].cur.button = BTN_CLEFT;
    ++play.gameplayFrames; Wand_TickInput(&play, &player);
    assert(actors.empty() && gSaveContext.save.saveInfo.playerData.magic == 48 &&
        "held Sand must use the same blocker gate as a pressed cast");
    player.stateFlags1 = 0; play.state.input[0].press.button = BTN_CLEFT;
    ++play.gameplayFrames; Wand_TickInput(&play, &player);
    assert(actors.size() == 1 && gSaveContext.save.saveInfo.playerData.magic == 46);
    player.actor.world.pos.z = 120; play.state.input[0].press.button = 0;
    ++play.gameplayFrames; Wand_TickInput(&play, &player);
    assert(actors.size() == 2 && gSaveContext.save.saveInfo.playerData.magic == 44);
    for (int frame = 0; frame < 12; ++frame) { ++play.gameplayFrames; Wand_TickInput(&play, &player); }
    assert(actors.size() == 2 && gSaveContext.save.saveInfo.playerData.magic == 44);
}

void CheckNativeCDDispatch() {
    Player player{}; PlayState play{};
    for (u16 button : {BTN_CLEFT, BTN_DUP}) {
        for (u8 mode = 0; mode < 6; ++mode) {
            ResetWorld(player, play); Nei_Save()->wandMode = mode;
            gSaveContext.save.saveInfo.playerData.health = 0x30;
            std::memset(gSaveContext.buttonStatus, BTN_ENABLED, sizeof(gSaveContext.buttonStatus));
            std::memset(gSaveContext.shipSaveContext.dpad.status, BTN_ENABLED, sizeof(gSaveContext.shipSaveContext.dpad.status));
            if (button == BTN_CLEFT) BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_ELEMENTAL_WAND;
            else DPAD_BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_D_UP) = ITEM_ELEMENTAL_WAND;
            CompleteWandFrame(&player, &play, 0, 0);
            CompleteWandFrame(&player, &play, button, button);
            assert(player.heldItemAction == PLAYER_IA_ELEMENTAL_WAND && player.heldItemId == ITEM_ELEMENTAL_WAND);
            assert(mode == 0 ? sSandSlabs[0] != nullptr : mode == 1 ? sWindOn : mode == 2 ? sWaterGeyser != nullptr :
                mode == 3 ? actors.back()->id == ACTOR_EN_BOM : mode == 4 ? actors.back()->id == ACTOR_EN_OKARINA_EFFECT : sShadowBolt.active);
        }
    }
}

int main() {
    CheckSixModeDispatchAndMagic();
    CheckSandWaterAndMeteor();
    CheckWindStormAndShadow();
    CheckHeldSandBlocked();
    CheckOwnedModesAndInput();
    CheckNativeCDDispatch();
    for (Actor* actor : actors) std::free(actor);
    std::cout << "PASS six actual MM wand modes, C/D input, ownership, wheel, magic, spawns, gameplay updates and scene cleanup\n";
}
