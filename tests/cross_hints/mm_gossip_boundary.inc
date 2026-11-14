namespace MmGossip {
using u32=uint32_t; using u16=uint16_t; using s32=int32_t;
/* NATIVE_ENUMS */
constexpr int ACTOR_EN_GS=42,ENGS_0=0,ITEM_MASK_TRUTH=1;
enum {RO_HINTS_GOSSIP_STONE_STRENGTH,RO_HINTS_GOSSIP_STONES,RO_HINTS_PURCHASEABLE,RO_HINTS_MOON_GOSSIP_STONES};
struct Actor {int id=ACTOR_EN_GS,params=1;struct{struct{float x=4,z=7;}pos;}home;};
struct EnGs {Actor actor;int unk_195=0;} stone;
struct Player {Actor* talkActor=&stone.actor;} player;
struct MessageContext {int choiceIndex=0;};
struct PlayState {int sceneId=9;MessageContext msgCtx;} play;
PlayState* gPlayState=&play;
struct State {int frames=99;} gameState;
State* gGameState=&gameState;
struct SaveContext {
    int fileNum=0,rupeeAccumulator=0;
    struct {struct {struct {uint32_t finalSeed=1000u;}rando;}shipSaveInfo;
            struct {struct {int rupees=500;}playerData;}saveInfo;}save;
} gSaveContext;
struct SaveCheck {RandoItemId randoItemId=RI_JUNK;bool shuffled=false,obtained=false;};
using RandoSaveCheck=SaveCheck;
std::map<RandoCheckId,SaveCheck> saveChecks;
std::map<int,u32> options;
#undef GET_PLAYER
#define GET_PLAYER(play) (&MmGossip::player)
#define RANDO_SAVE_CHECKS (MmGossip::saveChecks)
#define RANDO_SAVE_OPTIONS (MmGossip::options)
#define MAX(a,b) std::max((a),(b))
#define MIN(a,b) std::min((a),(b))
u32 forcedRoll=0,lastSeed=0,lastTotal=0,drawCalls=0;
void Ship_Random_Seed(u32 seed){lastSeed=seed;}
u32 Ship_Random(u32 low,u32 high){++drawCalls;lastTotal=high;return low+forcedRoll%(high-low);}
bool Ship_IsCStringEmpty(const char* value){return !value||!*value;}
bool Combo_IsCombinedGame(){return true;}
struct Reveal {int kind,index;std::string key,text;};
std::vector<Reveal> reveals;
void RevealSink(int,int kind,int index,const char* key,const char* text){reveals.push_back({kind,index,key,text});}
void (*gMMComboHintReveal)(int,int,int,const char*,const char*)=RevealSink;
namespace Rando {
namespace StaticData {
    struct Item {const char* name="unknown";const char* article="";RandoItemType randoItemType=RITYPE_JUNK;u32 itemId=uint32_t(-1);};
    struct Check {RandoCheckType randoCheckType=RCTYPE_CHEST;};
    std::map<RandoItemId,Item> Items;
    std::map<RandoCheckId,Check> Checks;
    std::map<RandoCheckId,std::string> CheckNames;
    std::map<RandoItemId,std::string> ItemDisplayNames;
    std::map<RandoCheckId,std::string> locationNames;
    std::unordered_map<RandoItemId,std::vector<std::string>> fakeItemNames;
    std::string GetItemName(RandoItemId,bool=false,RandoCheckId=RC_UNKNOWN,bool=true);
    const std::string& GetCheckDisplayName(RandoCheckId id){return CheckNames[id];}
    const std::string& GetItemDisplayName(RandoItemId id){return ItemDisplayNames[id];}
    // Native world-graph lookup is an input boundary; consumers and color encoding stay real.
    std::string GetLocationNameForHint(RandoCheckId id,bool){auto it=locationNames.find(id);return it==locationNames.end()?"in Fixture Native Area":it->second;}
}
namespace MiscBehavior {
    uint64_t generation=1;
    std::map<RandoCheckId,ComboRando::ForeignItem> foreign;
    uint64_t ComboRandoGen(){return generation;}
    const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId id){auto it=foreign.find(id);return it==foreign.end()?nullptr:&it->second;}
}
RandoItemId ConvertItem(RandoItemId id,RandoCheckId){return id;}
RandoItemId CurrentJunkItem(RandoCheckId){return RI_RUPEE_GREEN;}
RandoItemId CurrentTrapItem(RandoCheckId){return RI_MASK_DEKU;}
const char* ComboForeignLiveName(RandoCheckId){return nullptr;}
RandoItemType GetItemTypeForCheck(RandoItemId,RandoCheckId=RC_UNKNOWN);
RandoCheckId FindItemPlacement(RandoItemId);
std::string GetItemLocationHintName(RandoItemId,bool);
std::string GetHintLocationText(RandoItemId,RandoCheckId,bool){return "in Fixture Paid Area";}
}
namespace CustomMessage {
    struct Entry {std::string msg="vanilla";bool autoFormat=true;};
    Entry font;
    Entry LoadVanillaMessageTableEntry(u16){return {};}
    void Replace(std::string* text,const std::string& before,const std::string& after){size_t i=0;while((i=text->find(before,i))!=std::string::npos){text->replace(i,before.size(),after);i+=after.size();}}
    void ReplaceColorChars(std::string*);
    // Wrapping and actual font writes remain engine service boundaries.
    void AddLineBreaks(std::string*){}
    void EnsureMessageEnd(std::string*){}
    void LoadCustomMessageIntoFont(const Entry& entry){font=entry;if(font.autoFormat)ReplaceColorChars(&font.msg);}
}
/* NATIVE_COLOR_ENCODER */
std::vector<std::string> flavorText={"flavor"};
/* NATIVE_ALGORITHMS */

