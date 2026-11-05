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
struct Reveal {int kind,index;std::string key,text;};
std::vector<Reveal> reveals;
void RevealSink(int,int kind,int index,const char* key,const char* text){reveals.push_back({kind,index,key,text});}
void (*gMMComboHintReveal)(int,int,int,const char*,const char*)=RevealSink;
namespace Rando {
namespace StaticData {
    struct Item {const char* name="unknown";const char* article="";RandoItemType randoItemType=RITYPE_JUNK;};
    struct Check {RandoCheckType randoCheckType=RCTYPE_CHEST;};
    std::map<RandoItemId,Item> Items;
    std::map<RandoCheckId,Check> Checks;
    std::map<RandoCheckId,std::string> CheckNames;
    std::unordered_map<RandoItemId,std::vector<std::string>> fakeItemNames;
    std::string GetItemName(RandoItemId,bool=false,RandoCheckId=RC_UNKNOWN,bool=true);
    const std::string& GetCheckDisplayName(RandoCheckId id){return CheckNames[id];}
    std::string GetLocationNameForHint(RandoCheckId,bool){return "in Fixture Native Area";}
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
std::string GetHintLocationText(RandoItemId,RandoCheckId,bool){return "in Fixture Paid Area";}
}
namespace CustomMessage {
    struct Entry {std::string msg="vanilla";bool autoFormat=true;};
    Entry font;
    Entry LoadVanillaMessageTableEntry(u16){return {};}
    void Replace(std::string* text,const std::string& before,const std::string& after){size_t i=0;while((i=text->find(before,i))!=std::string::npos){text->replace(i,before.size(),after);i+=after.size();}}
    // Color encoding, wrapping and actual font writes belong to the engine service boundary.
    void ReplaceColorChars(std::string*){}
    void AddLineBreaks(std::string*){}
    void EnsureMessageEnd(std::string*){}
    void LoadCustomMessageIntoFont(const Entry& entry){font=entry;}
}
std::vector<std::string> flavorText={"flavor"};
/* NATIVE_ALGORITHMS */

void Check(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
void Reset(const json& hints) {
    options={{RO_HINTS_GOSSIP_STONE_STRENGTH,50},{RO_HINTS_GOSSIP_STONES,1},{RO_HINTS_PURCHASEABLE,0},{RO_HINTS_MOON_GOSSIP_STONES,0}};
    Rando::StaticData::Checks.clear();Rando::StaticData::Items.clear();saveChecks.clear();
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
    gSaveContext.rupeeAccumulator=0;gSaveContext.save.saveInfo.playerData.rupees=500;
    drawCalls=0;forcedRoll=0;reveals.clear();
}
json Run(const json& hints) {
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
        Check(!fromTable&&CustomMessage::font.msg=="They say "+text+".","MM first textbox displayed wrong cross text");
        Check(reveals.size()==1&&reveals[0].kind==0&&reveals[0].index==int(i),"MM cross-pool tracker reveal used wrong index");
        bad+=text.find("No Hint")!=std::string::npos;
        cumulative+=100+(std::max<u32>(1,loaded.gossipPool[i].weight)-1)*50;
    }
    forcedRoll=0;u16 id=0x20d1;bool table=true;reveals.clear();FirstMessage(&id,&table);
    Check(!table&&CustomMessage::font.msg=="They say %ga Native Mask%w is hidden %yin Fixture Native Area%w.","native MM item/message branch changed");
    Check(reveals.size()==1&&reveals[0].kind==1&&reveals[0].key=="Fixture Native Check","native MM check tracker route changed");
    forcedRoll=1200;table=true;reveals.clear();FirstMessage(&id,&table);
    Check(CustomMessage::font.msg=="They say %gImported Relic (OOT)%w is hidden %yin Fixture Native Area%w.","OOT item at MM check lost its real display name");
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
    Check(CustomMessage::font.msg=="Wise choice... They say %ga Native Mask%w is hidden %yin Fixture Paid Area%w.","paid hint used OOT cross pool or wrong native location route");
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
    return {{"poolEntriesRouted",loaded.gossipPool.size()},{"storedNoHintEntries",bad},{"fontService","recorded before engine wrapping/color encoding"}};
}
#undef GET_PLAYER
#undef RANDO_SAVE_CHECKS
#undef RANDO_SAVE_OPTIONS
#undef MAX
#undef MIN
}
