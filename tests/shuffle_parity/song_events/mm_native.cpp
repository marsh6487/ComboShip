#include "Rando/SongEvents.h"
#include "Rando/VanillaCompatibility.h"
#include "Rando/SongEventsNativeActors.h"
#include "combo/rando/SongEventsBridge.h"
#include "tests/test_require.h"
#include <cstring>
#include <array>
extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState;
u32 gBitFlags[32]{};
u8 gItemSlots[77]{};
s32 Flags_GetRandoInf(s32 flag) { return gSaveContext.save.shipSaveInfo.rando.randoInf[flag/16] & (1 << (flag%16)); }
int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
unsigned char ExtEquip_HasItem(short, unsigned char) { return 0; }
#define CALLBACK(name,type) void name(type*,PlayState*) {}
CALLBACK(func_80AA8698,DmChar01)
CALLBACK(func_80B3EEDC,EnDai)
CALLBACK(DmChar08_WaitForSong,DmChar08)
CALLBACK(func_80B0FEBC,EnGb2)
CALLBACK(func_80B35450,EnGg)
CALLBACK(EnHg_PlayCutscene,EnHg)
CALLBACK(EnHg_HandleCutscene,EnHg)
CALLBACK(EnYb_Idle,EnYb)
CALLBACK(func_80B9451C,EnZog)
CALLBACK(func_80B948A8,EnZog)
CALLBACK(func_80C0BFE8,EnBsb)
CALLBACK(func_80BDB2B8,EnHiddenNuts)
CALLBACK(func_80B5202C,EnGk)
CALLBACK(EnPoComposer_PlayCurse,EnPoComposer)
CALLBACK(EnTimeTag_RooftopOath_Wait,EnTimeTag)
#undef CALLBACK
}
#include "mm_save_json.inc"
#include "mm_message_reservation.inc"
#include "keeta_native_rules.inc"
#include "mm_pool_song_mapping.inc"
// Execute the actual pool-entry producer phase and native oracle cache methods.
// A zero-seed menu snapshot must not impersonate a fresh generated seed.
static void TestOracleMappingAfterPreview() {
    using namespace Rando;
    for (uint32_t seed : {0u, 42u}) {
        for (bool shufflePreview : {false, true}) {
            RandoSaveInfo loaded{};
            loaded.finalSeed = seed;
            loaded.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] = 1;
            nlohmann::json mapping = nlohmann::json::array();
            for (unsigned i = 0; i < 13; ++i) mapping.push_back(3);
            SongEvents::ClearImportedMapping();
            SongEvents::Import(loaded, {{"songEvents", mapping}});
            RandoSaveInfo preview{};
            preview.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] = shufflePreview;
            NativePoolSongMapping(preview, true);
            SongEvents::RememberSavedMapping(loaded);
            RandoSaveInfo replay = loaded;
            memset(replay.songEvents, 0, sizeof(replay.songEvents));
            SongEvents::RestoreForOracle(replay);
            REQUIRE(memcmp(loaded.songEvents, replay.songEvents, sizeof(loaded.songEvents)) == 0);
        }
    }
    // Genuine new generation with the same seed still replaces an older map.
    RandoSaveInfo loaded{};
    loaded.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] = 1;
    nlohmann::json mapping = nlohmann::json::array();
    for (unsigned i = 0; i < 13; ++i) mapping.push_back(3);
    SongEvents::ClearImportedMapping();
    SongEvents::Import(loaded, {{"songEvents", mapping}});
    RandoSaveInfo generated = loaded;
    NativePoolSongMapping(generated, false);
    REQUIRE(memcmp(loaded.songEvents, generated.songEvents, sizeof(loaded.songEvents)) != 0);
    SongEvents::RememberSavedMapping(loaded);
    RandoSaveInfo replay = loaded;
    SongEvents::RestoreForOracle(replay);
    REQUIRE(memcmp(generated.songEvents, replay.songEvents, sizeof(replay.songEvents)) == 0);
    SongEvents::ClearImportedMapping();
}
#include "mm_song_save_load.inc"
static void TestLoadedSaveMappingAfterGeneration() {
    using namespace Rando;
    for (uint32_t seed : {0u, 42u}) {
        for (bool staleImport : {false, true}) {
            RandoSaveInfo generated{};
            generated.finalSeed = seed;
            generated.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] = 1;
            NativePoolSongMapping(generated, false);
            std::array<uint8_t,13> fresh{};
            std::copy(std::begin(generated.songEvents), std::end(generated.songEvents), fresh.begin());
            if (staleImport) {
                SongEvents::Import(generated, {{"songEvents", std::vector<uint8_t>(13, 5)}});
            }
            auto loaded = generated;
            std::fill(std::begin(loaded.songEvents), std::end(loaded.songEvents), 3);
            REQUIRE(memcmp(generated.songEvents, loaded.songEvents, sizeof(loaded.songEvents)) != 0);
            gSaveContext.save.shipSaveInfo.rando = nlohmann::json(loaded).get<RandoSaveInfo>();
            gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
            NativeSongSaveLoadHandler(); // Actual native post-JSON save-load prefix.
            SongEvents::RememberSavedMapping(gSaveContext.save.shipSaveInfo.rando);
            auto replay = loaded;
            memset(replay.songEvents, 0, sizeof(replay.songEvents));
            SongEvents::RestoreForOracle(replay);
            REQUIRE(memcmp(loaded.songEvents, replay.songEvents, sizeof(loaded.songEvents)) == 0);
            // A slot with song shuffle off must clear the earlier map binding.
            auto disabled = loaded;
            disabled.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] = 0;
            gSaveContext.save.shipSaveInfo.rando = nlohmann::json(disabled).get<RandoSaveInfo>();
            NativeSongSaveLoadHandler();
            auto disabledReplay = disabled;
            SongEvents::RestoreForOracle(disabledReplay);
            const uint8_t defaults[] = {14,15,17,12,12,12,12,12,14,14,15,3,19};
            REQUIRE(memcmp(defaults, disabledReplay.songEvents, sizeof(defaults)) == 0);
            // A subsequent authored slot is authoritative after the off slot.
            std::fill(std::begin(loaded.songEvents), std::end(loaded.songEvents), 5);
            gSaveContext.save.shipSaveInfo.rando = nlohmann::json(loaded).get<RandoSaveInfo>();
            NativeSongSaveLoadHandler();
            replay = loaded;
            SongEvents::RestoreForOracle(replay);
            REQUIRE(memcmp(loaded.songEvents, replay.songEvents, sizeof(loaded.songEvents)) == 0);
            // Loading a normal game must unbind the prior randomizer map too.
            gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
            NativeSongSaveLoadHandler();
            replay = generated;
            SongEvents::RestoreForOracle(replay);
            REQUIRE(std::equal(fresh.begin(), fresh.end(), replay.songEvents));
        }
    }
    SongEvents::ClearImportedMapping();
    gSaveContext = {};
}


