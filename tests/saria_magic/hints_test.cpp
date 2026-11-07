#include "combo/rando/CrossHints.h"
#include <cassert>
#include <iostream>
using json=nlohmann::json;
static json owned;
static void ResetOracle(){}
static void SetOwned(const char* value){owned=json::parse(value);}
static const char* ReachOot(){static std::string out;json arr=json::array({"Win", "Key"});
 if(std::find(owned.begin(),owned.end(),"Hookshot")!=owned.end())arr.push_back("Reachable magic");out=arr.dump();return out.c_str();}
static const char* ReachMm(){return "[]";}
int main(){
 ComboRando::OracleFns oot{ResetOracle,SetOwned,ReachOot,nullptr},mmO{ResetOracle,SetOwned,ReachMm,nullptr};
 json optionalSeed{{"oot",{{"Win","Triforce Piece"},{"Key","Hookshot"},{"Reachable magic","Magic Stat Upgrade"},{"A unreachable magic","Magic Stat Upgrade"}}},{"mm",json::object()}};
 auto full=ComboRando::FindSariaMagicSphere(optionalSeed.dump(),oot,mmO,"{}","{}",false,false,0,"");
 auto opt=ComboRando::ParseSpoilerPlacements(optionalSeed.dump());
 assert(opt[ComboRando::SelectSariaMagicPlacement(opt,full,true).value()].check=="Reachable magic");
 assert(full[0]["sphere"]==1); // ignores short hunt win at sphere0; unreachable lexical first is excluded
 optionalSeed["oot"].erase("Reachable magic");
 auto none=ComboRando::FindSariaMagicSphere(optionalSeed.dump(),oot,mmO,"{}","{}",false,false,0,"");
 assert(none.empty());assert(!ComboRando::SelectSariaMagicPlacement(ComboRando::ParseSpoilerPlacements(optionalSeed.dump()),none,true));

 std::vector<ComboRando::CwPlacedItem> p;
 auto add=[&](auto game,const char* check){ComboRando::CwPlacedItem x{};x.checkGame=game;x.itemGame=ComboRando::GAME_OOT;x.check=check;x.item="Magic Stat Upgrade";p.push_back(x);};
 add(ComboRando::GAME_OOT,"A late");add(ComboRando::GAME_MM,"A early");add(ComboRando::GAME_OOT,"Z early");add(ComboRando::GAME_OOT,"B early");
 json spheres=json::array({{{"sphere",2},{"steps",json::array({{{"game","oot"},{"check","A late"}}})}},{{"sphere",0},{"steps",json::array({{{"game","mm"},{"check","A early"}},{{"game","oot"},{"check","Z early"}},{{"game","oot"},{"check","B early"}}})}}});
 assert(ComboRando::SelectSariaMagicPlacement(p,spheres)==3);
 std::reverse(p.begin(),p.end());assert(p[ComboRando::SelectSariaMagicPlacement(p,spheres).value()].check=="B early");
 assert(p[ComboRando::SelectSariaMagicPlacement(p,json::array()).value()].check=="A late");
 for(auto& x:p)x.item="Progressive Magic Meter";
 assert(!ComboRando::SelectSariaMagicPlacement(p,spheres));
 json seed{{"oot",{{"A late","Magic Stat Upgrade"},{"B early","Magic Stat Upgrade"},{"Z early","Magic Stat Upgrade"},{"Meter","Progressive Magic Meter"}}},{"mm",{{"A early","Magic Stat Upgrade"}}},{"foreign",json::array({{{"checkGame","mm"},{"checkName","A early"},{"itemGame","oot"},{"itemName","Magic Stat Upgrade"}}})}};
 json hd{{"options",{{"sariaHint",1},{"magicStatUpgrade",1}}},{"checks",json::array()},{"hintTextTable",json::object()}};
 for(const char* name:{"A late","B early","Z early","Meter"})hd["checks"].push_back({{"name",name},{"area",name}});
 for(const char* key:{"RHT_SARIA_TALK_HINT","RHT_SARIA_SONG_HINT"})hd["hintTextTable"][key]={{"clear",{{"en","magic [[1]]"},{"de","Magie [[1]]"},{"fr","magie [[1]]"}}}};
 json mm{{"locationHints",{{"A early","in Snowhead Temple"}}}};
 auto gen=[&](const json& sp){return ComboRando::Generate(7,"{}",hd.dump(),mm.dump(),json::array(),seed.dump(),{},sp);};
 auto hint=[&](const json& out)->json{for(const auto& x:out.at("oot"))if(x.at("checkName")=="__STATIC__RH_SARIA_HINT")return x;return nullptr;};
 auto selected=hint(gen(spheres));assert(!selected.is_null());assert(selected["messages"].size()==2);
 assert(selected["messages"][0]["en"]=="magic B early");assert(selected["messages"][1]["en"]=="magic B early");
 assert(hint(gen(spheres))==selected); // composed target remains fixed for a seed
 json mmFirst=json::array({{{"sphere",0},{"steps",json::array({{{"game","mm"},{"check","A early"}}})}}});
 assert(hint(gen(mmFirst))["messages"][0]["en"]=="magic Snowhead Temple");
 assert(hint(gen(json::array()))["messages"][0]["en"]=="magic A late");
 hd["options"]["magicStatUpgrade"]=0;assert(hint(gen(spheres))["messages"][0]["en"]=="magic Meter");
 hd["options"]["sariaHint"]=0;assert(hint(gen(spheres)).is_null());
 hd["options"]["magicStatUpgrade"]=1;
 assert(hint(gen(spheres))["messages"][0]["en"]=="magic B early");
 assert(hd["options"]["sariaHint"]==0); // stat guidance does not rewrite the classic preference
 hd["options"]["sariaHint"]=1;hd["options"]["magicStatUpgrade"]=1;seed["oot"]=json::object();seed["mm"]=json::object();seed["foreign"]=json::array();
 assert(hint(gen(spheres)).is_null()); // absent stat upgrades never invent a location
 std::cout<<"PASS: full composer talk/song parity, cross-game target, original meter, absent items and disabled hint\n";
 std::cout<<"PASS: earliest sphere, OoT and name ties, stable fallback, mode change\n";
}
