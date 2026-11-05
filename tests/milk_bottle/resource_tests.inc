 auto mm=std::make_shared<Ship::ResourceManager>(); auto oot=std::make_shared<Ship::ResourceManager>();
 Ship::Context ctx; Ship::Context::instance=&ctx; ctx.active=oot;
 Ship::CrossRMRegistry::Register("oot",oot); Ship::CrossRMRegistry::Register("mm",mm);
 auto original=std::make_shared<Ship::Archive>(); original->path="/actual/mm.o2r";
 auto decoy=std::make_shared<Ship::Archive>(); decoy->path="/mods/mm.o2r";
 auto stock=std::make_shared<Fast::DisplayList>(); stock->Instructions.push_back({{0,0}});
 auto invisible=std::make_shared<Fast::DisplayList>();
 invisible->Instructions.push_back({{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("objects/object_gi_milk/mat_gGiMilkBottleContentsDL_ItemInvis")}});
 auto ordinary=std::make_shared<Fast::DisplayList>(); ordinary->Instructions.push_back({{0,0}});
 const std::string content="objects/object_gi_milk/gGiMilkBottleContentsDL",glass="objects/object_gi_milk/gGiMilkBottleGlassDL";
 original->files[content]=stock; original->files[glass]=stock;
 for(const char* key:{"vertices","material","label-texture"}) { original->files[key]=stock; decoy->files[key]=invisible; }
 decoy->files[content]=stock; decoy->files[glass]=stock; decoy->files["alt/"+content]=invisible;
 mm->manager->archives->push_back(original); mm->manager->archives->push_back(decoy); mm->alt=true;
 MM_BindMilkBaseOwner(mm,original);
 auto base=Ship::CrossRMRegistry::Get("mm-gi-base"); assert(base && base->pinned==original && !base->alt);
 stock->dependencies={"vertices","material","label-texture"};
 std::unordered_set<std::string> visited;
 std::function<void(const std::string&)> walk=[&](const std::string& key) {
  if(!visited.insert(key).second) return;
  auto resource=base->LoadResource(key); assert(resource==stock);
  for(const auto& dependency:resource->dependencies) walk(dependency);
 };
 walk(content); walk(glass); assert(visited.size()==5);
 auto select=[&]() { const char* a=gGiMilkBottleContentsDL; const char* b=gGiMilkBottleGlassDL; MM_MilkBottleSelectPair(&a,&b,"export"); assert(ctx.active==oot); return std::pair<std::string,std::string>{a,b}; };
 const auto repaired=select(); assert(repaired.first.starts_with("__OTR__@mm-gi-base:"));
 // MM suspended while OoT remains active: repeated export must still use MM Alt.
 assert(select()==repaired && ctx.active==oot);
 glassMod=true; assert(select().first==gGiMilkBottleContentsDL);
 contentsMod=false; assert(select().first==gGiMilkBottleContentsDL);
 contentsMod=true; glassMod=false;
 MM_MilkBottleSelectPair(nullptr,nullptr,"export"); const char* nullPath=nullptr; MM_MilkBottleSelectPair(&nullPath,&nullPath,"export");
 decoy->files["alt/"+content]=ordinary; assert(select().first==gGiMilkBottleContentsDL); decoy->files["alt/"+content]=invisible;
 mm->alt=false; oot->alt=true; assert(select().first==gGiMilkBottleContentsDL); mm->alt=true; oot->alt=false;
 original->files.erase(content); assert(select().first==gGiMilkBottleContentsDL); original->files[content]=stock;
 original->files.erase(glass); assert(select().first==gGiMilkBottleContentsDL); original->files[glass]=stock;
 original->files[glass]=std::make_shared<Ship::IResource>(); assert(select().first==gGiMilkBottleContentsDL); original->files[glass]=stock;
 stock->Instructions.clear(); assert(select().first==gGiMilkBottleContentsDL); stock->Instructions.push_back({{0,0}});
 Ship::CrossRMRegistry::Unregister("mm-gi-base"); assert(select().first==gGiMilkBottleContentsDL);
 MM_BindMilkBaseOwner(mm,original); assert(select()==repaired);
 // An unrelated alias replacement cannot masquerade as this initialization's view.
 Ship::CrossRMRegistry::Register("mm-gi-base",oot); assert(select().first==gGiMilkBottleContentsDL);
 MM_BindMilkBaseOwner(mm,original); assert(select()==repaired);
 #ifdef COMBO_BUILD
 auto replacement=std::make_shared<Ship::ResourceManager>(); Ship::CrossRMRegistry::Register("mm",replacement);
 assert(select().first==gGiMilkBottleContentsDL); assert(!Ship::CrossRMRegistry::Get("mm-gi-base"));
 replacement->manager->archives->push_back(original); replacement->manager->archives->push_back(decoy); replacement->alt=true;
 MM_BindMilkBaseOwner(replacement,original); assert(select()==repaired);
 // Same parent, different mounted archive identity: the old pointer is never reused.
 auto remounted=std::make_shared<Ship::Archive>(*original); replacement->manager->archives->at(0)=remounted;
 assert(select().first==gGiMilkBottleContentsDL);
 MM_BindMilkBaseOwner(replacement,remounted); assert(select()==repaired);
 replacement->ready=false; MM_BindMilkBaseOwner(replacement,remounted); assert(select().first==gGiMilkBottleContentsDL);
 replacement->ready=true; MM_BindMilkBaseOwner(replacement,nullptr); assert(select().first==gGiMilkBottleContentsDL);
 MM_BindMilkBaseOwner(replacement,remounted);
 #endif
 for(int i=0;i<100;++i) { select(); }
 assert(diagnosticCount<=32);
 for(int i=0;i<200;++i) { glassMod=(i%2)==0; select(); }
 glassMod=false;
 assert(diagnosticCount<=32);
 // Actual teardown clears the route; reinit binds a fresh view on the same parent.
 auto current=Ship::CrossRMRegistry::Get("mm");
 auto currentArchive=current->manager->archives->front();
 MM_BindMilkBaseOwner(nullptr,nullptr); assert(!Ship::CrossRMRegistry::Get("mm-gi-base"));
 assert(select().first==gGiMilkBottleContentsDL);
 MM_BindMilkBaseOwner(current,currentArchive); assert(select()==repaired);
 // Leave a valid orphan for the real native draw and exported recipe checks.