// Observe the real message branch at the audio device boundary. Native
// inventory, song-mask assembly, form indexing and dispatch are unchanged.
static u32 observedRecognitionMask;
static u8 observedInstrument;
extern "C" void AudioOcarina_StartDefault(u32 mask) { observedRecognitionMask=mask; }
extern "C" void AudioOcarina_StartAtSongStartingPos(u32 mask) { observedRecognitionMask=mask; }
extern "C" void AudioOcarina_SetInstrument(u8 instrument) { observedInstrument=instrument; }
#include "mm_native_instruments.inc"
static PlayerTransformation observedTransformation;
extern "C" void func_808388B8(PlayState*, Player*, PlayerTransformation form) { observedTransformation=form; }
extern "C" void func_808389BC(PlayState*, Player*) {}
extern "C" void func_80838A20(PlayState*, Player*) {}
extern "C" bool NativeMaskTransition(Player*,PlayState*);
extern "C" u8 gPlayerFormItemRestrictions[PLAYER_FORM_MAX][114];
int main() {
    TestOracleMappingAfterPreview();
    TestLoadedSaveMappingAfterGeneration();
    using namespace Rando;
    auto& info = gSaveContext.save.shipSaveInfo.rando;
    for (unsigned i=0;i<32;++i) gBitFlags[i]=1u<<i;
    gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
    memset(gSaveContext.save.saveInfo.inventory.items,ITEM_NONE,sizeof(gSaveContext.save.saveInfo.inventory.items));
    gItemSlots[ITEM_OCARINA_OF_TIME]=SLOT_OCARINA;
    info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=1;info.finalSeed=42;SongEvents::Generate(info);
    gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    for(unsigned event=0;event<13;++event) {
        gSaveContext.save.playerForm=event==10?PLAYER_FORM_GORON:PLAYER_FORM_HUMAN;
        REQUIRE(!SongEvents::CanPlay(event));
        INV_CONTENT(ITEM_OCARINA_OF_TIME)=ITEM_OCARINA_OF_TIME;REQUIRE(!SongEvents::CanPlay(event));
        gSaveContext.save.saveInfo.inventory.questItems=0xffffffffu;REQUIRE(!SongEvents::CanPlay(event));
        memset(info.randoInf,0xff,sizeof(info.randoInf));REQUIRE(SongEvents::CanPlay(event));
        REQUIRE(SongEvents_MmSong(event,-1)>=0);
        INV_CONTENT(ITEM_OCARINA_OF_TIME)=ITEM_NONE;
        gSaveContext.save.saveInfo.inventory.questItems=0;memset(info.randoInf,0,sizeof(info.randoInf));
    }
    // Native listeners require Human for healing and Goron for the child.
    // A transformed starting player without its removable mask cannot change form.
    INV_CONTENT(ITEM_OCARINA_OF_TIME)=ITEM_OCARINA_OF_TIME;memset(info.randoInf,0xff,sizeof(info.randoInf));
    gSaveContext.save.saveInfo.inventory.questItems=0xffffffffu;
    gItemSlots[ITEM_MASK_GORON]=SLOT_MASK_GORON;INV_CONTENT(ITEM_MASK_GORON)=ITEM_NONE;
    gItemSlots[ITEM_MASK_ZORA]=SLOT_MASK_ZORA;INV_CONTENT(ITEM_MASK_ZORA)=ITEM_NONE;
    gSaveContext.save.playerForm=PLAYER_FORM_GORON;
    for(unsigned event:{4,5,6,7})REQUIRE(!SongEvents::CanPlay(event));
    REQUIRE(SongEvents::CanPlay(3)&&SongEvents::CanPlay(10)); // unrestricted Poe listener and native Goron
    INV_CONTENT(ITEM_MASK_GORON)=ITEM_MASK_GORON;
    for(unsigned event:{4,5,6,7})REQUIRE(SongEvents::CanPlay(event));
    INV_CONTENT(ITEM_MASK_GORON)=ITEM_NONE;gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    REQUIRE(!SongEvents::CanPlay(10));
    info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=0;
    for(unsigned event:{4,5,6,7})REQUIRE(SongEvents::CanPlay(event));
    REQUIRE(!SongEvents::CanPlay(10));info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=1;
    auto map=ComboRando::SongEvents::Generate(ComboRando::SongEvents::Game::Mm,88,true);
    nlohmann::json spoiler={{"songEvents",map}};SongEvents::Import(info,spoiler);
    REQUIRE(std::equal(map.begin(),map.end(),info.songEvents));
    nlohmann::json savedJson=info;memset(info.songEvents,0,sizeof(info.songEvents));savedJson.get_to(info);
    REQUIRE(std::equal(map.begin(),map.end(),info.songEvents));
    memset(info.songEvents,0,sizeof(info.songEvents));SongEvents::RestoreForOracle(info);
    REQUIRE(std::equal(map.begin(),map.end(),info.songEvents));
    auto saved=info;SongEvents::ClearImportedMapping();SongEvents::RememberSavedMapping(saved);
    memset(info.songEvents,0,sizeof(info.songEvents));SongEvents::RestoreForOracle(info);
    REQUIRE(std::equal(map.begin(),map.end(),info.songEvents));
    map[1]=255;spoiler["songEvents"]=map;bool rejected=false;
    try{SongEvents::Import(info,spoiler);}catch(const std::invalid_argument&){rejected=true;}REQUIRE(rejected);
    info.songEvents[0]=16;INV_CONTENT(ITEM_OCARINA_OF_TIME)=ITEM_OCARINA_OF_TIME;
    memset(info.randoInf,0xff,sizeof(info.randoInf));gSaveContext.save.saveInfo.inventory.questItems=1u<<QUEST_SONG_LULLABY;
    REQUIRE(SongEvents::CanPlay(0) && SongEvents_MmSong(0,-1)==OCARINA_SONG_GORON_LULLABY);
    static PlayState play{};static Player player{};gPlayState=&play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    auto setActor=[&](Actor* actor,s16 scene){play.actorCtx.actorLists[ACTORCAT_BG].first=actor;play.sceneId=scene;};
    static DmChar01 temple{};temple.dyna.actor.id=ACTOR_DM_CHAR01;temple.actionFunc=func_80AA8698;
    setActor(&temple.dyna.actor,SCENE_21MITURINMAE);
    player.actor.world.pos.x=0;player.actor.world.pos.z=1040;player.transformation=PLAYER_FORM_DEKU;
    info.songEvents[0]=5;info.randoSaveOptions[RO_ACCESS_DUNGEONS]=RO_ACCESS_DUNGEONS_FORM_AND_SONG;
    REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    RecognizeNativeSong(&play,OCARINA_SONG_TIME);
    temple.actionFunc=nullptr; // Native event switches to a cutscene after recognizing the assignment.
    CloseNativeSong(&play);
    REQUIRE(play.msgCtx.ocarinaMode==OCARINA_MODE_EVENT && play.msgCtx.ocarinaAction==OCARINA_ACTION_CHECK_NOTIME_DONE && sLastPlayedSong==0xff);
    REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    play.msgCtx.ocarinaMode=OCARINA_MODE_NONE;play.msgCtx.ocarinaAction=OCARINA_ACTION_FREE_PLAY_DONE;
    RecognizeNativeSong(&play,OCARINA_SONG_TIME);CloseNativeSong(&play);
    REQUIRE(sLastPlayedSong==OCARINA_SONG_TIME && play.msgCtx.ocarinaMode==OCARINA_MODE_NONE); // Completed actor: ordinary Time effect remains available.
    temple.actionFunc=func_80AA8698;
    sLastPlayedSong=0xff;sSongEventReservedSong=-1;play.msgCtx.ocarinaMode=OCARINA_MODE_NONE;
    CloseNativeSong(&play);REQUIRE(play.msgCtx.ocarinaMode==OCARINA_MODE_NONE); // Closing unrelated dialogue cannot replay stale notes.

    WEEKEVENTREG(WEEKEVENTREG_20_01 >> 8) |= WEEKEVENTREG_20_01 & 0xff;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));WEEKEVENTREG(WEEKEVENTREG_20_01 >> 8) &= ~(WEEKEVENTREG_20_01 & 0xff);
    temple.dyna.actor.playerHeightRel=100;REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_TIME)); // Native dyna origin height does not gate the pedestal.temple.dyna.actor.playerHeightRel=0;
    player.transformation=PLAYER_FORM_HUMAN;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    info.randoSaveOptions[RO_ACCESS_DUNGEONS]=RO_ACCESS_DUNGEONS_SONG_ONLY;REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    player.actor.world.pos.z=1500;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    static EnZog mikau{};mikau.actor.id=ACTOR_EN_ZOG;mikau.actionFunc=func_80B9451C;info.songEvents[7]=13;
    setActor(&mikau.actor,SCENE_30GYOSON);REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_SOARING));
    mikau.actionFunc=nullptr;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_SOARING));mikau.actionFunc=func_80B9451C;
    mikau.actor.xzDistToPlayer=121;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_SOARING));mikau.actor.xzDistToPlayer=0;
    mikau.actor.playerHeightRel=81;REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_SOARING)); // Native Mikau checks horizontal range only.
    static EnHg father{};father.actor.id=ACTOR_EN_HG;father.actor.colChkInfo.health=1;info.songEvents[5]=5;
    setActor(&father.actor,SCENE_MUSICHOUSE);REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    father.actor.playerHeightRel=80;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));father.actor.playerHeightRel=0;
    father.actor.colChkInfo.health=0;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));father.actor.colChkInfo.health=1;
    father.actionFunc=EnHg_HandleCutscene;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    static EnPoComposer composer{};composer.actor.id=ACTOR_EN_PO_COMPOSER;composer.actionFunc=EnPoComposer_PlayCurse;info.songEvents[11]=5;
    setActor(&composer.actor,SCENE_IKANA);REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    composer.actor.params=0x8000;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));composer.actor.params=0;
    composer.actionFunc=nullptr;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    static EnTimeTag tag{};tag.actor.id=ACTOR_EN_TIME_TAG;tag.actor.params=0x4000;tag.actionFunc=EnTimeTag_RooftopOath_Wait;info.songEvents[12]=5;
    setActor(&tag.actor,SCENE_OKUJOU);REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    tag.actor.xzDistToPlayer=301;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));tag.actor.xzDistToPlayer=0;
    tag.actor.playerHeightRel=81;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));tag.actor.playerHeightRel=0;
    tag.actor.params=0;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));tag.actor.params=0x4000;
    tag.actionFunc=nullptr;REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
