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
 puts("PASS all211 native CHECK lambdas and item/form/bugs/spring/rolling/ice-arrow negatives");
}
