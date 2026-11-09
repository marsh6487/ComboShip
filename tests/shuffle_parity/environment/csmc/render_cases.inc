static constexpr RandoItemId csmcRustyItems[]={
#define RUSTY_KEY(game,key,rg,ri,flag,name) ri,
#include "combo/rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY
};
static_assert(std::size(csmcRustyItems)==66);
#include "environment_csmc_native_bounds.inc"
static Gfx csmcOpa[4096],csmcXlu[4096],csmcOverlay[4096];
static GraphicsContext csmcGfx;
static int csmcNativeGeometry;
static void CsmcNativeDraw(Actor*,PlayState*){++csmcNativeGeometry;}
static void CsmcBegin(PlayState& play){
 memset(csmcOpa,0x35,sizeof(csmcOpa));memset(csmcXlu,0,sizeof(csmcXlu));memset(csmcOverlay,0x43,sizeof(csmcOverlay));
 csmcGfx={};csmcGfx.polyOpa.p=csmcOpa;csmcGfx.polyXlu.p=csmcXlu;csmcGfx.overlay.p=csmcOverlay;
 play.state.gfxCtx=&csmcGfx;play.billboardMtxF={};
 play.billboardMtxF.xx=play.billboardMtxF.yy=play.billboardMtxF.zz=play.billboardMtxF.ww=1;
 sCurrentMatrix=csmcMatrixStack;csmcMatrixStack[0]={};
 sCurrentMatrix->xx=2;sCurrentMatrix->yy=3;sCurrentMatrix->zz=4;sCurrentMatrix->ww=1;sCurrentMatrix->xw=17;sCurrentMatrix->yw=-8;
 csmcMatrices.clear();csmcResources=csmcMatrixDepth=csmcFrameDepth=csmcNativeGeometry=csmcCategoryQueries=0;
}
static std::vector<uint32_t> CsmcPrimColors(){
 std::vector<uint32_t> result;
 for(auto* p=csmcXlu;p<csmcGfx.polyXlu.p;++p)if(((p->words.w0>>24)&0xff)==G_SETPRIMCOLOR)result.push_back(p->words.w1);
 return result;
}
static uint32_t CsmcColor(ComboRando::ContainerCategoryColors::Category type){
 auto c=ComboRando::ContainerCategoryColors::Get(type);return (uint32_t(c.r)<<24)|(uint32_t(c.g)<<16)|(uint32_t(c.b)<<8)|190;
}
static void CsmcDraw(Actor& actor,PlayState& play){
 auto before=*sCurrentMatrix;const auto oldOpa=csmcOpa[0],oldOverlay=csmcOverlay[0];auto* original=actor.draw;auto oldSave=std::make_unique<SaveContext>(gSaveContext);
 auto oldAllocations=allocations;auto oldPending=pendingDrops.size(),oldActive=activeDrops.size();
 if(actor.draw)actor.draw(&actor,&play);
 Hooks::OnActorDraw.at(actor.id)(&actor);
 REQUIRE(actor.draw==original);REQUIRE(sCurrentMatrix==csmcMatrixStack);
 REQUIRE(!memcmp(&before,sCurrentMatrix,sizeof(before)));REQUIRE(csmcMatrixDepth==0&&csmcFrameDepth==0);
 REQUIRE(csmcGfx.polyOpa.p==csmcOpa&&csmcGfx.overlay.p==csmcOverlay);
 REQUIRE(!memcmp(&oldOpa,&csmcOpa[0],sizeof(Gfx))&&!memcmp(&oldOverlay,&csmcOverlay[0],sizeof(Gfx)));
 REQUIRE(allocations==oldAllocations&&pendingDrops.size()==oldPending&&activeDrops.size()==oldActive);
 REQUIRE(!memcmp(oldSave.get(),&gSaveContext,sizeof(gSaveContext)));
 if(csmcResources){
  Gfx* last=nullptr;for(auto* p=csmcXlu;p<csmcGfx.polyXlu.p;++p)if(((p->words.w0>>24)&0xff)==G_DL)last=reinterpret_cast<Gfx*>(p->words.w1);
  REQUIRE(last==gSetupDLs[SETUPDL_25]);
 }
}
static Actor CsmcSource(const Rando::MMEnvironment::Source& source,PlayState& play){
 play.sceneId=source.scene;play.roomCtx.curRoom.num=source.room;gSaveContext.sceneLayer=source.setup;
 Actor actor{};actor.id=source.actorId==ACTOR_OBJ_MURE2?ACTOR_EN_ISHI:source.actorId;
 actor.room=source.room;actor.params=source.type==RCTYPE_SOIL?0x200:0;
 actor.scale={.1f,.1f,.1f};if(actor.id==ACTOR_OBJ_HAMISHI)actor.scale={.4f,.4f,.4f};
 actor.world.pos={100,200,300};if(actor.id!=ACTOR_OBJ_SWPRIZE)actor.draw=CsmcNativeDraw;
 indices[&actor]=source.index;
 return actor;
}
static void CsmcIdentify(Actor& actor,const Rando::MMEnvironment::Source& source){
 indices[&actor]=source.index;bool should=true;
 if(source.actorId==ACTOR_OBJ_MURE2){
  ObjMure2 parent{};parent.actor.id=ACTOR_OBJ_MURE2;parent.actor.room=source.room;parent.actor.params=2;
  indices[&parent.actor]=source.index;parent.actors[source.slice]=&actor;
  GameInteractor_Should(VB_OBJ_MURE2_SET_CHILD_ROOM,true,&parent,int(source.slice));
 }else Hooks::ShouldActorInit.at(actor.id)(&actor,&should);
 REQUIRE(should);
}
#include "environment_csmc_rusty_export.inc"
#include "rusty_export_cases.inc"
static void TestEnvironmentCSMC(){
 using namespace Rando;using Category=MMEnvironmentCSMC::Category;
 auto originalSave=std::make_unique<SaveContext>(gSaveContext);auto* originalPlay=gPlayState;
 static PlayState play{};gPlayState=&play;ActorBehavior::InitBgIcicleBehavior();
 unsigned rustyFailures=0;
 for(auto item:csmcRustyItems){
  auto check=MMEnvironment::Sources[0].check;RANDO_SAVE_CHECKS[check].randoItemId=item;
  const auto category=GetItemTypeForCheck(item,check);
  if(category!=RITYPE_SMALL_KEY){fprintf(stderr,"Rusty native presentation item=%d raw=%d actual=%d expected=%d\n",item,StaticData::Items.at(item).randoItemType,category,RITYPE_SMALL_KEY);++rustyFailures;}
 }
 REQUIRE(rustyFailures==0);

 // Every actual source alias, including Mure2 children and a soil source whose
 // first slice is absent, must keep its authored identity and native geometry.
 for(const auto& source:MMEnvironment::Sources)for(bool contents:{false,true}){
  gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;Hooks::scene(0,0);checkIds.clear();indices.clear();
  csmcEnabled=contents;auto& saved=RANDO_SAVE_CHECKS[source.check];saved.shuffled=true;saved.randoItemId=RI_HEART_PIECE;
  auto actor=CsmcSource(source,play);auto* draw=actor.draw;CsmcIdentify(actor,source);
  REQUIRE(actor.id==ACTOR_OBJ_SWPRIZE?actor.draw!=nullptr:actor.draw==draw);
  CsmcBegin(play);CsmcDraw(actor,play);
  REQUIRE(csmcResources==1&&csmcMatrices.size()==1);
  REQUIRE(csmcNativeGeometry==int(actor.id!=ACTOR_OBJ_SWPRIZE));
  REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(contents?Category::Health:Category::Major)});
  auto geometry=MMEnvironmentCSMC::GeometryFor(actor);
  REQUIRE(csmcMatrices[0].xw==actor.world.pos.x+(actor.id==ACTOR_OBJ_SWPRIZE?(source.slice-1)*geometry.spacing:0));
  REQUIRE(csmcMatrices[0].yw==actor.world.pos.y+geometry.height&&csmcMatrices[0].zw==actor.world.pos.z);
  REQUIRE(csmcMatrices[0].yw>actor.world.pos.y+NativeCsmcColliderHeight(actor));
 }
 auto source=MMEnvironment::Sources[0];
 for(const auto& candidate:MMEnvironment::Sources)if(candidate.actorId==ACTOR_EN_ISHI){source=candidate;break;}
 REQUIRE(source.actorId==ACTOR_EN_ISHI);
 gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;Hooks::scene(0,0);checkIds.clear();indices.clear();
 auto actor=CsmcSource(source,play);CsmcIdentify(actor,source);
 auto& saved=RANDO_SAVE_CHECKS[source.check];saved.shuffled=true;saved.randoItemId=RI_HEART_PIECE;CsmcIdentify(actor,source);
 // Native categories use the actual item table, complete ConvertItem and
 // GetItemTypeForCheck. Obtainability remains the existing engine boundary.
 const std::pair<RandoItemId,RandoItemType> native[]={
  {RI_RUPEE_GREEN,RITYPE_JUNK},{RI_BOMB_BAG_30,RITYPE_LESSER},{RI_HEART_PIECE,RITYPE_HEALTH},
  {RI_WOODFALL_BOSS_KEY,RITYPE_BOSS_KEY},{RI_WOODFALL_SMALL_KEY,RITYPE_SMALL_KEY},
  {RI_MASK_DEKU,RITYPE_MASK},{RI_GS_TOKEN_SWAMP,RITYPE_SKULLTULA_TOKEN},
  {RI_WOODFALL_STRAY_FAIRY,RITYPE_STRAY_FAIRY},{RI_HOOKSHOT,RITYPE_MAJOR}};
 for(auto [item,type]:native)for(bool contents:{false,true}){
  saved.randoItemId=item;csmcEnabled=contents;CsmcBegin(play);CsmcDraw(actor,play);
  REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(contents?MMEnvironmentCSMC::CategoryForType(type):Category::Major)});
 }
 TestRustyPresentation(actor,play,source);
 saved.randoItemId=RI_PROGRESSIVE_MAGIC;csmcEnabled=true;
 gSaveContext.save.saveInfo.playerData.isMagicAcquired=false;gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired=false;
 REQUIRE(ConvertItem(RI_PROGRESSIVE_MAGIC,source.check)==RI_SINGLE_MAGIC);
 CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(Category::Major)});
 gSaveContext.save.saveInfo.playerData.isMagicAcquired=true;REQUIRE(ConvertItem(RI_PROGRESSIVE_MAGIC,source.check)==RI_DOUBLE_MAGIC);
 CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(Category::Lesser)});
 csmcUnobtainable.insert(RI_PROGRESSIVE_MAGIC);CsmcBegin(play);CsmcDraw(actor,play);
 REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(Category::Junk)});csmcUnobtainable.clear();
