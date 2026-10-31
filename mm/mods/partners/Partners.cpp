#include "../../../combo/encounters/NativeCustomEnemies.h"
#include "Partners.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "../../../combo/partners/PartnerPolicy.h"
#include "mods/nei_save.h"
#include "2s2h/BenPort.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/FleetShipCombo/FleetShipCombo.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/BenGui/BenGui.hpp"
#include "2s2h/BenGui/BenMenu.h"
#include "2s2h/BenPort.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" bool Partners_IsActive(void) {
    return gPlayState && gGameState == &gPlayState->state && gPlayState->state.running &&
           FleetShipCombo_IsThisGameActive();
}
extern "C" bool Partners_SongOwned(void) {
    return Partners_IsActive() && (Nei_Save()->ootQuestItems & (1u << OOT_QUEST_SONG_COMMAND_MELODY));
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
    // Grog's completed chick quest unlocks the partner additively in solo MM.
    // The ordinary Bunny Hood/randomized reward is unchanged.
    if (CHECK_WEEKEVENTREG(WEEKEVENTREG_25_08))
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
        for (Actor* a = list.first; a; a = a->next)
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
namespace {
// Set the canonical learned-song flag so pause playback and free play agree.
// Never grant it in a randomized save or write into the dormant game's slot.
static void LearnVanillaCommandMelody() {
    if (!Partners_IsActive() || IS_RANDO)
        return;
    Nei_Save()->ootQuestItems |= (1u << OOT_QUEST_SONG_COMMAND_MELODY);
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
    gi->RegisterGameHook<GameInteractor::OnGameStateUpdate>(Tick);
    gi->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        CustomEnemies_Reset();
        Reset();
    });
    gi->RegisterGameHook<GameInteractor::OnSceneInit>([](s16, s8) { Reset(); });
    gi->RegisterGameHook<GameInteractor::OnSaveLoad>([](s16) {
        Reset();
        NativeCojiro_ClearRecovery();
    });
    gi->RegisterGameHook<GameInteractor::BeforeEndOfCycleSave>(Reset);
    gi->RegisterGameHook<GameInteractor::OnPassPlayerInputs>([](Input* input) {
        if (Partners_IsActive())
            Partners_Filter(GET_PLAYER(gPlayState), input);
    });
    gi->RegisterGameHook<GameInteractor::OnActorUpdate>([](Actor* actor) {
        if (Partners_IsActive() && actor == &GET_PLAYER(gPlayState)->actor)
            NativeCojiro_PlayerUpdated();
    });
    gi->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>([] {
        if (Partners_IsActive())
            Partners_DrawHint();
    });
}
void Menu() {
    WidgetPath path = { "Enhancements", "Gameplay", SECTION_COLUMN_1 };
    BenGui::GetBenMenu()->AddWidget(path, "Custom Enemies", WIDGET_SEPARATOR_TEXT);
    BenGui::GetBenMenu()
        ->AddWidget(path, "Natural Custom Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.NaturalEncounters")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(false).Tooltip(
            "Add sparse, collision-checked encounters in outdoor regions. Independent of enemy randomization."));
    BenGui::GetBenMenu()
        ->AddWidget(path, "Include Custom Enemies", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Randomizer")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(false).Tooltip(
            "OoT: add custom enemies to Enemy Randomizer. MM: replace eligible outdoor ground enemies. Requires custom "
            "enemy assets."));
    BenGui::GetBenMenu()
        ->AddWidget(path, "Miniblin Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Miniblin")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip(
            "Allow Miniblins in natural encounters and custom enemy randomization."));
    BenGui::GetBenMenu()
        ->AddWidget(path, "Molmauk Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Molmauk")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip("Allow Molmauk / Hammergeist encounters."));
    BenGui::GetBenMenu()
        ->AddWidget(path, "Scissors Beetle Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Beetle")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip("Allow Scissors Beetle encounters."));
    BenGui::GetBenMenu()
        ->AddWidget(path, "Lynel Encounters", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.CustomEnemies.Lynel")
        .Options(UIWidgets::CheckboxOptions().DefaultValue(true).Tooltip("Allow red-maned Lynel encounters."));
    BenGui::GetBenMenu()->AddWidget(path, "Partners", WIDGET_SEPARATOR_TEXT);
    BenGui::GetBenMenu()
        ->AddWidget(path, "Malon Companion", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Partners.Malon")
        .Options(UIWidgets::CheckboxOptions().Tooltip(
            "Malon follows, fights with her bow and rides Epona. Requires companion assets."));
    BenGui::GetBenMenu()
        ->AddWidget(path, "Cojiro Companion", WIDGET_CVAR_CHECKBOX)
        .CVar("gEnhancements.Partners.Cojiro")
        .Options(UIWidgets::CheckboxOptions().Tooltip("Enable Cojiro after Grog's chick quest or the shared Cojiro "
                                                      "trade. His equipment slot calls or dismisses him."));
    BenGui::GetBenMenu()
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
                for (Actor* a = list.first; a; a = a->next)
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
