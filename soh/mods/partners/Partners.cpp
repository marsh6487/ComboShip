#include "../../../combo/encounters/NativeCustomEnemies.h"
#include "Partners.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "../../../combo/partners/PartnerPolicy.h"
#include "mods/nei_save.h"
#include "soh/ActorDB.h"
#include "soh/FleetShipCombo/FleetShipCombo.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/ResourceManagerHelpers.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" bool Partners_IsActive(void) {
    return gPlayState && gGameState == &gPlayState->state && gPlayState->state.running &&
           FleetShipCombo_IsThisGameActive();
}
extern "C" bool Partners_SongOwned(void) {
    return Partners_IsActive() && (Nei_Save()->mmQuestItems & FC_MMQ_SONG_TIME);
}
extern "C" bool Partners_CojiroOwned(void) {
    return Nei_Save()->cojiroPartnerOwned != 0;
}
extern "C" bool Partners_CojiroEnabled(void) {
    return CVarGetInteger("gEnhancements.Partners.Cojiro", 0) && Partners_CojiroOwned() &&
           !Nei_Save()->cojiroPartnerDismissed;
}
extern "C" void Partners_ToggleCojiro(void) {
    if (Partners_CojiroOwned())
        Nei_Save()->cojiroPartnerDismissed ^= 1;
}
extern "C" void Partners_UpdateCojiroUnlock(void) {
    if (!Partners_IsActive() || !CVarGetInteger("gEnhancements.Partners.Cojiro", 0))
        return;
    // Read the completed check, never give or replace its ordinary randomized reward.
    if (PartnerPolicy::CojiroEligible(Partners_CojiroOwned(),
                                      Flags_GetRandomizerInf(RAND_INF_ADULT_TRADES_LW_TRADE_COJIRO)))
        Nei_Save()->cojiroPartnerOwned = 1;
}
extern "C" bool Partners_CanControl(PlayState* play, Actor* actor, bool enabled) {
    if (!Partners_IsActive() || play != gPlayState || !actor)
        return false;
    Player* player = GET_PLAYER(play);
    if (!player)
        return false;
    PartnerPolicy::ControlConditions c;
    c.enabled = enabled;
    c.songOwned = Partners_SongOwned();
    c.activeWorld = true;
    for (const auto& list : play->actorCtx.actorLists)
        for (Actor* a = list.head; a; a = a->next)
            if (a == actor)
                c.partnerLive = a->update != nullptr;
    c.grounded = player->actor.bgCheckFlags & BGCHECKFLAG_GROUND;
    c.held = player->rideActor || player->heldActor || player->actor.parent;
    c.busy = play->pauseCtx.state || play->msgCtx.msgMode != MSGMODE_NONE || play->csCtx.state != CS_STATE_IDLE;
    c.transitioning = play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF;
    if (c.partnerLive)
        c.distance = Actor_WorldDistXYZToActor(actor, &player->actor);
    return PartnerPolicy::CanBegin(c);
}
extern "C" int Partners_RegisterActor(const char* name, const char* desc, int category, unsigned flags, size_t size,
                                      ActorFunc init, ActorFunc destroy, ActorFunc update, ActorFunc draw) {
    ActorDBInit def;
    def.name = name;
    def.desc = desc;
    def.category = category;
    def.flags = flags;
    def.objectId = OBJECT_GAMEPLAY_KEEP;
    def.instanceSize = size;
    def.init = init;
    def.destroy = destroy;
    def.update = update;
    def.draw = draw;
    return ActorDB::Instance->AddEntry(def).entry.id;
}
namespace {
// Set the canonical learned-song flag so pause playback and free play agree.
// Never grant it in a randomized save or write into the dormant game's slot.
static void LearnVanillaCommandMelody() {
    if (!Partners_IsActive() || IS_RANDO)
        return;
    Nei_Save()->mmQuestItems |= FC_MMQ_SONG_TIME;
}
void Tick() {
    if (!Partners_IsActive()) {
        Partners_ControlTick();
        return;
    }
    LearnVanillaCommandMelody();
    if (CVarGetInteger("gEnhancements.Partners.Malon", 0))
        NativeMalon_Register();
    if (CVarGetInteger("gEnhancements.Partners.Cojiro", 0))
        NativeCojiro_Register();
    NativeMalon_Tick();
    NativeCojiro_Tick();
    Partners_ControlTick();
    CustomEnemies_Tick();
}
void Reset() {
    Partners_Reset();
    NativeMalon_Reset();
    NativeCojiro_Reset();
}
void Register() {
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    auto* gi = GameInteractor::Instance;
    gi->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(Tick);
    gi->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        CustomEnemies_Reset();
        Reset();
    });
    gi->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { Reset(); });
    gi->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) {
        Reset();
        NativeCojiro_ClearRecovery();
    });
    gi->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t) { Reset(); });
    gi->RegisterGameHook<GameInteractor::OnOcarinaNote>(Partners_Notes);
    gi->RegisterGameHook<GameInteractor::OnPlayerUpdate>([] {
        if (Partners_IsActive())
            NativeCojiro_PlayerUpdated();
    });
    gi->RegisterGameHook<GameInteractor::OnPlayDrawEnd>([] {
        if (Partners_IsActive())
            Partners_DrawHint();
    });
}
void Menu() {
    WidgetPath path = { "Enhancements", "Extra Modes", SECTION_COLUMN_1 };
    SohGui::GetSohMenu()->AddWidget(path, "Custom Enemies", WIDGET_SEPARATOR_TEXT);
    SohGui::GetSohMenu()
        ->AddWidget(path, "Natural Custom Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.NaturalEncounters")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(false).Tooltip(
            "Add sparse, collision-checked encounters in outdoor regions. Independent of enemy randomization."));
    SohGui::GetSohMenu()
        ->AddWidget(path, "Include Custom Enemies", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Randomizer")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(false).Tooltip(
            "OoT: add custom enemies to Enemy Randomizer. MM: replace eligible outdoor ground enemies. Requires custom "
            "enemy assets."));
    SohGui::GetSohMenu()
        ->AddWidget(path, "Miniblin Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Miniblin")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip(
            "Allow Miniblins in natural encounters and custom enemy randomization."));
    SohGui::GetSohMenu()
        ->AddWidget(path, "Molmauk Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Molmauk")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip("Allow Molmauk / Hammergeist encounters."));
    SohGui::GetSohMenu()
        ->AddWidget(path, "Scissors Beetle Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Beetle")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip("Allow Scissors Beetle encounters."));
    SohGui::GetSohMenu()
        ->AddWidget(path, "Lynel Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Lynel")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip("Allow red-maned Lynel encounters."));
    SohGui::GetSohMenu()->AddWidget(path, "Partners", WIDGET_SEPARATOR_TEXT);
    SohGui::GetSohMenu()
        ->AddWidget(path, "Malon Companion", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Partners.Malon")
        .Options(UIWidgets::CheckboxOptions().Tooltip(
            "Malon follows, fights with her bow and rides Epona. Requires companion assets."));
    SohGui::GetSohMenu()
        ->AddWidget(path, "Cojiro Companion", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Partners.Cojiro")
        .Options(UIWidgets::CheckboxOptions().Tooltip(
            "Enable Cojiro after Grog's Cojiro trade. His equipment slot calls or dismisses him."));
    SohGui::GetSohMenu()
        ->AddWidget(path, "Command Melody Control", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Partners.CommandMelody")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip(
            "Command Melody controls a nearby companion. Learned by default in vanilla; shuffled in randomizer and "
            "eligible as a Black Market wildcard. C-Up returns to Link."));
}
static RegisterShipInitFunc init(Register, {});
static RegisterMenuInitFunc menu(Menu);
} // namespace

