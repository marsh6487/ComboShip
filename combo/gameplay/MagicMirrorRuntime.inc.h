// Included by each native adapter after its real engine headers and MirrorHost
// boundaries. State is local to that engine; only permanent ownership is shared.
namespace MirrorRuntime {
constexpr u16 kReceiptText = 0x3FEB, kPromptText = 0x3FEC;
struct Context {
    int slot = -1, scene = -1, mode = -1;
    u32 seed = 0;
};
static Context sContext;
static int sSerial = 0, sPickupToken = 0;
static bool sPrompt = false, sPromptAccepted = false, sAnchorValid = false;
static const CollisionHeader* sCollision = nullptr;
static MagicMirrorAnchor::Point sAnchor{};
static PlayState* sPromptPlay = nullptr;
static Player* sPromptPlayer = nullptr;
static u32 sPromptAddedFlags = 0;
// The menu's closing A/B/stick belongs to the prompt until the next input tick.
static PlayState* sPromptInputPlay = nullptr;
static Player* sPromptInputPlayer = nullptr;
static PlayState* sCastPlay = nullptr;
static Player* sCastPlayer = nullptr;
static bool sDeparted = false;
static unsigned sCastTicks = 0;
static unsigned sDepartureTick = 0;
// Arrival owns no old PlayState/player pointer and survives departure teardown.
static Context sArrivalContext;
static bool sArrivalPending = false;
static PlayState* sArrivalPlay = nullptr;
static ActorEntry* sArrivalOriginal = nullptr;
static ActorEntry sArrivalSpawn;
void QueueArrival() {
    sArrivalContext = { gSaveContext.fileNum, -1, gComboGameMode, MirrorHost::Seed() };
    sArrivalPending = true;
}
#ifdef COMBO_MAGIC_MIRROR_MM
constexpr u32 kPromptFreeze = PLAYER_STATE1_20000000;
#else
constexpr u32 kPromptFreeze = PLAYER_STATE1_IN_CUTSCENE;
#endif

u16 CurrentText(const PlayState* play) {
#ifdef COMBO_MAGIC_MIRROR_MM
    return play->msgCtx.currentTextId;
#else
    return play->msgCtx.textId;
#endif
}
void ReleasePrompt(bool close) {
    // Teardown/slot changes can invalidate the old actor. Touch only the same
    // live player, and close only the private message this prompt opened.
    if (sPromptPlay && gPlayState == sPromptPlay && gGameState == &gPlayState->state &&
        GET_PLAYER(gPlayState) == sPromptPlayer) {
        if (close && CurrentText(gPlayState) == kPromptText)
            Message_CloseTextbox(gPlayState);
        sPromptPlayer->stateFlags1 &= ~sPromptAddedFlags;
    }
    sPrompt = false;
    sPromptAccepted = false;
    sPromptPlay = nullptr;
    sPromptPlayer = nullptr;
    sPromptAddedFlags = 0;
}
void ReleaseCast() {
    if (sCastPlay && gPlayState == sCastPlay && gGameState == &gPlayState->state &&
        GET_PLAYER(gPlayState) == sCastPlayer)
        MagicMirrorPlayer_End(gPlayState, sCastPlayer);
    sCastPlay = nullptr;
    sCastPlayer = nullptr;
    sDeparted = false;
    sCastTicks = 0;
    sDepartureTick = 0;
}

int Scene(const PlayState* play) {
#ifdef COMBO_MAGIC_MIRROR_MM
    return play->sceneId;
#else
    return play->sceneNum;
#endif
}
bool Matches(PlayState* play) {
    return play && sContext.slot == gSaveContext.fileNum && sContext.seed == MirrorHost::Seed() &&
           sContext.scene == Scene(play) && sContext.mode == gComboGameMode;
}
void Reset() {
    ReleasePrompt(true);
    ReleaseCast();
    sContext = {};
    sPickupToken = 0;
    sPrompt = sAnchorValid = false;
    sPromptInputPlay = nullptr;
    sPromptInputPlayer = nullptr;
    sCollision = nullptr;
}
void Destroy() {
    // MM notifies OnPlayDestroy after its arena has been freed.
    sPromptPlay = nullptr;
    sCastPlay = nullptr;
    Reset();
}
void Bind(PlayState* play) {
    if (Matches(play))
        return;
    Reset();
    if (play)
        sContext = { gSaveContext.fileNum, Scene(play), gComboGameMode, MirrorHost::Seed() };
}
bool Mode() {
#ifdef COMBO_MAGIC_MIRROR_MM
    return gComboGameMode == COMBO_GAME_BOTH || gComboGameMode == COMBO_GAME_MM;
#else
    return gComboGameMode == COMBO_GAME_BOTH || gComboGameMode == COMBO_GAME_OOT;
#endif
}
bool Forest(PlayState* play) {
#ifdef COMBO_MAGIC_MIRROR_MM
    return ComboForestActors_IsExploration(play);
#else
    return play &&
           ComboSoloForest_IsOotVisit(play->sceneNum, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                      gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.cutsceneIndex);
#endif
}
bool AllowedScene(PlayState* play) {
    if (!play)
        return false;
    if (Forest(play))
        return true;
#ifdef COMBO_MAGIC_MIRROR_MM
    switch (play->sceneId) {
        case SCENE_20SICHITAI2:
        case SCENE_IKANA:
        case SCENE_13HUBUKINOMITI:
        case SCENE_ROMANYMAE:
        case SCENE_22DEKUCITY:
        case SCENE_00KEIKOKU:
        case SCENE_F01:
        case SCENE_30GYOSON:
        case SCENE_31MISAKI:
        case SCENE_24KEMONOMITI:
        case SCENE_BOTI:
        case SCENE_20SICHITAI:
        case SCENE_21MITURINMAE:
        case SCENE_11GORONNOSATO2:
        case SCENE_35TAKI:
        case SCENE_11GORONNOSATO:
        case SCENE_10YUKIYAMANOMURA:
        case SCENE_IKANAMAE:
        case SCENE_F40:
        case SCENE_F41:
        case SCENE_10YUKIYAMANOMURA2:
        case SCENE_14YUKIDAMANOMITI:
        case SCENE_12HAKUGINMAE:
        case SCENE_17SETUGEN:
        case SCENE_17SETUGEN2:
        case SCENE_26SARUNOMORI:
        case SCENE_TOWN:
        case SCENE_ICHIBA:
        case SCENE_BACKTOWN:
        case SCENE_CLOCKTOWER:
        case SCENE_ALLEY:
            return true;
        default:
            return false;
    }
#else
    return play->sceneNum >= SCENE_HYRULE_FIELD && play->sceneNum <= SCENE_OUTSIDE_GANONS_CASTLE;
#endif
}
bool Safe(PlayState* play, bool ownMessage) {
    if (!Mode() || !play || gGameState != &play->state || !play->state.running ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->pauseCtx.state != 0 || play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger || MirrorHost::TimedActivity() ||
        MirrorHost::TransitionPending())
        return false;
    const auto* player = GET_PLAYER(play);
    if (!player || player->heldActor || player->rideActor || !(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
        (player->stateFlags1 & PLAYER_STATE1_DEAD))
        return false;
#ifdef COMBO_MAGIC_MIRROR_MM
    if (gSaveContext.save.saveInfo.playerData.health <= 0 || play->soaringCsOrSoTCsPlaying ||
        (player->stateFlags1 & PLAYER_STATE1_8000000))
        return false;
#else
    if (gSaveContext.health <= 0 || (player->stateFlags1 & PLAYER_STATE1_IN_WATER))
        return false;
#endif
    return ownMessage || (play->msgCtx.msgMode == MSGMODE_NONE && !Player_InCsMode(play) && !MirrorHost::Busy(play));
}
void HoldPrompt() {
    // Native action changes clear this bit. Reacquire only the bits this
    // prompt now adds, so release still preserves a pre-existing owner.
    sPromptAddedFlags |= kPromptFreeze & ~sPromptPlayer->stateFlags1;
    sPromptPlayer->stateFlags1 |= kPromptFreeze;
    MagicMirrorPlayer_StopMotion(sPromptPlayer);
}
bool PromptOwnsPlayer(PlayState* play, const Player* player) {
    // This independent ownership survives a native action clearing its flags.
    // Never inspect an old actor after scene/file/player replacement.
    if (!(sPrompt && play == sPromptPlay && player == sPromptPlayer) &&
        !(sPromptInputPlay && play == sPromptInputPlay && player == sPromptInputPlayer))
        return false;
    // Eligibility is checked by Update, which cancels/cleans up the prompt.
    // Collision and interaction state can change during the native player
    // update; they must not hand a live choice box's input back to movement.
    if (!play || play != gPlayState || gGameState != &play->state || GET_PLAYER(play) != player || !Matches(play) ||
        !MagicMirror_IsOwned())
        return false;
    return (play == sPromptInputPlay && player == sPromptInputPlayer) ||
           (sPrompt && play == sPromptPlay && player == sPromptPlayer &&
            (CurrentText(play) == kPromptText ||
             (sPromptAccepted && play->msgCtx.msgMode == MSGMODE_NONE && play->msgCtx.msgLength == 0)) &&
            (sPromptAccepted ||
             (play->msgCtx.msgMode != MSGMODE_NONE && play->msgCtx.msgMode != MSGMODE_TEXT_CLOSING)));
}
void ConsumePromptInput(PlayState* play) {
    sPromptInputPlay = play;
    sPromptInputPlayer = GET_PLAYER(play);
}
bool Travel(PlayState* play, bool casting = false) {
    if (casting && (!play || play != sCastPlay || GET_PLAYER(play) != sCastPlayer ||
                    MagicMirrorPlayer_State(sCastPlayer) != MAGIC_MIRROR_CAST_READY))
        return false;
    if (!MagicMirror_IsOwned() || !Safe(play, casting) || !AllowedScene(play) || !MirrorHost::PackAvailable())
        return false;
#ifndef COMBO_MAGIC_MIRROR_MM
    if (Combo_IsCombinedGame())
        return MirrorHost::RequestHandoff();
    play->nextEntranceIndex = COMBO_SOLO_FOREST_OOT_ENTRANCE;
    gSaveContext.cutsceneIndex = 0;
#else
    play->nextEntrance = Combo_IsCombinedGame() ? ENTRANCE(LOST_WOODS, 0) : COMBO_SOLO_FOREST_MM_ENTRANCE;
    gSaveContext.save.cutsceneIndex = 0;
#endif
    QueueArrival();
    // Resolve the placed stump in the destination's loaded collision. TOP/Farore
    // and RETURN/grotto retain their owning systems' contents.
    gSaveContext.respawnFlag = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    play->transitionType = gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
    return true;
}
int BeginPickup() {
    if (sPickupToken || MagicMirror_IsOwned())
        return 0;
    sSerial = sSerial >= 0x7FFF ? 1 : sSerial + 1;
    sPickupToken = sSerial;
    return sPickupToken;
}
void FinishPickup(int token) {
    if (!token || token != sPickupToken || !Matches(gPlayState) || !Mode() ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum))
        return;
    sPickupToken = 0;
    if (!MagicMirror_IsOwned()) {
        TravelTools_Grant(EXT_ITEM_MAGIC_MIRROR);
        MirrorHost::Save();
    }
}
bool Locate(PlayState* play) {
    const auto* header = play->colCtx.colHeader;
    if (sCollision == header)
        return sAnchorValid;
    sCollision = header;
    sAnchorValid = false;
    if (!header || !header->vtxList || !header->polyList)
        return false;
    std::vector<MagicMirrorAnchor::Triangle> triangles;
    for (int index = 0; index < header->numPolygons; ++index) {
        const auto& poly = header->polyList[index];
        if (poly.normal.y < 30000)
            continue;
        MagicMirrorAnchor::Triangle triangle;
        bool valid = true;
        for (int i = 0; i < 3; ++i) {
            const unsigned vertex = poly.vtxData[i] & 0x1FFF;
            if (vertex >= header->numVertices) {
                valid = false;
                break;
            }
            const auto& p = header->vtxList[vertex];
            triangle.p[i] = { static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z) };
        }
        if (valid && MagicMirrorAnchor::Candidate(triangle))
            triangles.push_back(triangle);
    }
    sAnchorValid = MagicMirrorAnchor::Find(triangles, sAnchor);
    return sAnchorValid;
}

