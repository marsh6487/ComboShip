#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <set>
#include <array>
#include <vector>
#include "2s2h/Rando/NeiArticulatedPresentation.h"
#include "2s2h/Rando/NeiLanternPresentation.h"
extern "C" {
#include "global.h"
#include "din_fire_sword.h"
#include "din_fire_shield.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/weapon_upgrades.h"
#include "mods/extended_equipment.h"
#include "mods/nei_oot_compat.h"
}
#include "combo/NeiHeldSword.h"

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ++failures; } } while (0)
#define ALB(sym) "__OTR__objects/object_link_boy/" sym
#define ADULT_LINK_LIMB_COUNT 21
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
static const char* sLimbNearDL[21]{};
static u8 sReady, sSetupFailedPermanently;
static std::string sModEyePath[PLAYER_EYES_MAX], sModMouthPath[PLAYER_MOUTH_MAX];
static void* sModEye[PLAYER_EYES_MAX];
static void* sModMouth[PLAYER_MOUTH_MAX];
static const char* sOotEyeName[PLAYER_EYES_MAX];
static const char* sOotMouthName[PLAYER_MOUTH_MAX];
static const char* sOotChildEyeName[PLAYER_EYES_MAX];
static const char* sOotChildMouthName[PLAYER_MOUTH_MAX];
struct RitoResource { const char* path; std::initializer_list<const char*> calls; };
struct RitoLimb { Vec3s position; u8 child, sibling; const char* display; };
/* RITO_INVENTORY */
static constexpr const char* kRitoBase = "objects/forms/rito/object_link_boy/";
static std::map<std::string,std::vector<std::string>> ritoCalls;
static int failures = 0, cases = 0;
static u8 sIsMod, sIsForm, sIsChildRig;
static std::string sRigBase;
static FlexSkeletonHeader model;
static FlexSkeletonHeader* sSkel = &model;
static std::map<std::string,Gfx> meshes;
static Gfx original{}, nativeSword{}, selectedSword{}, nativeFP{}, prop{};
static Gfx *lastLeft, *lastRight, *lastHead, *lastTorso;
static Gfx *lastLeftForearm, *lastRightForearm, *lastRightShoulder;
static bool alt = true, bodyMod, hideSword, four, odolwa, goht, mirror;
static int form = CUSTOM_FORM_NONE;
static u8 tier, masterTier, fairyTier;
static bool vanilla;
static std::set<std::string> selected, unavailable;
static Gfx* skeletonHands[PLAYER_LIMB_MAX];
static Vec3f projectedFocus;
static Gfx* sPlayerFirstPersonLeftForearmDLs[PLAYER_FORM_MAX];
static Gfx* sPlayerFirstPersonLeftHandDLs[PLAYER_FORM_MAX];
static Gfx* sPlayerFirstPersonRightShoulderDLs[PLAYER_FORM_MAX];
static Gfx* sPlayerFirstPersonRightHandDLs[PLAYER_FORM_MAX];
static Gfx* sPlayerFirstPersonRightHandHookshotDLs[PLAYER_FORM_MAX];
static std::array<Gfx,128> frameAllocation;
static Gfx* Allocate(size_t n) { if (n > sizeof(frameAllocation)) std::abort(); return frameAllocation.data(); }
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(context, size) Allocate(size)
static Gfx* Mesh(const char* path) { return &meshes[path]; }
extern "C" {
SaveContext gSaveContext{};
void gSPDisplayList(Gfx* command,Gfx* display) { __gSPDisplayList(command,display); }
u16 gEquipMasks[] = {0xf, 0xf0, 0xf00, 0xf000};
u8 gEquipShifts[] = {0, 4, 8, 12};
PlayerModelType sPlayerLeftHandType, sPlayerRightHandType;
bool ResourceMgr_IsAltAssetsEnabled() { return alt; }
u8 ResourceMgr_FileExists(const char* path) { return meshes.contains(path); }
u8 ResourceMgr_FileAltExists(const char*) { return 0; }
Gfx* ResourceMgr_LoadGfxByName(const char* path) { return meshes.contains(path) ? Mesh(path) : nullptr; }
int ResourceMgr_IsModAsset(const char* path) { return selected.contains(path); }
int ResourceMgr_IsModAssetForGame(const char*,const char* path) { return selected.contains(path); }
int NeiResource_Available(const char* path) { return path && !unavailable.contains(path); }
int NeiResource_IsMod(const char* path) { return selected.contains(path); }
int NeiResource_EnsureGiBaseOwner() { return 1; }
const char* NeiResource_Route(const char* path) { return path; }
s32 CVarGetInteger(const char* path,s32 value) { return std::strstr(path,"UseVanillaHeldSwords") ? vanilla : value; }
u8 Player_IsCustomLinkModel(Player*) { return bodyMod; }
u8 FourSword_IsEquipped() { return four; }
u8 FourSword_HeldSwordDLForFrame(void**, void**,int) { return 0; }
u8 WeaponUpgrade_KokiriLevel() { return tier; }
u8 WeaponUpgrade_HasGilded() { return tier == 2; }
u8 WeaponUpgrade_HasTrueMaster() { return masterTier; }
u8 WeaponUpgrade_HasGreatFairy() { return fairyTier; }
void* MmAssets_LoadResource(const char*) { return &prop; }
s32 AdultLink_UsesAdultPresentation(const Player*) { return !sIsChildRig; }
void DinFireSword_SetHandModel(Player*,void*,int) {}
void DinFireSword_CopyHandModel(Player*,void*,void*) {}
void* DinFireSword_HandDL(PlayState*,Player*,void*) { return nullptr; }
void* DinFireShield_HandDL(PlayState*,Player*,void*) { return nullptr; }
u8 ExtEquip_ShouldHideSwordDL() { return hideSword; }
const char* ExtEquip_GetShieldDLOverride() { return nullptr; }
u8 Trident_GoldenArmor() { return 0; }
s32 BossRemains_IsOdolwaWorn() { return odolwa; }
s32 BossRemains_IsGohtWorn() { return goht; }
s32 CustomForms_ActiveForm() { return form; }
const char* CustomForms_BasePath() { return form == CUSTOM_FORM_RITO ? kRitoBase : nullptr; }
s32 AdultLink_IsActive() { return 1; }
SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path,SkelAnime*) {
    return form == CUSTOM_FORM_RITO && std::string(path) == std::string(kRitoBase) + "gLinkAdultSkel" ? &model.sh : nullptr;
}
void* MmAssets_LoadFromOotArchive(const char*,size_t*) { return nullptr; }
void* OotAssets_LoadGfxDirect(const char*) { return &nativeFP; }
char* ResourceMgr_LoadTexOrDListByName(const char*) { return nullptr; }
f32 CustomForms_RootScale() { return 1.f; }
f32 CustomForms_RootDropBefore() { return 0.f; }
f32 CustomForms_RootDropAfter() { return 0.f; }
Gfx* CustomForms_HandDL(Player*,s32,u8* claimed) { *claimed = 0; return nullptr; }
u8 CustomForms_HidesSheath(Player*) { return 0; }
void Player_ApplyBackEquipmentVisibility(s32,Gfx**) {}
void Player_ApplyNeiHeldHand(PlayState*,Player*,s32,Gfx**) {}
void SkinMatrix_Vec3fMtxFMultXYZ(MtxF*,Vec3f*,Vec3f* out) { *out = projectedFocus; }
bool Player_IsHoldingHookshot(Player* p) { return p->heldItemAction == PLAYER_IA_HOOKSHOT; }
s32 Player_OverrideLimbDrawGameplayCommon(PlayState*,s32 index,Gfx**,Vec3f*,Vec3s*,Actor* actor) {
    auto p = reinterpret_cast<Player*>(actor);
    if (index == PLAYER_LIMB_ROOT) {
        sPlayerLeftHandType = static_cast<PlayerModelType>(p->leftHandType);
        sPlayerRightHandType = static_cast<PlayerModelType>(p->rightHandType);
    }
    return 0;
}
s32 Player_OverrideLimbDrawGameplayDefault(PlayState* play,s32 index,Gfx** dl,Vec3f* pos,Vec3s* rot,Actor* actor) {
    Player_OverrideLimbDrawGameplayCommon(play,index,dl,pos,rot,actor);
    auto p = reinterpret_cast<Player*>(actor);
    // The native equipment seam is a resource boundary; state/rig remapping
    // under test is the actual production adult override below.
    if (index == PLAYER_LIMB_LEFT_HAND) {
        *dl = p->leftHandType == PLAYER_MODELTYPE_LH_ONE_HAND_SWORD || p->leftHandType == PLAYER_MODELTYPE_LH_TWO_HAND_SWORD
                  ? &nativeSword : &original;
        if (hideSword) sPlayerLeftHandType = PLAYER_MODELTYPE_LH_OPEN;
    } else if (index == PLAYER_LIMB_RIGHT_HAND) *dl = &original;
    return 0;
}
void Player_DrawImpl(PlayState* play,void**,Vec3s*,s32,s32,PlayerTransformation,s32,s32,
                     OverrideLimbDrawFlex override,PostLimbDrawFlex,Actor* actor) {
    for (int index = PLAYER_LIMB_ROOT; index < PLAYER_LIMB_MAX; ++index) {
        Gfx* dl = skeletonHands[index]; Vec3f pos{}; Vec3s rot{};
        override(play,index,&dl,&pos,&rot,actor);
        if(index == PLAYER_LIMB_HEAD) lastHead = dl;
        if(index == PLAYER_LIMB_TORSO) lastTorso = dl;
        if(index == PLAYER_LIMB_LEFT_HAND) lastLeft = dl;
        if(index == PLAYER_LIMB_RIGHT_HAND) lastRight = dl;
        if(index == PLAYER_LIMB_LEFT_FOREARM) lastLeftForearm = dl;
        if(index == PLAYER_LIMB_RIGHT_FOREARM) lastRightForearm = dl;
        if(index == PLAYER_LIMB_RIGHT_SHOULDER) lastRightShoulder = dl;
    }
}
}
bool NeiArticulated_ApplySwitchHookHand(PlayState*,Player*,Gfx**,Gfx*) { return false; }
bool NeiLantern_UsesGrip(const Player*) { return false; }
static void AdultLink_PostLimb(PlayState*,s32,Gfx**,Gfx**,Vec3s*,Actor*) {}
/* NATIVE_FIRST_PERSON */
/* PRODUCTION */

