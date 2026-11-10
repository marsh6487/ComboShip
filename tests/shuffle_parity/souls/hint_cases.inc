#include <cassert>
#include <set>
static bool HasClue(const nlohmann::json& output, const std::string& name, const std::string& area) {
    for (const auto& hint : output.at("oot")) {
        for (const auto& message : hint.at("messages")) {
            const auto text = message.at("en").get<std::string>();
            if (text.find(name) != std::string::npos && text.find(area) != std::string::npos &&
                text.find("No Hint") == std::string::npos && text.find("[[") == std::string::npos)
                return true;
        }
    }
    return false;
}
int main() {
    using nlohmann::json;
    Rando::StaticData::hintTextTable[RHT_NONE] = HintText(CustomMessage("No Hint", "Kein Hinweis", "Pas d'Indice"));
    InitItems();
    InitSoulHints();
    InitTemplates();
    std::set<RandomizerHintTextKey> keys;
    std::set<std::string> frenchNames,germanNames;
    json exported = json::array();
    for (const auto& [id, soul] : itemTable) {
        const auto name = soul.GetName().GetEnglish();
        assert(soul.GetName().GetFrench()!=name && soul.GetName().GetGerman()!=name);
        assert(frenchNames.insert(soul.GetName().GetFrench()).second);
        assert(germanNames.insert(soul.GetName().GetGerman()).second);
        assert(soul.GetHintKey() != RHT_NONE && keys.insert(soul.GetHintKey()).second);
        assert(soul.GetHint().GetClear().GetEnglish(MF_ENCODE) == name);
        Rando::Context::GetInstance()->locations[1].placed = &soul;
        const Rando::Hint nativeHint;
        assert(nativeHint.GetItemHintText(0).GetClear().GetEnglish(MF_ENCODE) == name);
        assert(soul.GetHint().GetAmbiguous(0).GetEnglish(MF_ENCODE) == name);
        assert(soul.GetHint().GetObscure(0).GetEnglish(MF_ENCODE) == name);
        const auto hint = exportItem(soul);
        assert(hint.at("clear").at("en") == name);
        assert(hint.at("clear").at("fr") == soul.GetName().GetFrench());
        assert(hint.at("clear").at("de") == soul.GetName().GetGerman());
        auto composed=Rando::StaticData::hintTextTable[RHT_CAN_BE_FOUND_AT].GetClear();
        composed.Replace("[[1]]",soul.GetHint().GetClear());
        composed.Replace("[[2]]",CustomMessage("Kokiri Forest","Kokiri-Wald","Forêt Kokiri"));
        const std::vector<std::string> localizedRewards={name,soul.GetName().GetGerman(),soul.GetName().GetFrench()};
        const std::vector<std::string> localizedSources={"Kokiri Forest","Kokiri-Wald","Forêt Kokiri"};
        for(int language:{LANGUAGE_ENG,LANGUAGE_GER,LANGUAGE_FRA}){
            auto encoded=composed.GetForLanguage(language,MF_ENCODE);
            assert(encoded.find("%g"+localizedRewards[language]+"%w")!=std::string::npos);
            assert(encoded.find("%r"+localizedSources[language]+"%w")!=std::string::npos);
            assert(encoded.find("[[")==std::string::npos&&encoded.find('#')==std::string::npos);
            composed.ReplaceColors(encoded);
            assert(encoded.find(CustomMessage::COLOR(QM_GREEN)+localizedRewards[language]+CustomMessage::COLOR(QM_WHITE))!=std::string::npos);
            assert(encoded.find(CustomMessage::COLOR(QM_RED)+localizedSources[language]+CustomMessage::COLOR(QM_WHITE))!=std::string::npos);
        }
        exported.push_back({{"name",name},{"hint",hint}});
    }
    assert(itemTable.at(RG_SOUL_ENEMY_OCTOROK).GetName().GetGerman()=="Gegnerseele: Oktorok");
    assert(itemTable.at(RG_SOUL_NPC_SARIA).GetName().GetGerman()=="Personenseele: Salia");
    assert(itemTable.at(RG_SOUL_NPC_DAMPE).GetName().GetFrench()=="Âme de personnage : Igor");
    assert(itemTable.at(RG_SOUL_ANIMAL_COW).GetName().GetFrench()=="Âme animale : Vache");
    assert(keys.size() == 147);
    assert(Rando::StaticData::hintTextTable[RHT_NONE].GetClear().GetEnglish(MF_ENCODE) == "No Hint");
    json hd{{"options",{{"gossipStoneHints",1},{"hintDistribution",1},{"hintClarity",2}}},
            {"items",exported},{"stones",json::array({"SFM Saria Gossip Stone"})},
            {"checks",json::array({{{"name","KF Kokiri Sword Chest"},{"area","Kokiri Forest"}}})},
            {"hintTextTable",{{"RHT_CAN_BE_FOUND_AT",Combo_HintTextToJson(Rando::StaticData::hintTextTable[RHT_CAN_BE_FOUND_AT])},
                              {"RHT_HOARDS",Combo_HintTextToJson(Rando::StaticData::hintTextTable[RHT_HOARDS])}}}};
    // All real native/imported soul item rows flow through native export and
    // the actual combined-seed hint generator, not a label-only test.
    for (const auto& [id, soul] : itemTable) {
        const auto name = soul.GetName().GetEnglish();
        json seed{{"oot",{{"KF Kokiri Sword Chest",name}}},{"mm",json::object()}};
        for (int clarity = 0; clarity < 3; ++clarity) {
            hd["options"]["hintClarity"] = clarity;
            const auto output = ComboRando::Generate(17,"{}",hd.dump(),"{}",json::array(),seed.dump(),{});
            assert(HasClue(output,name,"Kokiri Forest"));
            const auto& gossip = output.at("mm").at("gossipPool");
            assert(gossip.size() == 1);
            assert(gossip[0]["text"] == name + " can be found at Kokiri Forest (OOT)");
        }
    }
    const char* names[] = {"Octorok Enemy Soul","Dampe NPC Soul","Dog Animal Soul","Gold Skulltula Misc Soul"};
    for (const char* name : names) {
        json foreign = json::array({{{"checkGame","mm"},{"checkName","Astral Observatory Passage Chest"},
                                      {"itemGame","oot"},{"itemName",name},{"shared",true},{"advancement",true}}});
        json seed{{"oot",json::object()},{"mm",{{"Astral Observatory Passage Chest",name}}},
                  {"foreign",foreign},{"sharedItems",{"sharedSoulsEnemy","sharedSoulsNpc","sharedSoulsAnimal","sharedSoulsMisc"}}};
        json mm{{"locationHints",{{"Astral Observatory Passage Chest","in Astral Observatory"}}}};
        const auto placement = ComboRando::ParseSpoilerPlacements(seed.dump());
        assert(placement.size() == 1 && placement[0].checkGame == ComboRando::GAME_MM);
        assert(placement[0].itemGame == ComboRando::GAME_OOT && placement[0].check == "Astral Observatory Passage Chest");
        const auto output = ComboRando::Generate(17,"{}",hd.dump(),mm.dump(),foreign,seed.dump(),{});
        assert(HasClue(output,name,"Astral Observatory"));
        json excluded{{"fixed",json::array({{{"check","Astral Observatory Passage Chest"},{"hintable",false}}})},
                      {"locationHints",mm.at("locationHints")}};
        const auto hidden = ComboRando::Generate(17,"{}",hd.dump(),excluded.dump(),foreign,seed.dump(),{});
        assert(!HasClue(hidden,name,"Astral Observatory"));
        seed["startKnown"] = json::array({{{"checkGame","mm"},{"checkName","Astral Observatory Passage Chest"}}});
        const auto owned = ComboRando::Generate(17,"{}",hd.dump(),mm.dump(),foreign,seed.dump(),{});
        assert(!HasClue(owned,name,"Astral Observatory"));
    }
    std::cout << "PASS 147 actual soul rows/native item hints, multilingual registration, all clarities, real combined-seed names/locations and excluded/start-owned controls\n";
}
