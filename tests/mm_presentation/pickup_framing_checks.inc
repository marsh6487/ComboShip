// Use the production CustomItem visible receipt pose, native Item0 closeup
// camera values and renderer submissions. The vertex bounds come from every
// serialized OPA/XLU asset, independently of the runtime frame-fit catalog.
const auto receiptPose=FixtureMmReceiptPose();
assert(std::abs(receiptPose.first-.21f)<.00001f && std::abs(receiptPose.second-51.3f)<.00001f);
const float height=44.f; // Player_GetHeight's normal human row.
const float normalizedHeight=.5f*(height+68.f);
const float cameraAt=height+kMmPickupCamera[0]*.01f*normalizedHeight;
const float cameraDistance=kMmPickupCamera[1]*.01f*normalizedHeight;
const float cameraPitch=kMmPickupCamera[2]*NeiGi::Tau/360.f;
const float cameraTan=std::tan(kMmPickupCamera[3]*NeiGi::Tau/720.f);
size_t nativePickupBindings=0;
for(const auto& f:frames) {
  Reset();
  const std::string raw=std::string("__OTR__objects/nei_gi_redesign/")+f.slug+"/gi_dl";
  const std::string shell=std::string("__OTR__objects/nei_gi_redesign/")+f.slug+"/gi_xlu_dl";
  files.insert(raw);if(f.xlu)files.insert(shell);
  FixtureMmReceiptPose();
  Actor receipt{};receipt.id=ACTOR_EN_ITEM00;
  receipt.home.rot.x=CustomItem::CALLED_ACTION|CustomItem::GIVE_ITEM_CUTSCENE;
  CwItemDrawInfo recipe{};
  const auto binding=std::find_if(std::begin(kBindings),std::end(kBindings),
      [&](const auto& b){return !std::strcmp(b.slug,f.slug);});
  if(binding!=std::end(kBindings)) {
    ++nativePickupBindings;
    Rando::DrawResolvedItem(binding->id,1,&receipt);
  } else {
    // Author catalog entries lacking a native MM alias still run through the
    // foreign receipt renderer when a combined check awards that OoT item.
    assert(OOT_GetNeiGiDrawInfo(f.slug,&recipe));
    ComboForeignDrawInfoOOT foreign{};foreign.ok=true;
    foreign.drawKind=recipe.drawKind;foreign.count=recipe.dlistCount;foreign.xluStart=recipe.xluStartIndex;
    foreign.scale=recipe.scale;foreign.neiEffect=recipe.neiEffect;
    std::copy_n(recipe.neiEffectCenter,3,foreign.neiEffectCenter);
    for(int i=0;i<recipe.dlistCount;++i)foreign.dls[i]=NeiResource_Route(recipe.dlists[i]);
    selectedForeignInfoMM=&foreign;
    Rando::DrawResolvedItem(RI_COMBO_FOREIGN,1,&receipt);
    selectedForeignInfoMM=nullptr;
  }
  assert(!submitted.empty() && mmShopLegacyDraws==0);
  const auto [s,y]=submitted.front();
  // Spin extrema include the near side of the model, where perspective raises
  // the projected tip. A local Y cap alone cannot establish screen containment.
  for(int spin=0;spin<360;spin+=3)for(float localY:{f.low,f.high}) {
    const float z=.5f*s*f.width*std::cos(spin*NeiGi::Tau/360.f);
    const float dy=y+s*localY-cameraAt;
    const float depth=cameraDistance-dy*std::sin(cameraPitch)-z*std::cos(cameraPitch);
    const float screenY=(dy*std::cos(cameraPitch)-z*std::sin(cameraPitch))/(depth*cameraTan);
    if(!(depth>0 && screenY<.9f && screenY>-.9f))
      std::cerr<<"MM receipt clips: "<<f.slug<<" spin="<<spin<<" projectedY="<<screenY<<"\n";
    assert(depth>0 && screenY<.9f && screenY>-.9f);
  }
  assert(stack.empty() && interpolation==0);
}
assert(nativePickupBindings>=50);
std::cout<<"PASS actual MM CustomItem/DrawResolvedItem receipt route: all 61 authored OPA/XLU envelopes inside native closeup camera through a full spin\n";
// Lantern's donor-only legacy mesh previously fell through to MM's host loader
// when a selected mod displaced the authored model. Preserve that exact mesh
// and its owning manager through the same native receipt renderer.
for(const char* owner:{"oot","mm"})for(int selectedAlt:{0,1}) {
  Reset();alt=selectedAlt;ownerAlt=selectedAlt;
  const char* legacy="__OTR__objects/object_poh/gPoeLanternDL";
  files.insert(legacy);if(selectedAlt)files.insert(std::string("alt/")+legacy);
  files.insert("__OTR__objects/nei_gi_redesign/lantern/gi_dl");
  files.insert("__OTR__objects/nei_gi_redesign/lantern/gi_xlu_dl");
  modFiles.insert({owner,selectedAlt?std::string("alt/")+legacy:std::string(legacy)});
  CwItemDrawInfo selected{};
  assert(MM_DescribeNeiGi(RI_OOT_NEI_LANTERN,&selected) && "native MM Lantern must retain its selected legacy mod");
  const std::string routed=std::string("__OTR__@")+owner+":objects/object_poh/gPoeLanternDL";
  assert(selected.drawKind==CW_DRAW_KIND_CUSTOM_GI && selected.dlistCount==1 && selected.opCount==0);
  CwItemDrawInfo identity{};assert(!OOT_GetNeiGiDrawInfo("lantern",&identity));
  assert(selected.neiShimmer==identity.neiShimmer && selected.scale==.025f);
  assert(selected.dlists[0]==routed);
  FixtureMmReceiptPose();Actor receipt{};receipt.id=ACTOR_EN_ITEM00;
  receipt.home.rot.x=CustomItem::CALLED_ACTION|CustomItem::GIVE_ITEM_CUTSCENE;
  Rando::DrawResolvedItem(RI_OOT_NEI_LANTERN,1,&receipt);
  assert(Drawn()==std::vector<std::string>{routed} && mmShopLegacyDraws==0);
  assert(stack.empty() && interpolation==0);
}
ownerAlt=false;
std::cout<<"PASS native MM Lantern selected OOT/MM base/Alt legacy model ownership through receipt route\n";
