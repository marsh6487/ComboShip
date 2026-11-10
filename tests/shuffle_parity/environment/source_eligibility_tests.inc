static void TestActualSourceAvailability(){
 using namespace Rando;
#ifdef COMBO_BUILD
 const int modes[]={COMBO_GAME_MM,COMBO_GAME_BOTH};
#else
 const int modes[]={0};
#endif
 for(int mode:modes)for(bool boots:{false,true}){
#ifdef COMBO_BUILD
  gComboGameMode=mode;
#else
  (void)mode;
#endif
  auto save=std::make_unique<RandoSaveInfo>();for(const auto& [id,row]:StaticData::Options)save->randoSaveOptions[id]=row.defaultValue;
  save->randoSaveOptions[RO_SHUFFLE_ROCK_DROPS]=1;
  if(boots)save->randoStartingItems[0]=RI_OOT_IRON_BOOTS;
  excluded.clear();std::vector<RandoCheckId> checks;std::vector<RandoItemId> items;Logic::GeneratePools(*save,checks,items);
  const bool eligible=Combo_IsCombinedGame()||FleetCombo_UnifiedPoolActive()||boots;
  int count=0,water=0;
  for(const auto& [id,row]:StaticData::Checks)if(row.randoCheckType==RCTYPE_ROCK){
   if(std::find(checks.begin(),checks.end(),id)!=checks.end())++count;
   if(MMEnvironment::RequiresIronBoots(id)){
    ++water;if(!eligible)REQUIRE(!save->randoSaveChecks[id].shuffled);
    REQUIRE(std::count(checks.begin(),checks.end(),id)==int(eligible));
   }
  }
  REQUIRE(water==30&&count==(eligible?127:97));
  if(!eligible)REQUIRE(std::find(items.begin(),items.end(),RI_OOT_IRON_BOOTS)==items.end());
 }
#ifdef COMBO_BUILD
 gComboGameMode=COMBO_GAME_BOTH;
#endif
 puts("PASS actual solo/combined native rock pools with and without a real Iron Boots starting source");
}