#ifdef COMBO_BUILD
 saved.randoItemId=RI_COMBO_FOREIGN;
 const std::pair<const char*,RandoItemType> foreignTypes[]={{"junk",RITYPE_JUNK},{"lesser",RITYPE_LESSER},{"health",RITYPE_HEALTH},
  {"bossKey",RITYPE_BOSS_KEY},{"smallKey",RITYPE_SMALL_KEY},{"token",RITYPE_SKULLTULA_TOKEN},{"mask",RITYPE_MASK},
  {"strayFairy",RITYPE_STRAY_FAIRY},{"major",RITYPE_MAJOR},{"unknown",RITYPE_MAJOR}};
 for(auto [name,type]:foreignTypes)for(bool contents:{false,true}){
  csmcForeign[source.check]={};csmcForeign[source.check].category=name;csmcEnabled=contents;CsmcBegin(play);CsmcDraw(actor,play);
  REQUIRE(csmcCategoryQueries==int(contents));
  REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(contents?MMEnvironmentCSMC::CategoryForType(type):Category::Major)});
 }
 for(bool advancement:{false,true}){
  csmcForeign[source.check]={};csmcForeign[source.check].advancement=advancement;csmcEnabled=true;
  CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(advancement?Category::Major:Category::Junk)});
 }
 csmcForeign[source.check].category="junk";csmcForeign[source.check].trap=true;
 CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(Category::Major)});csmcForeign.clear();
