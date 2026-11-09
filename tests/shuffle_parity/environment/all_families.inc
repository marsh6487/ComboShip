// Fixture service boundaries: real catalog/pool/CustomItem and production callbacks above.
static void TestAllEnvironmentFamilies() {
  using namespace Rando;
  static PlayState play{}; gPlayState=&play; ActorBehavior::InitBgIcicleBehavior();
  const std::map<RandoCheckType,int> counts={{RCTYPE_ROCK,127},{RCTYPE_BOULDER,23},{RCTYPE_RED_BOULDER,7},{RCTYPE_ICICLE,25},{RCTYPE_SOIL,29}};
  auto& region=Logic::Regions[static_cast<RandoRegionId>(0)];
  for(const auto& [id,row]:StaticData::Checks)if(id!=RC_UNKNOWN)region.checks[id]={[]{return true;},"catalogue membership boundary"};
  size_t baseline=0;
  for(int mask=0;mask<32;++mask){
    excluded.clear(); auto state=std::make_unique<RandoSaveInfo>();
    for(const auto& [id,row]:StaticData::Options)state->randoSaveOptions[id]=row.defaultValue;
    int expected=0,bit=0;
    for(const auto& [type,count]:counts){
      auto option=MMEnvironment::OptionForType(type); REQUIRE(StaticData::Options.at(option).defaultValue==0);
      state->randoSaveOptions[option]=(mask>>bit++)&1;if(state->randoSaveOptions[option])expected+=count-(type==RCTYPE_ROCK&&!MMEnvironment::SourceEligibleForSeed(RC_GREAT_BAY_COAST_ROCK_WATER_01,Combo_IsCombinedGame()||FleetCombo_UnifiedPoolActive(),*state)?30:0);
    }
    std::vector<RandoCheckId> checks;std::vector<RandoItemId> items;Logic::GeneratePools(*state,checks,items);
    if(!mask)baseline=items.size(); REQUIRE(items.size()==baseline+expected);
    for(const auto& p:MMEnvironment::Sources)REQUIRE(std::count(checks.begin(),checks.end(),p.check)==int(state->randoSaveOptions[MMEnvironment::OptionForType(p.type)]!=0&&MMEnvironment::SourceEligibleForSeed(p.check,Combo_IsCombinedGame()||FleetCombo_UnifiedPoolActive(),*state)));
    if(mask==31){
      for(const auto& p:MMEnvironment::Sources)excluded.push_back(p.check);
      std::sort(excluded.begin(),excluded.end());excluded.erase(std::unique(excluded.begin(),excluded.end()),excluded.end());
      checks.clear();items.clear();Logic::GeneratePools(*state,checks,items);REQUIRE(items.size()==baseline);
      for(const auto& p:MMEnvironment::Sources)REQUIRE(!state->randoSaveChecks[p.check].shuffled);
    }
  }
  for(const auto& p:MMEnvironment::Sources){
    gSaveContext={};play={};play.sceneId=p.scene;play.roomCtx.curRoom.num=p.room;gSaveContext.sceneLayer=p.setup;
    Hooks::scene(0,0);checkIds.clear();indices.clear();allocations=0;
    REQUIRE(MMEnvironment::Identify(p.scene,p.setup,p.room,p.index,p.slice,p.actorId)==p.check);
    REQUIRE(MMEnvironment::Identify(p.scene,p.setup,p.room,p.index,p.slice,ACTOR_PLAYER)==RC_UNKNOWN);
    Actor actor{};actor.id=p.actorId==ACTOR_OBJ_MURE2?ACTOR_EN_ISHI:p.actorId;actor.room=p.room;actor.params=p.type==RCTYPE_SOIL?0x200:0;indices[&actor]=p.index;
    auto& saved=RANDO_SAVE_CHECKS[p.check];saved.shuffled=true;bool should=true;
    if(p.actorId==ACTOR_OBJ_MURE2){
      ObjMure2 parent{};parent.actor.id=ACTOR_OBJ_MURE2;parent.actor.params=2;parent.actor.room=p.room;indices[&parent.actor]=p.index;parent.actors[p.slice]=&actor;
      GameInteractor_Should(VB_OBJ_MURE2_SET_CHILD_ROOM,true,&parent,int(p.slice));
    }else Hooks::ShouldActorInit[p.actorId](&actor,&should);
    if(!p.slice||p.actorId==ACTOR_OBJ_MURE2)REQUIRE(ActorBehavior::GetObjectRandoCheckId(&actor)==p.check);
    auto emit=[&]{return p.type==RCTYPE_ICICLE?GameInteractor_Should(VB_ICICLE_DROP_COLLECTIBLE,true,&actor):GameInteractor_Should(VB_ENVIRONMENT_DROP_COLLECTIBLE,true,&actor,int(p.slice));};
    failAllocation=true;REQUIRE(!emit());REQUIRE(!saved.eligible&&pendingDrops.size()==1&&activeDrops.empty());
    failAllocation=false;Hooks::OnActorUpdate[ACTOR_PLAYER](nullptr);REQUIRE(activeDrops.size()==1&&pendingDrops.empty()&&!saved.eligible);
    auto count=allocations;REQUIRE(!emit());REQUIRE(allocations==count);
    auto* drop=reinterpret_cast<EnItem00*>(activeDrops.at(p.check));Hooks::OnActorDestroy[ACTOR_EN_ITEM00](&drop->actor);REQUIRE(!saved.eligible);
    REQUIRE(!emit());drop=reinterpret_cast<EnItem00*>(activeDrops.at(p.check));drop->actionFunc(drop,&play);REQUIRE(saved.eligible);
    saved.eligible=false;saved.obtained=true;saved.cycleObtained=false;Hooks::scene(0,0);count=allocations;REQUIRE(emit());REQUIRE(allocations==count);
  }
  allocations=0;
  puts("PASS211 check slots,32 pool combinations,279 native setup/slice callback lifecycles");
}