extern "C" SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char*, SkelAnime*);
extern "C" uint8_t ResourceMgr_FileExists(const char*);
extern "C" bool Partners_RigAvailable(const char* path, unsigned capacity) {
    if (!ResourceMgr_FileExists(path))
        return false;
    SkeletonHeader* rig = ResourceMgr_LoadSkeletonByName(path, nullptr);
    return rig && rig->segment && static_cast<unsigned>(rig->limbCount) + 1 == capacity;
}

extern "C" bool Partners_MustRelease(PlayState* play, Actor* actor, bool damaged, bool cameraLost) {
    PartnerPolicy::ControlConditions c;
    c.enabled = CVarGetInteger("gEnhancements.Partners.CommandMelody", 1) && Partners_SongOwned();
    c.activeWorld = Partners_IsActive() && play == gPlayState;
    c.damaged = damaged;
    c.cameraLost = cameraLost;
    if (c.activeWorld) {
        Player* player = GET_PLAYER(play);
        if (player) {
            for (auto& list : play->actorCtx.actorLists)
                for (Actor* a = list.head; a; a = a->next)
                    if (a == actor)
                        c.partnerLive = a->update != nullptr;
            c.grounded = player->actor.bgCheckFlags & BGCHECKFLAG_GROUND;
            c.transitioning = play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF;
            if (c.partnerLive)
                c.distance = Actor_WorldDistXYZToActor(actor, &player->actor);
        }
    }
    return PartnerPolicy::MustRelease(c);
}
