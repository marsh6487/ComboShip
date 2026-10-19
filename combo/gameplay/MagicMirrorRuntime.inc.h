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
static bool sPrompt = false, sAnchorValid = false;
static const CollisionHeader* sCollision = nullptr;
static MagicMirrorAnchor::Point sAnchor{};
static PlayState* sPromptPlay = nullptr;
static Player* sPromptPlayer = nullptr;
static u32 sPromptAddedFlags = 0;
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
    sPromptPlay = nullptr;
    sPromptPlayer = nullptr;
    sPromptAddedFlags = 0;
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
    sContext = {};
    sPickupToken = 0;
    sPrompt = sAnchorValid = false;
    sCollision = nullptr;
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
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger || MirrorHost::TimedActivity())
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
bool Travel(PlayState* play) {
    if (!MagicMirror_IsOwned() || !Safe(play, false) || !AllowedScene(play) || !MirrorHost::PackAvailable())
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
    // Use the ordinary safe forest spawn; TOP/Farore and RETURN/grotto data
    // retain their owning systems' contents. Native age/form and clock stay put.
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
u16 PressedButton(PlayState* play) {
    static constexpr u16 buttons[] = { 0, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT, BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT };
    for (s32 index = 1; index < 8; ++index)
        if ((play->state.input[0].press.button & buttons[index]) && !MirrorHost::ButtonClaimed(buttons[index]) &&
            MirrorHost::ButtonItem(index) == EXT_ITEM_MAGIC_MIRROR)
            return buttons[index];
    return 0;
}
void Update() {
    auto* play = gPlayState;
    Bind(play);
    if (!play || !Mode())
        return;
    if (sPrompt) {
        const auto text = CurrentText(play);
        if (text != kPromptText || !Safe(play, true) || !MagicMirror_IsOwned()) {
            ReleasePrompt(text == kPromptText);
            return;
        }
        if (play->state.input[0].press.button & BTN_B) {
            ReleasePrompt(true);
            return;
        }
        if (Message_GetState(&play->msgCtx) == TEXT_STATE_CHOICE && Message_ShouldAdvance(play)) {
            const bool yes = play->msgCtx.choiceIndex == 0;
            ReleasePrompt(true);
            // The message has completed, but its final native closing frame
            // can still report a textbox. Recheck every other safety gate.
            if (yes && Safe(play, true) && AllowedScene(play) && MirrorHost::PackAvailable()) {
                play->msgCtx.msgMode = MSGMODE_NONE;
                if (!Travel(play))
                    MirrorHost::Error();
            }
        } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) {
            ReleasePrompt(false);
        }
        return;
    }
    if (!MagicMirror_IsOwned()) {
        if (!sPickupToken && Safe(play, false) && Forest(play) && play->roomCtx.curRoom.num == 0 && Locate(play)) {
            const auto& p = GET_PLAYER(play)->actor.world.pos;
            if (MagicMirrorAnchor::CanClaim({ p.x, p.y, p.z }, sAnchor)) {
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
    sPromptAddedFlags = kPromptFreeze & ~sPromptPlayer->stateFlags1;
    sPromptPlayer->stateFlags1 |= kPromptFreeze;
    play->state.input[0].press.button &= ~pressed;
    MirrorHost::OpenPrompt();
}
void DrawWorld() {
    auto* play = gPlayState;
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
    Matrix_Pop();
}
} // namespace MirrorRuntime