#if SONG_KEETA_ROCK_COUNT == 5
    // Execute the five exact current East.cpp CHECK predicates, not copied policy.
    info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=1;info.songEvents[8]=3;
    gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    gItemSlots[ITEM_BOMB]=SLOT_BOMB;INV_CONTENT(ITEM_BOMB)=ITEM_BOMB;
    INV_CONTENT(ITEM_OCARINA_OF_TIME)=ITEM_OCARINA_OF_TIME;
    memset(info.randoInf,0xff,sizeof(info.randoInf));
    gSaveContext.save.saveInfo.inventory.questItems=1u<<QUEST_SONG_STORMS;
    for(const auto& rule:NativeKeetaRockRules()) REQUIRE(rule());
    gSaveContext.save.saveInfo.inventory.questItems=1u<<QUEST_SONG_SONATA;
    for(const auto& rule:NativeKeetaRockRules()) REQUIRE(!rule());
    gSaveContext.save.saveInfo.inventory.questItems=1u<<QUEST_SONG_STORMS;
    INV_CONTENT(ITEM_BOMB)=ITEM_NONE;
    for(const auto& rule:NativeKeetaRockRules()) REQUIRE(!rule());
    INV_CONTENT(ITEM_BOMB)=ITEM_BOMB;info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=0;
    for(const auto& rule:NativeKeetaRockRules()) REQUIRE(!rule());
    gSaveContext.save.saveInfo.inventory.questItems=1u<<QUEST_SONG_SONATA;
    for(const auto& rule:NativeKeetaRockRules()) REQUIRE(rule());
