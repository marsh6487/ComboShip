static void TestAllNativeEnvironmentRules(){
 using namespace Rando;using namespace Rando::Logic;
 const std::map<RandoCheckId,std::pair<std::function<bool()>,std::string>> rules={
#include "environment_all_rules.inc"
 };
 REQUIRE(rules.size()==211);
 auto reset=[]{gSaveContext={};gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;memset(gSaveContext.save.saveInfo.inventory.items,ITEM_NONE,sizeof(gSaveContext.save.saveInfo.inventory.items));};
 reset();REQUIRE(!rules.at(RC_LONE_PEAK_SHRINE_LARGE_BOULDER_1).first());INV_CONTENT(ITEM_MASK_GORON)=ITEM_MASK_GORON;REQUIRE(!rules.at(RC_LONE_PEAK_SHRINE_LARGE_BOULDER_1).first());INV_CONTENT(ITEM_BOMB)=ITEM_BOMB;REQUIRE(rules.at(RC_LONE_PEAK_SHRINE_LARGE_BOULDER_1).first());
 reset();REQUIRE(!rules.at(RC_DEKU_PALACE_RED_BOULDER_1).first());INV_CONTENT(ITEM_MASK_GORON)=ITEM_MASK_GORON;REQUIRE(rules.at(RC_DEKU_PALACE_RED_BOULDER_1).first());REQUIRE(!rules.at(RC_MOUNTAIN_VILLAGE_SPRING_RED_BOULDER_1).first());RANDO_EVENTS[RE_CLEARED_SNOWHEAD_TEMPLE]=true;REQUIRE(rules.at(RC_MOUNTAIN_VILLAGE_SPRING_RED_BOULDER_1).first());
 reset();REQUIRE(!rules.at(RC_SECRET_SHRINE_SOIL_ITEM_1).first());INV_CONTENT(ITEM_BOTTLE)=ITEM_BOTTLE;REQUIRE(!rules.at(RC_SECRET_SHRINE_SOIL_ITEM_1).first());RANDO_EVENTS[RE_ACCESS_BUGS]=true;REQUIRE(rules.at(RC_SECRET_SHRINE_SOIL_ITEM_1).first());RANDO_EVENTS[RE_ACCESS_BUGS]=false;REQUIRE(!rules.at(RC_SECRET_SHRINE_SOIL_ITEM_1).first());
 reset();INV_CONTENT(ITEM_BOW)=ITEM_BOW;REQUIRE(!rules.at(RC_MOON_TRIAL_GORON_ICICLE_1).first());INV_CONTENT(ITEM_MASK_GORON)=ITEM_MASK_GORON;gSaveContext.save.saveInfo.playerData.isMagicAcquired=true;REQUIRE(rules.at(RC_MOON_TRIAL_GORON_ICICLE_1).first());
 reset();INV_CONTENT(ITEM_BOW)=ITEM_BOW;REQUIRE(!rules.at(RC_GREAT_BAY_TEMPLE_ICICLE_COMPASS_ROOM_1).first());INV_CONTENT(ITEM_ARROW_ICE)=ITEM_ARROW_ICE;gSaveContext.save.saveInfo.playerData.isMagicAcquired=true;REQUIRE(rules.at(RC_GREAT_BAY_TEMPLE_ICICLE_COMPASS_ROOM_1).first());
 // The native C/table suite proves all nine ordinary form/mask routes to
 // Human. Check the consequence against EVERY real CHECK, with actual native
 // inventory/time/tool prerequisites. A reachable Human witness must remain
 // reachable from an ordinary form that can natively return to Human.
 auto saved=std::make_unique<SaveContext>(gSaveContext);
 const auto savedTime=gCurrentRegionTime;
 std::set<RandoCheckId> humanWitnesses;
 unsigned comparisons=0;
 std::map<std::pair<int,int>,unsigned> routeComparisons;
 for(auto current:{PLAYER_FORM_DEKU,PLAYER_FORM_GORON,PLAYER_FORM_ZORA})
  for(auto owned:{ITEM_MASK_DEKU,ITEM_MASK_GORON,ITEM_MASK_ZORA})
   for(bool explosives:{false,true})for(int day:{1,2,3}) {
    reset();auto& info=gSaveContext.save.shipSaveInfo.rando;
    info.randoSaveOptions[RO_LOGIC]=RO_LOGIC_GLITCHLESS;
    memset(info.randoInf,0xff,sizeof(info.randoInf));
    memset(info.randoEvents,1,sizeof(info.randoEvents));
    // Imported boots have an explicit native starting source in this access
    // witness. The separate pool tests still enforce source availability.
    info.randoStartingItems[0]=RI_OOT_IRON_BOOTS;
    gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOOTS_IRON]=1;
    gSaveContext.save.day=day;gSaveContext.save.time=CLOCK_TIME(12,0);
    gCurrentRegionTime=GetHalfDayTimeMask((day-1)*2);
    gSaveContext.save.saveInfo.playerData.isMagicAcquired=true;
    gSaveContext.save.saveInfo.inventory.questItems=0xffffffffu;
    SET_EQUIP_VALUE(EQUIP_TYPE_SWORD,EQUIP_VALUE_SWORD_KOKIRI);
    SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD,EQUIP_VALUE_SHIELD_HERO);
    for(auto item:{ITEM_OCARINA_OF_TIME,ITEM_BOW,ITEM_HOOKSHOT,ITEM_ARROW_ICE,
                   ITEM_DEKU_STICK,ITEM_DEKU_NUT,ITEM_BOTTLE})INV_CONTENT(item)=item;
    INV_CONTENT(owned)=owned;
    if(explosives)for(auto item:{ITEM_BOMB,ITEM_BOMBCHU,ITEM_MASK_BLAST})INV_CONTENT(item)=item;
    gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    std::map<RandoCheckId,bool> fromHuman;
    for(const auto& [id,rule]:rules) {
     fromHuman[id]=rule.first();
     if(fromHuman[id])humanWitnesses.insert(id);
    }
    gSaveContext.save.playerForm=current;
    REQUIRE(CanReturnToHuman());
    for(const auto& [id,rule]:rules)if(fromHuman.at(id)) {
     if(!rule.first())fprintf(stderr,"Native Human-return mismatch check=%d form=%d mask=%d explosives=%d day=%d\n",id,current,owned,explosives,day);
     REQUIRE(rule.first());++comparisons;++routeComparisons[{current,owned}];
    }
   }
 for(const auto& [id,rule]:rules)if(!humanWitnesses.contains(id))fprintf(stderr,"Missing native tool/time witness check=%d condition=%s\n",id,rule.second.c_str());
 REQUIRE(humanWitnesses.size()==211);REQUIRE(routeComparisons.size()==9);
 for(const auto& [route,count]:routeComparisons)REQUIRE(count>0);
 gSaveContext=*saved;gCurrentRegionTime=savedTime;
 printf("PASS all211 source CHECKs witnessed; %u native Human-return comparisons across9mask routes/3days/2tool sets\n",comparisons);
 puts("PASS all211 native CHECK lambdas and item/form/bugs/spring/rolling/ice-arrow negatives");
}
