// Reuse actual native transition/save boundaries; only statue graphics and
// sound are outside this warp ownership regression.
#define main ExistingRuntimeFixtureMain
#include "tests/solo_mm_entrances/runtime_test.cpp"
#undef main

extern "C" void Actor_PlaySfx(Actor*, u16) {}
Actor* SpiritualStoneStatue_Spawn(PlayState*, Vec3f*, s16, int) { return nullptr; }
SpiritualStonesSaveData& State() { return gSaveContext.save.shipSaveInfo.spiritualStones; }

/* NATIVE_STONE_FUNCTIONS */

int main() {
    gPlayState = &play;
    InitNativeRegions();
    RegisterNativeHooks();
    auto catalogue = ConfigureNativeMap(4);
    SwapEntrances(catalogue, ENTRANCE(ODOLWAS_LAIR, 0), ENTRANCE(GOHTS_LAIR, 0));
    GameInteractor_ExecuteOnSaveLoad(0);
    play.sceneId = SCENE_MITURIN;
    const int goht = Transition(ENTRANCE(ODOLWAS_LAIR, 0));
    REQUIRE(goht == ENTRANCE(GOHTS_LAIR, 0));
    ObserveArrival(SCENE_HAKUGIN_BS, goht);
    Player player{};
    player.actor.world.pos = { 12, 34, -56 };
    player.actor.shape.rot.y = 1234;
    SummonStatueHere(&play, &player, 0);
    const int captured = Combo_MM_SaveCrossDungeonCheckpoint();

    play.sceneId = SCENE_SEA;
    const int gyorg = Transition(ENTRANCE(GYORGS_LAIR, 0));
    ObserveArrival(SCENE_SEA_BS, gyorg);
    player.actor.world.pos = { 98, 76, -54 };
    SummonStatueHere(&play, &player, 1);
    REQUIRE(State().warp[1].soloEntranceCheckpoint != captured);
    ExecuteWarp(&play, 0);
    REQUIRE(gSaveContext.respawnFlag == 3);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x == 12);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 34);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.z == -56);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_TOP].yaw == 1234);
    const int recalled = Transition(play.nextEntrance);
    REQUIRE(recalled == goht);
    ObserveArrival(SCENE_HAKUGIN_BS, recalled);
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == captured);
    Combo_MM_RequestCrossBossDeathRecovery();
    REQUIRE(Transition(ENTRANCE(SNOWHEAD_TEMPLE, 0)) == ENTRANCE(WOODFALL_TEMPLE, 0));

    // Two logical grotto identities share physical room4. Recall must restore
    // the saved contents and exit metadata while retaining the point's position.
    catalogue = ConfigureNativeMap(64);
    SwapEntrances(catalogue, ENTRANCE(GROTTOS, 18), ENTRANCE(GROTTOS, 19));
    GameInteractor_ExecuteOnSaveLoad(0);
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x9a;
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GROTTOS, 4));
    ObserveArrival(SCENE_KAKUSIANA, ENTRANCE(GROTTOS, 4));
    play.roomCtx.curRoom.num = 4;
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(GROTTOS, 19));
    player.actor.world.pos = { 123, 45, -67 };
    SummonStatueHere(&play, &player, 2);
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x3f;
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GROTTOS, 4));
    ObserveArrival(SCENE_KAKUSIANA, ENTRANCE(GROTTOS, 4));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(GROTTOS, 18));
    REQUIRE(static_cast<uint8_t>(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data) == 0x9a);
    ExecuteWarp(&play, 2);
    REQUIRE(gSaveContext.respawnFlag == 3);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x == 123);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data == 0x3f);
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(GROTTOS, 4));
    ObserveArrival(SCENE_KAKUSIANA, ENTRANCE(GROTTOS, 4));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(GROTTOS, 19));
    gSaveContext.respawnFlag = 4;
    REQUIRE(Transition(0xffff) == ENTRANCE(TERMINA_FIELD, 0));
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 2367);

    // New Solo ownership metadata does not install or alter combined routing.
    gComboGameMode = COMBO_GAME_BOTH;
    const int before = ComboSoloMmRuntime::checkpoint.Saved();
    SummonStatueHere(&play, &player, 0);
    REQUIRE(State().warp[0].soloEntranceCheckpoint == -1);
    ExecuteWarp(&play, 0);
    REQUIRE(ComboSoloMmRuntime::checkpoint.Saved() == before);
    REQUIRE(foreignCalls == 0);
    std::puts("PASS native Spiritual Stone capture/recall: independent points, source boss recovery, shared grotto identity and combined isolation");
}