void Check(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
std::string VisibleHintWords(const std::string& text) {
    std::string visible;
    for(size_t i=0;i<text.size();++i) {
        if(text[i]=='%'&&i+1<text.size()&&std::string("wrgbcpyB").find(text[i+1])!=std::string::npos) {
            ++i;
        } else if(static_cast<unsigned char>(text[i])>6) {
            visible+=text[i];
        }
    }
    return visible;
}
void Reset(const json& hints) {
    options={{RO_HINTS_GOSSIP_STONE_STRENGTH,50},{RO_HINTS_GOSSIP_STONES,1},{RO_HINTS_PURCHASEABLE,0},{RO_HINTS_MOON_GOSSIP_STONES,0}};
    Rando::StaticData::Checks.clear();Rando::StaticData::Items.clear();saveChecks.clear();
    Rando::StaticData::CheckNames.clear();Rando::StaticData::ItemDisplayNames.clear();Rando::StaticData::locationNames.clear();
    auto add=[](RandoCheckId rc,RandoItemId ri,bool shuffled,bool obtained){Rando::StaticData::Checks[rc]={};saveChecks[rc]={ri,shuffled,obtained};Rando::StaticData::CheckNames[rc]="Fixture Native Check";};
    add(RC_ANCIENT_CASTLE_OF_IKANA_BOSS,RI_MASK_DEKU,true,false);
    add(RC_ANCIENT_CASTLE_OF_IKANA_PIECE_OF_HEART,RI_REMAINS_GOHT,true,true);
    add(RC_ASTRAL_OBSERVATORY_MOON_TEAR,RI_JUNK,true,false);
    add(RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST,RI_COMBO_FOREIGN,true,false);
    add(RC_BENEATH_THE_GRAVEYARD_CHEST,RI_MASK_DEKU,false,false);
    Rando::StaticData::Items[RI_MASK_DEKU]={"Native Mask","a",RITYPE_MASK};
    Rando::StaticData::Items[RI_REMAINS_GOHT]={"Native Remains","",RITYPE_MAJOR};
    Rando::StaticData::Items[RI_COMBO_FOREIGN]={"foreign sentinel","",RITYPE_JUNK};
    Rando::StaticData::Items[RI_JUNK]={"Junk","",RITYPE_JUNK};
    ComboRando::ForeignItem fi{};fi.itemGame=ComboRando::GAME_OOT;fi.category="major";fi.advancement=true;fi.displayName="Imported Relic (OOT)";
    Rando::MiscBehavior::foreign={{RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST,fi}};
    ComboRando::g_comboForeignJson=json{{"hints",hints}}.dump();++Rando::MiscBehavior::generation;
    player.talkActor=&stone.actor;stone.actor.params=1;stone.unk_195=0;
    gSaveContext.fileNum=0;gSaveContext.rupeeAccumulator=0;gSaveContext.save.saveInfo.playerData.rupees=500;
    drawCalls=0;forcedRoll=0;reveals.clear();
}
void CheckStaticFirstDrop(const json& hints) {
    Reset(hints);
    Rando::StaticData::Items[RI_MASK_TRUTH]={"Mask of Truth","the",RITYPE_MASK,ITEM_MASK_TRUTH};
    Rando::StaticData::ItemDisplayNames[RI_MASK_TRUTH]="Mask of Truth";
    options[RO_HINTS_MOON_GOSSIP_STONES]=1;stone.actor.params=ENGS_0;stone.unk_195=1;
    const auto saved=ComboRando::g_comboForeignJson;
    bool table=true;u16 id=0x20d4;
    MoonMessage(&id,&table);
    Check(!table&&CustomMessage::font.msg==
          "They say \x02" "Mask of Truth\x00" " is hidden \x04" "at the first Stalfos drop (OOT)\x00..."s,
          "static MM Moon hint must name the Stalfos source with green item/yellow source/white reset");
    Check(CustomMessage::font.msg.find("Hyrule Field")==std::string::npos,
          "static MM Moon hint exposed the first-drop tracker bucket as a physical source");
    Check(reveals.size()==1&&reveals[0].kind==2&&reveals[0].key=="Mask of Truth"&&
          reveals[0].text=="at the first Stalfos drop (OOT)",
          "static MM first-drop reveal lost its item identity or plain enemy source");
    Check(drawCalls==0&&ComboRando::g_comboForeignJson==saved,
          "static MM first-drop presentation must not draw RNG or rewrite saved hints");

    // A native placement takes precedence over stale foreign metadata for the same item.
    const auto nativeCheck=RC_SWAMP_SPIDER_HOUSE_MASK_OF_TRUTH;
    Rando::StaticData::Checks[nativeCheck]={RCTYPE_NPC};saveChecks[nativeCheck]={RI_MASK_TRUTH,true,false};
    Rando::StaticData::CheckNames[nativeCheck]="Swamp Spider House Mask of Truth";
    Rando::StaticData::locationNames[nativeCheck]="in Southern Swamp";
    table=true;reveals.clear();MoonMessage(&id,&table);
    Check(!table&&CustomMessage::font.msg==
          "They say \x02" "Mask of Truth\x00" " is hidden \x04" "in Southern Swamp\x00..."s,
          "static MM Moon hint lost native placement priority or native colors");
    Check(reveals.size()==1&&reveals[0].kind==1&&reveals[0].key=="Swamp Spider House Mask of Truth",
          "native Moon hint must reveal its local check rather than the foreign item map");
    options[RO_HINTS_MOON_GOSSIP_STONES]=0;table=true;reveals.clear();CustomMessage::font.msg="unchanged";
    MoonMessage(&id,&table);
    Check(table&&reveals.empty()&&CustomMessage::font.msg=="unchanged",
          "disabled Moon hints displayed or revealed a static first-drop hint");
    std::cout<<"PASS native static MM lookup/Moon callback: generated Stalfos source, final green/yellow/white bytes, native priority and disabled control\n";
}
void CheckNewCheckColors() {
    struct Case {RandoCheckId check;RandoCheckType type;RandoItemId item;const char* name;RandoItemType itemType;
                 const char* location;std::string expected;};
    const Case cases[]={
        {RC_SNOWHEAD_TEMPLE_ICICLE_ENTRANCE_1,RCTYPE_ICICLE,RI_HOOKSHOT,"Hookshot",RITYPE_MAJOR,"in Snowhead Temple",
         "They say \x02" "the Hookshot\x00" " is hidden \x04" "in Snowhead Temple\x00."s},
        {RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RCTYPE_NPC,RI_MASK_GORON,"Goron Mask",RITYPE_MASK,"in West Clock Town",
         "They say \x02" "the Goron Mask\x00" " is hidden \x04" "in West Clock Town\x00."s},
        {RC_DEKU_PALACE_SOIL_ITEM_1,RCTYPE_SOIL,RI_SOUL_NPC_KAFEI,"Kafei NPC Soul",RITYPE_MAJOR,"in Deku Palace",
         "They say \x02" "the Kafei NPC Soul\x00" " is hidden \x04" "in Deku Palace\x00."s}
    };
    for(const auto& row:cases) {
        Reset(json::object());Rando::StaticData::Checks.clear();saveChecks.clear();
        Rando::StaticData::Checks[row.check]={row.type};saveChecks[row.check]={row.item,true,false};
        Rando::StaticData::Items[row.item]={row.name,"the",row.itemType};
        Rando::StaticData::CheckNames[row.check]="Selected new check";
        Rando::StaticData::locationNames[row.check]=row.location;
        u16 id=0x20d1;bool table=true;FirstMessage(&id,&table);
        Check(!table&&CustomMessage::font.msg==row.expected,
              "environment/lottery/soul gossip must display its actual item green and source yellow with white resets");
        Check(reveals.size()==1&&reveals[0].kind==1&&reveals[0].key=="Selected new check",
              "new check gossip revealed the wrong selected check");
    }
    std::cout<<"PASS native MM environmental/lottery/soul gossip: actual item names and final green/yellow/white bytes\n";
}
json Run(const json& hints) {
    CheckNewCheckColors();
    const json weighted={{"mm",{{"gossipPool",json::array({{{"weight",1},{"text","One-weight rumor"}},
                                                           {{"weight",3},{"text","Three-weight rumor"}}})},
                                {"itemLocations",{{"Fixture Mask","in OOT Fixture Area (OOT)"}}}}}};
    Reset(weighted);
    for(auto [roll,wantedIndex,wantedText]:{std::tuple{1700u,0,"One-weight rumor"},std::tuple{1799u,0,"One-weight rumor"},
                                         std::tuple{1800u,1,"Three-weight rumor"},std::tuple{1999u,1,"Three-weight rumor"}}) {
        forcedRoll=roll;int index=-1;std::string text;
        Check(GetRandomCheck(false,&text,&index)==RC_UNKNOWN&&index==wantedIndex&&text==wantedText&&lastTotal==2000,
              "literal MM weighted cross-pool boundaries changed");
    }
    Check(ComboRando::LoadHintsMM(0).itemLocations.at("Fixture Mask")=="in OOT Fixture Area (OOT)","MM imported-item location loader changed");
    const std::string screenshotText="a Bomb Bag can be found at Gerudo Fortress (OOT)";
    const json screenshot={{"mm",{{"gossipPool",json::array({{{"weight",1},{"text",screenshotText}}})}}}};
    Reset(screenshot);forcedRoll=1700;
    bool screenshotTable=true;u16 screenshotId=0x20d1;
    FirstMessage(&screenshotId,&screenshotTable);
    Check(!screenshotTable&&CustomMessage::font.msg==
          "They say \x01" "a Bomb Bag\x00" " can be found at \x04" "Gerudo Fortress (OOT)\x00."s,
          "saved foreign gossip must color its item red and location yellow");
    Check(ComboRando::LoadHintsMM(0).gossipPool[0].text==screenshotText,
          "hint presentation must not rewrite existing saved pool text");
    for(const auto& [raw,expected]:std::vector<std::pair<std::string,std::string>>{
        {"%ga Bomb Bag%w can be found at %cGerudo Fortress%B (OOT)",
         "%ra Bomb Bag%w can be found at %yGerudo Fortress (OOT)%w"},
        {"%rDisguised Relic%w can be found hoarded in %ySpirit Temple (OOT)%w",
         "%rDisguised Relic%w can be found hoarded in %ySpirit Temple (OOT)%w"},
        {"Épée can be found at Forêt (OOT)","%rÉpée%w can be found at %yForêt (OOT)%w"},
        {"A rumor without a placement.","A rumor without a placement."},
        {"%gA harmless joke%w.","%gA harmless joke%w."},
        {"can be found at Gerudo Fortress (OOT)","can be found at Gerudo Fortress (OOT)"},
        {"A relic can be found at  (OOT)","A relic can be found at  (OOT)"},
        {"A relic can be found at Termina (MM)","A relic can be found at Termina (MM)"},
        {"A 50%x relic can be found at Coast (OOT)","%rA 50%x relic%w can be found at %yCoast (OOT)%w"}
    }) {
        Check(ComboHintPresentation::MmForeignGossip(raw)==expected,
              "foreign placement presentation corrupted colors, visible text or unrelated rumors");
        Check(ComboHintPresentation::MmForeignGossip(expected)==expected,
              "foreign placement presentation must be idempotent");
    }
    Reset(hints);
    auto loaded=ComboRando::LoadHintsMM(0);
    Check(loaded.gossipPool.size()==hints.at("mm").at("gossipPool").size(),"MM loader dropped cross pool entries");
    for(size_t i=0;i<loaded.gossipPool.size();++i) {
        const auto& original=hints.at("mm").at("gossipPool").at(i);
        Check(loaded.gossipPool[i].text==original.value("text","")&&loaded.gossipPool[i].weight==original.value("weight",1u),
              "MM loader reordered or rewrote original saved gossip text/weight");
    }
    Check(ComboRando::LoadHintsMM(3).gossipPool.size()==loaded.gossipPool.size(),"pushed MM hint blob incorrectly depends on native slot");
    Check(GetRandomCheck()==RC_ANCIENT_CASTLE_OF_IKANA_BOSS,"first native check routed incorrectly");
    Check(lastSeed==1020u&&drawCalls==1,"stone draw changed deterministic seed or used extra RNG");
    forcedRoll=650;Check(GetRandomCheck()==RC_ANCIENT_CASTLE_OF_IKANA_PIECE_OF_HEART,"normal gossip lost obtained native check");
    forcedRoll=1200;Check(GetRandomCheck()==RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST,"foreign item was rejected as sentinel junk");
    Check(Rando::GetItemTypeForCheck(RI_COMBO_FOREIGN,RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST)==RITYPE_MAJOR,"actual foreign type normalization failed");
    forcedRoll=0;Check(GetRandomCheck(true)==RC_ANCIENT_CASTLE_OF_IKANA_BOSS&&lastTotal==1150&&lastSeed==99,
        "paid repeat must exclude obtained checks and entire OOT cross pool");
    u32 cumulative=1700;int bad=0;
    for(size_t i=0;i<loaded.gossipPool.size();++i) {
        forcedRoll=cumulative;std::string text;int index=-1;u32 previous=drawCalls;
        Check(GetRandomCheck(false,&text,&index)==RC_UNKNOWN&&index==int(i)&&text==loaded.gossipPool[i].text,
              "cross-pool weighted boundary routed wrong index or text");
        Check(drawCalls==previous+1,"cross pool must share exactly one draw with native checks");
        bool fromTable=true;u16 id=0x20d1;reveals.clear();FirstMessage(&id,&fromTable);
        const std::map<std::string,std::string> display={
            {"OOT Native Relic can be found at Oot Home Area (OOT)",
             "They say \x01" "OOT Native Relic\x00" " can be found at \x04" "Oot Home Area (OOT)\x00."s},
            {"MM Abroad Relic can be found hoarded in Oot Foreign Area (OOT)",
             "They say \x01" "MM Abroad Relic\x00" " can be found hoarded in \x04" "Oot Foreign Area (OOT)\x00."s}
        };
        Check(!fromTable&&VisibleHintWords(CustomMessage::font.msg)=="They say "+VisibleHintWords(text)+".",
              "MM first textbox changed the selected saved hint's visible words");
        if(display.contains(text))
            Check(CustomMessage::font.msg==display.at(text),"MM fixture hint has wrong literal color bytes");
        Check(reveals.size()==1&&reveals[0].kind==0&&reveals[0].index==int(i),"MM cross-pool tracker reveal used wrong index");
        bad+=text.find("No Hint")!=std::string::npos;
        cumulative+=100+(std::max<u32>(1,loaded.gossipPool[i].weight)-1)*50;
    }
    forcedRoll=0;u16 id=0x20d1;bool table=true;reveals.clear();FirstMessage(&id,&table);
    Check(!table&&CustomMessage::font.msg=="They say \x02" "a Native Mask\x00" " is hidden \x04" "in Fixture Native Area\x00."s,"native MM item/message branch changed");
    Check(reveals.size()==1&&reveals[0].kind==1&&reveals[0].key=="Fixture Native Check","native MM check tracker route changed");
    forcedRoll=1200;table=true;reveals.clear();FirstMessage(&id,&table);
    Check(CustomMessage::font.msg=="They say \x02" "Imported Relic (OOT)\x00" " is hidden \x04" "in Fixture Native Area\x00."s,"OOT item at MM check lost its real display name");
    auto& foreign=Rando::MiscBehavior::foreign.at(RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST);
    foreign.category="junk";forcedRoll=1200;std::string junkText;int junkIndex=-1;
    Check(Rando::GetItemTypeForCheck(RI_COMBO_FOREIGN,RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST)==RITYPE_JUNK&&
          GetRandomCheck(false,&junkText,&junkIndex)==RC_UNKNOWN&&junkIndex==0,"real foreign junk entered the native hint candidate pool");
    foreign.fakeTrickName="Disguised Relic";foreign.trap=true;foreign.category="junk";
    Check(Rando::GetItemTypeForCheck(RI_COMBO_FOREIGN,RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST)==RITYPE_MAJOR,"foreign trap was silently classified as junk");
    Check(Rando::StaticData::GetItemName(RI_COMBO_FOREIGN,true,RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST)=="Disguised Relic","foreign trap leaked true item name");
    saveChecks[RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST].obtained=true;
    Check(Rando::StaticData::GetItemName(RI_COMBO_FOREIGN,true,RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST)=="Imported Relic (OOT)","obtained trap retained old disguise");
    Reset(hints);options[RO_HINTS_PURCHASEABLE]=1;play.msgCtx.choiceIndex=1;forcedRoll=0;table=true;id=0x20c0;
    const int cost=GetNormalizedCost();Check(cost==70,"normalized purchase cost changed in controlled four-check fixture");
    SecondMessage(&id,&table);Check(!table&&gSaveContext.rupeeAccumulator==-70,"paid message failed to charge its selected hint");
    Check(CustomMessage::font.msg=="Wise choice... They say \x02" "a Native Mask\x00" " is hidden \x04" "in Fixture Paid Area\x00."s,"paid hint used OOT cross pool or wrong native location route");
    gSaveContext.save.saveInfo.playerData.rupees=0;gSaveContext.rupeeAccumulator=0;SecondMessage(&id,&table);
    Check(gSaveContext.rupeeAccumulator==0&&CustomMessage::font.msg=="Foolish... You don't have enough rupees...","failed purchase charged or displayed a hint");
    play.msgCtx.choiceIndex=0;SecondMessage(&id,&table);
    Check(gSaveContext.rupeeAccumulator==0&&CustomMessage::font.msg=="Foolish... Come back later when you have more sense.","declined purchase charged or displayed a hint");
    Reset(hints);options[RO_HINTS_MOON_GOSSIP_STONES]=1;stone.actor.params=ENGS_0;stone.unk_195=1;
    table=true;u32 before=drawCalls;FirstMessage(&id,&table);Check(table&&drawCalls==before,"Moon mask-priority stone consumed ordinary hint pool");
    Reset(hints);player.talkActor=nullptr;Check(GetRandomCheck()==RC_UNKNOWN,"missing talk actor produced a hint");
    player.talkActor=&stone.actor;ComboRando::g_comboForeignJson="not json";
    Check(ComboRando::LoadHintsMM(0).gossipPool.empty(),"corrupt pushed hints were accepted");
    Reset(hints);forcedRoll=1700;std::string previous;int index=-1;
    GetRandomCheck(false,&previous,&index);
    auto changed=hints;changed["mm"]["gossipPool"]=json::array({{{"weight",1},{"text","Another slot's rumor"}}});
    ComboRando::g_comboForeignJson=json{{"hints",changed}}.dump();++Rando::MiscBehavior::generation;
    gSaveContext.fileNum=3;
    std::string current;index=-1;Check(GetRandomCheck(false,&current,&index)==RC_UNKNOWN&&index==0&&current=="Another slot's rumor",
        "MM generation change retained previous slot's cached hint pool");
    std::cout<<"PASS native MM loader, weighted selection, all cross entries, native/foreign names, paid repeat, Moon priority and cache reload\n";
    return {{"poolEntriesRouted",loaded.gossipPool.size()},{"storedNoHintEntries",bad},{"fontService","native color bytes recorded before engine wrapping/font upload"}};
}
#undef GET_PLAYER
#undef RANDO_SAVE_CHECKS
#undef RANDO_SAVE_OPTIONS
#undef MAX
#undef MIN
}
