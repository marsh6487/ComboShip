// Real OoT descriptor; authored GI unavailable. Each root must remain a portable legacy recipe.
{
    struct Legacy {
        RandomizerGet id;
        const char* path;
        float scale;
    };
    const Legacy rows[] = {
        { RG_DOMINION_ROD, "__OTR__objects/nei_legacy_gi/dominion_rod/gi_dl", 2.5f },
        { RG_ELEMENTAL_WAND, "__OTR__objects/nei_legacy_gi/dominion_rod/gi_dl", 2.5f },
        { RG_WHIP, "__OTR__objects/object_nei_whip/whip_give_opaque_dl", 0.5f },
        { RG_SPINNER, "__OTR__objects/object_nei_spinner/n0b0_opaque_dl", 0.3f },
        { RG_DEKU_LEAF, "__OTR__objects/object_nei_deku_leaf/g_dekuleaf_dl", 0.5f },
        { RG_SWITCH_HOOK, "__OTR__objects/object_nei_switchhook/gSwitchHookGiveDL", 0.01f },
        { RG_MOGMA_MITTS, "__OTR__objects/object_nei_mogma_mitts/gMogmaMittsGiveDL", 0.5f },
        { RG_GUST_JAR, "__OTR__objects/object_nei_gust_jar/jar_model_dl", 5.0f },
        { RG_TIME_GATE, "__OTR__objects/object_nei_time_gate/g_timegate_dl", 0.5f },
        { RG_MINISH_CAP, "__OTR__objects/object_nei_minish_cap/Cylinder_opaque_dl", 0.5f },
        { RG_ROCS_CAPE, "__OTR__objects/object_nei_rocs_cape/rocs_cape_mesh_dl", 0.6f },
        { RG_FIRE_ROD, "__OTR__objects/object_nei_fire_rod/Cylinder_001_opaque_dl", 0.2f },
        { RG_ICE_ROD, "__OTR__objects/object_nei_ice_rod/ice_rod_opaque_dl", 0.2f },
        { RG_LIGHT_ROD, "__OTR__objects/object_nei_light_rod/Cylinder_002_opaque_dl", 0.2f },
        { RG_BALL_AND_CHAIN, "__OTR__objects/object_nei_ball_and_chain/g_ball_and_chain_dl", 0.25f },
        { RG_BEETLE, "__OTR__objects/object_nei_beetle/g_beetle_dl", 0.3f },
        { RG_SHOVEL, "__OTR__objects/object_nei_shovel/gShovelGiveDL_opaque_dl", 0.2f },
        { RG_HYLIAS_GRACE, "__OTR__objects/object_nei_magic_spell/gHyliaGraceGiveDL", 1.0f },
        { RG_ZONAI_PERMAFROST, "__OTR__objects/object_nei_magic_spell/gZonaiPermafrostGiveDL", 1.0f },
        { RG_DEMISE_DESTRUCTION, "__OTR__objects/object_nei_magic_spell/gDemiseDestructionGiveDL", 1.0f },
    };
    for (const auto& row : rows) {
        resources.insert(row.path);
        concrete = row.id;
        CwItemDrawInfo info{};
        assert(OOT_FillItemDrawInfo(row.id, &info) == 1 && "missing optional GI must retain native custom model");
        assert(info.drawKind == CW_DRAW_KIND_CUSTOM_GI && info.dlistCount == 1 && !strcmp(info.dlists[0], row.path) &&
               info.scale == row.scale);
    }
    for (auto id : { RG_DOMINION_ROD, RG_ELEMENTAL_WAND }) {
        authoredAvailable = true;
        concrete = id;
        CwItemDrawInfo info{};
        assert(OOT_FillItemDrawInfo(id, &info) == 1 && info.drawKind == CW_DRAW_KIND_NEI_GI &&
               !strcmp(info.dlists[0], "__OTR__authored_optional"));
    }
    authoredAvailable = false;
    concrete = RG_NONE;
    resources.clear();
}
