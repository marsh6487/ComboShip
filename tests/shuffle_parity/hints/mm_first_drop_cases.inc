int main() {
    using namespace Rando;
    const auto check = RC_ENEMY_DROP_OCTOROK;
#define RC(id,type,scene,flagType,flag,item) {id,{id,#id,type,scene,flagType,flag,item}}
    const std::map<RandoCheckId,StaticData::RandoStaticCheck> nativeDrops = {
#include "first_drop_checks.inc"
    };
#undef RC
    assert(nativeDrops.size() == 56);
    StaticData::Checks.insert(nativeDrops.begin(),nativeDrops.end());
    const auto ordinary = RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST;
    StaticData::Checks[ordinary] = {ordinary,"RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST",RCTYPE_CHEST,SCENE_TENMON_DAI,FLAG_NONE,0,RI_JUNK};
    StaticData::PopulateCheckNames();
    RANDO_SAVE_CHECKS[check].shuffled = true;
    RANDO_SAVE_CHECKS[check].randoItemId = RI_PROGRESSIVE_MAGIC;
    Logic::Regions[static_cast<RandoRegionId>(0)].checks[check] = { [] { return true; }, "native reachability boundary" };
    Logic::Regions[static_cast<RandoRegionId>(0)].checks[ordinary] = { [] { return true; }, "ordinary source boundary" };
    SetTier(RI_DOUBLE_MAGIC);
    std::vector<RandoItemId> wish{RI_PROGRESSIVE_MAGIC};
    SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando,wish);
    assert(GetSariaHintTarget().check == check);
    assert(GetSariaHintTarget().item == RI_DOUBLE_MAGIC);
    assert(StaticData::GetLocationNameForHint(check,true) == "at Enemy Drop Octorok");
    for (const auto& [drop, metadata] : nativeDrops) {
        const auto text = StaticData::GetLocationNameForHint(drop,false);
        assert(metadata.randoCheckType == RCTYPE_ENEMY_DROP);
        assert(text.starts_with("in the first ") && text.ends_with(" drop"));
        assert(text.find("foe") == std::string::npos);
    }
    assert(StaticData::GetLocationNameForHint(RC_ENEMY_DROP_BLUE_BUBBLE,false) == "in the first Blue Bubble drop");
    const auto vague = StaticData::GetLocationNameForHint(check,false);
    std::fprintf(stderr,"observed MM first-drop vague source: %s\n",vague.c_str());
    assert(vague == "in the first Octorok drop");
    configuredUpgrades[std::string(CVAR_SENSOR_DESIRE_PREFIX)+"0"] = RI_PROGRESSIVE_MAGIC;
    assert(SensorRune_PrepareHint());
    assert(sSensorHintText == "The slate senses %ga Magic Upgrade%w %yin the first Octorok drop%w...");
    RANDO_SAVE_CHECKS[check].obtained = true;
    assert(!GetSariaHintTarget().IsValid() && !SensorRune_PrepareHint());
    RANDO_SAVE_CHECKS[check].obtained = false;
    RANDO_SAVE_CHECKS[check].skipped = true;
    assert(!GetSariaHintTarget().IsValid() && !SensorRune_PrepareHint());
    RANDO_SAVE_CHECKS[check].skipped = false;
    reachable = false;
    assert(!GetSariaHintTarget().IsValid());
    assert(SensorRune_PrepareHint()); // explicit raw wish retains inaccessible placement
    reachable = true;
    assert(StaticData::GetLocationNameForHint(ordinary,true) == "at Astral Observatory Passage Chest");
    assert(StaticData::GetLocationNameForHint(ordinary,false) == "in Fixture scene boundary");
    assert(StaticData::GetLocationNameForHint(RC_UNKNOWN,false) == "in an Unknown Location");
    using nlohmann::json;
    json seed{{"oot",json::object()},{"mm",{{"Enemy Drop Octorok","Progressive Magic"}}}};
    json hd{{"options",{{"gossipStoneHints",1},{"hintDistribution",1},{"hintClarity",2}}},
            {"stones",json::array({"SFM Saria Gossip Stone"})},
            {"hintTextTable",{{"RHT_CAN_BE_FOUND_AT",{{"clear",{{"en","They say that [[1]] can be found at [[2]]."},{"de","[[1]] [[2]]"},{"fr","[[1]] [[2]]"}}}}}}}};
    json mm{{"locationHints",{{"Enemy Drop Octorok",vague}}},
            {"items",json::array({{{"name","Progressive Magic"},{"displayName","Progressive Magic"},{"weightClass",9}}})}};
    const auto hints = ComboRando::Generate(17,"{}",hd.dump(),mm.dump(),json::array(),seed.dump(),{});
    bool rendered = false;
    for (const auto& hint : hints.at("oot")) for (const auto& message : hint.at("messages"))
        if (message.at("en") == "They say that Progressive Magic can be found at the first Octorok drop.") rendered = true;
    assert(rendered);
    puts("PASS actual MM first-drop source/current tier, Desired text, exclusions and combined grammar");
}