bool OnStumpCap(PlayState* play, const Player* player) {
    const auto* header = play->colCtx.colHeader;
    const auto* floor = player->actor.floorPoly;
    if (!header || !header->polyList || !header->vtxList || !floor || player->actor.floorBgId != BGCHECK_SCENE ||
        !std::isfinite(player->actor.floorHeight) || std::abs(player->actor.floorHeight - sAnchor.y) > 1)
        return false;
    // Require the actual static fan under Link's feet. A nearby ramp, another
    // floor at the same height, or a platform above the stump cannot grant it.
    for (int index = 0; index < header->numPolygons; ++index) {
        const auto& poly = header->polyList[index];
        if (&poly != floor)
            continue;
        if (poly.normal.y < 30000)
            return false;
        bool hub = false;
        for (int i = 0; i < 3; ++i) {
            const unsigned vertex = poly.vtxData[i] & 0x1FFF;
            if (vertex >= header->numVertices)
                return false;
            const auto& v = header->vtxList[vertex];
            const MagicMirrorAnchor::Point point{ static_cast<float>(v.x), static_cast<float>(v.y),
                                                  static_cast<float>(v.z) };
            if (std::abs(point.y - sAnchor.y) >= 1)
                return false;
            hub |= MagicMirrorAnchor::Near(point, sAnchor);
        }
        return hub;
    }
    return false;
}

