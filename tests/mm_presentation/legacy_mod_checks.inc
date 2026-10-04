// Execute the real MM selector against MM-specific archive GI roots. OoT's
// corresponding callbacks may be inline models or use different native parts.
{
  struct MmLegacy { RandoItemId item; const char* slug; const char* path; };
  const MmLegacy models[]={
    {RI_OOT_NEI_FIRE_ROD,"fire_rod","objects/object_nei_fire_rod/Cylinder_001_opaque_dl"},
    {RI_OOT_NEI_ICE_ROD,"ice_rod","objects/object_nei_ice_rod/ice_rod_opaque_dl"},
    {RI_OOT_NEI_ICE_ROD,"ice_rod","objects/object_nei_ice_rod/ice_rod_transparent_dl"},
    {RI_OOT_NEI_LIGHT_ROD,"light_rod","objects/object_nei_light_rod/Cylinder_002_opaque_dl"},
    {RI_OOT_NEI_LIGHT_ROD,"light_rod","objects/object_nei_light_rod/Cylinder_002_transparent_dl"},
    {RI_OOT_NEI_BALL_AND_CHAIN,"ball_and_chain","objects/object_nei_ball_and_chain/g_ball_and_chain_dl"},
    {RI_OOT_NEI_BEETLE,"beetle","objects/object_nei_beetle/g_beetle_dl"},
    {RI_OOT_NEI_SHOVEL,"shovel","objects/object_nei_shovel/gShovelGiveDL_opaque_dl"},
    {RI_OOT_NEI_HYLIAS_GRACE,"hylia_grace","objects/object_nei_magic_spell/gHyliaGraceGiveDL"},
    {RI_OOT_NEI_ZONAI_PERMAFROST,"zonai_permafrost","objects/object_nei_magic_spell/gZonaiPermafrostGiveDL"},
    {RI_OOT_NEI_DEMISE_DESTRUCTION,"demise_destruction","objects/object_nei_magic_spell/gDemiseDestructionGiveDL"},
    {RI_SWORD_KOKIRI,"mm_kokiri_sword","objects/object_gi_sword_1/gGiKokiriSwordGuardDL"},
    {RI_SWORD_KOKIRI,"mm_kokiri_sword","objects/object_gi_sword_1/gGiKokiriSwordBladeHiltDL"},
    {RI_OOT_EXT_FOUR_SWORD,"four_sword","objects/object_gi_sword_1/gGiKokiriSwordGuardDL"},
    {RI_OOT_EXT_FOUR_SWORD,"four_sword","objects/object_gi_sword_1/gGiKokiriSwordBladeHiltDL"},
    {RI_SHIELD_MIRROR,"shield_of_ikana","objects/object_gi_shield_3/gGiMirrorShieldEmptyDL"},
    {RI_SHIELD_MIRROR,"shield_of_ikana","objects/object_gi_shield_3/gGiMirrorShieldDL"},
    {RI_OOT_EXT_MAGIC_CAPE,"magic_cape","objects/object_gi_clothes/gGiTunicCollarDL"},
    {RI_OOT_EXT_MAGIC_CAPE,"magic_cape","objects/object_gi_clothes/gGiTunicDL"},
  };
  for(const auto& model:models) for(bool activeAlt:{false,true}){
    Reset();ownerAlt=false;alt=activeAlt;
    const auto authored=std::string("__OTR__objects/nei_gi_redesign/")+model.slug+"/gi_dl";
    const auto legacy=std::string("__OTR__")+model.path;
    files.insert(authored);files.insert(authored.substr(0,authored.size()-5)+"gi_xlu_dl");
    files.insert(legacy);if(activeAlt)files.insert("alt/"+legacy);
    CwItemDrawInfo info{};assert(MM_DescribeNeiGi(model.item,&info));
    assert(MM_TryDrawNeiGi(model.item));
    gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;
    modFiles.insert({"mm",activeAlt?"alt/"+legacy:legacy});
    assert(!MM_DescribeNeiGi(model.item,&info));
    assert(!MM_TryDrawNeiGi(model.item)&&gfx.polyOpa.p==opa&&gfx.polyXlu.p==xlu);
    // MM-specific GI paths must not disable the OoT inline source models.
    if(std::strcmp(model.slug,"mm_kokiri_sword")&&std::strcmp(model.slug,"shield_of_ikana")&&std::strcmp(model.slug,"magic_cape"))
      assert(OOT_GetNeiGiDrawInfo(model.slug,&info));
    modFiles.clear();assert(MM_DescribeNeiGi(model.item,&info));
  }
  Reset();ownerAlt=false;
  files.insert("__OTR__objects/nei_gi_redesign/shadow_scepter/gi_dl");
  modFiles.insert({"mm","__OTR__objects/object_nei_wand_shadow_scepter/gNeiShadowScepterDL"});
  CwItemDrawInfo info{};assert(MM_DescribeNeiGi(RI_OOT_NEI_WAND_SHADOW_SCEPTER,&info));
  std::cout<<"PASS MM legacy GI priority: 19 actual MM roots, both split parts, base/Alt, real native describe/draw selectors, held-only wand exclusion\n";
}
