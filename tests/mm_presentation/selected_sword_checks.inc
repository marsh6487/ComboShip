// A selected standalone owner sword must reach native MM as well as its
// foreign descriptor. No authored fallback may replace that selected mesh.
for (auto item : {RI_SWORD_KOKIRI, RI_SWORD_RAZOR, RI_SWORD_GILDED,
                  RI_OOT_MASTER_SWORD, RI_OOT_TRUE_MASTER_SWORD,
                  RI_OOT_BIGGORON_SWORD, RI_GREAT_FAIRY_SWORD}) {
  Reset(); ownerAlt = true; alt = true;
  const char* selected = item == RI_OOT_MASTER_SWORD || item == RI_OOT_TRUE_MASTER_SWORD ?
    "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL" :
    item == RI_OOT_BIGGORON_SWORD || item == RI_GREAT_FAIRY_SWORD ?
    "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL" :
    "__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL";
  files.insert(selected);
  assert(MM_TryDrawNeiGi(item) && "native MM discarded the selected owner sword");
  assert(Drawn() == std::vector<std::string>{std::string("__OTR__@oot:") + (selected + 7)});
  assert(arena.size() == 2 && stack.empty() && matrix == 1 && matrixY == 0);
}
Reset(); ownerAlt = false;
std::cout << "PASS native MM selected owner swords: seven awards retain mesh, intrinsic particles and shimmer\n";

for (bool nativeMm : {false, true}) for (bool missingFlame : {false, true}) {
  Reset(); ownerAlt = alt = true; dinSword = true;
  const char* selected = "__OTR__objects/din_fire_sword/progressive/adult/SwordDL";
  files.insert(selected);
  const auto& layers = DinSwordGi::profiles[0];
  for (const auto* dependency : {layers.core, layers.flame, layers.coreVertices, layers.flameVertices,
                                 DinSwordGi::coreTexture, DinSwordGi::flameTexture})
    files.insert(!std::strncmp(dependency,"__OTR__",7)?dependency:std::string("__OTR__")+dependency);
  if (missingFlame) files.erase(std::string("__OTR__")+layers.flameVertices);
  if (nativeMm) assert(MM_TryDrawNeiGi(RI_OOT_MASTER_SWORD));
  else {
    GetItemEntry selectedEntry{}; selectedEntry.drawFunc=Randomizer_DrawMasterSword;
    assert(NeiGi_Draw(&play,&selectedEntry));
  }
  std::vector<std::string> expected{
    nativeMm ? std::string("__OTR__@oot:")+(selected+7) : selected};
  if (!missingFlame) {expected.push_back(layers.core);expected.push_back(layers.flame);}
  assert(Drawn()==expected && "native selected Din sword lost its layers or queued an incomplete pack");
  assert(arena.size()==2 && stack.empty() && matrix==1 && matrixY==0);
}
Reset(); ownerAlt=false;
std::cout << "PASS native OoT/MM Din GI layers: complete owner resources, missing-dependency fallback, sword particles and shimmer\n";