#endif
 saved.randoItemId=RI_HEART_PIECE;
 // Independent loaded-source lifecycle gates; render neither claims nor spawns.
 for(bool contents:{false,true})for(int gate=0;gate<8;++gate){
  saved={};saved.shuffled=true;saved.randoItemId=RI_HEART_PIECE;csmcEnabled=contents;gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
  if(gate==0)saved.shuffled=false;if(gate==1)saved.skipped=true;if(gate==2)saved.obtained=true;
  if(gate==3)saved.cycleObtained=true;if(gate==4)saved.eligible=true;
  if(gate==5)pendingDrops[source.check]={play.sceneId,actor.room,actor.world.pos};
  if(gate==6)activeDrops[source.check]=&actor;
  if(gate==7)gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_VANILLA;
  CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(csmcResources==0&&CsmcPrimColors().empty());
  pendingDrops.clear();activeDrops.clear();
 }
 gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;saved={};saved.shuffled=true;saved.randoItemId=RI_HEART_PIECE;csmcEnabled=true;
 failAllocation=true;bool should=true;QueueSourceDrop(&actor,source.check,&should);REQUIRE(!should&&pendingDrops.contains(source.check));
 CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(csmcResources==0);
 failAllocation=false;Hooks::OnActorUpdate[ACTOR_PLAYER](nullptr);REQUIRE(activeDrops.contains(source.check));
 CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(csmcResources==0);
 Hooks::OnActorDestroy[ACTOR_EN_ITEM00](activeDrops.at(source.check));CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(csmcResources==1);
 // Carried/scaled sources use the live position and dimensions, never home.
 actor.parent=&actor;actor.world.pos={-500,1000,2000};actor.scale.y*=2;
 CsmcBegin(play);CsmcDraw(actor,play);auto geometry=MMEnvironmentCSMC::GeometryFor(actor);
 REQUIRE(csmcMatrices[0].xw==-500&&csmcMatrices[0].yw==1000+geometry.height&&csmcMatrices[0].zw==2000);
 actor.parent=nullptr;
 auto oldScale=actor.scale.y;actor.scale.y=0;CsmcBegin(play);CsmcDraw(actor,play);REQUIRE(csmcResources==0);actor.scale.y=oldScale;
 // Three exact soil slices remain independently categorized and disappear
 // individually. A native/custom non-null draw is preserved without duplicates.
 bool triple=false;
 for(const auto& first:MMEnvironment::Sources)if(first.actorId==ACTOR_OBJ_SWPRIZE&&first.slice==0){
  auto second=MMEnvironment::Identify(first.scene,first.setup,first.room,first.index,1,first.actorId);
  auto third=MMEnvironment::Identify(first.scene,first.setup,first.room,first.index,2,first.actorId);
  if(second==RC_UNKNOWN||third==RC_UNKNOWN)continue;
  triple=true;gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;Hooks::scene(0,0);checkIds.clear();indices.clear();
  auto soil=CsmcSource(first,play);RandoCheckId checks[]={first.check,second,third};
  RandoItemId items[]={RI_HEART_PIECE,RI_MASK_DEKU,RI_WOODFALL_SMALL_KEY};
  for(int i=0;i<3;++i){auto& check=RANDO_SAVE_CHECKS[checks[i]];check.shuffled=true;check.randoItemId=items[i];}
  CsmcIdentify(soil,first);
  for(bool contents:{false,true}){
   csmcEnabled=contents;CsmcBegin(play);CsmcDraw(soil,play);REQUIRE(csmcResources==3&&csmcMatrices.size()==3);
   REQUIRE(csmcMatrices[0].xw==82&&csmcMatrices[1].xw==100&&csmcMatrices[2].xw==118);
   REQUIRE(CsmcPrimColors()==(contents?std::vector<uint32_t>{CsmcColor(Category::Health),CsmcColor(Category::Mask),CsmcColor(Category::SmallKey)}:std::vector<uint32_t>(3,CsmcColor(Category::Major))));
  }
  RANDO_SAVE_CHECKS[first.check].shuffled=false;RANDO_SAVE_CHECKS[third].obtained=true;
  soil.draw=nullptr;CsmcIdentify(soil,first);REQUIRE(soil.draw!=nullptr);
  CsmcBegin(play);CsmcDraw(soil,play);REQUIRE(csmcResources==1&&csmcMatrices[0].xw==100);
  soil.draw=CsmcNativeDraw;CsmcIdentify(soil,first);REQUIRE(soil.draw==CsmcNativeDraw);
  CsmcBegin(play);CsmcDraw(soil,play);REQUIRE(csmcResources==1&&csmcNativeGeometry==1);
  break;
 }
 REQUIRE(triple);Hooks::scene(0,0);gSaveContext=*originalSave;gPlayState=originalPlay;csmcEnabled=false;
 puts("PASS native CSMC211checks/279sourcealiases: genericOff/resolvedOn, native+foreign categories, nativeGraphics/matrices/state, soil slices/carried scale and source lifecycle");
}