bool ArrivalDestination(const PlayState* play) {
    if (!play || !Mode() || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) || gSaveContext.respawnFlag != 0 ||
        gSaveContext.cutsceneTrigger)
        return false;
#ifdef COMBO_MAGIC_MIRROR_MM
    return ComboOpeningForest_IsExploration(play) ||
           (ComboSoloForest_IsMmVisit(play->sceneId, 0, gSaveContext.gameMode, gSaveContext.fileNum,
                                      gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex) &&
            gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
#else
    return ComboSoloForest_IsOotVisit(play->sceneNum, 0, gSaveContext.gameMode, gSaveContext.fileNum,
                                      gSaveContext.sceneLayer, gSaveContext.cutsceneIndex) &&
           gSaveContext.entranceIndex == COMBO_SOLO_FOREST_OOT_ENTRANCE;
#endif
}

bool Landing(PlayState* play, Vec3s& position, s16& yaw) {
    // Prefer the center of the actual placed cap, facing the ramp. Retain the
    // checked ground fallbacks if a scene edit obstructs or removes its top.
    constexpr int offsets[][2] = { { 0, 0 },   { 0, -120 }, { 85, -85 }, { 120, 0 },  { 85, 85 },
                                   { 0, 120 }, { -85, 85 }, { -120, 0 }, { -85, -85 } };
    for (const auto& offset : offsets) {
        const bool cap = offset[0] == 0 && offset[1] == 0;
        Vec3f point{ std::round(sAnchor.x) + offset[0], sAnchor.y + 1, std::round(sAnchor.z) + offset[1] };
        CollisionPoly* floor = nullptr;
        s32 bgId = BGCHECK_SCENE;
        const float height = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &bgId, &point);
        if (!std::isfinite(height) || !floor || bgId != BGCHECK_SCENE || floor->normal.y < 30000 ||
            (cap ? std::abs(height - sAnchor.y) > 1 : height > sAnchor.y - 1) || height < sAnchor.y - 220 ||
            height <= BGCHECK_Y_MIN)
            continue;
        point.y = std::ceil(height);
        if (std::abs(point.x) > 32700 || std::abs(point.y) > 32700 || std::abs(point.z) > 32700)
            continue;
        bool clear = true;
        // Support the complete footprint and reject nearby walls/low ceilings.
        for (const auto& side : offsets) {
            Vec3f edge{ point.x + side[0] / 3.0f, point.y + 5, point.z + side[1] / 3.0f };
            CollisionPoly* support = nullptr;
            const float supportY = BgCheck_EntityRaycastFloor3(&play->colCtx, &support, &bgId, &edge);
            if (!support || bgId != BGCHECK_SCENE || support->normal.y < 30000 || !std::isfinite(supportY) ||
                std::abs(supportY - point.y) > 5) {
                clear = false;
                break;
            }
            for (float elevation : { 10.0f, 55.0f, 100.0f }) {
                Vec3f from{ point.x, point.y + elevation, point.z };
                Vec3f to{ edge.x, point.y + elevation, edge.z }, hit{};
                if (BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hit, &support, true, true, true, true, &bgId))
                    clear = false;
            }
            Vec3f bottom{ edge.x, point.y + 5, edge.z }, top{ edge.x, point.y + 100, edge.z }, hit{};
            if (BgCheck_EntityLineTest1(&play->colCtx, &bottom, &top, &hit, &support, true, true, true, true, &bgId))
                clear = false;
            if (!clear)
                break;
        }
        if (!clear)
            continue;
        position = { static_cast<s16>(point.x), static_cast<s16>(point.y), static_cast<s16>(point.z) };
        const float angle = cap ? 3.14159265359f / 2 : std::atan2(sAnchor.x - point.x, sAnchor.z - point.z);
