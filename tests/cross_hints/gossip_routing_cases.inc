using namespace ComboRando;
void require(bool value,const char* message) {
    if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}
}
json hintText(const std::string& en) {
    return {{"clear",{{"en",en},{"de",en},{"fr",en}}},{"ambiguous",json::array()},{"obscure",json::array()}};
}
json GenerateCase(bool repaired,int clarity,uint32_t seed,int distribution=1,bool disguised=false,int stoneCount=8) {
    const Rando::Item a(Text("OOT Native Relic","Relique native","Heimisches Relikt"),RHT_NONE);
    const Rando::Item b(Text("OOT Abroad Relic","Relique lointaine","Fremdes Relikt"),RHT_NONE);
    json sentinel=Combo_HintTextToJson(Rando::StaticData::hintTextTable[RHT_NONE]);
    json hints={{"options",{{"gossipStoneHints",1},{"hintClarity",clarity},{"hintDistribution",distribution}}},
                {"stones",json::array()},
                {"checks",json::array({{{"name","Oot Home"},{"area","Oot Home Area"},{"overworld",true},{"locationHint",hintText("Home has [[1]].")}},
                                       {{"name","Oot Foreign"},{"area","Oot Foreign Area"},{"dungeon",true},{"locationHint",hintText("Foreign has [[1]].")}},
                                       {{"name","Oot Junk"},{"area","Oot Junk Area"},{"overworld",true}}})},
                {"items",json::array({{{"name","OOT Native Relic"},{"hint",repaired?exportItem(a):sentinel}},
                                      {{"name","OOT Abroad Relic"},{"hint",repaired?exportItem(b):sentinel}},
                                      {{"name","Junk Relic"},{"hint",sentinel}}})},
                {"hintTextTable",{{"RHT_CAN_BE_FOUND_AT",hintText("[[1]] at [[2]].")},
                                   {"RHT_HOARDS",hintText("[[2]] hoards [[1]].")},
                                   {"RHT_JUNK01",hintText("A harmless junk rumor.")}}}};
    for(int i=0;i<stoneCount;++i)hints["stones"].push_back(i);
    json oot={{"pool",json::array({{{"name","OOT Native Relic"},{"advancement",true}},
                                    {{"name","OOT Abroad Relic"},{"advancement",true}},
                                    {{"name","Junk Relic"},{"advancement",false}}})}};
    json mm={{"items",json::array({{{"name","RI_NATIVE_RELIC"},{"displayName","MM Native Relic"}},
                                   {{"name","RI_FOREIGN_RELIC"},{"displayName","MM Abroad Relic"}}})},
             {"pool",json::array({{{"name","RI_NATIVE_RELIC"},{"advancement",true}}})},
             {"locationHints",{{"Mm Home","in Mm Home Area"},{"Mm Foreign","in Mm Foreign Area"}}}};
    json foreign=json::array({{{"checkGame","oot"},{"checkName","Oot Foreign"},{"itemGame","mm"},
                                {"itemName","RI_FOREIGN_RELIC"},{"advancement",true}},
                              {{"checkGame","mm"},{"checkName","Mm Foreign"},{"itemGame","oot"},
                                {"itemName","OOT Abroad Relic"},{"advancement",true}}});
    if(disguised)foreign[1]["fakeItemName"]="OOT Native Relic";
    json spoiler={{"oot",{{"Oot Home","OOT Native Relic"},{"Oot Foreign","placeholder"},{"Oot Junk","Junk Relic"}}},
                  {"mm",{{"Mm Home","RI_NATIVE_RELIC"},{"Mm Foreign","placeholder"}}},{"foreign",foreign}};
    return Generate(seed,oot.dump(),hints.dump(),mm.dump(),foreign,spoiler.dump(),{});
}
void CheckGeneration() {
    Rando::StaticData::hintTextTable[RHT_NONE]=HintText(CustomMessage("No Hint","Kein Hinweis","Pas d'Indice"));
    for(int clarity=0;clarity<3;++clarity)for(uint32_t seed:{1u,0x12345678u,0xffffffffu}) {
        auto before=GenerateCase(false,clarity,seed),after=GenerateCase(true,clarity,seed);
        auto encoded=after.dump();
        for(const char* name:{"OOT Native Relic","OOT Abroad Relic","MM Native Relic","MM Abroad Relic"})
            require(encoded.find(name)!=std::string::npos,"full generator omitted a native/cross-game item name");
        require(encoded.find("No Hint")==std::string::npos,"full generator emitted RHT_NONE item sentinel");
        require(after["mm"]["gossipPool"].size()==2,"MM cross pool must use only OOT checks and exclude junk items");
        std::set<std::string> crossTexts;
        for(auto& row:after["mm"]["gossipPool"])crossTexts.insert(row["text"]);
        require(crossTexts==std::set<std::string>{"OOT Native Relic can be found at Oot Home Area (OOT)",
                 "MM Abroad Relic can be found hoarded in Oot Foreign Area (OOT)"},"MM cross-pool item/check origin or region routing changed");
        require(after["stats"]["junkProduced"]==4,"exhausted candidate pool must retain genuine junk fill");
        auto normalized=after;
        for(auto& row:normalized["oot"])for(auto& message:row["messages"])for(auto lang:{"en","de","fr"}) {
            std::string value=message[lang];
            std::vector<std::string> words=lang==std::string("en")?std::vector<std::string>{"OOT Native Relic","OOT Abroad Relic"}:
                lang==std::string("de")?std::vector<std::string>{"Heimisches Relikt","Fremdes Relikt"}:
                                        std::vector<std::string>{"Relique native","Relique lointaine"};
            const std::string old=lang==std::string("en")?"No Hint":lang==std::string("de")?"Kein Hinweis":"Pas d'Indice";
            for(auto& name:words){size_t p;while((p=value.find(name))!=std::string::npos)value.replace(p,name.size(),old);}
            message[lang]=value;
        }
        for(auto& row:normalized["mm"]["gossipPool"]){std::string value=row["text"];for(const char* name:{"OOT Native Relic","OOT Abroad Relic"}){
            size_t p;while((p=value.find(name))!=std::string::npos)value.replace(p,std::strlen(name),"No Hint");}row["text"]=value;}
        require(normalized==before,"item fallback changed slots/categories/weights or unrelated junk text");
        const auto junk=GenerateCase(true,clarity,seed,0);
        require(junk["stats"]["junkProduced"]==8,"Useless distribution must retain all eight junk stones");
        for(auto& row:junk["oot"])require(row["type"]=="junk"&&row["messages"][0]["en"]=="A harmless junk rumor.","genuine junk fallback changed");
        const auto trap=GenerateCase(true,clarity,seed,1,true);
        bool sawTrap=false;
        for(auto& row:trap["oot"])for(auto& message:row["messages"]) {
            const std::string en=message["en"];
            if(en.find("Mm Foreign Area")!=std::string::npos) {
                require(en=="OOT Native Relic at Mm Foreign Area.","full generator leaked trap's true item instead of fallback disguise");
                require(message["de"]=="Heimisches Relikt at Mm Foreign Area."&&message["fr"]=="Relique native at Mm Foreign Area.","full generator lost translated disguise text");
                sawTrap=true;
            }
        }
        require(sawTrap,"full generator omitted cross-game trap disguise route");
        // Very Strong has no Random category, forcing the authored location/region branches.
        const auto locations=GenerateCase(true,clarity,seed,3);
        std::set<std::string> locationTexts;
        for(auto& row:locations["oot"])locationTexts.insert(row["messages"][0]["en"]);
        require(locationTexts==std::set<std::string>{"Home has OOT Native Relic.","Foreign has MM Abroad Relic.",
                 "MM Native Relic at Mm Home Area.","OOT Abroad Relic at Mm Foreign Area.","A harmless junk rumor."},
                "full generator misrouted one of four item-game/check-game combinations");
    }
    std::cout<<"PASS full native Generate: four game/check routes, all clarity tiers, stable categories/slots/weights and genuine junk\n";
}
void CheckOotClaimContract() {
    InitLocations();Rando::StaticData::locationNameToEnum["A physical stone"]=RC_ZF_FAIRY_GOSSIP_STONE;
    json entries=json::array();
    auto add=[&](const char* key){entries.push_back({{"checkName",key},{"messages",json::array({{{"en",key},{"de",key},{"fr",key}}})}});};
    for(const char* key:{"__GANONDORF__","__ALTAR_CHILD__","__TRIAL__control","__STATIC__RH_DAMPES_DIARY",
                         "__STONE__0","__JUNK__0","A physical stone","Missing stone","__STATIC__UNKNOWN"})add(key);
    add("__GANONDORF__");add("A physical stone");
    entries.push_back({{"checkName","__STONE__empty"},{"messages",json::array()}});
    std::map<RandomizerHint,std::string> claims;int applied=0,skipped=0;
    Combo_WalkComboHints({{"oot",entries}},[&](RandomizerHint rh){return claims.count(rh);},
        [&](RandomizerHint rh,const std::string& key,std::vector<CustomMessage>&){claims[rh]=key;},applied,skipped);
    const std::map<RandomizerHint,std::string> expected={{RH_GANONDORF_HINT,"__GANONDORF__"},{RH_ALTAR_CHILD,"__ALTAR_CHILD__"},
        {RH_KF_DEKU_TREE_LEFT_GOSSIP_STONE,"__TRIAL__control"},{RH_DAMPES_DIARY,"__STATIC__RH_DAMPES_DIARY"},
        {RH_KF_DEKU_TREE_RIGHT_GOSSIP_STONE,"__STONE__0"},{RH_KF_GOSSIP_STONE,"__JUNK__0"},{RH_ZF_FAIRY_GOSSIP_STONE,"A physical stone"}};
    require(claims==expected&&applied==7&&skipped==5,"literal special/ordinal/named-stone claiming or duplicate rejection changed");
    claims={{RH_KF_DEKU_TREE_LEFT_GOSSIP_STONE,"already claimed"}};applied=skipped=0;
    Combo_WalkComboHints({{"oot",entries}},[&](RandomizerHint rh){return claims.count(rh);},
        [&](RandomizerHint rh,const std::string& key,std::vector<CustomMessage>&){claims[rh]=key;},applied,skipped);
    require(claims.at(RH_KF_DEKU_TREE_LEFT_GOSSIP_STONE)=="already claimed"&&
        claims.at(RH_KF_DEKU_TREE_RIGHT_GOSSIP_STONE)=="__TRIAL__control"&&claims.at(RH_KF_GOSSIP_STONE)=="__STONE__0"&&
        claims.at(RH_KF_STORMS_GROTTO_GOSSIP_STONE)=="__JUNK__0","preclaimed native stone was overwritten or ordinal hint drifted");
    std::cout<<"PASS literal OOT slot/special/named-stone claiming, invalid entries and preclaimed slots\n";
}
json CheckOotRouting(const json& hints,const json& savedRows=json::array()) {
    InitLocations();context.hints.clear();int applied=0,skipped=0;
    std::map<RandomizerHint,std::string> keys;
    Combo_WalkComboHints(hints,[&](RandomizerHint rh){return context.hints.count(rh);},
        [&](RandomizerHint rh,const std::string& key,std::vector<CustomMessage>& msgs){context.hints[rh]={msgs};keys[rh]=key;},applied,skipped);
    require(skipped==0,"valid save hint entry skipped during native application");
    require(applied==int(hints.at("oot").size()),"native apply dropped a saved hint");
    std::map<RandomizerHint,std::string> replay;int replayed=0,replaySkipped=0;
    Combo_WalkComboHints(hints,[&](RandomizerHint rh){return replay.count(rh);},
        [&](RandomizerHint rh,const std::string& key,std::vector<CustomMessage>&){replay[rh]=key;},replayed,replaySkipped);
    require(keys==replay,"apply and lazy tracker replay disagree on physical stone keys");
    json rows=json::array();Actor stone,chest;player.talkActor=&stone;
    for(auto rc:Rando::StaticData::GetGossipStoneLocations()) {
        const auto rh=Rando::StaticData::gossipStoneCheckToHint.at(rc);
        if(!keys.count(rh))continue;
        const auto& stored=context.hints.at(rh);
        auto message=stored.GetHintMessage(MF_RAW,0);
        if(!savedRows.empty()) {
            const auto& saved=savedRows.at(int(rh));require(saved.value("enabled",false),"physical stone disabled in saved native cache");
            require(saved["comboMessagesEn"][0]==message.GetEnglish(MF_RAW),"saved English cache differs from routed canonical hint");
            require(saved["comboMessagesDe"][0]==message.GetGerman(MF_RAW),"saved German cache differs from routed canonical hint");
            require(saved["comboMessagesFr"][0]==message.GetFrench(MF_RAW),"saved French cache differs from routed canonical hint");
        }
        const int actorParam=locationActorParams.at(rc);
        for(auto& list:play.actorCtx.actorLists)list={};
        if(actorParam<0){stone.params=0x18;chest.id=10;chest.params=-actorParam;play.actorCtx.actorLists[3]={1,&chest};}
        else {
            // These six catalog entries use position-based check identity (params=0).
            // Text lookup still receives the physical actor's native message parameter.
            static const std::map<RandomizerCheck,int> physicalParams={{RC_TOT_LEFT_CENTER_GOSSIP_STONE,7},{RC_TOT_LEFTMOST_GOSSIP_STONE,6},
                {RC_TOT_RIGHT_CENTER_GOSSIP_STONE,14},{RC_TOT_RIGHTMOST_GOSSIP_STONE,16},
                {RC_ZF_JABU_GOSSIP_STONE,2},{RC_ZF_FAIRY_GOSSIP_STONE,1}};
            stone.params=physicalParams.count(rc)?physicalParams.at(rc):actorParam;
        }
        bool nativeTable=true;uint16_t text=0;revealed.clear();fontMessages=nullptr;gossipMode=0;
        BuildHintStoneMessage(&text,&nativeTable);
        if(nativeTable||revealed!=std::vector<RandomizerHint>{rh}) {
            std::cerr<<"route rc="<<int(rc)<<" want="<<int(rh)<<" key="<<keys.at(rh)
                     <<" raw="<<actorParam<<" params="<<stone.params<<" got=";
            for(auto value:revealed)std::cerr<<int(value)<<',';
            std::cerr<<" font="<<fontMessages.dump()<<'\n';
        }
        require(!nativeTable&&revealed==std::vector<RandomizerHint>{rh},"actor/grotto text route selected wrong native hint key");
        auto expected=json{{"en",message.GetEnglish(MF_RAW)},{"de",message.GetGerman(MF_RAW)},{"fr",message.GetFrench(MF_RAW)}};
        message.Replace("Buy ","");message.Replace("Acheter: ","");message.Replace(" kaufen ","");message.Replace(" kaufen","");
        require(fontMessages["en"]==message.GetEnglish(MF_RAW),"wrong English message reached font boundary");
        require(fontMessages["de"]==message.GetGerman(MF_RAW)&&fontMessages["fr"]==message.GetFrench(MF_RAW),"wrong translated message reached font boundary");
        rows.push_back({{"nativeHint",int(rh)},{"comboKey",keys.at(rh)},{"actorParams",int(stone.params)},
                        {"grottoChestParams",actorParam<0?-actorParam:0},{"messages",expected}});
    }
    require(rows.size()==40,"saved seed must route all forty physical gossip stones");
    gossipMode=RO_GOSSIP_STONES_NEED_TRUTH;mask=0;bool table=true;uint16_t text=0;fontMessages=nullptr;
    BuildHintStoneMessage(&text,&table);require(table&&fontMessages.is_null(),"truth mask gate was bypassed");
    mask=PLAYER_MASK_TRUTH;BuildHintStoneMessage(&text,&table);require(!table,"truth mask gate rejected owned mask");
    gossipMode=RO_GOSSIP_STONES_NEED_STONE;hasStone=false;table=true;fontMessages=nullptr;
    BuildHintStoneMessage(&text,&table);require(table&&fontMessages.is_null(),"Stone of Agony gate was bypassed");
    hasStone=true;BuildHintStoneMessage(&text,&table);require(!table,"Stone of Agony gate rejected owned stone");gossipMode=0;
    std::cout<<"PASS native OOT walk, saved translations, all40 actor/grotto routes, access gates and tracker replay\n";
    return rows;
}
int main(int argc,char**argv) {
    CheckGeneration();CheckOotClaimContract();
    json save=argc>1?json::parse(std::ifstream(argv[1])):json::object();
    json hints;
    if(!save.empty())hints=save.at("combo").at("rando").at("hints");
    else hints=GenerateCase(true,2,0x12345678u,1,false,40);
    const auto rows=CheckOotRouting(hints,save.empty()?json::array():save.at("oot").at("sections").at("randomizer").at("data").at("hintLocations"));
    if(save.empty()) {
        auto savedPayload=hints;
        savedPayload["mm"]["gossipPool"]=json::array({
            {{"weight",1},{"text","a Bomb Bag can be found at Gerudo Fortress (OOT)"}},
            {{"weight",3},{"text","%cDisguised Key%w can be found hoarded in Shadow Temple (OOT)"}},
            {{"weight",1},{"text","An unrelated old-save rumor."}}
        });
        const auto original=savedPayload.dump();
        MmGossip::Run(savedPayload);
        require(savedPayload.dump()==original,"display audit rewrote arbitrary saved gossip payload");
    }
    const auto mm=MmGossip::Run(hints);
    json report={{"masterSeed",save.empty()?0x12345678u:save["combo"]["rando"]["masterSeed"].get<uint32_t>()},
                 {"ootRoutes",rows},{"mmRouting",mm},{"stats",hints.value("stats",json::object())}};
    std::cout<<"AUDIT_RESULT "<<report.dump()<<'\n';
}
