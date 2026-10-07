// A nonidentity binary fit must change geometry, not the award's procedural
// effect footprint. Compare against the real authored presentation as control.
#ifndef SWORD_TOGGLE_REGRESSION_ONLY
for (const auto& sword : kPresentations) {
  if (!NeiGi::IsSword(sword.effect)) continue;
  const char* selected = SelectedSwordPath(sword, true, [](const char*) { return true; });
  if (!selected) continue;
  for (bool shop : {false, true}) for (int pickup : {0, 1, 2}) {
    Reset(); ownerAlt=alt=true;
    const float center[3]={sword.effectCenter.x,sword.effectCenter.y,sword.effectCenter.z};
    NeiGi_DrawPresentation(&play,sword.opaque,sword.translucent,sword.scale,
                          int(sword.effect),center,true,"oot",shop,pickup);
    const auto expectedEffects=effectSubmitted;
    assert(!expectedEffects.empty());
    Reset(); ownerAlt=alt=true; selectedFitScale=.01f; selectedFitLift=-29;
    NeiGi_DrawExternalPresentation(&play,selected,nullptr,.04f,int(sword.effect),
                                  true,"oot",shop,1.8f,true,pickup);
    assert(std::abs(submitted.front().first-.0004f)<.000001f && submitted.front().second==-29 &&
           "binary sword geometry lost its accepted resource fit");
    assert(effectSubmitted==expectedEffects && "binary model fitting shrank the sword particles/shimmer");
    assert(matrix==1 && matrixY==0 && stack.empty());
    if (!pickup) {
      Reset(); ownerAlt=alt=true; files.insert(selected);
      selectedFitScale=.01f; selectedFitLift=-29;
      GetItemEntry custom{};custom.drawFunc=sword.draw;custom.gid=sword.nativeGid;
      assert(shop?NeiGi_DrawShop(&play,&custom):NeiGi_Draw(&play,&custom));
      assert(effectSubmitted==expectedEffects && "native OoT binary sword effects inherited mesh fit");
      assert(matrix==1 && matrixY==0 && stack.empty());
    }
  }
}
std::cout<<"PASS binary sword effects retain authored footprint across world/shop/Human/Goron fits\n";
// Non-sword receipts keep their established model/shimmer fit.
Reset();selectedFitScale=.01f;selectedFitLift=-29;selectedCustomLists=true;
ComboForeignDrawInfoOOT nonSword{};nonSword.count=1;nonSword.xluStart=-1;
nonSword.drawKind=CW_DRAW_KIND_CUSTOM_GI;nonSword.scale=.04f;
nonSword.dls[0]="__OTR__@oot:objects/custom_gi/lantern";
nonSword.neiShimmer=int(Kind::Neutral)+1;nonSword.itemShimmer=true;
selectedForeignInfoMM=&nonSword;
MM_DrawComboForeign(1,false,1);
// The production mesh packer converts its vertices from sixteenth units.
assert(!effectSubmitted.empty() && effectSubmitted.front()==std::pair(.01f/16,-29.f) &&
       "non-sword receipt shimmer lost its existing geometry fit");
assert(matrix==1 && matrixY==0 && stack.empty());selectedForeignInfoMM=nullptr;

#endif

// Base-path mod geometry must disappear in native OoT vanilla mode, including
// Four Sword. Ordinary Alt namespace checks cannot exclude these mod entries.
for (const auto& sword : kPresentations) {
  if (!NeiGi::IsSword(sword.effect)) continue;
  for (bool host : {true, false, true, false}) {
    Reset(); alt=ownerAlt=host;
    files.insert(sword.opaque);
    modFiles.insert({"oot",sword.opaque});
    GetItemEntry custom{};custom.drawFunc=sword.draw;custom.gid=sword.nativeGid;
    assert(NeiGi_Draw(&play,&custom));
    const std::string expected=host?sword.opaque:
      std::string("__OTR__@oot-gi-base:")+(sword.opaque+7);
    assert(Drawn(true)==std::vector<std::string>{expected} &&
           "native OoT vanilla sword retained mod-capable root selection");
    assert(!effectSubmitted.empty() && matrix==1 && matrixY==0 && stack.empty());
  }
}
// Four Sword keeps intrinsic particles when optional shimmer is disabled.
for (const auto& sword : kPresentations) {
  if (sword.effect!=Kind::FourSword) continue;
  Reset(); ownerAlt=false;
  const float center[3]={sword.effectCenter.x,sword.effectCenter.y,sword.effectCenter.z};
  NeiGi_DrawPresentation(&play,sword.opaque,sword.translucent,sword.scale,
                        int(sword.effect),center,false,"oot-gi-base");
  const auto intrinsic=effectSubmitted;
  Reset(); ownerAlt=false; files.insert(sword.opaque);
  GetItemEntry custom{};custom.drawFunc=sword.draw;custom.gid=sword.nativeGid;
  assert(NeiGi_Draw(&play,&custom));
  assert(!intrinsic.empty() && effectSubmitted==intrinsic &&
         "vanilla Four Sword ignored the optional shimmer setting");
}
Reset();ownerAlt=false;
std::cout<<"PASS native OoT swords/Four Sword: repeated custom/vanilla transitions use shipped geometry\n";
#ifdef SWORD_REGRESSIONS_ONLY
return 0;
#endif