#ifdef COMBO_MAGIC_MIRROR_MM
        const int degrees = (static_cast<int>(std::round(angle * (180.0f / 3.14159265359f))) + 360) % 360;
        yaw = static_cast<s16>((degrees << 7) | 127); // native packed yaw + no actor cutscene
#else
        yaw = static_cast<s16>(static_cast<int>(std::round(angle * (32768.0f / 3.14159265359f))));
#endif
        return true;
    }
    return false;
}

void PrepareArrival(PlayState* play) {
    const bool pending = sArrivalPending;
    sArrivalPending = false; // The very next scene consumes it, including mismatches.
    sArrivalPlay = nullptr;
    sArrivalOriginal = nullptr;
    if (!pending || sArrivalContext.slot != gSaveContext.fileNum || sArrivalContext.seed != MirrorHost::Seed() ||
        sArrivalContext.mode != gComboGameMode || !ArrivalDestination(play) || !play->linkActorEntry)
        return;
    sCollision = nullptr; // Fresh collision may reuse the departed scene's address.
    Vec3s position;
    s16 yaw;
    if (!Locate(play) || !Landing(play, position, yaw))
        return; // Missing/ambiguous geometry keeps the scene's ordinary safe spawn.
    sArrivalOriginal = play->linkActorEntry;
    sArrivalSpawn = *sArrivalOriginal; // Never mutate the cached Prelude resource.
    sArrivalSpawn.pos = position;
    sArrivalSpawn.rot.y = yaw;
#ifdef COMBO_MAGIC_MIRROR_MM
    sArrivalSpawn.id &= ~0x8000; // Actor_SpawnEntry must decode packed degrees.
    sArrivalSpawn.params = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D);
