#pragma once
// Owner-authored GI miniatures. Paths/rig layouts come from the OoT actor/resource
// definitions named on each row; this catalog never instantiates an Actor.
// Group representatives: Lizalfos, Biri, Redead, red Gerudo fighter; child Ruto,
// courtyard Zelda, child Malon, first carpenter, male Kokiri, market citizen (1),
// graveyard kid (Bombers), Flat (composers), Ingo (Gorman), and native EnHy roles
// 7/0/5 (astronomer/dog lady/banker). These are native OoT semantic identities.
#include "ComboItemDrawABI.h"
#include "ComboSoulModelSettings.h"
#include <initializer_list>

namespace ComboSoulMiniOOT {
struct Model {
    int32_t id;
    const char* skel;
    const char* anim;
    int32_t joints;
    int32_t nonFlex;
    float scale;
    float centerY;
};
inline constexpr Model Catalog[] = {
    { RG_SOUL_ENEMY_STALFOS, "__OTR__objects/object_sk2/gStalfosSkel",
      "__OTR__objects/object_sk2/gStalfosMiddleGuardAnim", 61, 1, 0.006f, -20.0f }, // En_Test
    { RG_SOUL_ENEMY_OCTOROK, "__OTR__objects/object_okuta/gOctorokSkel",
      "__OTR__objects/object_okuta/gOctorokFloatAnim", 38, 1, 0.009f, -15.0f }, // En_Okuta
    { RG_SOUL_ENEMY_WALLMASTER, "__OTR__objects/object_wallmaster/gWallmasterSkel",
      "__OTR__objects/object_wallmaster/gWallmasterWaitAnim", 25, 0, 0.007f, -8.0f }, // En_Wallmas
    { RG_SOUL_ENEMY_DODONGO, "__OTR__objects/object_dodongo/gDodongoSkel",
      "__OTR__objects/object_dodongo/gDodongoWaitAnim", 31, 1, 0.009f, -10.0f }, // En_Dodongo
    { RG_SOUL_ENEMY_KEESE, "__OTR__objects/object_firefly/gKeeseSkeleton",
      "__OTR__objects/object_firefly/gKeeseFlyAnim", 28, 1, 0.016f, 0.0f }, // En_Firefly
    { RG_SOUL_ENEMY_TEKTITE, "__OTR__objects/object_tite/object_tite_Skel_003A20",
      "__OTR__objects/object_tite/object_tite_Anim_0012E4", 25, 1, 0.012f, -8.0f }, // En_Tite
    { RG_SOUL_ENEMY_LEEVER, "__OTR__objects/object_reeba/object_reeba_Skel_001EE8",
      "__OTR__objects/object_reeba/object_reeba_Anim_0001E4", 18, 1, 0.022f, -15.0f }, // En_Reeba
    { RG_SOUL_ENEMY_PEAHAT, "__OTR__objects/object_peehat/gPeehatSkel",
      "__OTR__objects/object_peehat/gPeehatFlyingAnim", 24, 1, 0.012f, -12.0f }, // En_Peehat
    { RG_SOUL_ENEMY_LIZALFOS_DINOLFOS, "__OTR__objects/object_zf/gZfLizalfosSkel",
      "__OTR__objects/object_zf/gZfWalkingAnim", 49, 1, 0.009f, -20.0f }, // En_Zf
    { RG_SOUL_ENEMY_GOHMA_LARVA, "__OTR__objects/object_gol/gObjectGolSkel",
      "__OTR__objects/object_gol/gObjectGolStandAnim", 24, 1, 0.008f, -10.0f }, // En_Goma
    { RG_SOUL_ENEMY_SHABOM, nullptr, nullptr, 0, 1, 0.5f, 0.0f },               // En_Bubble
    { RG_SOUL_ENEMY_BABY_DODONGO, "__OTR__objects/object_dodojr/object_dodojr_Skel_0020E0",
      "__OTR__objects/object_dodojr/object_dodojr_Anim_0009D4", 15, 1, 0.018f, -10.0f }, // En_Dodojr
    { RG_SOUL_ENEMY_DARK_LINK, "__OTR__objects/object_torch2/gDarkLinkSkel", nullptr, 22, 0, 0.006f,
      -20.0f }, // En_Torch2
    { RG_SOUL_ENEMY_BIRI_BARI, "__OTR__objects/object_bl/gBiriSkel", "__OTR__objects/object_bl/gBiriDefaultAnim", 5, 1,
      0.012f, 0.0f },                                                   // En_Bili
    { RG_SOUL_ENEMY_TAILPASARN, nullptr, nullptr, 0, 1, 0.5f, 0.0f },   // En_Tp
    { RG_SOUL_ENEMY_PARASITE, nullptr, nullptr, 0, 1, 0.008f, -20.0f }, // En_Ba
    { RG_SOUL_ENEMY_SKULLTULA, "__OTR__objects/object_st/object_st_Skel_005298",
      "__OTR__objects/object_st/object_st_Anim_000304", 30, 1, 0.022f, 0.0f }, // En_St
    { RG_SOUL_ENEMY_TORCH_SLUG, "__OTR__objects/object_bw/gTorchSlugSkel",
      "__OTR__objects/object_bw/gTorchSlugEyestalkWaveAnim", 12, 1, 0.013f, -10.0f }, // En_Bw
    { RG_SOUL_ENEMY_MOBLIN, "__OTR__objects/object_mb/gEnMbSpearSkel",
      "__OTR__objects/object_mb/gEnMbSpearStandStillAnim", 28, 0, 0.006f, -25.0f }, // En_Mb
    { RG_SOUL_ENEMY_ARMOS, "__OTR__objects/object_am/gArmosSkel", "__OTR__objects/object_am/gArmosHopAnim", 14, 1,
      0.01f, -15.0f }, // En_Am
    { RG_SOUL_ENEMY_DEKU_BABA, "__OTR__objects/object_dekubaba/gDekuBabaSkel",
      "__OTR__objects/object_dekubaba/gDekuBabaFastChompAnim", 8, 1, 0.006f, -20.0f }, // En_Dekubaba
    { RG_SOUL_ENEMY_DEKU_SCRUB, "__OTR__objects/object_dekunuts/gDekuNutsSkel",
      "__OTR__objects/object_dekunuts/gDekuNutsLookAroundAnim", 25, 1, 0.014f, -15.0f }, // En_Dekunuts
    { RG_SOUL_ENEMY_BUBBLE, "__OTR__objects/object_Bb/object_Bb_Skel_001A30",
      "__OTR__objects/object_Bb/object_Bb_Anim_000444", 16, 1, 0.018f, 0.0f }, // En_Bb
    { RG_SOUL_ENEMY_BEAMOS, "__OTR__objects/object_vm/gBeamosSkel", "__OTR__objects/object_vm/gBeamosAnim", 11, 1,
      0.01f, -20.0f }, // En_Vm
    { RG_SOUL_ENEMY_FLOORMASTER, "__OTR__objects/object_wallmaster/gWallmasterSkel",
      "__OTR__objects/object_wallmaster/gFloormasterTapFingerAnim", 25, 0, 0.007f, -15.0f }, // En_Floormas
    { RG_SOUL_ENEMY_POE, "__OTR__objects/object_poh/gPoeSkel", "__OTR__objects/object_poh/gPoeFloatAnim", 21, 1, 0.008f,
      -10.0f }, // En_Poh
    { RG_SOUL_ENEMY_REDEAD_GIBDO, "__OTR__objects/object_rd/gRedeadSkel",
      "__OTR__objects/object_rd/gGibdoRedeadIdleAnim", 26, 0, 0.007f, -20.0f }, // En_Rd
    { RG_SOUL_ENEMY_SKULLWALLTULA, "__OTR__objects/object_st/object_st_Skel_005298",
      "__OTR__objects/object_st/object_st_Anim_000304", 30, 1, 0.015f, 0.0f }, // En_Sw
    { RG_SOUL_ENEMY_FLARE_DANCER, "__OTR__objects/object_fw/gFlareDancerSkel",
      "__OTR__objects/object_fw/gFlareDancerTwirlAnim", 27, 0, 0.007f, -20.0f }, // En_Fd
    { RG_SOUL_ENEMY_DEAD_HAND, "__OTR__objects/object_dh/object_dh_Skel_007E88",
      "__OTR__objects/object_dh/object_dh_Anim_005880", 16, 0, 0.008f, -15.0f }, // En_Dh
    { RG_SOUL_ENEMY_SHELL_BLADE, "__OTR__objects/object_sb/object_sb_Skel_002BF0",
      "__OTR__objects/object_sb/object_sb_Anim_000194", 9, 0, 0.009f, -10.0f }, // En_Sb
    { RG_SOUL_ENEMY_LIKE_LIKE, nullptr, nullptr, 0, 1, 0.009f, -20.0f },        // En_Rr
    { RG_SOUL_ENEMY_SPIKE, nullptr, nullptr, 0, 1, 0.012f, 0.0f },              // En_Ny
    { RG_SOUL_ENEMY_ANUBIS, "__OTR__objects/object_anubice/gAnubiceSkel",
      "__OTR__objects/object_anubice/gAnubiceIdleAnim", 16, 1, 0.01f, -20.0f }, // En_Anubice
    { RG_SOUL_ENEMY_IRON_KNUCKLE, "__OTR__objects/object_ik/gIronKnuckleSkel",
      "__OTR__objects/object_ik/gIronKnuckleWalkAnim", 30, 0, 0.006f, -22.0f }, // En_Ik
    { RG_SOUL_ENEMY_SKULL_KID, "__OTR__objects/object_skj/gSkullKidSkel",
      "__OTR__objects/object_skj/gSkullKidPlayFluteAnim", 19, 0, 0.01f, -20.0f }, // En_Skj
    { RG_SOUL_ENEMY_FLYING_POT, nullptr, nullptr, 0, 1, 0.08f, -15.0f },          // En_Tubo_Trap
    { RG_SOUL_ENEMY_FREEZARD, nullptr, nullptr, 0, 1, 0.006f, -20.0f },           // En_Fz
    { RG_SOUL_ENEMY_STINGER, "__OTR__objects/object_ei/gStingerSkel", "__OTR__objects/object_ei/gStingerIdleAnim", 19,
      1, 0.016f, 0.0f }, // En_Eiyer
    { RG_SOUL_ENEMY_WOLFOS, "__OTR__objects/object_wf/gWolfosNormalSkel", "__OTR__objects/object_wf/gWolfosWaitingAnim",
      22, 0, 0.006f, -20.0f }, // En_Wf
    { RG_SOUL_ENEMY_STALCHILD, "__OTR__objects/object_skb/gStalchildSkel",
      "__OTR__objects/object_skb/gStalchildWalkingAnim", 20, 1, 0.012f, -18.0f }, // En_Skb
    { RG_SOUL_ENEMY_GUAY, "__OTR__objects/object_crow/gGuaySkel", "__OTR__objects/object_crow/gGuayFlyAnim", 9, 0,
      0.016f, 0.0f }, // En_Crow
    { RG_SOUL_ENEMY_THIEVES, "__OTR__objects/object_geldb/gGerudoRedSkel",
      "__OTR__objects/object_geldb/gGerudoRedNeutralAnim", 24, 0, 0.006f, -22.0f }, // En_GeldB
    { RG_SOUL_NPC_SARIA, "__OTR__objects/object_sa/gSariaSkel", "__OTR__objects/object_sa/gSariaWaitArmsToSideAnim", 17,
      0, 0.009f, -20.0f }, // En_Sa
    { RG_SOUL_NPC_DARUNIA, "__OTR__objects/object_du/gDaruniaSkel", "__OTR__objects/object_du/gDaruniaIdleAnim", 18, 0,
      0.006f, -20.0f }, // En_Du
    { RG_SOUL_NPC_RUTO, "__OTR__objects/object_ru1/gRutoChildSkel",
      "__OTR__objects/object_ru1/gRutoChildWaitHandsOnHipsAnim", 17, 0, 0.009f, -20.0f }, // En_Ru1
    { RG_SOUL_NPC_KING_ZORA, "__OTR__objects/object_kz/gKzSkel", "__OTR__objects/object_kz/gKzIdleAnim", 12, 0, 0.005f,
      -20.0f }, // En_Kz
    { RG_SOUL_NPC_ZELDA, "__OTR__objects/object_zl1/gChildZelda1Skel",
      "__OTR__objects/object_zl1/gChildZelda1Anim_00438", 18, 0, 0.009f, -20.0f }, // En_Zl1
    { RG_SOUL_NPC_SHEIK, "__OTR__objects/object_xc/gSheikSkel", "__OTR__objects/object_xc/gSheikIdleAnim", 17, 0,
      0.006f, -20.0f }, // En_Xc
    { RG_SOUL_NPC_ANJU, "__OTR__objects/object_ane/gCuccoLadySkel", "__OTR__objects/object_os_anime/gObjOsAnim_07D0",
      16, 0, 0.006f, -20.0f }, // En_Niw_Lady
    { RG_SOUL_NPC_CARPENTERS, "__OTR__objects/object_daiku/object_daiku_Skel_007958",
      "__OTR__objects/object_daiku/object_daiku_Anim_001AB0", 17, 0, 0.006f, -20.0f }, // En_Daiku
    { RG_SOUL_NPC_GURU_GURU, "__OTR__objects/object_fu/gWindmillManSkel",
      "__OTR__objects/object_fu/gWindmillManPlayStillAnim", 16, 0, 0.007f, -20.0f }, // En_Fu
    { RG_SOUL_NPC_MIDO, "__OTR__objects/object_md/gMidoSkel", "__OTR__objects/object_md/gMidoHandsOnHipsIdleAnim", 17,
      0, 0.009f, -20.0f }, // En_Md
    { RG_SOUL_NPC_KOKIRI, "__OTR__objects/object_km1/gKm1Skel", "__OTR__objects/object_os_anime/gKokiriIdleAnim", 16, 0,
      0.009f, -20.0f }, // En_Ko
    { RG_SOUL_NPC_KOKIRI_SHOPKEEPER, "__OTR__objects/object_km1/gKm1Skel",
      "__OTR__objects/object_masterkokiri/object_masterkokiri_Anim_0004A8", 16, 0, 0.009f, -20.0f }, // En_Ossan
    { RG_SOUL_NPC_HYLIAN_GUARD, "__OTR__objects/object_sd/gEnHeishiSkel", "__OTR__objects/object_sd/gEnHeishiIdleAnim",
      17, 1, 0.006f, -20.0f }, // En_Heishi2
    { RG_SOUL_NPC_CITIZEN, "__OTR__objects/object_cob/object_cob_Skel_0021F8",
      "__OTR__objects/object_os_anime/gObjOsAnim_4F28", 16, 0, 0.006f, -20.0f }, // En_Hy
    { RG_SOUL_NPC_MALON, "__OTR__objects/object_ma1/gMalonChildSkel", "__OTR__objects/object_ma1/gMalonChildIdleAnim",
      19, 0, 0.009f, -20.0f }, // En_Ma1
    { RG_SOUL_NPC_TALON, "__OTR__objects/object_ta/gTalonSkel", "__OTR__objects/object_ta/gTalonStandAnim", 17, 0,
      0.006f, -20.0f }, // En_Ta
    { RG_SOUL_NPC_FISHING_POND_OWNER, "__OTR__objects/object_fish/gFishingOwnerSkel",
      "__OTR__objects/object_fish/gFishingOwnerAnim", 9, 0, 0.006f, -20.0f }, // Fishing
    { RG_SOUL_NPC_GORON, "__OTR__objects/object_oF1d_map/gGoronSkel",
      "__OTR__objects/object_oF1d_map/gGoronAnim_0029A8", 18, 0, 0.007f, -20.0f }, // En_Go2
    { RG_SOUL_NPC_MEDIGORON, "__OTR__objects/object_oF1d_map/gGoronSkel",
      "__OTR__objects/object_gm/object_gm_Anim_0002B8", 18, 0, 0.007f, -20.0f }, // En_Gm
    { RG_SOUL_NPC_BIGGORON, "__OTR__objects/object_oF1d_map/gGoronSkel",
      "__OTR__objects/object_oF1d_map/gGoronAnim_0029A8", 18, 0, 0.008f, -20.0f }, // En_Go2
    { RG_SOUL_NPC_GORON_CHILD, "__OTR__objects/object_oF1d_map/gGoronSkel",
      "__OTR__objects/object_oF1d_map/gGoronAnim_0029A8", 18, 0, 0.0055f, -20.0f }, // En_Go2
    { RG_SOUL_NPC_GORON_SHOPKEEPER, "__OTR__objects/object_oF1d_map/gGoronSkel",
      "__OTR__objects/object_mastergolon/gGoronShopkeeperAnim", 18, 0, 0.007f, -20.0f }, // En_Ossan
    { RG_SOUL_NPC_ZORA, "__OTR__objects/object_zo/gZoraSkel", "__OTR__objects/object_zo/gZoraIdleAnim", 20, 0, 0.006f,
      -20.0f }, // En_Zo
    { RG_SOUL_NPC_ZORA_SHOPKEEPER, "__OTR__objects/object_zo/gZoraSkel",
      "__OTR__objects/object_masterzoora/gZoraShopkeeperAnim", 20, 0, 0.006f, -20.0f }, // En_Ossan
    { RG_SOUL_NPC_BAZAAR_SHOPKEEPER, "__OTR__objects/object_ossan/gObjectOssanSkel",
      "__OTR__objects/object_ossan/gObjectOssanAnim_000338", 9, 0, 0.007f, -20.0f }, // En_Ossan
    { RG_SOUL_NPC_BOMBCHU_SHOPKEEPER, "__OTR__objects/object_rs/gBombchuShopkeeperSkel",
      "__OTR__objects/object_rs/gBombchuShopkeeperIdleAnim", 9, 0, 0.007f, -20.0f }, // En_Ossan
    { RG_SOUL_NPC_POTION_SHOPKEEPER, "__OTR__objects/object_ds2/object_ds2_Skel_004258",
      "__OTR__objects/object_ds2/object_ds2_Anim_0002E4", 9, 0, 0.007f, -20.0f }, // En_Ossan
    { RG_SOUL_NPC_BOMBCHU_BOWLING_LADY, "__OTR__objects/object_bg/gChuGirlSkel",
      "__OTR__objects/object_bg/gChuGirlLeanOverCounterAnim", 11, 0, 0.008f, -20.0f }, // En_Bom_Bowl_Man
    { RG_SOUL_NPC_CHEST_GAME_OWNER, "__OTR__objects/object_ts/object_ts_Skel_004FE0",
      "__OTR__objects/object_ts/object_ts_Anim_000498", 10, 0, 0.008f, -20.0f }, // En_Takara_Man
    { RG_SOUL_NPC_SHOOTING_GALLERY_OWNER, "__OTR__objects/object_ossan/gObjectOssanSkel",
      "__OTR__objects/object_ossan/gObjectOssanAnim_000338", 9, 0, 0.007f, -20.0f }, // En_Syateki_Man
    { RG_SOUL_NPC_DAMPE, "__OTR__objects/object_tk/gDampeSkel", "__OTR__objects/object_tk/gDampeRestAnim", 18, 0,
      0.007f, -20.0f }, // En_Tk
    { RG_SOUL_NPC_BOMBERS, "__OTR__objects/object_cs/gGraveyardKidSkel",
      "__OTR__objects/object_cs/gGraveyardKidIdleAnim", 16, 0, 0.009f, -20.0f }, // En_Cs
    { RG_SOUL_NPC_POE_COLLECTOR, "__OTR__objects/object_ps/gPoeSellerSkel",
      "__OTR__objects/object_ps/gPoeSellerIdleAnim", 12, 0, 0.007f, -20.0f }, // En_Gb
    { RG_SOUL_NPC_COMPOSER_BROS, "__OTR__objects/object_po_composer/gPoeComposerSkel",
      "__OTR__objects/object_po_composer/gPoeComposerFloatAnim", 12, 0, 0.008f, -20.0f }, // En_Poh
    { RG_SOUL_NPC_HONEY_DARLING, "__OTR__objects/object_mu/gDancingCoupleSkel",
      "__OTR__objects/object_mu/gDancingCoupleAnim", 21, 0, 0.006f, -20.0f }, // En_Tg
    { RG_SOUL_NPC_ASTRONOMER, "__OTR__objects/object_bji/object_bji_Skel_0000F0",
      "__OTR__objects/object_os_anime/gObjOsAnim_265C", 16, 0, 0.006f, -20.0f }, // En_Hy
    { RG_SOUL_NPC_ROOFTOP_MAN, "__OTR__objects/object_ani/gRoofManSkel", "__OTR__objects/object_ani/gRoofManIdleAnim",
      16, 0, 0.006f, -20.0f }, // En_Ani
    { RG_SOUL_NPC_BEAN_SALESMAN, "__OTR__objects/object_ms/gBeanSalesmanSkel",
      "__OTR__objects/object_ms/gBeanSalesmanEatingAnim", 9, 0, 0.01f, -15.0f }, // En_Ms
    { RG_SOUL_NPC_SCIENTIST, "__OTR__objects/object_mk/object_mk_Skel_005DF0",
      "__OTR__objects/object_mk/object_mk_Anim_000D88", 13, 0, 0.006f, -20.0f }, // En_Mk
    { RG_SOUL_NPC_GORMAN, "__OTR__objects/object_in/gIngoSkel", "__OTR__objects/object_in/object_in_Anim_0003B4", 20, 0,
      0.006f, -20.0f }, // En_In
    { RG_SOUL_NPC_GROG, "__OTR__objects/object_hs/object_hs_Skel_006260",
      "__OTR__objects/object_hs/object_hs_Anim_0005C0", 16, 0, 0.007f, -20.0f }, // En_Hs
    { RG_SOUL_NPC_DOG_LADY, "__OTR__objects/object_aob/gDogLadySkel", "__OTR__objects/object_os_anime/gObjOsAnim_092C",
      16, 0, 0.006f, -20.0f }, // En_Hy
    { RG_SOUL_NPC_CARPET_MAN, "__OTR__objects/object_js/gCarpetMerchantSkel",
      "__OTR__objects/object_js/gCarpetMerchantIdleAnim", 13, 0, 0.008f, -10.0f }, // En_Js
    { RG_SOUL_NPC_OLD_HAG, "__OTR__objects/object_ds/gPotionShopLadySkel",
      "__OTR__objects/object_ds/gPotionShopLadyAnim", 6, 0, 0.009f, -10.0f }, // En_Ds
    { RG_SOUL_NPC_BANKER, "__OTR__objects/object_boj/object_boj_Skel_0000F0",
      "__OTR__objects/object_os_anime/gObjOsAnim_28DC", 16, 0, 0.007f, -20.0f }, // En_Hy
    { RG_SOUL_NPC_THIEVES, "__OTR__objects/object_ge1/gGerudoWhiteSkel",
      "__OTR__objects/object_ge1/gGerudoWhiteIdleAnim", 16, 0, 0.006f, -20.0f }, // En_Ge1
};
inline const Model* Find(int32_t id) {
    for (const auto& model : Catalog)
        if (model.id == id)
            return &model;
    return nullptr;
}
inline bool UsesDetailed(int32_t id) {
    if (!Find(id))
        return false;
    return id >= RG_SOUL_NPC_SARIA && id <= RG_SOUL_NPC_THIEVES ? !ComboSoulModels::SimplerNpc()
                                                                : !ComboSoulModels::SimplerEnemy();
}
inline CwAnimSegBind& Segment(CwItemAnimDrawInfo& out, int seg, const char* path = nullptr) {
    auto& bind = out.segs[out.segCount++];
    bind.segment = seg;
    bind.onOpa = bind.onXlu = 1;
    bind.kind = path ? CW_ANIM_SEG_PATH : CW_ANIM_SEG_EMPTY_DL;
    bind.path = path;
    return bind;
}
inline void Color(CwItemAnimDrawInfo& out, int seg, int r, int g, int b, int a = 255, int pr = 255, int pg = 255,
                  int pb = 255) {
    auto& bind = out.colorSegments[out.colorSegmentCount++];
    bind.segment = seg;
    bind.onOpa = bind.onXlu = 1;
    bind.prim[0] = pr;
    bind.prim[1] = pg;
    bind.prim[2] = pb;
    bind.prim[3] = 255;
    bind.env[0] = r;
    bind.env[1] = g;
    bind.env[2] = b;
    bind.env[3] = a;
}
inline void Face(CwItemAnimDrawInfo& out, const char* eye, const char* mouth = nullptr) {
    Segment(out, 8, eye);
    if (mouth)
        Segment(out, 9, mouth);
}
inline void Limb(CwItemAnimDrawInfo& out, int index, const char* path) {
    auto& limb = out.limbDLs[out.limbDLCount++];
    limb.limbIndex = index;
    limb.dlPath = path;
}
inline void PostLimb(CwItemAnimDrawInfo& out, int index, const char* path, bool xlu = false) {
    auto& limb = out.limbDLs[out.limbDLCount++];
    limb.limbIndex = index;
    limb.postDlPath = path;
    limb.postXlu = xlu;
}
inline CwAnimPart& Part(CwItemAnimDrawInfo& out, const char* path, bool xlu = false) {
    auto& part = out.parts[out.partCount++];
    part.dlPath = path;
    part.xlu = xlu;
    return part;
}
inline void Scroll(CwItemAnimDrawInfo& out, int seg, int w1, int h1, int w2, int h2, int y2) {
    auto& bind = Segment(out, seg);
    bind.kind = CW_ANIM_SEG_TEXSCROLL;
    bind.width1 = w1;
    bind.height1 = h1;
    bind.width2 = w2;
    bind.height2 = h2;
    bind.yStep2 = y2;
}
inline int32_t Describe(int32_t id, CwItemAnimDrawInfo* result) {
    const Model* model = Find(id);
    if (!result || !model || !UsesDetailed(id))
        return 0;
    auto& out = *result;
    out = CwItemAnimDrawInfo{};
    out.skelPath = model->skel;
    out.animPath = model->anim;
    out.limbCount = model->joints;
    out.nonFlexSkeleton = model->nonFlex;
    out.scale = model->scale;
    out.translatePre[1] = model->centerY;
    out.hiddenLimb = -1;
    out.opa = 1;
    out.stateDependent = 1;
    out.hasModelEnvColor = 1;
    out.modelEnvColor[3] = 255;
    out.fallbackTokenPath = "__OTR__objects/object_gi_sutaru/gGiSkulltulaTokenDL";
    out.fallbackFlameColor[0] = out.fallbackFlameColor[1] = out.fallbackFlameColor[2] = 155;
    out.fallbackFlameColor[3] = 255;
    out.flameDlPath = "__OTR__objects/object_boss_soul/gBossSoulFlameDL";
    out.flameGrayscale = 1;
    out.flameTranslate[1] = -70.0f;
    out.flameScale[0] = out.flameScale[1] = out.flameScale[2] = 5.0f;
    out.flameColor[0] = out.flameColor[1] = out.flameColor[2] = 155;
    out.flameColor[3] = 255;
    out.flameHasSeg = 1;
    out.flameSeg.kind = CW_ANIM_SEG_TEXSCROLL;
    out.flameSeg.segment = 8;
    out.flameSeg.onXlu = 1;
    out.flameSeg.width1 = out.flameSeg.width2 = 16;
    out.flameSeg.height1 = out.flameSeg.height2 = 32;
    out.flameSeg.xStep2 = 1;
    out.flameSeg.yStep2 = -8;
    switch (id) {
        case RG_SOUL_ENEMY_STALFOS: {
            // TP's owning replacement also branches through the native OPA material slot.
            Segment(out, 12);
            auto& color = out.limbColors[out.limbColorCount++];
            color.limbFrom = color.limbTo = 11;
            color.env[0] = 255;
            color.env[3] = 255;
            break;
        }
        case RG_SOUL_ENEMY_GOHMA_LARVA:
            out.modelEnvColor[0] = 255;
            out.modelEnvColor[2] = 50;
            break;
        case RG_SOUL_ENEMY_KEESE:
            PostLimb(out, 27, "__OTR__objects/object_firefly/gKeeseEyesDL");
            break;
        case RG_SOUL_ENEMY_TEKTITE:
            Face(out, "__OTR__objects/object_tite/object_tite_Tex_001300",
                 "__OTR__objects/object_tite/object_tite_Tex_001700");
            Segment(out, 10, "__OTR__objects/object_tite/object_tite_Tex_001900");
            break;
        case RG_SOUL_ENEMY_LEEVER:
            out.playSpeed = -1.0f;
            break;
        case RG_SOUL_ENEMY_LIZALFOS_DINOLFOS:
            Segment(out, 8);
            Segment(out, 9);
            break;
        case RG_SOUL_ENEMY_SHABOM: {
            out.opa = 1;
            auto& part = Part(out, "__OTR__objects/object_bubble/gBubbleDL", true);
            part.billboard = 1;
            part.hasPrimColor = part.hasEnvColor = 1;
            part.primColor[0] = part.primColor[1] = part.primColor[2] = part.primColor[3] = 255;
            part.envColor[0] = part.envColor[1] = part.envColor[2] = 150;
            part.envColor[3] = 255;
            break;
        }
        case RG_SOUL_ENEMY_DARK_LINK:
            // The owner export fills the checked first native player-animation frame.
            // A player animation is never passed to a normal SkelAnime animation reader.
            out.bindPose = 1;
            out.posePath = "__OTR__misc/link_animetion/gPlayerAnimData_1C3030";
            Segment(out, 12);
            out.hasPrimColor = 1;
            out.primColor[3] = 255;
            out.modelEnvColor[0] = 255;
            break;
        case RG_SOUL_ENEMY_BIRI_BARI:
            out.opa = 0;
            out.xlu = 1;
            Face(out, "__OTR__objects/object_bl/gBiriTentacles0Tex");
            Segment(out, 9, "__OTR__overlays/ovl_En_Bili/D_809C1700");
            break;
        case RG_SOUL_ENEMY_TAILPASARN: {
            Part(out, "__OTR__objects/object_tp/gTailpasaranHeadDL");
            Segment(out, 8, "__OTR__objects/object_tp/gTailpasaranTailSegmentTex");
            for (int i = 0; i < 6; ++i) {
                auto& part = Part(out, "__OTR__objects/object_tp/gTailpasaranTailSegmentDL", true);
                part.billboard = 1;
                part.transform.translate[2] = -12.0f * (i + 1);
                part.transform.scale[0] = part.transform.scale[1] = part.transform.scale[2] = 0.45f - i * 0.045f;
                part.hasPrimColor = part.hasEnvColor = 1;
                part.primColor[0] = 255;
                part.primColor[2] = part.primColor[3] = 255;
                part.envColor[2] = part.envColor[3] = 255;
            }
            break;
        }
        case RG_SOUL_ENEMY_PARASITE: {
            Part(out, "__OTR__objects/object_bxa/object_bxa_DL_000890");
            Segment(out, 8, "__OTR__objects/object_bxa/object_bxa_Tex_0024F0");
            Scroll(out, 9, 16, 16, 32, 32, -10);
            out.matrixCount = 14;
            out.matrixSegment = 12;
            // Native EnBa's 14 links, 448 model units per link. A compact hanging
            // pose replaces world-coordinate simulation while retaining the mesh palette.
            for (int i = 0; i < 14; ++i) {
                out.matrices[i].translate[1] = 5000.0f - (i % 7) * 448.0f;
                out.matrices[i].translate[0] = i < 7 ? -180.0f : 180.0f;
                out.matrices[i].rotate[2] = i < 7 ? 0x800 : -0x800;
                if (i == 6 || i == 13)
                    out.matrices[i].scale[0] = out.matrices[i].scale[1] = 0.5f;
            }
            break;
        }
        case RG_SOUL_ENEMY_TORCH_SLUG:
            Segment(out, 8);
            Scroll(out, 9, 32, 32, 32, 32, 1);
            out.modelEnvColor[0] = 255;
            break;
        case RG_SOUL_ENEMY_DEKU_BABA:
            out.skeletonTranslate[1] = 6000.0f;
            Part(out, "__OTR__objects/object_dekubaba/gDekuBabaBaseLeavesDL");
            // Native extended stem sections are 2000 units along their local Z.
            // A -quarter-turn raises a compact upright three-section stalk.
            for (const char* path : { "__OTR__objects/object_dekubaba/gDekuBabaStemBaseDL",
                                      "__OTR__objects/object_dekubaba/gDekuBabaStemMiddleDL",
                                      "__OTR__objects/object_dekubaba/gDekuBabaStemTopDL" }) {
                auto& part = Part(out, path);
                part.transform.rotate[0] = -0x4000;
                part.transform.translate[1] = (out.partCount - 2) * 2000.0f;
            }
            break;
        case RG_SOUL_ENEMY_DEKU_SCRUB:
            Part(out, "__OTR__objects/object_dekunuts/gDekuNutsFlowerDL");
            break;
        case RG_SOUL_ENEMY_BEAMOS:
            Scroll(out, 8, 32, 32, 32, 32, 0);
            break;
        case RG_SOUL_ENEMY_POE:
            Segment(out, 8);
            out.modelEnvColor[0] = 255;
            out.modelEnvColor[1] = 170;
            out.modelEnvColor[2] = 255;
            PostLimb(out, 18, "__OTR__objects/object_poh/gPoeLanternDL");
            break;
        case RG_SOUL_ENEMY_REDEAD_GIBDO:
        case RG_SOUL_ENEMY_DEAD_HAND:
        case RG_SOUL_ENEMY_STINGER:
            Segment(out, 8);
            break;
        case RG_SOUL_ENEMY_FLARE_DANCER:
            out.opa = 0;
            out.xlu = 1;
            Scroll(out, 8, 32, 64, 8, 64, -6);
            Segment(out, 9);
            out.hasPrimColor = 1;
            out.primColor[0] = 255;
            out.primColor[1] = 255;
            out.primColor[3] = 255;
            out.modelEnvColor[0] = 255;
            break;
        case RG_SOUL_ENEMY_LIKE_LIKE:
            Part(out, "__OTR__objects/object_rr/gLikeLikeDL", true);
            Scroll(out, 8, 32, 16, 32, 16, -6);
            out.matrixCount = 4;
            out.matrixSegment = 12;
            for (int i = 0; i < 4; ++i)
                out.matrices[i].translate[1] = (i + 1) * 1000.0f;
            out.hasPrimColor = 1;
            out.primColor[0] = out.primColor[1] = out.primColor[2] = out.primColor[3] = 255;
            out.modelEnvColor[0] = 200;
            out.modelEnvColor[1] = 100;
            break;
        case RG_SOUL_ENEMY_SPIKE:
            Part(out, "__OTR__objects/object_ny/gEnNyMetalBodyDL");
            Part(out, "__OTR__objects/object_ny/gEnNySpikeDL");
            break;
        case RG_SOUL_ENEMY_ANUBIS:
            PostLimb(out, 13, "__OTR__objects/object_anubice/gAnubiceEyesDL", true);
            break;
        case RG_SOUL_ENEMY_IRON_KNUCKLE:
            Color(out, 8, 0, 0, 0, 255, 55, 65, 55);
            Color(out, 9, 25, 20, 0, 255, 205, 165, 75);
            Color(out, 10, 25, 20, 0, 255, 205, 165, 75);
            break;
        case RG_SOUL_ENEMY_SKULL_KID:
            Segment(out, 12);
            break;
        case RG_SOUL_ENEMY_FLYING_POT:
            Part(out, "__OTR__objects/gameplay_dangeon_keep/gPotDL");
            break;
        case RG_SOUL_ENEMY_FREEZARD: {
            // EnFz_Draw's active intact body: translucent two-layer ice material.
            // opa=1 selects the composite renderer; the actual part stays on XLU.
            auto& part = Part(out, "__OTR__objects/object_fz/gFreezardIntactDL", true);
            part.materialProfile = CW_ANIM_MATERIAL_FREEZARD;
            part.hasPrimColor = part.hasEnvColor = 1;
            part.primLodFrac = 128;
            part.primColor[0] = 155;
            part.primColor[1] = part.primColor[2] = part.primColor[3] = 255;
            part.envColor[0] = part.envColor[1] = part.envColor[2] = 200;
            part.envColor[3] = 255;
            Scroll(out, 8, 32, 32, 32, 32, 2);
            auto& scroll = out.segs[out.segCount - 1];
            scroll.yStep1 = 1;
            scroll.yMask1 = scroll.yMask2 = 0x7F;
            scroll.onOpa = 0;
            break;
        }
        case RG_SOUL_ENEMY_WOLFOS:
            Face(out, "__OTR__objects/object_wf/gWolfosNormalEyeOpenTex");
            break;
        case RG_SOUL_ENEMY_THIEVES:
            Face(out, "__OTR__objects/object_geldb/gGerudoRedEyeOpenTex");
            break;
        case RG_SOUL_NPC_SARIA:
            Face(out, "__OTR__objects/object_sa/gSariaEyeOpenTex", "__OTR__objects/object_sa/gSariaEyeOpenTex");
            Segment(out, 10, "__OTR__objects/object_sa/gSariaMouthClosedTex");
            Segment(out, 12);
            break;
        case RG_SOUL_NPC_DARUNIA:
            Face(out, "__OTR__objects/object_du/gDaruniaEyeOpenTex",
                 "__OTR__objects/object_du/gDaruniaMouthSeriousTex");
            Segment(out, 10, "__OTR__objects/object_du/gDaruniaNoseSeriousTex");
            Segment(out, 12);
            break;
        case RG_SOUL_NPC_RUTO:
            Face(out, "__OTR__objects/object_ru1/gRutoChildEyeOpenTex",
                 "__OTR__objects/object_ru1/gRutoChildMouthClosedTex");
            Segment(out, 12);
            break;
        case RG_SOUL_NPC_KING_ZORA:
            Face(out, "__OTR__objects/object_kz/gKzEyeOpenTex");
            break;
        case RG_SOUL_NPC_ZELDA:
            Face(out, "__OTR__objects/object_zl1/gChildZelda1EyeOpenLookingRightTex",
                 "__OTR__objects/object_zl1/gChildZelda1EyeOpenLookingLeftTex");
            Segment(out, 10, "__OTR__objects/object_zl1/gChildZelda1MouthNeutralTex");
            break;
        case RG_SOUL_NPC_SHEIK:
            Face(out, "__OTR__objects/object_xc/gSheikEyeOpenTex", "__OTR__objects/object_xc/gSheikEyeOpenTex");
            break;
        case RG_SOUL_NPC_ANJU:
            Face(out, "__OTR__objects/object_ane/gCuccoLadyEyeOpenTex");
            Segment(out, 12);
            break;
        case RG_SOUL_NPC_CARPENTERS:
            PostLimb(out, 15, "__OTR__objects/object_daiku/object_daiku_DL_005BD0");
            break;
        case RG_SOUL_NPC_GURU_GURU:
            Face(out, "__OTR__objects/object_fu/gWindmillManEyeClosedTex",
                 "__OTR__objects/object_fu/gWindmillManMouthOpenTex");
            break;
        case RG_SOUL_NPC_MIDO:
            Face(out, "__OTR__objects/object_md/gMidoEyeOpenTex");
            Segment(out, 12);
            break;
        case RG_SOUL_NPC_KOKIRI:
            Color(out, 8, 0, 130, 70);
            Color(out, 9, 110, 170, 20);
            Segment(out, 12);
            Limb(out, 15, "__OTR__objects/object_km1/gKm1DL");
            break;
        case RG_SOUL_NPC_KOKIRI_SHOPKEEPER:
            Color(out, 8, 0, 130, 70);
            Color(out, 9, 110, 170, 20);
            Segment(out, 12);
            Segment(out, 10, "__OTR__objects/object_masterkokirihead/gKokiriShopkeeperEyeDefaultTex");
            Limb(out, 15, "__OTR__objects/object_masterkokirihead/gKokiriShopkeeperHeadDL");
            break;
        case RG_SOUL_NPC_CITIZEN:
            Limb(out, 15, "__OTR__objects/object_cob/object_cob_DL_001300");
            break;
        case RG_SOUL_NPC_MALON:
            Face(out, "__OTR__objects/object_ma1/gMalonChildEyeOpenTex",
                 "__OTR__objects/object_ma1/gMalonChildNeutralMouthTex");
            break;
        case RG_SOUL_NPC_TALON:
            Face(out, "__OTR__objects/object_ta/gTalonEyeOpenTex", "__OTR__objects/object_ta/gTalonHeadSkinTex");
            break;
        case RG_SOUL_NPC_FISHING_POND_OWNER:
            Face(out, "__OTR__objects/object_fish/gFishingOwnerEyeOpenTex");
            PostLimb(out, 8, "__OTR__objects/object_fish/gFishingOwnerHatDL");
            break;
        case RG_SOUL_NPC_GORON:
        case RG_SOUL_NPC_MEDIGORON:
        case RG_SOUL_NPC_GORON_CHILD:
        case RG_SOUL_NPC_GORON_SHOPKEEPER:
            Face(out, "__OTR__objects/object_oF1d_map/gGoronCsEyeOpenTex",
                 "__OTR__objects/object_oF1d_map/gGoronCsMouthNeutralTex");
            break;
        case RG_SOUL_NPC_BIGGORON:
            Face(out, "__OTR__objects/object_oF1d_map/gGoronCsEyeClosed2Tex",
                 "__OTR__objects/object_oF1d_map/gGoronCsMouthSmileTex");
            break;
        case RG_SOUL_NPC_ZORA:
            Face(out, "__OTR__objects/object_zo/gZoraEyeOpenTex");
            Segment(out, 12);
            break;
        case RG_SOUL_NPC_ZORA_SHOPKEEPER:
            Face(out, "__OTR__objects/object_zo/gZoraEyeOpenTex");
            Segment(out, 12);
            break;
        case RG_SOUL_NPC_BAZAAR_SHOPKEEPER:
        case RG_SOUL_NPC_SHOOTING_GALLERY_OWNER:
            Face(out, "__OTR__objects/object_ossan/gOssanEyeOpenTex");
            break;
        case RG_SOUL_NPC_BOMBCHU_SHOPKEEPER:
            Face(out, "__OTR__objects/object_rs/gBombchuShopkeeperEyeOpenTex");
            break;
        case RG_SOUL_NPC_POTION_SHOPKEEPER:
            Face(out, "__OTR__objects/object_ds2/gPotionShopkeeperEyeOpenTex");
            break;
        case RG_SOUL_NPC_BOMBCHU_BOWLING_LADY:
            Face(out, "__OTR__objects/object_bg/gChuGirlEyeOpenTex");
            break;
        case RG_SOUL_NPC_CHEST_GAME_OWNER:
            Face(out, "__OTR__objects/object_ts/object_ts_Tex_000970");
            break;
        case RG_SOUL_NPC_DAMPE:
            Face(out, "__OTR__objects/object_tk/gDampeEyeOpenTex");
            PostLimb(out, 14, "__OTR__objects/object_tk/gDampeShovelDL");
            break;
        case RG_SOUL_NPC_BOMBERS:
            Face(out, "__OTR__objects/object_cs/gGraveyardKidEyesOpenTex");
            break;
        case RG_SOUL_NPC_COMPOSER_BROS:
            Color(out, 8, 255, 255, 210, 255);
            Color(out, 10, 80, 110, 90, 255);
            Color(out, 11, 100, 90, 100, 255);
            Segment(out, 12);
            Limb(out, 10, "__OTR__objects/object_po_composer/gPoeComposerFlatHeadDL");
            PostLimb(out, 9, "__OTR__objects/object_po_composer/gPoeComposerLanternDL");
            PostLimb(out, 9, "__OTR__objects/object_po_composer/gPoeComposerLanternBottomDL");
            PostLimb(out, 9, "__OTR__objects/object_po_composer/gPoeComposerLanternTopDL");
            break;
        case RG_SOUL_NPC_HONEY_DARLING:
            Color(out, 8, 0, 50, 160, 0);
            Color(out, 9, 255, 255, 255, 0);
            break;
        case RG_SOUL_NPC_ASTRONOMER:
            Color(out, 8, 0, 50, 160, 0);
            Color(out, 9, 255, 255, 255, 0);
            Segment(out, 10, "__OTR__objects/object_bji/object_bji_Tex_0005FC");
            Limb(out, 15, "__OTR__objects/object_bji/object_bji_DL_002560");
            break;
        case RG_SOUL_NPC_ROOFTOP_MAN:
            Face(out, "__OTR__objects/object_ani/gRoofManEyeOpenTex");
            break;
        case RG_SOUL_NPC_GORMAN:
            Face(out, "__OTR__objects/object_in/gIngoEyeOpenTex", "__OTR__objects/object_in/gIngoHeadGradient2Tex");
            break;
        case RG_SOUL_NPC_DOG_LADY:
            Segment(out, 10, "__OTR__objects/object_aob/gDogLadyEyeOpenTex");
            Limb(out, 15, "__OTR__objects/object_aob/gDogLadyHeadDL");
            break;
        case RG_SOUL_NPC_BANKER:
            Color(out, 8, 50, 80, 0, 0);
            Color(out, 9, 50, 80, 0, 0);
            Limb(out, 15, "__OTR__objects/object_boj/object_boj_DL_005528");
            break;
        case RG_SOUL_NPC_THIEVES:
            Face(out, "__OTR__objects/object_ge1/gGerudoWhiteEyeOpenTex");
            break;
        default:
            break; // Entirely self-contained native skeleton/materials.
    }
    return 1;
}
} // namespace ComboSoulMiniOOT
