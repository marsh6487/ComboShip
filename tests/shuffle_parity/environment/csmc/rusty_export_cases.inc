static void TestRustyPresentation(Actor& actor,PlayState& play,const Rando::MMEnvironment::Source& source){
 using namespace Rando;using Category=MMEnvironmentCSMC::Category;
 auto& saved=RANDO_SAVE_CHECKS[source.check];const auto before=saved;const bool beforeEnabled=csmcEnabled;
#ifdef COMBO_BUILD
 const std::vector<RandoItemId> ids(std::begin(csmcRustyItems),std::end(csmcRustyItems));
 const auto exported=CsmcExportActualPool(ids);REQUIRE(exported.size()==66);
#endif
 unsigned index=0;
 for(auto item:csmcRustyItems){
  const auto rawType=StaticData::Items.at(item).randoItemType;
  REQUIRE(rawType==RITYPE_MAJOR); // Presentation must not change pool/solver metadata.
  saved=before;saved.randoItemId=item;
  REQUIRE(GetItemTypeForCheck(item,source.check)==RITYPE_SMALL_KEY);
  for(bool contents:{false,true}){
   csmcEnabled=contents;CsmcBegin(play);CsmcDraw(actor,play);
   REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(contents?Category::SmallKey:Category::Major)});
   REQUIRE(csmcCategoryQueries==0);
  }
  // Actual ConvertItem must run first: an unavailable duplicate remains junk,
  // instead of the presentation helper recoloring its raw rusty identity.
  csmcUnobtainable.insert(item);REQUIRE(ConvertItem(item,source.check)==RI_JUNK);
  REQUIRE(GetItemTypeForCheck(item,source.check)==RITYPE_JUNK);
  csmcEnabled=true;CsmcBegin(play);CsmcDraw(actor,play);
  REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(Category::Junk)});csmcUnobtainable.erase(item);
  REQUIRE(StaticData::Items.at(item).randoItemType==rawType);
#ifdef COMBO_BUILD
  // These fields come from the exact native BenPort category/advancement
  // lambdas and pool serialization body, not a test-authored category map.
  const auto& row=exported.at(index);
  REQUIRE(row.at("name")==StaticData::GetItemDisplayName(item));
  REQUIRE(row.at("category")=="smallKey");REQUIRE(row.at("advancement")==true);
  auto& foreign=csmcForeign[source.check];foreign={};
  foreign.category=row.at("category").get<std::string>();foreign.advancement=row.at("advancement").get<bool>();
  saved.randoItemId=RI_COMBO_FOREIGN;
  for(bool contents:{false,true}){
   csmcEnabled=contents;CsmcBegin(play);CsmcDraw(actor,play);
   REQUIRE(csmcCategoryQueries==int(contents));
   REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(contents?Category::SmallKey:Category::Major)});
  }
  // The existing foreign trap early return must remain ahead of the rusty
  // presentation override, even when the exported disguise is a small key.
  foreign.trap=true;csmcEnabled=true;CsmcBegin(play);CsmcDraw(actor,play);
  REQUIRE(CsmcPrimColors()==std::vector<uint32_t>{CsmcColor(Category::Major)});
  csmcForeign.erase(source.check);
#endif
  ++index;
 }
 REQUIRE(index==66);saved=before;csmcEnabled=beforeEnabled;
#ifdef COMBO_BUILD
 puts("PASS all66Rusty native+actualBenPortexport->foreign CSMC; rawMajor/advancement unchanged, convertedJunk/trapMajor preserved");
#else
 puts("PASS all66Rusty native CSMC; rawMajor metadata unchanged and actual convertedJunk preserved");
#endif
}
