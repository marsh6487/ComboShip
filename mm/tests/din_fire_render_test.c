#include "din_fire_fixture.h"

static void Tick(void) {
    ++play.gameplayFrames;
    DinFireSword_Update(&play, &player);
    DinFireShield_Update(&play, &player);
}
static void SetupEnabled(void) {
    Setup();
    swordOption = shieldOption = damageOption = 1;
    DinFireSword_Reset();
    DinFireShield_Reset();
}
static size_t DrawSword(void) {
    ResetBuffers();
    Player before = player;
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_Draw(&play, &player);
    DinFireSword_DrawAfterPlayer(&play, &player);
    REQUIRE(!memcmp(&before, &player, sizeof(player)) && matrixDepth == 0);
    return (gfx.polyOpa.p - opa) + (gfx.polyXlu.p - xlu);
}
static size_t DrawShield(void) {
    ResetBuffers();
    Player before = player;
    DinFireShield_Draw(&play, &player);
    REQUIRE(!memcmp(&before, &player, sizeof(player)) && matrixDepth == 0);
    return gfx.polyXlu.p - xlu;
}
static void TestOwnership(void) {
    Setup();
    DinFireSword_Reset();
    DinFireShield_Reset();
    Tick();
    REQUIRE(DrawSword() == 0 && DrawShield() == 0); // unset options stay off
    REQUIRE(DinFireSword_DamageFlags(&play, &player, DMG_SWORD) == DMG_SWORD);
    REQUIRE(DinFireSword_HandDL(&play, &player, hand) == NULL);
    REQUIRE(DinFireShield_HandDL(&play, &player, hand) == NULL);
    for (int guard = 0; guard < 10; ++guard) {
        SetupEnabled();
        switch (guard) {
            case 0:
                alt = 0;
                break;
            case 1:
                assets = 0;
                break;
            case 2:
                customForm = CUSTOM_FORM_GERUDO;
                break;
            case 3:
                player.transformation = PLAYER_FORM_GORON;
                break;
            case 4:
                bossOwner = 1;
                break;
            case 5:
                bossOwner = 2;
                break;
            case 6:
                player.csAction = 1;
                break;
            case 7:
                play.transitionTrigger = TRANS_TRIGGER_START;
                break;
            case 8:
                player.stateFlags1 |= PLAYER_STATE1_DEAD;
                break;
            case 9:
                player.stateFlags2 |= PLAYER_STATE2_20000000;
                break;
        }
        Tick();
        REQUIRE(DrawSword() == 0 && DrawShield() == 0);
        REQUIRE(DinFireSword_HandDL(&play, &player, hand) == NULL);
        REQUIRE(DinFireShield_HandDL(&play, &player, hand) == NULL);
        REQUIRE(DinFireSword_DamageFlags(&play, &player, DMG_SWORD) == DMG_SWORD);
    }
    SetupEnabled();
    player.heldItemId = ITEM_NET; // borrowed sword action must not claim net geometry
    Tick();
    REQUIRE(DrawSword() == 0 && DinFireSword_HandDL(&play, &player, hand) == NULL);
    REQUIRE(DinFireSword_DamageFlags(&play, &player, DMG_SWORD) == DMG_SWORD);
    SetupEnabled();
    hideSword = hideShield = 1;
    Tick();
    REQUIRE(DrawSword() == 0 && DrawShield() == 0);
    SetupEnabled();
    kokiriUpgrade = 1;
    Tick();
    REQUIRE(DrawSword() == 0);
    SetupEnabled();
    player.currentShield = PLAYER_SHIELD_MIRROR_SHIELD;
    Tick();
    REQUIRE(DrawShield() == 0 && DinFireShield_HandDL(&play, &player, hand) == NULL);
    SetupEnabled();
    Tick();
    Player other = player;
    ResetBuffers();
    DinFireSword_BeginPlayerDraw(&play, &other);
    DinFireSword_Draw(&play, &other);
    DinFireSword_DrawAfterPlayer(&play, &other);
    DinFireShield_Draw(&play, &other);
    REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
    REQUIRE(DinFireSword_HandDL(&play, &other, hand) == NULL);
    REQUIRE(DinFireShield_HandDL(&play, &other, hand) == NULL);
    puts("PASS MM Din: default-off, native/forced forms, custom weapons, boss/remote ownership and fallback guards");
}
static void TestProfilesAndHandCapture(void) {
    const char* profiles[] = { "/child/", "/adult/", "/bgs/" };
    for (int profile = 0; profile < 3; ++profile) {
        SetupEnabled();
        adult = profile != 0; // raw saved timeGateAdultMode deliberately remains zero
        if (profile == 1) {
            player.heldItemId = ITEM_SWORD_MASTER;
            player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_MASTER;
        } else if (profile == 2) {
            player.heldItemId = ITEM_SWORD_BGS;
            player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_TWO_HANDED;
            player.leftHandType = PLAYER_MODELTYPE_LH_TWO_HAND_SWORD;
        }
        Tick();
        Player before = player;
        Gfx* swordHand = DinFireSword_HandDL(&play, &player, hand);
        if (profile < 2) {
            REQUIRE(swordHand && HasLayer(swordHand, swordHand + 4, hand));
            REQUIRE(HasLayer(swordHand, swordHand + 4, &blade[adult ? 0 : 1]));
            if (adult)
                REQUIRE(HasColor(swordHand, swordHand + 5, G_SETENVCOLOR, 0x1E691B));
        } else {
            REQUIRE(swordHand == NULL); // native Biggoron geometry keeps ownership
        }
        Gfx* shieldHand = DinFireShield_HandDL(&play, &player, hand);
        REQUIRE(shieldHand && HasLayer(shieldHand, shieldHand + 4, hand));
        REQUIRE(HasLayer(shieldHand, shieldHand + 4, &bracer[adult ? 0 : 1]));
        if (adult)
            REQUIRE(HasColor(shieldHand, shieldHand + 5, G_SETENVCOLOR, 0x1E691B));
        REQUIRE(!memcmp(&before, &player, sizeof(player)));

        ResetBuffers();
        currentMatrix.xw = 91;
        currentMatrix.yw = -32;
        currentMatrix.zw = 67;
        MtxF wanted = currentMatrix;
        DinFireSword_BeginPlayerDraw(&play, &player);
        DinFireSword_Draw(&play, &player);
        REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu); // no body material contamination
        currentMatrix.xw = 999;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(matrixCount == 1 && !memcmp(&captured[0], &wanted, sizeof(wanted)));
        REQUIRE(currentMatrix.xw == 999 && matrixDepth == 0);
        REQUIRE(HasLayer(opa, gfx.polyOpa.p, core) && HasLayer(xlu, gfx.polyXlu.p, flame));
        REQUIRE(strstr(lastCorePath, profiles[profile]));
        RequireNamedTexture(opa, gfx.polyOpa.p, "__OTR__objects/din_fire_sword/poc1/CoreTex");
        RequireNamedTexture(xlu, gfx.polyXlu.p, "__OTR__objects/din_fire_sword/poc1/FlameTex");
        Gfx* end = gfx.polyXlu.p;
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(gfx.polyXlu.p == end);
    }
    SetupEnabled();
    Tick();
    player.actor.scale.y = -.01f;
    REQUIRE(DrawSword() == 0 && DrawShield() == 0);
    player.actor.scale.y = .01f;
    REQUIRE(DrawSword() > 0 && DrawShield() > 0); // reflection cannot erase foreground state
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_Draw(&play, &player);
    ResetBuffers();
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_DrawAfterPlayer(&play, &player);
    REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
    puts("PASS MM Din: child/adult/BGS hand composition, captured pose, opaque core, no duplicate/reflection leakage");
}
static void TestPauseResourcesAndShield(void) {
    SetupEnabled();
    Tick();
    REQUIRE(DrawSword() > 0);
    Gfx savedOpa[512], savedXlu[512];
    memcpy(savedOpa, opa, sizeof(savedOpa));
    memcpy(savedXlu, xlu, sizeof(savedXlu));
    play.pauseCtx.state = 6;
    Tick();
    DrawSword();
    REQUIRE(!memcmp(savedOpa, opa, sizeof(savedOpa)) && !memcmp(savedXlu, xlu, sizeof(savedXlu)));
    play.pauseCtx.state = 0;
    Tick();
    DrawSword();
    REQUIRE(memcmp(savedXlu, xlu, sizeof(savedXlu)) != 0);
    customColors = 1;
    DrawSword();
    REQUIRE(HasColor(opa, gfx.polyOpa.p, G_SETPRIMCOLOR, 0x29F1C7));
    REQUIRE(HasColor(xlu, gfx.polyXlu.p, G_SETENVCOLOR, 0x0C21B1));
    swordOption = shieldOption = 0;
    REQUIRE(DrawSword() == 0 && DrawShield() == 0); // pause-time changes apply at draw

    const char* swordDeps[] = { "DinSleekEquipment", "CoreDL",  "FlameDL", "CoreVertices",
                                "FlameVertices",     "CoreTex", "FlameTex" };
    for (size_t i = 0; i < ARRAY_COUNT(swordDeps); ++i) {
        SetupEnabled();
        Tick();
        missing = swordDeps[i];
        REQUIRE(DrawSword() == 0 && DinFireSword_HandDL(&play, &player, hand) == NULL);
        REQUIRE(DinFireSword_DamageFlags(&play, &player, DMG_SWORD) == DMG_SWORD);
    }
    const char* shieldDeps[] = { "DinSleekEquipment", "SurfaceDL", "RimDL",   "SurfaceVertices",
                                 "RimVertices",       "FlowTex",   "FlameTex" };
    for (size_t i = 0; i < ARRAY_COUNT(shieldDeps); ++i) {
        SetupEnabled();
        Tick();
        missing = shieldDeps[i];
        REQUIRE(DrawShield() == 0 && DinFireShield_HandDL(&play, &player, hand) == NULL);
    }
    SetupEnabled();
    soundOption = 1;
    player.actor.sfxId = 0x1234;
    Tick();
    REQUIRE(audioRequests == 1 && soundActor == &player.actor);
    REQUIRE(lastSound == NA_SE_PL_ARROW_CHARGE_FIRE - SFX_FLAG && player.actor.sfxId == 0x1234);
    REQUIRE(DrawShield() > 0);
    RequireNamedTexture(xlu, gfx.polyXlu.p, "__OTR__objects/din_fire_shield/poc1/FlowTex");
    RequireNamedTexture(xlu, gfx.polyXlu.p, "__OTR__objects/din_fire_shield/poc1/FlameTex");
    memcpy(savedXlu, xlu, sizeof(savedXlu));
    play.pauseCtx.state = 6;
    Tick();
    DrawShield();
    REQUIRE(audioRequests == 1 && !memcmp(savedXlu, xlu, sizeof(savedXlu)));
    play.pauseCtx.state = 0;
    for (int i = 0; i < 6; ++i)
        Tick();
    player.stateFlags1 = 0;
    Tick();
    REQUIRE(DrawShield() > 0);
    for (int i = 0; i < 8; ++i)
        Tick();
    REQUIRE(DrawShield() == 0 && audioRequests == 7);
    SetupEnabled();
    REQUIRE(DinFireShield_ItemIcon(ITEM_SHIELD_HERO) != NULL);
    REQUIRE(DinFireShield_ItemIcon(ITEM_SHIELD_MIRROR) == NULL);
    REQUIRE(DinFireShield_DrawItem(&play, GID_SHIELD_HERO));
    REQUIRE(gfx.polyOpa.p > opa && gfx.polyXlu.p > xlu && matrixDepth == 0);
    ResetBuffers();
    missing = "GIBracerVertices";
    REQUIRE(!DinFireShield_DrawItem(&play, GID_SHIELD_HERO));
    REQUIRE(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
    puts("PASS MM Din: pause clock, live colors, missing dependencies, shield fade/SFX, icon and native item fallback");
}
int main(void) {
    TestOwnership();
    TestProfilesAndHandCapture();
    TestPauseResourcesAndShield();
    return 0;
}
