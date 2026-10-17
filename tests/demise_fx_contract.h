/* Runs against the actual shared ribbon builder and each host's item/draw
 * entrypoints. */
static s32 drawOwnedFx(void) {
  gfx.polyXlu.p = commands;
  gfx.polyOpa.p = (Gfx *)ownedArena.data;
  gfx.polyOpa.d = (Gfx *)(ownedArena.data + sizeof(ownedArena.data));
  CustomItems_DrawDemiseDestruction(&player, &play);
  REQUIRE(gfx.polyXlu.p - commands < 512);
  if (gfx.polyXlu.p > commands) {
    int cycle = 0, translucent = 0, alpha = 0, texture = 0, batches = 0,
        matrixLoads = 0;
    for (Gfx *cmd = commands; cmd < gfx.polyXlu.p; ++cmd) {
      u32 op = cmd->words.w0 >> 24;
      if (op == G_SETOTHERMODE_H &&
          ((cmd->words.w0 >> 8) & 0xFF) == 32 - G_MDSFT_CYCLETYPE - 2 &&
          (cmd->words.w0 & 0xFF) == 1 && cmd->words.w1 == G_CYC_1CYCLE)
        ++cycle;
      if (op == G_SETOTHERMODE_L &&
          cmd->words.w1 == (G_RM_AA_ZB_XLU_SURF | G_RM_AA_ZB_XLU_SURF2))
        ++translucent;
      if (op == G_SETOTHERMODE_L &&
          ((cmd->words.w0 >> 8) & 0xFF) == 32 - G_MDSFT_ALPHACOMPARE - 2 &&
          (cmd->words.w0 & 0xFF) == 1 && cmd->words.w1 == G_AC_NONE)
        ++alpha;
      if (op == G_SETTIMG && cmd->words.w1 == (uintptr_t)sDemiseFxStrip)
        ++texture;
      if (op == G_MTX)
        ++matrixLoads;
      if (op == G_VTX) {
        int n = (cmd->words.w0 >> 12) & 0xFF;
        REQUIRE(n > 0 && n <= 32 && n % 4 == 0);
        ++batches;
      }
    }
    REQUIRE(cycle == 1 && translucent == 1 && alpha == 1 && texture == 1 &&
            matrixLoads == 1 && batches > 20);
  }
  return (s32)(gfx.polyXlu.p - commands);
}

static void testOwnedFxGeometryAndLifetime(void) {
  reset();
  play.gameplayFrames = 100;
  Demise_Start(&player, &play);
  ddTimer = 35;
  REQUIRE(drawOwnedFx() > 0);
  REQUIRE(ddTimer == 35 && collisionCalls == 0 && magicSpent == 12);
  play.pauseCtx.state = 1;
  REQUIRE(drawOwnedFx() ==
          0); // The equipment preview must not draw world-space electricity.
  play.pauseCtx.state = 0;
  REQUIRE(drawOwnedFx() > 0);
  Demise_FinalDischarge(&play, &player);
  ddState = DEMISE_STATE_FINISH;
  ddTimer = 0;
  for (int i = 0; i < 5; ++i) {
    Demise_StateFinish(&player, &play);
    ++play.gameplayFrames;
  }
  REQUIRE(!ddActive && collisionCalls == 5 && bombCount == 0);
  REQUIRE(drawOwnedFx() > 0); // Visual afterglow survives the five-frame
                              // attack, without another hit.
  player.actor.world.pos.x += 50.0f;
  REQUIRE(drawOwnedFx() > 0);
  REQUIRE(renderedMatrix.xw ==
          100.0f); // The discharge stays at its impact location.
  play.gameplayFrames = 127;
  REQUIRE(drawOwnedFx() > 0);
  play.gameplayFrames = 128;
  REQUIRE(drawOwnedFx() == 0);
  REQUIRE(collisionCalls == 5 && magicSpent == 12);

  s32 peak = 0;
  for (int phase = 1; phase <= 2; ++phase) {
    for (int tick = 0; tick < 29; ++tick) {
      s32 count =
          DemiseFx_Build(ownedVertices, 35 + tick, 65530 + tick, phase, tick);
      REQUIRE(count <= 384);
      if (count > peak)
        peak = count;
      for (int v = 0; v < count * 4; ++v) {
        Vtx *vertex = &ownedVertices[v];
        REQUIRE(hypotf(vertex->v.ob[0], vertex->v.ob[2]) <= 402.0f);
        REQUIRE(vertex->v.ob[1] >= 2 && vertex->v.ob[1] <= 64);
        REQUIRE(vertex->v.cn[3] > 0);
      }
      if (phase == 2 && tick == 28)
        REQUIRE(count == 0);
    }
  }
  REQUIRE(peak > 200 && peak < 384);
  printf("PASS owned Demise packets: peak %d quads, anchored afterglow, "
         "unchanged five damage ticks\n",
         peak);
}