static PlayState play;
static Player player;
static void Reset() {
    player = {}; player.transformation = PLAYER_FORM_HUMAN; player.actor.scale.y = .01f;
    player.leftHandType = PLAYER_MODELTYPE_LH_OPEN; player.rightHandType = PLAYER_MODELTYPE_RH_OPEN;
    player.unk_AA5 = PLAYER_UNKAA5_3;
    sIsMod = 1; sIsForm = sIsChildRig = 0; sRigBase.clear(); form = CUSTOM_FORM_NONE;
    bodyMod = hideSword = four = odolwa = goht = mirror = vanilla = false;
    tier = masterTier = fairyTier = 0; selected.clear(); unavailable.clear();
    for(auto& name : sOotEyeName) name = "unused-eye";
    for(auto& name : sOotChildEyeName) name = "unused-eye";
    for(auto& name : sOotMouthName) name = "unused-mouth";
    for(auto& name : sOotChildMouthName) name = "unused-mouth";
    projectedFocus = {0,0,-5};
    for(auto& hand : skeletonHands) hand = &original;
    for(int i=0; i<PLAYER_FORM_MAX; ++i) {
        sPlayerFirstPersonLeftForearmDLs[i] = sPlayerFirstPersonLeftHandDLs[i] = &nativeFP;
        sPlayerFirstPersonRightShoulderDLs[i] = sPlayerFirstPersonRightHandDLs[i] = sPlayerFirstPersonRightHandHookshotDLs[i] = &nativeFP;
    }
    sDL_LHOpen = Mesh("left-open"); sDL_LHClosed = Mesh("left-closed");
    sDL_RHOpen = Mesh("right-open"); sDL_RHClosed = Mesh("right-closed");
    sDL_LHSword = Mesh("master-composite"); sDL_LHBgs = Mesh("bgs-composite");
    sDL_RHBow = Mesh("bow-composite"); sDL_RHHookshot = Mesh("hookshot-composite");
#ifdef MODEL_FIRST_PERSON
    sDL_RHSlingshot = Mesh("slingshot-composite");
    sDL_FPLeftForearm = Mesh("left-forearm-fp"); sDL_FPLeftHand = Mesh("left-hand-fp");
    sDL_FPRightForearm = Mesh("right-forearm-fp");
    sDL_FPBow = Mesh("bow-fp"); sDL_FPHookshot = Mesh("hookshot-fp"); sDL_FPSlingshot = Mesh("slingshot-fp");
#endif
}
static void Draw() { ActualDraw(&play,&player); ++cases; }
static std::string ResourceName(Gfx* pointer) {
    for(auto& [path,mesh] : meshes) if(&mesh == pointer) return path;
    return "native-or-missing-boundary";
}
static void ResourceLeaves(const std::string& path,std::multiset<std::string>& leaves,int depth=0) {
    if(depth > 16) std::abort();
    auto found = ritoCalls.find(path);
    if(found == ritoCalls.end() || found->second.empty()) { leaves.insert(path); return; }
    for(auto& child : found->second) ResourceLeaves(child,leaves,depth+1);
}
static std::multiset<std::string> Leaves(Gfx* pointer) {
    std::multiset<std::string> leaves; ResourceLeaves(ResourceName(pointer),leaves); return leaves;
}
static void ActualRitoSetup() {
    Reset(); meshes.clear(); ritoCalls.clear();
    for(auto& resource : kRitoResources) {
        Mesh(resource.path);
        for(auto call : resource.calls) ritoCalls[resource.path].push_back(call);
    }
    static LodLimb limbs[21]; static void* limbPointers[21];
    for(int index=0; index<21; ++index) {
        auto& source = kRitoLimbs[index];
        Gfx* dl = *source.display ? Mesh(source.display) : nullptr;
        limbs[index] = {source.position,source.child,source.sibling,{dl,dl}};
        // The native walker passes source skeleton index + 1 to callbacks.
        limbPointers[index] = &limbs[index]; skeletonHands[index+1] = dl;
    }
    model = {{limbPointers,21},kRitoMatrixCount};
    form = CUSTOM_FORM_RITO; sReady = sSetupFailedPermanently = 0; sSkel = nullptr;
    CHECK(AdultLink_Setup()); CHECK(sIsForm && !sIsMod && !sIsChildRig && sSkel == &model);
    const std::string base = kRitoBase;
    for(int action : {PLAYER_IA_BOW,PLAYER_IA_HOOKSHOT}) {
        player.heldItemAction = static_cast<PlayerItemAction>(action);
        player.heldItemId = action == PLAYER_IA_BOW ? ITEM_BOW : ITEM_HOOKSHOT;
        player.rightHandType = action == PLAYER_IA_BOW ? PLAYER_MODELTYPE_RH_BOW : PLAYER_MODELTYPE_RH_HOOKSHOT;
        player.stateFlags1 |= PLAYER_STATE1_100000;
        Draw(); CHECK(!lastHead && !lastTorso);
        CHECK(Leaves(lastLeft) == std::multiset<std::string>{base+"gLinkAdultSkelLimb_015_DL"});
        const std::string propPath = action == PLAYER_IA_BOW
            ? "objects/object_link_child/gLinkHumanBowDL" : "objects/object_link_child/gLinkHumanHookshotDL";
        CHECK((Leaves(lastRight) == std::multiset<std::string>{base+"gLinkAdultSkelLimb_018_DL",propPath}));
        CHECK(Leaves(lastLeftForearm) == std::multiset<std::string>{base+"gLinkAdultSkelLimb_014_DL"});
        CHECK(Leaves(lastRightForearm) == std::multiset<std::string>{base+"gLinkAdultSkelLimb_017_DL"});
        CHECK(Leaves(lastRightShoulder) == std::multiset<std::string>{base+"gLinkAdultSkelLimb_016_DL"});
    }
}
static void Sword(int item,int action,int model,int kokiriTier=0,int trueMaster=0,int greatFairy=0) {
    Reset(); player.leftHandType = action == PLAYER_IA_SWORD_BIGGORON || action == PLAYER_IA_SWORD_TWO_HANDED
                          ? PLAYER_MODELTYPE_LH_TWO_HAND_SWORD : PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    player.heldItemId = item; player.heldItemAction = static_cast<PlayerItemAction>(action);
    tier = kokiriTier; masterTier = trueMaster; fairyTier = greatFairy;
    bodyMod = true;
    // A generic body-pack Master alias must not claim every equipped sword.
    if(model != NEI_HELD_SWORD_MASTER) selected.insert(ALB("gLinkAdultLeftHandHoldingMasterSwordNearDL"));
    if(model != NEI_HELD_SWORD_BIGGORON) selected.insert(ALB("gLinkAdultLeftHandHoldingBgsNearDL"));
    const Player before = player;
    Draw();
    CHECK(std::memcmp(&before,&player,sizeof(player)) == 0);
    CHECK(lastLeft != sDL_LHSword && lastLeft != sDL_LHBgs && lastLeft != &nativeSword);
    if (lastLeft && (lastLeft[0].words.w0 >> 24) == G_DL) {
        auto* blade = reinterpret_cast<Gfx*>(lastLeft[0].words.w1);
        CHECK(blade && (blade[0].words.w0 >> 24) == G_DL_OTR_FILEPATH);
        if (blade) CHECK(std::string(reinterpret_cast<char*>(blade[0].words.w1)) == sNeiHeldSwordPaths[model][NEI_HELD_SWORD_OOT_ADULT][0]);
        CHECK(lastLeft[1].words.w1 == reinterpret_cast<uintptr_t>(sDL_LHClosed));
    } else CHECK(false);
}
int main() {
    Reset(); Draw(); CHECK(lastLeft == &original && lastRight == &original);
    Reset(); player.leftHandType = PLAYER_MODELTYPE_LH_CLOSED; player.rightHandType = PLAYER_MODELTYPE_RH_CLOSED;
    player.heldItemAction = PLAYER_IA_DEKU_STICK; Draw(); CHECK(lastLeft == sDL_LHClosed && lastRight == sDL_RHClosed);
    Reset(); player.leftHandType = PLAYER_MODELTYPE_LH_TWO_HAND_SWORD; player.rightHandType = PLAYER_MODELTYPE_RH_CLOSED;
    Draw(); CHECK(lastRight == sDL_RHClosed);
    Sword(ITEM_SWORD_KOKIRI,PLAYER_IA_SWORD_KOKIRI,NEI_HELD_SWORD_MM_KOKIRI);
    Sword(ITEM_SWORD_RAZOR,PLAYER_IA_SWORD_RAZOR,NEI_HELD_SWORD_RAZOR);
    Sword(ITEM_SWORD_GILDED,PLAYER_IA_SWORD_GILDED,NEI_HELD_SWORD_GILDED);
    Sword(ITEM_SWORD_KOKIRI,PLAYER_IA_SWORD_KOKIRI,NEI_HELD_SWORD_RAZOR,1);
    Sword(ITEM_SWORD_KOKIRI,PLAYER_IA_SWORD_KOKIRI,NEI_HELD_SWORD_GILDED,2);
    Sword(ITEM_SWORD_MASTER,PLAYER_IA_SWORD_MASTER,NEI_HELD_SWORD_MASTER);
    Sword(ITEM_SWORD_MASTER,PLAYER_IA_SWORD_MASTER,NEI_HELD_SWORD_TRUE_MASTER,0,1);
    Sword(ITEM_SWORD_BGS,PLAYER_IA_SWORD_BIGGORON,NEI_HELD_SWORD_BIGGORON);
    Sword(ITEM_SWORD_BGS,PLAYER_IA_SWORD_BIGGORON,NEI_HELD_SWORD_GREAT_FAIRY,0,0,1);
    Sword(ITEM_SWORD_GREAT_FAIRY,PLAYER_IA_SWORD_TWO_HANDED,NEI_HELD_SWORD_GREAT_FAIRY);
    for(int action : std::array<int,3>{PLAYER_IA_BOW,PLAYER_IA_HOOKSHOT,PLAYER_IA_SLINGSHOT}) {
        Reset(); player.stateFlags1 |= PLAYER_STATE1_100000; player.heldItemAction = static_cast<PlayerItemAction>(action);
        Draw(); CHECK(lastHead == nullptr && lastTorso == nullptr);
        CHECK(lastLeft && lastRight); // Actual equipment must remain visible while aiming.
#ifdef MODEL_FIRST_PERSON
        CHECK(lastLeft == sDL_FPLeftHand);
        CHECK(lastRight == (action == PLAYER_IA_HOOKSHOT ? sDL_FPHookshot : action == PLAYER_IA_SLINGSHOT ? sDL_FPSlingshot : sDL_FPBow));
#endif
        for(int state=PLAYER_UNKAA5_0; state<=PLAYER_UNKAA5_5; ++state) {
            if(state == PLAYER_UNKAA5_3) continue;
            player.unk_AA5 = state; Draw(); CHECK(!lastLeft && !lastRight && !lastHead && !lastTorso);
        }
        player.unk_AA5 = PLAYER_UNKAA5_3; projectedFocus.z = -4.f; Draw(); CHECK(lastHead && lastTorso);
        projectedFocus.z = -5.f; player.stateFlags1 &= ~PLAYER_STATE1_100000; Draw(); CHECK(lastHead && lastTorso);
    }
#ifdef MODEL_FIRST_PERSON
    // Model-owned optional resources resolve by the active source, and never
    // borrow a base/body part when the corresponding mod/form alias is absent.
    Reset(); const char* symbol = ALB("gLinkAdultLeftHandClosedNearDL");
    meshes.erase("alt/objects/object_link_boy/gLinkAdultLeftHandClosedNearDL");
    CHECK(AdultLink_LoadModelDL(symbol) == nullptr);
    Gfx* modFist = Mesh("alt/objects/object_link_boy/gLinkAdultLeftHandClosedNearDL");
    CHECK(AdultLink_LoadModelDL(symbol) == modFist);
    alt = false; CHECK(AdultLink_LoadModelDL(symbol) == nullptr);
    sIsMod = false; CHECK(AdultLink_LoadModelDL(symbol) == &nativeFP);
    sIsForm = true; sRigBase = "objects/forms/test/object_link_boy/";
    CHECK(AdultLink_LoadModelDL(symbol) == nullptr);
    Gfx* formFist = Mesh("objects/forms/test/object_link_boy/gLinkAdultLeftHandClosedNearDL");
    CHECK(AdultLink_LoadModelDL(symbol) == formFist);
    // Child rigs keep native FP parts; adult form arms use their own resources.
    Reset(); sIsChildRig = true; player.stateFlags1 |= PLAYER_STATE1_100000; Draw();
    CHECK(!lastHead && !lastTorso && lastLeft == &nativeFP && lastRight == &nativeFP);
    Reset(); sIsForm = true; form = CUSTOM_FORM_KEATON; player.stateFlags1 |= PLAYER_STATE1_100000; Draw();
    CHECK(!lastHead && !lastTorso && lastLeft == sDL_FPLeftHand && lastRight == sDL_FPBow);
    for(bool rito : {false,true}) for(int action : std::array<int,3>{PLAYER_IA_BOW,PLAYER_IA_HOOKSHOT,PLAYER_IA_SLINGSHOT}) {
        Reset(); sIsMod = false; sIsForm = rito; form = rito ? CUSTOM_FORM_RITO : CUSTOM_FORM_NONE;
        player.heldItemAction = static_cast<PlayerItemAction>(action); player.stateFlags1 |= PLAYER_STATE1_100000;
        Draw(); CHECK(!lastHead && !lastTorso && lastLeft && lastRight);
        player.unk_AA5 = PLAYER_UNKAA5_1; Draw(); CHECK(!lastHead && !lastTorso && !lastLeft && !lastRight);
        player.stateFlags1 &= ~PLAYER_STATE1_100000; Draw(); CHECK(lastHead && lastTorso);
    }
    // Missing slingshot alias retains the native slingshot result, never bow.
    Reset(); sDL_FPSlingshot = nullptr; player.heldItemAction = static_cast<PlayerItemAction>(PLAYER_IA_SLINGSHOT);
    player.stateFlags1 |= PLAYER_STATE1_100000; Draw(); CHECK(lastRight == &nativeFP);
    Reset(); player.heldItemAction = static_cast<PlayerItemAction>(PLAYER_IA_SLINGSHOT);
    player.rightHandType = PLAYER_MODELTYPE_RH_BOW; Draw(); CHECK(lastRight == sDL_RHSlingshot);
    sDL_RHSlingshot = nullptr; Draw(); CHECK(lastRight == &original);
    // No replacement sword is synthesized for masks, tools, child rigs, forms,
    // mirrored passes, explicitly selected equipment, or a vanilla fallback.
    for(int guard=0; guard<8; ++guard) {
        Reset(); player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
        player.heldItemId = ITEM_SWORD_KOKIRI; player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
        if(guard == 0) sIsChildRig = true;
        if(guard == 1) { sIsForm = true; form = CUSTOM_FORM_KEATON; }
        if(guard == 2) player.actor.scale.y = -.01f;
        if(guard == 3) selected.insert("__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingKokiriSwordDL");
        if(guard == 4) vanilla = true;
        if(guard == 5) hideSword = true;
        if(guard == 6) odolwa = true;
        if(guard == 7) goht = true;
        Draw();
        CHECK(lastLeft == (guard == 5 ? &original : guard >= 6 ? sDL_LHClosed : &nativeSword));
    }
    Reset(); player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    player.heldItemId = ITEM_NET; Draw(); CHECK(lastLeft == sDL_LHClosed);
    for(bool bgs : {false,true}) {
        Reset(); player.leftHandType = bgs ? PLAYER_MODELTYPE_LH_TWO_HAND_SWORD : PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
        player.heldItemId = bgs ? ITEM_SWORD_BGS : ITEM_SWORD_MASTER;
        player.heldItemAction = static_cast<PlayerItemAction>(bgs ? PLAYER_IA_SWORD_BIGGORON : PLAYER_IA_SWORD_MASTER);
        selected.insert(bgs ? ALB("gLinkAdultLeftHandHoldingBgsNearDL") : ALB("gLinkAdultLeftHandHoldingMasterSwordNearDL"));
        Draw(); CHECK(lastLeft == (bgs ? sDL_LHBgs : sDL_LHSword));
    }
    // The established native helper still rejects a custom MM body. Only the
    // new explicit-hand entry point is permitted to compose that model's fist.
    Reset(); bodyMod = true; player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    player.heldItemId = ITEM_SWORD_KOKIRI; player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
    Gfx* out = &nativeSword;
    CHECK(!WeaponUpgrade_ApplyHeldSwordDL(&out,sDL_LHClosed,&player,30,105,27) && out == &nativeSword);
    player.transformation = PLAYER_FORM_DEKU;
    CHECK(!WeaponUpgrade_ApplyHeldSwordDLForModel(&out,sDL_LHClosed,&player,30,105,27) && out == &nativeSword);
    CHECK(!WeaponUpgrade_ApplyHeldSwordDLForModel(&out,nullptr,&player,30,105,27));
    ActualRitoSetup();
#endif
    if(failures) { std::fprintf(stderr,"%d renderer regression failures (%d frames)\n",failures,cases); return 1; }
    std::printf("PASS %d native-struct adult renderer frames\n",cases);
}
