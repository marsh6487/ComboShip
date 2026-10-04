#define MM_REAL_RENDERER
#include "../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "2s2h/Rando/NeiGiPresentation.h"
#include "2s2h/Rando/NeiLanternPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"
#include "2s2h/Rando/NeiUsedMagicPresentation.h"
#include "rod_runtime_test.cpp"
#include <set>
std::set<std::string> ownerBase, ownerAlt;
bool ownerAltEnabled = false, mmAltEnabled = false, ownerRegistered = true;
static std::set<std::string> descriptorPaths;
static int descriptorCalls;
extern "C" __attribute__((visibility("default"))) int32_t
OOT_NeiResourceExists(const char *path) {
  return ownerRegistered && (ownerBase.contains(path) ||
                             (ownerAltEnabled && ownerAlt.contains(path)));
}
extern "C" __attribute__((visibility("default"))) int32_t
OOT_GetNeiGiDrawInfo(const char* slug, CwItemDrawInfo* out) {
  ++descriptorCalls;
  std::string prefix=std::string("__OTR__objects/nei_gi_redesign/")+slug;
  const char* opa=descriptorPaths.insert(prefix+"/gi_dl").first->c_str();
  const bool split=!strcmp(slug,"phantom_hourglass") || !strcmp(slug,"shadow_crystal");
  const char* skin=split ? descriptorPaths.insert(prefix+"/gi_xlu_dl").first->c_str() : nullptr;
  if(!OOT_NeiResourceExists(opa) || (skin && !OOT_NeiResourceExists(skin))) return 0;
  *out={}; out->drawKind=CW_DRAW_KIND_NEI_GI; out->dlistCount=split ? 2 : 1;
  out->dlists[0]=opa; out->dlists[1]=skin; out->xluStartIndex=split ? 1 : -1;
  out->scale=1; out->itemShimmer=1;
  return 1;
}
static bool itemEffects;
extern "C" int32_t CVarGetInteger(const char* name,int32_t value) {
  return !strcmp(name,"gEnhancements.SkijerNEI.ItemEffects") ? itemEffects : value;
}
void DrawOotSlateRuneFlame(u8,u8,u8) {assert(false && "concrete non-Somaria GI must not borrow a flame");}
extern "C" void gSPVertex(Gfx *cmd, uintptr_t addr, int count, int v0) {
  cmd->words.w0 = (G_VTX << 24) | (count << 12) | ((v0 + count) << 1);
  cmd->words.w1 = addr;
}
u8 slots[8]{};
extern "C" u8 ItemEquip_GetItemOnSlot(u8 i) { return slots[i]; }
int main() {
  PlayState play{};
  Player p{};
  GraphicsContext gfx{};
  static Gfx opa[100000], xlu[100000];
  play.state.gfxCtx = &gfx;
  gPlayState=&play;
  play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
  auto reset = [&]() {
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + 100000;
    gfx.polyXlu.p = xlu;
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
  };
  // Execute the real MM binding/description code and its owner-routed draw.
  // Unknown items and missing/incomplete resources retain the native fallback.
  const std::pair<RandoItemId,const char*> candidates[] = {
    {RI_OOT_NEI_ELEMENTAL_WAND,"elemental_wand"},
    {RI_OOT_NEI_WAND_SAND_ROD,"sand_rod"}, {RI_OOT_NEI_WAND_TORNADO_ROD,"tornado_rod"},
    {RI_OOT_NEI_WAND_WATER_ROD,"water_rod"}, {RI_OOT_NEI_WAND_METEOR_ROD,"meteor_rod"},
    {RI_OOT_NEI_WAND_STORM_ROD,"storm_rod"}, {RI_OOT_NEI_WAND_SHADOW_SCEPTER,"shadow_scepter"},
    {RI_OOT_EXT_DIVINE_SHIELD,"divine_shield"}, {RI_OOT_EXT_SHEIKAH_SHIELD,"sheikah_shield"},
    {RI_SHIELD_MIRROR,"shield_of_ikana"}, {RI_OOT_EXT_MAGIC_CAPE,"magic_cape"},
    {RI_OOT_EXT_SPIRIT_BREASTPLATE,"spirit_breastplate"},
    {RI_OOT_EXT_WATER_DRAGON_SCALE,"sages_tunic"}, {RI_OOT_EXT_CHAMPIONS_TUNIC,"champions_tunic"},
    {RI_OOT_EXT_PEGASUS_ANKLET,"pegasus_anklet"}, {RI_OOT_EXT_TRIDENT,"trident"},
    {RI_OOT_EXT_CLIMB_BOOTS,"climb_boots"}, {RI_OOT_EXT_ROC_BOOTS,"roc_boots"},
    {RI_OOT_EXT_CANE_OF_BYRNA,"cane_of_byrna"}, {RI_OOT_EXT_FOUR_SWORD,"four_sword"},
    {RI_PENDANT_OF_MEMORIES,"pendant_of_memories"}, {RI_OOT_NEI_SHEIKAH_SLATE,"sheikah_slate"},
    {RI_OOT_NEI_SLATE_RUNE_BOMB,"slate_bomb"}, {RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE,"slate_master_cycle"},
    {RI_OOT_NEI_SLATE_RUNE_STASIS,"slate_stasis"}, {RI_OOT_NEI_SLATE_RUNE_CRYONIS,"slate_cryonis"},
    {RI_OOT_NEI_DESIRE_SENSOR,"slate_sensor"}, {RI_OOT_NEI_PHANTOM_HOURGLASS,"phantom_hourglass"},
    {RI_OOT_NEI_SHADOW_CRYSTAL,"shadow_crystal"}, {RI_OOT_NEI_ROD_OF_SEASONS,"rod_of_seasons"},
    {RI_SWORD_KOKIRI,"mm_kokiri_sword"}, {RI_SWORD_RAZOR,"razor_sword"}, {RI_SWORD_GILDED,"gilded_sword"},
    {RI_OOT_MASTER_SWORD,"master_sword"}, {RI_OOT_TRUE_MASTER_SWORD,"true_master_sword"},
    {RI_OOT_BIGGORON_SWORD,"biggoron_sword"}, {RI_GREAT_FAIRY_SWORD,"great_fairy_sword"},
    {RI_OOT_IRON_KNUCKLE_AXE,"iron_knuckle_axe"}
  };
  CwItemDrawInfo described{};
  assert(!MM_DescribeNeiGi(RI_BOW,&described) && descriptorCalls==0);
  for(auto [id,slug] : candidates) {
    reset(); ownerBase.clear(); ownerAlt.clear();
    assert(!MM_TryDrawNeiGi(id) && gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
    const std::string raw=std::string("__OTR__objects/nei_gi_redesign/")+slug;
    ownerAlt.insert(raw+"/gi_dl"); ownerAlt.insert(raw+"/gi_xlu_dl");
    ownerAltEnabled=false; mmAltEnabled=true;
    assert(!MM_DescribeNeiGi(id,&described));
    ownerAltEnabled=true; mmAltEnabled=false;
    assert(MM_DescribeNeiGi(id,&described));
    assert(std::string(described.dlists[0])==std::string("__OTR__@oot:")+raw.substr(7)+"/gi_dl");
    assert(MM_TryDrawNeiGi(id));
    int bodies=0,skins=0;
    for(auto range : {std::pair(opa,gfx.polyOpa.p),std::pair(xlu,gfx.polyXlu.p)})
      for(Gfx* cmd=range.first;cmd<range.second;++cmd)
        if(cmd->words.w0>>24==G_DL_OTR_FILEPATH) {
          std::string path=(const char*)cmd->words.w1;
          assert(path.starts_with("__OTR__@oot:objects/nei_gi_redesign/"));
          if(path.ends_with("/gi_dl")) ++bodies; else ++skins;
        }
    assert(bodies==1 && skins==(described.dlistCount==2) && matrices.empty());
    if(described.dlistCount==2) {
      ownerAlt.erase(raw+"/gi_xlu_dl"); reset();
      assert(!MM_TryDrawNeiGi(id) && gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
    }
  }
  // Missing authored resources still use the shared shimmer around the
  // existing model, without inheriting a legacy drawer's scale/translation.
  for (auto [id,slug] : candidates) {
    const bool sword = id == RI_SWORD_KOKIRI || id == RI_SWORD_RAZOR ||
      id == RI_SWORD_GILDED || id == RI_GREAT_FAIRY_SWORD ||
      id == RI_OOT_MASTER_SWORD || id == RI_OOT_TRUE_MASTER_SWORD ||
      id == RI_OOT_BIGGORON_SWORD || id == RI_OOT_IRON_KNUCKLE_AXE;
    for (bool enabled : {false,true}) {
      reset(); itemEffects = enabled;
      const auto before = current;
      {
        MM_NeiGiFallbackShimmer shimmer(id);
        Matrix_Scale(7,7,7,MTXMODE_APPLY);
        Matrix_Translate(1,2,3,MTXMODE_APPLY);
      }
      if (enabled || sword) {
        assert(gfx.polyXlu.p > xlu);
        assert(!memcmp(&current,&before,sizeof(current)));
      } else assert(gfx.polyXlu.p == xlu);
      assert(matrices.empty());
    }
  }
  reset(); itemEffects = true;
  { MM_NeiGiFallbackShimmer unrelated(RI_BOW); }
  assert(gfx.polyXlu.p == xlu && matrices.empty());
  for (int season = RI_OOT_NEI_SEASON_SPRING; season <= RI_OOT_NEI_SEASON_WINTER; ++season) {
    reset();
    { MM_NeiGiFallbackShimmer weather(static_cast<RandoItemId>(season)); }
    assert(gfx.polyXlu.p == xlu && matrices.empty());
  }
  itemEffects = false;
  std::cout << "PASS native MM fallback equipment/item shimmer, toggle, sword invariance, pose isolation and weather exclusion\n";
  ownerBase.clear(); ownerAlt.clear(); ownerAltEnabled=false;
  std::cout << "PASS native MM bindings: all 38 new meshes, explicit OoT ownership, divergent Alt, atomic missing-resource fallback\n";
  const char *body = NEI_HELD_PATH("fire_rod");
  const char *glass = NEI_HELD_PATH("fake_glass");
  reset();
  assert(!NeiHeld_DrawModel(&play, body, glass));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerBase.insert(body);
  assert(!NeiHeld_DrawModel(&play, body, glass));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerAlt.insert(glass);
  mmAltEnabled = true;
  assert(!NeiHeld_DrawModel(&play, body, glass));
  ownerAltEnabled = true;
  mmAltEnabled = false;
  assert(NeiHeld_DrawModel(&play, body, glass));
  assert((opa[1].words.w0 >> 24) == G_DL_OTR_FILEPATH);
  assert(std::string((const char *)opa[1].words.w1) ==
         "__OTR__@oot:objects/nei_held_redesign/fire_rod/gi_dl");
  const char *retained = (const char *)opa[1].words.w1;
  for (int i = 0; i < 10000; i++) {
    std::string temp = "__OTR__objects/transient/" + std::to_string(i);
    NeiResource_Route(temp.c_str());
  }
  assert(std::string(retained) ==
         "__OTR__@oot:objects/nei_held_redesign/fire_rod/gi_dl");
  ownerAltEnabled = false;
  reset();
  assert(!NeiHeld_DrawModel(&play, body, glass));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerBase.erase(body);
  assert(!NeiHeld_DrawRod(&play, 0));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  // Native texture commands retain raw paths inside a balanced owner bracket.
  const char *texture = "__OTR__objects/nei_rod_attack/fire_surge";
  NeiGi::TextureMaterial mat{texture, false, false};
  const auto mesh =
      NeiGi::SampleOrb(NeiGi::Kind::Fire, {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}});
  reset();
  assert(!NeiGi_DrawTexturedMesh(&play, mesh, mat));
  assert(gfx.polyXlu.p == xlu);
  ownerBase.insert(texture);
  assert(NeiGi_DrawTexturedMesh(&play, mesh, mat));
  std::string active = "mm";
  int textures = 0, depth = 0;
  for (Gfx *c = xlu; c != gfx.polyXlu.p; c++) {
    unsigned op = (c->words.w0 >> 24) & 255;
    if (op == G_COMBO_RM_PUSH) {
      assert(active == "mm");
      active = (const char *)c->words.w1;
      ++depth;
    }
    if (op == G_SETTIMG || op == G_SETTIMG_OTR_FILEPATH) {
      assert(active == "oot");
      assert(std::string((const char *)c->words.w1) == texture);
      ++textures;
    }
    if (op == G_COMBO_RM_POP) {
      assert(depth == 1);
      active = "mm";
      --depth;
    }
  }
  assert(textures == 1 && depth == 0 && active == "mm");

  // Real native paired Mitts and charge-colored Jar: no partial replacements.
  for (auto pair :
       {std::pair{NEI_HELD_PATH("mitt_left"), NEI_HELD_PATH("mitt_right")},
        std::pair{NEI_HELD_PATH("gust_jar"), NEI_HELD_PATH("gust_jar_band")}}) {
    reset();
    ownerBase.insert(pair.first);
    bool mitts = std::string(pair.first).find("mitt") != std::string::npos;
    assert(!(mitts ? NeiHeld_DrawMitts(&p, &play)
                   : NeiHeld_DrawGustJar(&p, &play, 0, .5f)));
    assert(gfx.polyOpa.p == opa);
    ownerBase.insert(pair.second);
    assert(mitts ? NeiHeld_DrawMitts(&p, &play)
                 : NeiHeld_DrawGustJar(&p, &play, 0, .5f));
    int paths = 0;
    for (Gfx *c = opa; c < gfx.polyOpa.p; c++)
      if ((c->words.w0 >> 24) == G_DL_OTR_FILEPATH)
        paths++;
    assert(paths == 2);
  }
  const char *lantern = "__OTR__objects/nei_gi_redesign/lantern/gi_dl";
  const char *lanternGlass = "__OTR__objects/nei_gi_redesign/lantern/gi_xlu_dl";
  reset();
  ownerBase.insert(lantern);
  assert(!NeiLantern_DrawHeld(&p, &play, 0));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerBase.insert(lanternGlass);
  for (int fire = 0; fire <= 4; fire++) {
    reset();
    assert(NeiLantern_DrawHeld(&p, &play, fire));
    assert(std::string((char *)opa[1].words.w1) ==
           "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_dl");
    int verts = 0, glassDraws = 0;
    for (Gfx *c = xlu; c < gfx.polyXlu.p; c++) {
      if ((c->words.w0 >> 24) == G_VTX)
        verts++;
      if ((c->words.w0 >> 24) == G_DL_OTR_FILEPATH) {
        assert(std::string((char *)c->words.w1) ==
               "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_xlu_dl");
        glassDraws++;
      }
    }
    assert((fire == 0 ? verts == 0 : verts > 0) && glassDraws == 1 &&
           matrices.empty());
  }
  p.transformation = PLAYER_FORM_HUMAN;
  p.heldItemAction = PLAYER_IA_NONE;
  gCustomItemState.lanternEquipped = 1;
  assert(!NeiLantern_UsesGrip(&p));
  for (int slot = 0; slot < 8; slot++) {
    slots[slot] = ITEM_LANTERN;
    assert(NeiLantern_UsesGrip(&p));
    slots[slot] = ITEM_NONE;
  }
  slots[7] = ITEM_LANTERN;
  p.heldItemAction = PLAYER_IA_BOW;
  assert(!NeiLantern_UsesGrip(&p));
  p.heldItemAction = PLAYER_IA_NONE;
  p.transformation = PLAYER_FORM_DEKU;
  assert(!NeiLantern_UsesGrip(&p));
  ownerRegistered = false;
  reset();
  assert(!NeiGi_DrawTexturedMesh(&play, mesh, mat));
  assert(gfx.polyXlu.p == xlu);
  std::cout << "PASS native MM DL ownership, retained deferred paths, atomic "
               "and late fallback, owner Alt, raw texture owner brackets\n";
}
