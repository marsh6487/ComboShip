static void TestNativeEnvironmentTransitions(){
 using namespace Rando;static PlayState play{};gPlayState=&play;ActorBehavior::InitBgIcicleBehavior();
 auto setup=[&](Actor* actor,const MMEnvironment::Source& p,s16 params){
  gSaveContext={};play={};play.sceneId=p.scene;play.roomCtx.curRoom.num=p.room;gSaveContext.sceneLayer=p.setup;
  Hooks::scene(0,0);checkIds.clear();indices.clear();allocations=0;failAllocation=false;
  actor->id=p.actorId;actor->params=params;actor->room=p.room;indices[actor]=p.index;RANDO_SAVE_CHECKS[p.check].shuffled=true;
  bool should=true;Hooks::ShouldActorInit[p.actorId](actor,&should);
 };
 for(const auto& p:MMEnvironment::Sources)if(p.type==RCTYPE_ROCK&&p.actorId==ACTOR_EN_ISHI){
  EnIshi rock{};setup(&rock.actor,p,0);int before=vanillaDrops;EnIshi_DropItem(&rock,&play);REQUIRE(allocations==1&&vanillaDrops==before);EnIshi_DropItem(&rock,&play);REQUIRE(allocations==1);break;
 }
 for(const auto& p:MMEnvironment::Sources)if(p.type==RCTYPE_BOULDER){
  ObjBombiwa rock{};setup(&rock.actor,p,0);ColliderElement hit{};Actor attack{};rock.collider.base.ac=&attack;rock.collider.elem.acHitElem=&hit;
  func_80939EF4(&rock,&play);REQUIRE(allocations==0);hit.atDmgInfo.dmgFlags=DMG_SWORD;rock.collider.base.acFlags=AC_HIT;func_80939EF4(&rock,&play);REQUIRE(allocations==0);
  hit.atDmgInfo.dmgFlags=DMG_GORON_PUNCH;rock.collider.base.acFlags=AC_HIT;func_80939EF4(&rock,&play);REQUIRE(allocations==1);
  setup(&rock.actor,p,0x100);rock.collider.base.acFlags=AC_HIT;REQUIRE(!func_80939470(&rock.actor));hit.atDmgInfo.dmgFlags=DMG_EXPLOSIVES;attack.world.pos.x=116;REQUIRE(!func_80939470(&rock.actor));attack.world.pos.x=114;REQUIRE(func_80939470(&rock.actor));func_80939EF4(&rock,&play);REQUIRE(allocations==1);
  REQUIRE(!GameInteractor_Should(VB_ENVIRONMENT_SOURCE_USED,true,&rock.actor));RANDO_SAVE_CHECKS[p.check].obtained=true;REQUIRE(GameInteractor_Should(VB_ENVIRONMENT_SOURCE_USED,true,&rock.actor));break;
 }
 for(const auto& p:MMEnvironment::Sources)if(p.type==RCTYPE_RED_BOULDER){
  ObjHamishi rock{};setup(&rock.actor,p,0);ColliderElement hit{};rock.collider.elem.acHitElem=&hit;hit.atDmgInfo.dmgFlags=DMG_GORON_PUNCH;
  rock.collider.base.acFlags=AC_HIT;ObjHamishi_Update(&rock.actor,&play);REQUIRE(allocations==0&&rock.unk_19E==1);rock.collider.base.acFlags=AC_HIT;ObjHamishi_Update(&rock.actor,&play);REQUIRE(allocations==1&&rock.unk_19E==2);break;
 }
 for(const auto& p:MMEnvironment::Sources)if(p.type==RCTYPE_SOIL&&!p.slice&&MMEnvironment::Identify(p.scene,p.setup,p.room,p.index,1,ACTOR_OBJ_SWPRIZE)!=RC_UNKNOWN){
  ObjSwprize soil{};setup(&soil.actor,p,0x200);play.actorCtx.actorLists[ACTORCAT_PROP].first=&soil.actor;
  for(int i=0;i<3;++i)RANDO_SAVE_CHECKS[MMEnvironment::Identify(p.scene,p.setup,p.room,p.index,i,ACTOR_OBJ_SWPRIZE)].shuffled=true;
  REQUIRE(!GameInteractor_Should(VB_ENVIRONMENT_SOURCE_USED,true,&soil.actor));REQUIRE(!GameInteractor_Should(VB_ENVIRONMENT_SOIL_TRIGGER,true,&soil.actor));
  ObjBean bean{};bean.dyna.actor.id=ACTOR_OBJ_BEAN;bean.dyna.actor.params=0x4000;bean.dyna.actor.room=p.room;bean.unk_1E4=5;
  Hooks::OnActorUpdate[ACTOR_OBJ_BEAN](&bean.dyna.actor);REQUIRE(bean.unk_1E4==0);REQUIRE(!GameInteractor_Should(VB_ENVIRONMENT_SOIL_USED,true,&bean.dyna.actor));
  bean.unk_1E8=[](ObjBean*){};bean.unk_1E0=2;func_80938298(&bean,&play);REQUIRE(!GameInteractor_Should(VB_ENVIRONMENT_SOIL_TRIGGER,true,&soil.actor));
  bean.unk_1E0=3;func_80938298(&bean,&play);REQUIRE(GameInteractor_Should(VB_ENVIRONMENT_SOIL_TRIGGER,true,&soil.actor));REQUIRE(!GameInteractor_Should(VB_ENVIRONMENT_SOIL_TRIGGER,true,&soil.actor));
  int before=vanillaDrops;func_80C253D0(&soil,&play);REQUIRE(allocations==3&&vanillaDrops==before);func_80C253D0(&soil,&play);REQUIRE(allocations==3);
  // After native timer->DoNothing and physical reward unload, a fresh burrow rearms
  // the native Wait state; merely staying/reloading never emits an award.
  std::vector<Actor*> unloaded;for(const auto& [id,actor]:activeDrops)unloaded.push_back(actor);
  for(auto* actor:unloaded)Hooks::OnActorDestroy[ACTOR_EN_ITEM00](actor);
  soil.actionFunc=ObjSwprize_DoNothing;func_80938298(&bean,&play);
  REQUIRE(soil.actionFunc==func_80C25654&&allocations==3);
  REQUIRE(GameInteractor_Should(VB_ENVIRONMENT_SOIL_TRIGGER,true,&soil.actor));
  func_80C253D0(&soil,&play);REQUIRE(allocations==6);
  soil.actionFunc=func_80C253D0;func_80938298(&bean,&play);
  REQUIRE(soil.actionFunc==func_80C253D0&&allocations==6);
  Hooks::scene(0,0);REQUIRE(!GameInteractor_Should(VB_ENVIRONMENT_SOIL_TRIGGER,true,&soil.actor));break;
 }
 allocations=0;puts("PASS actual native rock/boulder/red-boulder/soil/three-bug positive transitions and negatives");
}