#else
    sArrivalSpawn.params = 0x0DFF; // native stationary start, camera selected by floor
#endif
    play->linkActorEntry = &sArrivalSpawn;
    sArrivalPlay = play;
}
void FinishArrival(PlayState* play) {
    if (sArrivalPlay != play)
        return;
    sArrivalPlay = nullptr;
    if (play->linkActorEntry == &sArrivalSpawn)
        play->linkActorEntry = sArrivalOriginal;
    sArrivalOriginal = nullptr; // Legacy doorway anchors must keep the real scene spawn.
    if (auto* player = GET_PLAYER(play))
        MagicMirrorPlayer_Arrive(play, player);
}
u16 PressedButton(PlayState* play) {
    static constexpr u16 buttons[] = { 0, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT, BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT };
    for (s32 index = 1; index < 8; ++index)
        if ((play->state.input[0].press.button & buttons[index]) && !MirrorHost::ButtonClaimed(buttons[index]) &&
            MirrorHost::ButtonItem(index) == EXT_ITEM_MAGIC_MIRROR)
            return buttons[index];
    return 0;
}
void Update() {
    // OnGameStateMainStart runs once before native player/message updates.
    sPromptInputPlay = nullptr;
    sPromptInputPlayer = nullptr;
    auto* play = gPlayState;
    Bind(play);
    if (!play || !Mode())
        return;
    if (sCastPlay) {
        if (play != sCastPlay || GET_PLAYER(play) != sCastPlayer || !MagicMirrorPlayer_State(sCastPlayer) ||
            ++sCastTicks > 160 ||
            (!sDeparted && (!MagicMirror_IsOwned() || !Safe(play, true) || play->msgCtx.msgMode != MSGMODE_NONE ||
                            !AllowedScene(play) || !MirrorHost::PackAvailable()))) {
            MirrorHost::Trace("cast-cancel", play);
            ReleaseCast();
        } else if (!sDeparted && MagicMirrorPlayer_State(sCastPlayer) == MAGIC_MIRROR_CAST_READY) {
            sDeparted = Travel(play, true);
            if (sDeparted)
                sDepartureTick = sCastTicks;
            MirrorHost::Trace(sDeparted ? "travel-requested" : "travel-rejected", play);
            if (!sDeparted) {
                ReleaseCast();
                MirrorHost::Error();
            }
        }
        return;
    }
    if (sPrompt) {
        const auto text = CurrentText(play);
        const bool finished = sPromptAccepted && play->msgCtx.msgMode == MSGMODE_NONE && play->msgCtx.msgLength == 0;
        if (play != sPromptPlay || GET_PLAYER(play) != sPromptPlayer || (text != kPromptText && !finished) ||
            !Safe(play, true) || !MagicMirror_IsOwned()) {
            MirrorHost::Trace("prompt-invalidated", play);
            ReleasePrompt(text == kPromptText);
            return;
        }
        HoldPrompt();
        if (sPromptAccepted) {
            if (!finished)
                return;
            // Native closing clears text length/ID and restores the HUD. A
            // forced NONE with live text makes native drawing reopen the box.
            ConsumePromptInput(play);
            ReleasePrompt(false);
            if (Safe(play, false) && AllowedScene(play) && MirrorHost::PackAvailable()) {
                if (MagicMirrorPlayer_Begin(play, GET_PLAYER(play))) {
                    sCastPlay = play;
                    sCastPlayer = GET_PLAYER(play);
                    sCastTicks = 0;
                    MirrorHost::Trace("cast-started", play);
                } else {
                    MirrorHost::Trace("cast-rejected", play);
                    MirrorHost::Error();
                }
            } else {
                MirrorHost::Trace("cast-ineligible-after-close", play);
            }
            return;
        }
        if (play->state.input[0].press.button & BTN_B) {
            MirrorHost::Trace("prompt-cancel", play);
            ConsumePromptInput(play);
            ReleasePrompt(true);
            return;
        }
        if (Message_GetState(&play->msgCtx) == TEXT_STATE_CHOICE && Message_ShouldAdvance(play)) {
            const bool yes = play->msgCtx.choiceIndex == 0;
            ConsumePromptInput(play);
            if (yes && Safe(play, true) && AllowedScene(play) && MirrorHost::PackAvailable()) {
                sPromptAccepted = true;
                Message_CloseTextbox(play);
                MirrorHost::Trace("yes-closing", play);
            } else {
                MirrorHost::Trace(yes ? "yes-ineligible" : "no-closing", play);
                ReleasePrompt(true);
            }
        } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) {
            ReleasePrompt(false);
        }
        return;
    }
    if (!MagicMirror_IsOwned()) {
        if (!sPickupToken && Safe(play, false) && Forest(play) && play->roomCtx.curRoom.num == 0 && Locate(play)) {
            const auto& p = GET_PLAYER(play)->actor.world.pos;
            if (MagicMirrorAnchor::CanClaim({ p.x, p.y, p.z }, sAnchor) && OnStumpCap(play, GET_PLAYER(play))) {
                const int token = BeginPickup();
                if (token && !MirrorHost::QueuePickup(token))
                    sPickupToken = 0;
            }
        }
        return;
    }
    const u16 pressed = PressedButton(play);
    if (!pressed)
        return;
    if (!Safe(play, false) || !AllowedScene(play) || !MirrorHost::PackAvailable()) {
        MirrorHost::Error();
        return;
    }
    sPrompt = true;
    sPromptPlay = play;
    sPromptPlayer = GET_PLAYER(play);
    HoldPrompt();
    play->state.input[0].press.button &= ~pressed;
    MirrorHost::OpenPrompt();
    MirrorHost::Trace("prompt-open", play);
}
void DrawWorld() {
    auto* play = gPlayState;
    if (play && Mode() && Matches(play) && play == sCastPlay && GET_PLAYER(play) == sCastPlayer &&
        MagicMirrorPlayer_State(sCastPlayer) >= MAGIC_MIRROR_CAST_PLAYING) {
        // Keep Farore's exact motion. The established Time Gate seal/wisps
        // build around the actor, then dissolve during departure; no GI model.
        const float rise = std::clamp(sCastPlayer->skelAnime.curFrame / 20.0f, 0.0f, 1.0f);
        const float fade = sDeparted ? std::clamp(1.0f - (sCastTicks - sDepartureTick) / 12.0f, 0.0f, 1.0f) : 1.0f;
        if (rise > 0 && fade > 0)
            MirrorHost::DrawCastAura(play, sCastPlayer, 0.15f + 0.85f * rise, 255.0f * rise * fade);
    }
    if (!play || !Mode() || !Forest(play) || play->roomCtx.curRoom.num != 0 || MagicMirror_IsOwned() || sPickupToken ||
        !Locate(play))
        return;
    Matrix_Push();
    Matrix_Translate(sAnchor.x, sAnchor.y + 42 + std::sin(play->gameplayFrames * 0.05f) * 3, sAnchor.z, MTXMODE_NEW);
#ifdef COMBO_MAGIC_MIRROR_MM
    Matrix_RotateYF(play->gameplayFrames * 0.025f, MTXMODE_APPLY);
#else
    Matrix_RotateY(play->gameplayFrames * 0.025f, MTXMODE_APPLY);
#endif
    ComboMagicMirrorVisual::Draw(play);
    MirrorHost::DrawWorldShimmer(play);
    Matrix_Pop();
}
} // namespace MirrorRuntime

extern "C" int MagicMirror_PromptOwnsPlayer(PlayState* play, const Player* player) {
    return MirrorRuntime::PromptOwnsPlayer(play, player);
}
extern "C" void MagicMirror_QueueArrival(void) {
    MirrorRuntime::QueueArrival();
}
extern "C" void MagicMirror_PrepareArrival(PlayState* play) {
    MirrorRuntime::PrepareArrival(play);
}
extern "C" void MagicMirror_FinishArrival(PlayState* play) {
    MirrorRuntime::FinishArrival(play);
}
