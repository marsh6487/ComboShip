#pragma once
// Owner-authored MM soul models. Native actor sources are recorded per identity.
// Paths are canonical MM resources; these recipes never construct gameplay actors.
// Include after MM_Anim* helpers in ComboItemDrawMM.h.
struct MmNpcSoulModel {
    int32_t id;
    const char* skel;
    const char* anim;
    int32_t limbs;
    float scale;
};
// clang-format off
static const MmNpcSoulModel kMmNpcSoulModels[] = {
    // mm/src/overlays/actors/ovl_En_Al/z_en_al.c (EN_AL); MADAME_AROMA_LIMB_MAX
    { RI_SOUL_NPC_AROMA, "__OTR__objects/object_al/gMadameAromaSkel", "__OTR__objects/object_al/object_al_Anim_000C54", 27, 0.012f },
    // mm/src/overlays/actors/ovl_En_Syateki_Man/z_en_syateki_man.c (EN_SYATEKI_MAN); BURLY_GUY_LIMB_MAX
    { RI_SOUL_NPC_SHOOTING_GALLERY_OWNER, "__OTR__objects/object_shn/gBurlyGuySkel", "__OTR__objects/object_shn/gBurlyGuyHeadScratchLoopAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Syateki_Man/z_en_syateki_man.c (EN_SYATEKI_MAN); BURLY_GUY_LIMB_MAX
    { RI_SOUL_NPC_BAZAAR_SHOPKEEPER, "__OTR__objects/object_shn/gBurlyGuySkel", "__OTR__objects/object_shn/gBurlyGuyHeadScratchLoopAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Go/z_en_go.c (EN_GO); GORON_LIMB_MAX
    { RI_SOUL_NPC_GORON, "__OTR__objects/object_oF1d_map/gGoronSkel", "__OTR__objects/object_oF1d_map/gGoronShiverAnim", 18, 0.012f },
    // mm/src/overlays/actors/ovl_En_Gk/z_en_gk.c (EN_GK); OBJECT_GK_LIMB_MAX
    { RI_SOUL_NPC_GORON_CHILD, "__OTR__objects/object_gk/object_gk_Skel_0079C0", "__OTR__objects/object_gk/object_gk_Anim_00787C", 20, 0.014f },
    // mm/src/overlays/actors/ovl_En_Rsn/z_en_rsn.c (EN_RSN); BOMB_SHOPKEEPER_LIMB_MAX
    { RI_SOUL_NPC_BOMBCHU_SHOPKEEPER, "__OTR__objects/object_rsn/gBombShopkeeperSkel", "__OTR__objects/object_rsn/gBombShopkeeperWalkAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Bom_Bowl_Man/z_en_bom_bowl_man.c (EN_BOM_BOWL_MAN); OBJECT_CS_LIMB_MAX
    { RI_SOUL_NPC_BOMBERS, "__OTR__objects/object_cs/object_cs_Skel_00F82C", "__OTR__objects/object_cs/gBomberIdleAnim", 21, 0.014f },
    // mm/src/overlays/actors/ovl_En_Dno/z_en_dno.c (EN_DNO); DEKU_BUTLER_LIMB_MAX
    { RI_SOUL_NPC_BUTLER_DEKU, "__OTR__objects/object_dnj/gDekuButlerSkel", "__OTR__objects/object_dnj/gDekuButlerIdleAnim", 28, 0.012f },
    // mm/src/overlays/actors/ovl_En_Yb/z_en_yb.c (EN_YB); YB_LIMB_MAX
    { RI_SOUL_NPC_CITIZEN, "__OTR__objects/object_yb/gYbSkel", "__OTR__objects/object_yb/object_yb_Anim_000200", 22, 0.012f },
    // mm/src/overlays/actors/ovl_En_Po_Composer/z_en_po_composer.c (EN_PO_COMPOSER); POE_COMPOSER_LIMB_MAX
    { RI_SOUL_NPC_COMPOSER_BROS, "__OTR__objects/object_po_composer/gPoeComposerSkel", "__OTR__objects/object_po_composer/gPoeComposerIdleAnim", 12, 0.012f },
    // mm/src/overlays/actors/ovl_En_Tk/z_en_tk.c (EN_TK); OBJECT_TK_LIMB_MAX
    { RI_SOUL_NPC_DAMPE, "__OTR__objects/object_tk/object_tk_Skel_00B9E8", "__OTR__objects/object_tk/object_tk_Anim_001FA8", 18, 0.012f },
    // mm/src/overlays/actors/ovl_En_Dnq/z_en_dnq.c (EN_DNQ); DEKU_KING_LIMB_MAX
    { RI_SOUL_NPC_DEKU_KING, "__OTR__objects/object_dno/gDekuKingSkel", "__OTR__objects/object_dno/gDekuKingIdleAnim", 33, 0.008f },
    // mm/src/overlays/actors/ovl_En_Dnp/z_en_dnp.c (EN_DNP); DEKU_PRINCESS_LIMB_MAX
    { RI_SOUL_NPC_DEKU_PRINCESS, "__OTR__objects/object_dnq/gDekuPrincessSkel", "__OTR__objects/object_dnq/gDekuPrincessIdleAnim", 26, 0.015f },
    // mm/src/overlays/actors/ovl_En_Lift_Nuts/z_en_lift_nuts.c (EN_LIFT_NUTS); BUSINESS_SCRUB_LIMB_MAX
    { RI_SOUL_NPC_PLAYGROUND_SCRUBS, "__OTR__objects/object_dnt/gBusinessScrubSkel", "__OTR__objects/object_dnt/gBusinessScrubThinkAnim", 28, 0.012f },
    // mm/src/overlays/actors/ovl_En_Jg/z_en_jg.c (EN_JG); GORON_ELDER_LIMB_MAX
    { RI_SOUL_NPC_GORON_ELDER, "__OTR__objects/object_jg/gGoronElderSkel", "__OTR__objects/object_jg/gGoronElderIdleAnim", 35, 0.009f },
    // mm/src/overlays/actors/ovl_En_Jgame_Tsn/z_en_jgame_tsn.c (EN_JGAME_TSN); OBJECT_TSN_LIMB_MAX
    { RI_SOUL_NPC_CHEST_GAME_OWNER, "__OTR__objects/object_tsn/object_tsn_Skel_008AB8", "__OTR__objects/object_tsn/object_tsn_Anim_0092FC", 17, 0.012f },
    // mm/src/overlays/actors/ovl_En_Sob1/z_en_sob1.c (EN_SOB1); GORON_LIMB_MAX
    { RI_SOUL_NPC_GORON_SHOPKEEPER, "__OTR__objects/object_oF1d_map/gGoronSkel", "__OTR__objects/object_mastergolon/gGoronShopkeeperAnim", 18, 0.012f },
    // mm/src/overlays/actors/ovl_En_Fu/z_en_fu.c (EN_FU); HONEY_AND_DARLING_LIMB_MAX
    { RI_SOUL_NPC_HONEY_DARLING, "__OTR__objects/object_mu/gHoneyAndDarlingSkel", "__OTR__objects/object_mu/gHoneyAndDarlingCupCheeksLoopAnim", 21, 0.011f },
    // mm/src/overlays/actors/ovl_En_Toto/z_en_toto.c (EN_TOTO); OBJECT_ZM_LIMB_MAX
    { RI_SOUL_NPC_TOTO, "__OTR__objects/object_zm/object_zm_Skel_00A978", "__OTR__objects/object_zm/object_zm_Anim_00C880", 18, 0.012f },
    // mm/src/overlays/actors/ovl_En_Zov/z_en_zov.c (EN_ZOV); LULU_LIMB_MAX
    { RI_SOUL_NPC_RUTO, "__OTR__objects/object_zov/gLuluSkel", "__OTR__objects/object_zov/gLuluLookDownAnim", 23, 0.012f },
    // mm/src/overlays/actors/ovl_En_Zob/z_en_zob.c (EN_ZOB); OBJECT_ZOB_LIMB_MAX
    { RI_SOUL_NPC_ZORA_MUSICIANS, "__OTR__objects/object_zob/object_zob_Skel_010810", "__OTR__objects/object_zob/object_zob_Anim_006998", 24, 0.012f },
    // mm/src/overlays/actors/ovl_En_Test3/z_en_test3.c (EN_TEST3); KAFEI_LIMB_MAX
    { RI_SOUL_NPC_KAFEI, "__OTR__objects/object_test3/gKafeiSkel", nullptr, 22, 0.015f },
    // mm/src/overlays/actors/ovl_En_Kitan/z_en_kitan.c (EN_KITAN); KEATON_LIMB_MAX
    { RI_SOUL_NPC_KEATON, "__OTR__objects/object_kitan/gKeatonSkel", "__OTR__objects/object_kitan/gKeatonIdleAnim", 21, 0.016f },
    // mm/src/overlays/actors/ovl_En_Trt/z_en_trt.c (EN_TRT); KOTAKE_LIMB_MAX
    { RI_SOUL_NPC_KOUME_KOTAKE, "__OTR__objects/object_trt/gKotakeSkel", "__OTR__objects/object_trt/gKotakeSleepingAnim", 27, 0.012f },
    // mm/src/overlays/actors/ovl_En_Dt/z_en_dt.c (EN_DT); OBJECT_DT_LIMB_MAX
    { RI_SOUL_NPC_MAYOR_DOTOUR, "__OTR__objects/object_dt/object_dt_Skel_00B0CC", "__OTR__objects/object_dt/gDotourWaitAnim", 15, 0.012f },
    // mm/src/overlays/actors/ovl_En_Go/z_en_go.c (EN_GO); GORON_LIMB_MAX
    { RI_SOUL_NPC_MEDIGORON, "__OTR__objects/object_oF1d_map/gGoronSkel", "__OTR__objects/object_oF1d_map/gGoronShiverAnim", 18, 0.012f },
    // mm/src/overlays/actors/ovl_En_Dai/z_en_dai.c (EN_DAI); OBJECT_DAI_LIMB_MAX
    { RI_SOUL_NPC_BIGGORON, "__OTR__objects/object_dai/object_dai_Skel_0130D0", "__OTR__objects/object_dai/object_dai_Anim_0079E4", 19, 0.009f },
    // mm/src/overlays/actors/ovl_En_Tab/z_en_tab.c (EN_TAB); BARTEN_LIMB_MAX
    { RI_SOUL_NPC_TALON, "__OTR__objects/object_tab/gBartenSkel", "__OTR__objects/object_tab/gBartenIdleAnim", 20, 0.012f },
    // mm/src/overlays/actors/ovl_En_Bji_01/z_en_bji_01.c (EN_BJI_01); SHIKASHI_LIMB_MAX
    { RI_SOUL_NPC_ASTRONOMER, "__OTR__objects/object_bji/gShikashiSkel", "__OTR__objects/object_bji/object_bji_Anim_000FDC", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Gb2/z_en_gb2.c (EN_GB2); OBJECT_PS_LIMB_MAX
    { RI_SOUL_NPC_POE_COLLECTOR, "__OTR__objects/object_ps/object_ps_Skel_007230", "__OTR__objects/object_ps/object_ps_Anim_00049C", 12, 0.012f },
    // mm/src/overlays/actors/ovl_En_Kbt/z_en_kbt.c (EN_KBT); OBJECT_KBT_LIMB_MAX
    { RI_SOUL_NPC_BLACKSMITHS, "__OTR__objects/object_kbt/object_kbt_Skel_00DEE8", "__OTR__objects/object_kbt/object_kbt_Anim_000670", 20, 0.012f },
    // mm/src/overlays/actors/ovl_En_Takaraya/z_en_takaraya.c (EN_TAKARAYA); TREASURE_CHEST_SHOP_GAL_LIMB_MAX
    { RI_SOUL_NPC_BOMBCHU_BOWLING_LADY, "__OTR__objects/object_bg/gTreasureChestShopGalSkel", "__OTR__objects/object_bg/object_bg_Anim_009890", 24, 0.012f },
    // mm/src/overlays/actors/ovl_En_Bal/z_en_bal.c (EN_BAL); TINGLE_LIMB_MAX
    { RI_SOUL_NPC_TINGLE, "__OTR__objects/object_bal/gTingleSkel", "__OTR__objects/object_bal/gTingleFloatIdleAnim", 33, 0.011f },
    // mm/src/overlays/actors/ovl_En_Bjt/z_en_bjt.c (EN_BJT); TOILET_HAND_LIMB_MAX
    { RI_SOUL_NPC_TOILET_HAND, "__OTR__objects/object_bjt/gToiletHandSkel", "__OTR__objects/object_bjt/gToiletHandWaitingAnim", 7, 0.02f },
    // mm/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c (EN_OSSAN); FSN_LIMB_MAX
    { RI_SOUL_NPC_FISHING_POND_OWNER, "__OTR__objects/object_fsn/gFsnSkel", "__OTR__objects/object_fsn/gFsnIdleAnim", 18, 0.012f },
    // mm/src/overlays/actors/ovl_En_Ani/z_en_ani.c (EN_ANI); ANI_LIMB_MAX
    { RI_SOUL_NPC_ROOFTOP_MAN, "__OTR__objects/object_ani/gAniSkel", "__OTR__objects/object_ani/gAniStandingNormalAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Zot/z_en_zot.c (EN_ZOT); ZORA_LIMB_MAX
    { RI_SOUL_NPC_ZORA, "__OTR__objects/object_zo/gZoraSkel", "__OTR__objects/object_zo/gZoraIdleAnim", 20, 0.012f },
    // mm/src/overlays/actors/ovl_En_Sob1/z_en_sob1.c (EN_SOB1); ZORA_LIMB_MAX
    { RI_SOUL_NPC_ZORA_SHOPKEEPER, "__OTR__objects/object_zo/gZoraSkel", "__OTR__objects/object_masterzoora/gZoraShopkeeperAnim", 20, 0.012f },
    // mm/src/overlays/actors/ovl_En_Ma4/z_en_ma4.c (EN_MA4); ROMANI_LIMB_MAX
    { RI_SOUL_NPC_MALON, "__OTR__objects/object_ma1/gRomaniSkel", "__OTR__objects/object_ma1/gRomaniIdleAnim", 23, 0.012f },
    // mm/src/overlays/actors/ovl_En_Ms/z_en_ms.c (EN_MS); BEAN_SALESMAN_LIMB_MAX
    { RI_SOUL_NPC_BEAN_SALESMAN, "__OTR__objects/object_ms/gBeanSalesmanSkel", "__OTR__objects/object_ms/gBeanSalesmanEatingAnim", 9, 0.02f },
    // mm/src/overlays/actors/ovl_En_Daiku/z_en_daiku.c (EN_DAIKU); OBJECT_DAIKU_LIMB_MAX
    { RI_SOUL_NPC_CARPENTERS, "__OTR__objects/object_daiku/object_daiku_Skel_00A850", "__OTR__objects/object_daiku/object_daiku_Anim_002FA0", 17, 0.012f },
    // mm/src/overlays/actors/ovl_En_An/z_en_an.c (EN_AN); ANJU1_LIMB_MAX
    { RI_SOUL_NPC_ANJU, "__OTR__objects/object_an1/gAnju1Skel", "__OTR__objects/object_an1/gAnju1IdleAnim", 21, 0.012f },
    // mm/src/overlays/actors/ovl_En_Guruguru/z_en_guruguru.c (EN_GURUGURU); GURU_GURU_LIMB_MAX
    { RI_SOUL_NPC_GURU_GURU, "__OTR__objects/object_fu/gGuruGuruSkel", "__OTR__objects/object_fu/gGuruGuruPlayStillAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Mk/z_en_mk.c (EN_MK); MARINE_RESEARCHER_LIMB_MAX
    { RI_SOUL_NPC_SCIENTIST, "__OTR__objects/object_mk/gMarineResearcherSkel", "__OTR__objects/object_mk/gMarineResearcherIdleAnim", 13, 0.012f },
    // mm/src/overlays/actors/ovl_En_Gm/z_en_gm.c (EN_GM); OBJECT_IN2_LIMB_MAX
    { RI_SOUL_NPC_GORMAN, "__OTR__objects/object_in2/object_in2_Skel_0078B0", "__OTR__objects/object_in2/object_in2_Anim_009CDC", 20, 0.012f },
    // mm/src/overlays/actors/ovl_En_Hs/z_en_hs.c (EN_HS); OBJECT_HS_LIMB_MAX
    { RI_SOUL_NPC_GROG, "__OTR__objects/object_hs/gHsSkel", "__OTR__objects/object_hs/gHsIdleAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Aob_01/z_en_aob_01.c (EN_AOB_01); MAMAMU_YAN_LIMB_MAX
    { RI_SOUL_NPC_DOG_LADY, "__OTR__objects/object_aob/gMamamuYanSkel", "__OTR__objects/object_aob/gMamamuYanIdleAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Kendo_Js/z_en_kendo_js.c (EN_KENDO_JS); OBJECT_JS_LIMB_MAX
    { RI_SOUL_NPC_CARPET_MAN, "__OTR__objects/object_js/object_js_Skel_006990", "__OTR__objects/object_js/object_js_Anim_000F4C", 13, 0.012f },
    // mm/src/overlays/actors/ovl_En_Nb/z_en_nb.c (EN_NB); NB_LIMB_MAX
    { RI_SOUL_NPC_OLD_HAG, "__OTR__objects/object_nb/gNbSkel", "__OTR__objects/object_nb/gNbIdleAnim", 8, 0.015f },
    // mm/src/overlays/actors/ovl_En_Shn/z_en_shn.c (EN_SHN); BURLY_GUY_LIMB_MAX
    { RI_SOUL_NPC_TOURIST_CENTER, "__OTR__objects/object_shn/gBurlyGuySkel", "__OTR__objects/object_shn/gBurlyGuyHandsOnTableAnim", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Ginko_Man/z_en_ginko_man.c (EN_GINKO_MAN); OBJECT_BOJ_LIMB_MAX
    { RI_SOUL_NPC_BANKER, "__OTR__objects/object_boj/object_boj_Skel_00C240", "__OTR__objects/object_boj/object_boj_Anim_0043F0", 16, 0.012f },
    // mm/src/overlays/actors/ovl_En_Js/z_en_js.c (EN_JS); MOONCHILD_LIMB_MAX
    { RI_SOUL_NPC_MOON_CHILDREN, "__OTR__objects/object_ob/gMoonChildSkel", "__OTR__objects/object_ob/gMoonChildStandingAnim", 18, 0.016f },
    // mm/src/overlays/actors/ovl_En_Ge1/z_en_ge1.c (EN_GE1); GERUDO_WHITE_LIMB_MAX
    { RI_SOUL_NPC_THIEVES, "__OTR__objects/object_ge1/gGerudoWhiteSkel", "__OTR__objects/object_ge1/gGerudoWhiteArmsFoldedAnim", 16, 0.012f },
};
// clang-format on

static void MM_MiniFallback(CwItemAnimDrawInfo* out) {
    out->stateDependent = 1;
    out->fallbackTokenPath = "__OTR__objects/object_gi_sutaru/gGiSkulltulaTokenDL";
    out->fallbackFlameColor[0] = out->fallbackFlameColor[1] = out->fallbackFlameColor[2] = 155;
    out->fallbackFlameColor[3] = 255;
}
static void MM_MiniColorSegment(CwItemAnimDrawInfo* out, int segment, int r, int g, int b, int a) {
    CwAnimColorSegment& color = out->colorSegments[out->colorSegmentCount++];
    color.segment = segment;
    color.onOpa = 1;
    color.prim[0] = color.prim[1] = color.prim[2] = color.prim[3] = 255;
    color.env[0] = r;
    color.env[1] = g;
    color.env[2] = b;
    color.env[3] = a;
}
static void MM_MiniSwapLimb(CwItemAnimDrawInfo* out, int limb, const char* path) {
    CwAnimLimbDL& dl = out->limbDLs[out->limbDLCount++];
    dl.limbIndex = limb;
    dl.dlPath = path;
}
static int32_t MM_FillNpcSoulAnim(RandoItemId id, CwItemAnimDrawInfo* out) {
    if (ComboSoulModels::SimplerNpc())
        return 0;
    const MmNpcSoulModel* model = nullptr;
    for (const auto& candidate : kMmNpcSoulModels) {
        if (candidate.id == id) {
            model = &candidate;
            break;
        }
    }
    if (!model)
        return 0;
    out->opa = 1;
    out->hiddenLimb = -1;
    out->skelPath = model->skel;
    out->animPath = model->anim;
    out->limbCount = model->limbs;
    out->scale = model->scale;
    out->translatePre[1] = -24.f;
    // Kafei uses PlayerAnimation, rather than AnimationHeader, in the actor.
    // Sample the native al_yareyare clip at frame zero through its OPAM pose resource.
    out->bindPose = model->anim == nullptr;
    if (id == RI_SOUL_NPC_KAFEI)
        out->posePath = "__OTR__misc/link_animetion/gPlayerAnim_al_yareyare_Data";
    MM_AnimEnv(out, 0, 0, 0, 255);
    MM_MiniFallback(out);
    const uint8_t grey[3] = { 155, 155, 155 };
    MM_AnimSoulFlame(out, grey, .15f / model->scale, .15f / model->scale, .15f / model->scale);
    switch (id) {
        case RI_SOUL_NPC_SHOOTING_GALLERY_OWNER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_shn/gTownShootingGalleryManEyeOpenTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_shn/gTownShootingGalleryManEyeOpenTex");
            MM_MiniSwapLimb(out, 15, "__OTR__objects/object_shn/gTownShootingGalleryManHeadDL");
            break;
        case RI_SOUL_NPC_BAZAAR_SHOPKEEPER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_shn/gSwampShootingGalleryManEyeOpenTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_shn/gSwampShootingGalleryManEyeOpenTex");
            break;
        case RI_SOUL_NPC_GORON:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_oF1d_map/gGoronEyeOpenTex");
            break;
        case RI_SOUL_NPC_GORON_CHILD:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_gk/object_gk_Tex_00F720");
            break;
        case RI_SOUL_NPC_BOMBCHU_SHOPKEEPER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_rsn/gBombShopkeeperEyeTex");
            break;
        case RI_SOUL_NPC_BOMBERS:
            out->hiddenLimbMask = (1ULL << 15) | (1ULL << 20);
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_cs/object_cs_Tex_00C520");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_cs/object_cs_Tex_00E620");
            MM_AnimTexSeg(out, 10, "__OTR__overlays/ovl_En_Bom_Bowl_Man/gEnBomBowlMan_D_809C61E0");
            break;
        case RI_SOUL_NPC_COMPOSER_BROS:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 12);
            MM_MiniColorSegment(out, 8, 255, 255, 210, 255);
            MM_MiniColorSegment(out, 10, 75, 20, 25, 255);
            MM_MiniColorSegment(out, 11, 90, 85, 50, 255);
            break;
        case RI_SOUL_NPC_DAMPE:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_tk/object_tk_Tex_004390");
            break;
        case RI_SOUL_NPC_DEKU_PRINCESS:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_dnq/gDekuPrincessEyeOpenTex");
            break;
        case RI_SOUL_NPC_PLAYGROUND_SCRUBS:
            out->hiddenLimbMask = 161579008ULL;
            MM_MiniSwapLimb(out, 26, "__OTR__objects/object_dnt/gBusinessScrubEyesDL");
            break;
        case RI_SOUL_NPC_CHEST_GAME_OWNER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_tsn/object_tsn_Tex_0073B8");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_tsn/object_tsn_Tex_0073B8");
            break;
        case RI_SOUL_NPC_GORON_SHOPKEEPER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_oF1d_map/gGoronEyeOpenTex");
            break;
        case RI_SOUL_NPC_HONEY_DARLING:
            MM_MiniColorSegment(out, 8, 0, 50, 160, 0);
            MM_MiniColorSegment(out, 9, 255, 255, 255, 0);
            break;
        case RI_SOUL_NPC_TOTO:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_zm/gTotoEyesOpenTex");
            break;
        case RI_SOUL_NPC_RUTO:
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_zov/gLuluEyeOpenTex");
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_zov/gLuluMouthClosedTex");
            break;
        case RI_SOUL_NPC_KAFEI:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_test3/gKafeiEyesOpenTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_test3/gKafeiMouthClosedTex");
            break;
        case RI_SOUL_NPC_KOUME_KOTAKE:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_trt/gKotakeEyeOpenTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_trt/gKotakeEyeOpenTex");
            break;
        case RI_SOUL_NPC_MAYOR_DOTOUR:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_dt/gDotourEyeOpenTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_dt/gDotourEyebrowMidTex");
            break;
        case RI_SOUL_NPC_MEDIGORON:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_oF1d_map/gGoronEyeOpenTex");
            break;
        case RI_SOUL_NPC_BIGGORON:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_dai/object_dai_Tex_0107B0");
            break;
        case RI_SOUL_NPC_TALON:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_tab/gBartenEyeOpenTex");
            break;
        case RI_SOUL_NPC_ASTRONOMER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_bji/object_bji_Tex_0049F0");
            break;
        case RI_SOUL_NPC_BLACKSMITHS:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_kbt/object_kbt_Tex_00B5B8");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_kbt/object_kbt_Tex_00A5B8");
            break;
        case RI_SOUL_NPC_BOMBCHU_BOWLING_LADY:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_bg/gTreasureChestShopGalEyeOpenDownTex");
            break;
        case RI_SOUL_NPC_TINGLE:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_bal/gTingleEyeOpenTex");
            break;
        case RI_SOUL_NPC_FISHING_POND_OWNER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_fsn/gFsnEyeOpenTex");
            break;
        case RI_SOUL_NPC_ROOFTOP_MAN:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_ani/gAniOpenEyeTex");
            break;
        case RI_SOUL_NPC_ZORA:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_zo/gZoraEyeOpenTex");
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 12);
            break;
        case RI_SOUL_NPC_ZORA_SHOPKEEPER:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_zo/gZoraEyeOpenTex");
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 12);
            break;
        case RI_SOUL_NPC_MALON:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_ma1/gRomaniEyeOpenTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_ma1/gRomaniMouthHappyTex");
            break;
        case RI_SOUL_NPC_ANJU:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_an1/gAnju1EyeOpenTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_an1/gAnju1MouthClosedTex");
            break;
        case RI_SOUL_NPC_GURU_GURU:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_fu/gGuruGuruEyeClosedTex");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_fu/gGuruGuruMouthOpenTex");
            break;
        case RI_SOUL_NPC_GORMAN:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_in2/object_in2_Tex_0054A8");
            break;
        case RI_SOUL_NPC_DOG_LADY:
            MM_MiniSwapLimb(out, 15, "__OTR__objects/object_aob/gMamamuYanHeadDL");
            MM_AnimTexSeg(out, 10, "__OTR__objects/object_aob/gMamamuYanEyeOpenTex");
            MM_MiniColorSegment(out, 8, 50, 80, 0, 0);
            MM_MiniColorSegment(out, 9, 50, 80, 0, 0);
            break;
        case RI_SOUL_NPC_TOURIST_CENTER:
            MM_MiniSwapLimb(out, 15, "__OTR__objects/object_shn/gSwampGuideHeadDL");
            break;
        case RI_SOUL_NPC_BANKER:
            MM_MiniSwapLimb(out, 15, "__OTR__objects/object_boj/object_boj_DL_00B1D8");
            MM_MiniColorSegment(out, 8, 50, 80, 0, 0);
            MM_MiniColorSegment(out, 9, 50, 80, 0, 0);
            break;
        case RI_SOUL_NPC_THIEVES:
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_ge1/gGerudoWhiteEyeOpenTex");
            break;
        case RI_SOUL_NPC_GROG:
            out->hiddenLimb = 11; // native HS_LIMB_HIDDEN_HAIR
            break;
        case RI_SOUL_NPC_POE_COLLECTOR:
            out->hiddenLimb = 1;
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 12);
            MM_AnimEnv(out, 255, 255, 255, 255);
            break;
        default:
            break;
    }
    return 1;
}
// Source: mm/2s2h/Rando/DrawFuncs.cpp DrawBat through DrawLikeLike.
static CwAnimPart* MM_MiniPart(CwItemAnimDrawInfo* out, const char* path, int xlu, int before) {
    CwAnimPart* part = &out->parts[out->partCount++];
    part->dlPath = path;
    part->xlu = xlu;
    part->beforeSkeleton = before;
    return part;
}
static int32_t MM_FillSpecialEnemySoulAnim(RandoItemId id, CwItemAnimDrawInfo* out) {
    CwAnimPart* part;
    CwAnimSegBind* scroll;
    float size = 10.f;
    switch (id) {
        case RI_SOUL_ENEMY_BAD_BAT:
            out->scale = .02f;
            size = 6.f;
            part = MM_MiniPart(out, "__OTR__objects/object_bat/gBadBatSetupDL", 0, 0);
            part = MM_MiniPart(out, "__OTR__objects/object_bat/gBadBatBodyDL", 0, 0);
            part = MM_MiniPart(out, "__OTR__objects/object_bat/gBadBatWingsFrame0DL", 0, 0);
            part->frameCount = 9;
            part->framePaths[0] = "__OTR__objects/object_bat/gBadBatWingsFrame0DL";
            part->framePaths[1] = "__OTR__objects/object_bat/gBadBatWingsFrame1DL";
            part->framePaths[2] = "__OTR__objects/object_bat/gBadBatWingsFrame2DL";
            part->framePaths[3] = "__OTR__objects/object_bat/gBadBatWingsFrame3DL";
            part->framePaths[4] = "__OTR__objects/object_bat/gBadBatWingsFrame4DL";
            part->framePaths[5] = "__OTR__objects/object_bat/gBadBatWingsFrame5DL";
            part->framePaths[6] = "__OTR__objects/object_bat/gBadBatWingsFrame6DL";
            part->framePaths[7] = "__OTR__objects/object_bat/gBadBatWingsFrame7DL";
            part->framePaths[8] = "__OTR__objects/object_bat/gBadBatWingsFrame8DL";
            break;
        case RI_SOUL_ENEMY_BOE:
            out->scale = .01f;
            out->translatePost[1] = -1200.f;
            size = 9.f;
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 8);
            part = MM_MiniPart(out, "__OTR__objects/object_mkk/gBlackBoeEndDL", 0, 0);
            part = MM_MiniPart(out, "__OTR__objects/object_mkk/gBlackBoeBodyMaterialDL", 1, 0);
            part->hasEnvColor = 1;
            part->envColor[0] = part->envColor[1] = part->envColor[2] = part->envColor[3] = 255;
            part->hasPrimColor = 1;
            part->primColor[3] = 255;
            part = MM_MiniPart(out, "__OTR__objects/object_mkk/gBlackBoeBodyModelDL", 1, 0);
            part->billboard = 1;
            part->hasPrimColor = 1;
            part->primColor[3] = 255;
            part = MM_MiniPart(out, "__OTR__objects/object_mkk/gBlackBoeEyesDL", 1, 0);
            part->billboard = 1;
            part->hasPrimColor = 1;
            part->primColor[0] = 245;
            part->primColor[1] = 97;
            part->primColor[3] = 255;
            part = MM_MiniPart(out, "__OTR__objects/object_mkk/gBlackBoeEyesDL", 1, 0);
            part->billboard = 1;
            part->hasPrimColor = 1;
            part->primColor[0] = 245;
            part->primColor[1] = 214;
            part->primColor[3] = 255;
            part->transform.scale[0] = part->transform.scale[1] = part->transform.scale[2] = .009f;
            break;
        case RI_SOUL_ENEMY_CHUCHU:
            out->scale = .01f;
            out->translatePost[1] = -2700.f;
            out->texAnimPath = "__OTR__objects/object_slime/gChuchuSlimeFlowTexAnim";
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_slime/gChuchuEyeOpenTex");
            out->segs[out->segCount - 1].onOpa = 0;
            out->segs[out->segCount - 1].onXlu = 1;
            scroll = MM_AnimSeg(out, CW_ANIM_SEG_XLU_RENDERMODE_1CY, 12);
            scroll->onOpa = 0;
            scroll->onXlu = 1;
            part = MM_MiniPart(out, "__OTR__objects/object_slime/gChuchuBodyDL", 1, 0);
            part->hasPrimColor = part->hasEnvColor = 1;
            part->primLodFrac = 100;
            part->primColor[0] = part->primColor[1] = part->primColor[3] = 255;
            part->primColor[2] = 200;
            part->envColor[0] = part->envColor[3] = 255;
            part->envColor[1] = 180;
            part = MM_MiniPart(out, "__OTR__objects/object_slime/gChuchuEyesDL", 1, 0);
            part->hasPrimColor = part->hasEnvColor = 1;
            part->primLodFrac = 100;
            part->primColor[0] = part->primColor[1] = part->primColor[3] = 255;
            part->primColor[2] = 200;
            part->envColor[0] = part->envColor[3] = 255;
            part->envColor[1] = 180;
            break;
        case RI_SOUL_ENEMY_FREEZARD:
            out->scale = .006f;
            out->translatePost[1] = -4100.f;
            size = 20.f;
            scroll = MM_AnimSeg(out, CW_ANIM_SEG_TEXSCROLL, 8);
            scroll->onOpa = 0;
            scroll->onXlu = 1;
            scroll->width1 = scroll->width2 = scroll->height1 = scroll->height2 = 32;
            scroll->yStep1 = 1;
            scroll->yStep2 = 2;
            scroll->yMask1 = scroll->yMask2 = 127;
            part = MM_MiniPart(out, "__OTR__objects/object_fz/object_fz_DL_001130", 1, 0);
            part->materialProfile = CW_ANIM_MATERIAL_FREEZARD;
            part->hasPrimColor = part->hasEnvColor = 1;
            part->primLodFrac = 128;
            part->primColor[0] = 155;
            part->primColor[1] = part->primColor[2] = part->primColor[3] = 255;
            part->envColor[0] = part->envColor[1] = part->envColor[2] = 200;
            part->envColor[3] = 255;
            break;
        case RI_SOUL_ENEMY_LIKE_LIKE:
            out->scale = .01f;
            out->translatePost[1] = -3000.f;
            out->matrixCount = 4;
            out->matrixSegment = 12;
            for (int i = 0; i < 4; ++i) {
                out->matrices[i].translate[1] = (i + 1) * 1000.f;
                out->matrices[i].scale[0] = out->matrices[i].scale[2] = .8f;
            }
            scroll = MM_AnimSeg(out, CW_ANIM_SEG_TEXSCROLL, 8);
            scroll->width1 = scroll->width2 = 32;
            scroll->height1 = scroll->height2 = 16;
            scroll->yStep2 = -6;
            scroll->yMask2 = 127;
            part = MM_MiniPart(out, "__OTR__objects/object_rr/gLikeLikeDL", 0, 0);
            part->transform.scale[0] = part->transform.scale[2] = .8f;
            break;
        case RI_SOUL_ENEMY_DEXIHAND:
            out->skelPath = "__OTR__objects/object_wdhand/gDexihandSkel";
            out->animPath = "__OTR__objects/object_wdhand/gDexihandIdleAnim";
            out->limbCount = 8;
            out->scale = 0.015f;
            out->translatePost[1] = -1000.f;
            size = 10.f;
            part = MM_MiniPart(out, "__OTR__objects/object_wdhand/gDexihandBaseDL", 0, 1);
            part = MM_MiniPart(out, "__OTR__objects/object_wdhand/gDexihandArmSegmentDL", 0, 1);
            out->skeletonTranslate[1] = 2500.f;
            out->flameTranslate[1] = 2500.f;
            break;
        case RI_SOUL_ENEMY_GOMESS:
            out->skelPath = "__OTR__objects/object_death/gGomessSkel";
            out->animPath = "__OTR__objects/object_death/gGomessFloatAnim";
            out->limbCount = 22;
            out->scale = 0.005f;
            out->translatePost[1] = 0.f;
            size = 15.f;
            out->texAnimPath = "__OTR__objects/object_death/gGomessBodyMatAnim";
            out->texAnimStep = 24;
            out->texAnimPath2 = "__OTR__objects/object_death/gGomessCoreMatAnim";
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 12);
            MM_AnimEnv(out, 30, 30, 0, 255);
            break;
        case RI_SOUL_ENEMY_IRON_KNUCKLE:
            out->skelPath = "__OTR__objects/object_ik/gIronKnuckleSkel";
            out->animPath = "__OTR__objects/object_ik/gIronKnuckleWalkAnim";
            out->limbCount = 30;
            out->scale = 0.01f;
            out->translatePost[1] = -2900.f;
            size = 12.f;
            MM_AnimTexSeg(out, 8, "__OTR__objects/object_ik/gIronKnuckleBlackArmorMaterialDL");
            MM_AnimTexSeg(out, 9, "__OTR__objects/object_ik/gIronKnuckleBrownArmorMaterialDL");
            MM_AnimTexSeg(out, 10, "__OTR__objects/object_ik/gIronKnuckleBrownArmorMaterialDL");
            out->limbDLCount = 1;
            out->limbDLs[0].limbIndex = 12;
            out->limbDLs[0].postXlu = 1;
            out->limbDLs[0].postDlPath = "__OTR__objects/object_ik/gIronKnuckleHelmetMarkingDL";
            break;
        default:
            return 0;
    }
    out->opa = 1;
    out->hiddenLimb = -1;
    MM_MiniFallback(out);
    const uint8_t flame[3] = { 155, static_cast<uint8_t>(id == RI_SOUL_ENEMY_GOMESS ? 0 : 155),
                               static_cast<uint8_t>(id == RI_SOUL_ENEMY_GOMESS ? 0 : 155) };
    MM_AnimSoulFlame(out, flame, size, size, size);
    return 1;
}