static void testOwnedFxInvalidationAndAllocation(void) {
  for (int reason = 0; reason < 5; ++reason) {
    reset();
    play.gameplayFrames = 100;
    Demise_Start(&player, &play);
    ddTimer = 35;
    REQUIRE(drawOwnedFx() > 0);
    if (reason == 0)
      input.wasEquipped = 0;
    if (reason == 1)
      ddActive = 0;
    if (reason == 2)
      play.gameplayFrames = 99;
    if (reason == 3) {
#ifdef DEMISE_TEST_MM
      ++play.sceneId;
#else
      ++play.sceneNum;
#endif
    }
    if (reason == 4)
      sDemiseFx.play = NULL;
    REQUIRE(drawOwnedFx() == 0);
    REQUIRE(sDemiseFx.phase == 0 && bombCount == 0);
  }
  reset();
  play.gameplayFrames = 100;
  Demise_Start(&player, &play);
  ddTimer = 35;
  gfx.polyXlu.p = commands;
  gfx.polyOpa.d = gfx.polyOpa.p; // No tail space: the real bounded allocator
                                 // rejects the new geometry.
  CustomItems_DrawDemiseDestruction(&player, &play);
  REQUIRE(gfx.polyXlu.p == commands && magicSpent == 12 && collisionCalls == 0);
  puts("PASS owned Demise cancellation, scene/clock invalidation and graphics "
       "allocation fallback");
}

static void testOwnedFxForeignPlayerAndCommandSpace(void) {
  reset();
  play.gameplayFrames = 100;
  Demise_Start(&player, &play);
  ddTimer = 35;
  REQUIRE(drawOwnedFx() > 0);
  Player remote = player;
  ddActive =
      0; // Harpoon temporarily installs the remote player's visual flags.
  u8 phase = sDemiseFx.phase;
  gfx.polyXlu.p = commands;
  CustomItems_DrawDemiseDestruction(&remote, &play);
  REQUIRE(gfx.polyXlu.p == commands && sDemiseFx.phase == phase);
  ddActive = 1;
  gfx.polyXlu.p = commands;
  gfx.polyXlu.d = commands; // Command space is exhausted even though vertex
                            // space is available.
  CustomItems_DrawDemiseDestruction(&player, &play);
  REQUIRE(gfx.polyXlu.p == commands && magicSpent == 12 && collisionCalls == 0);
}

/* Optional production-geometry export, consumed by the offline preview
 * renderer. */
static void exportOwnedFx(void) {
  const char *path = getenv("DEMISE_FX_EXPORT");
  if (!path)
    return;
  FILE *out = fopen(path, "w");
  REQUIRE(out != NULL);
  fputs("[", out);
  for (int tick = 21; tick < 98; ++tick) {
    int phase = tick < 70 ? 1 : 2;
    s32 count = DemiseFx_Build(ownedVertices, tick, tick, phase,
                               tick < 70 ? 0 : tick - 70);
    fprintf(out, "%s{\"tick\":%d,\"phase\":%d,\"quads\":[",
            tick == 21 ? "" : ",", tick, phase);
    for (s32 q = 0; q < count; ++q) {
      fprintf(out, "%s[", q ? "," : "");
      for (s32 v = 0; v < 4; ++v) {
        Vtx *vertex = &ownedVertices[q * 4 + v];
        fprintf(out, "%s[%d,%d,%d,%u,%u,%u,%u,%d,%d]", v ? "," : "",
                vertex->v.ob[0], vertex->v.ob[1], vertex->v.ob[2],
                vertex->v.cn[0], vertex->v.cn[1], vertex->v.cn[2],
                vertex->v.cn[3], vertex->v.tc[0], vertex->v.tc[1]);
      }
      fputs("]", out);
    }
    fputs("]}", out);
  }
  fputs("]\n", out);
  REQUIRE(fclose(out) == 0);
}

static void testWorldFxBeforeModelFork(void) {
  reset();
  play.gameplayFrames = 100;
  Demise_Start(&player, &play);
  ddTimer = 35;
  gfx.polyXlu.p = commands;
  Player_Draw(&player.actor, &play);
  REQUIRE(gfx.polyXlu.p > commands); // Mario/custom-model early returns cannot
                                     // hide the world effect.
  REQUIRE(magicSpent == 12 && collisionCalls == 0);
}

static void testOwnedFxArenaMarkerHeadroom(void) {
  reset();
  // Two nested Debug OPEN_DISPS markers need headroom after the native matrix
  // allocation.
  size_t bytes = 384 * 4 * sizeof(Vtx);
  gfx.polyOpa.d = (Gfx *)(ownedArena.data + bytes + sizeof(Mtx) + sizeof(Gfx));
  REQUIRE(Demise_AllocDraw(&gfx, bytes) == NULL);
  gfx.polyOpa.d =
      (Gfx *)(ownedArena.data + bytes + sizeof(Mtx) + 2 * sizeof(Gfx));
  REQUIRE(Demise_AllocDraw(&gfx, bytes) != NULL);
  MtxF floor;
  SkinMatrix_SetScale(&floor, 1.0f, 1.0f, 1.0f);
  Mtx *matrix = SkinMatrix_MtxFToNewMtx(&gfx, &floor);
  REQUIRE((uintptr_t)matrix >= (uintptr_t)gfx.polyOpa.p + 2 * sizeof(Gfx));
}