#endif
    info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=0;
    REQUIRE(SongEvents_MmSong(0,OCARINA_SONG_SONATA)==OCARINA_SONG_SONATA && !SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    // Every native form keeps its native instrument, with events both Off and
    // On. Do not force the human ocarina to make transformed songs work.
    memset(gSaveContext.save.saveInfo.weekEventReg,0,sizeof(gSaveContext.save.saveInfo.weekEventReg));
    memset(gSaveContext.eventInf,0,sizeof(gSaveContext.eventInf));
    const u8 forms[]={PLAYER_FORM_HUMAN,PLAYER_FORM_DEKU,PLAYER_FORM_GORON,PLAYER_FORM_ZORA,PLAYER_FORM_FIERCE_DEITY};
    const u8 instruments[]={OCARINA_INSTRUMENT_DEFAULT,OCARINA_INSTRUMENT_DEKU_PIPES,
        OCARINA_INSTRUMENT_GORON_DRUMS,OCARINA_INSTRUMENT_ZORA_GUITAR,OCARINA_INSTRUMENT_DEFAULT};
    for(bool enabled:{false,true}) for(size_t form=0;form<std::size(forms);++form) {
        info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=enabled;
        gSaveContext.save.playerForm=forms[form];player.transformation=forms[form];
        for(unsigned song=0;song<=QUEST_SONG_SUN-QUEST_SONG_SONATA;++song) {
            gSaveContext.save.saveInfo.inventory.questItems=0;
            NativeOwnedSongMask(&play);REQUIRE(play.msgCtx.ocarinaAvailableSongs==0);
            gSaveContext.save.saveInfo.inventory.questItems=1u<<(QUEST_SONG_SONATA+song);
            NativeOwnedSongMask(&play);
            const u32 expected=1u<<song;
            const u32 extras=(QUEST_SONG_SONATA+song)==QUEST_SONG_TIME?0x3000:0;
            REQUIRE(play.msgCtx.ocarinaAvailableSongs==(expected|extras));
            for(auto action:{OCARINA_ACTION_FREE_PLAY,OCARINA_ACTION_CHECK_NOTIME}) {
                play.msgCtx.ocarinaAction=action;play.msgCtx.msgMode=MSGMODE_OCARINA_STARTING;
                observedInstrument=255;observedRecognitionMask=0;
                NativeFormInstrumentStart(&play);
                REQUIRE(observedInstrument==instruments[form]);
                REQUIRE(observedRecognitionMask==((expected|extras)|0xC0000000));
                REQUIRE(play.msgCtx.msgMode==MSGMODE_OCARINA_PLAYING);
            }
        }
    }
    // A different owned transformation mask also supplies a native route back
    // to Human: Goron -> Deku -> Human. Use the actual player transition branch
    // and real item restriction table, not a model of transformation behavior.
    player.transformation=PLAYER_FORM_GORON;player.currentMask=PLAYER_MASK_GORON;
    player.itemAction=PLAYER_IA_MASK_DEKU;
    REQUIRE(gPlayerFormItemRestrictions[PLAYER_FORM_GORON][ITEM_MASK_DEKU]);
    NativeMaskTransition(&player,&play);REQUIRE(observedTransformation==PLAYER_FORM_DEKU);
    player.transformation=PLAYER_FORM_DEKU;
    REQUIRE(gPlayerFormItemRestrictions[PLAYER_FORM_DEKU][ITEM_MASK_DEKU]);
    NativeMaskTransition(&player,&play);REQUIRE(observedTransformation==PLAYER_FORM_HUMAN);
    gItemSlots[ITEM_MASK_DEKU]=SLOT_MASK_DEKU;INV_CONTENT(ITEM_MASK_DEKU)=ITEM_MASK_DEKU;
    gItemSlots[ITEM_MASK_GORON]=SLOT_MASK_GORON;INV_CONTENT(ITEM_MASK_GORON)=ITEM_NONE;
    gSaveContext.save.playerForm=PLAYER_FORM_GORON;
    INV_CONTENT(ITEM_OCARINA_OF_TIME)=ITEM_OCARINA_OF_TIME;
    info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=1;
    gSaveContext.save.saveInfo.inventory.questItems=1u<<QUEST_SONG_TIME;
    memset(info.randoInf,0xff,sizeof(info.randoInf));
    for(unsigned event:{4,5,6,7}) { info.songEvents[event]=5;REQUIRE(SongEvents::CanPlay(event)); }
    INV_CONTENT(ITEM_MASK_DEKU)=ITEM_NONE;
    for(unsigned event:{4,5,6,7}) REQUIRE(!SongEvents::CanPlay(event));
    INV_CONTENT(ITEM_MASK_ZORA)=ITEM_MASK_ZORA;
    for(unsigned event:{4,5,6,7}) REQUIRE(SongEvents::CanPlay(event));
    // Logical ability to transform must not capture a Human-only listener
    // while still physically Goron; changing form makes that listener eligible.
    player.transformation=PLAYER_FORM_GORON;
    mikau.actionFunc=func_80B9451C;mikau.actor.xzDistToPlayer=0;
    setActor(&mikau.actor,SCENE_30GYOSON);
    REQUIRE(!SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
    player.transformation=PLAYER_FORM_HUMAN;gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    REQUIRE(SongEvents_MmInterceptSong(OCARINA_SONG_TIME));
}
