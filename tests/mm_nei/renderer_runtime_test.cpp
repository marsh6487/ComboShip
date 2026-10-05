#define MM_REAL_RENDERER
#include "../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "2s2h/Rando/NeiGiPresentation.h"
#include "2s2h/Rando/NeiLanternPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"
#include "2s2h/Rando/NeiUsedMagicPresentation.h"
#include "ComboMaskShimmer.h"
#include "ComboSongDrawMM.h"
#include "objects/object_gi_melody/object_gi_melody.h"
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
  *out = {};
  const auto *bounds = NeiGi::FindFrameBounds(opa);
  assert(bounds);
  out->neiEffect = static_cast<int>(bounds->effect);
  out->neiShimmer = out->neiEffect + 1;
  if(!OOT_NeiResourceExists(opa) || (skin && !OOT_NeiResourceExists(skin))) return 0;
  out->drawKind = CW_DRAW_KIND_NEI_GI;
  out->dlistCount = split ? 2 : 1;
  out->dlists[0]=opa; out->dlists[1]=skin; out->xluStartIndex=split ? 1 : -1;
  out->scale=1; out->itemShimmer=1;
  return 1;
}
static bool itemEffects;
extern "C" int32_t CVarGetInteger(const char* name,int32_t value) {
  return !strcmp(name,"gEnhancements.SkijerNEI.ItemEffects") ? itemEffects : value;
}
// Selected third-party resource graphs and Din layer eligibility have dedicated
// production fixtures. This native NEI fixture supplies neither resource family.
extern "C" int ResourceMgr_GetGiModelFitForGame(const char*,const char*,float,float,int,float[2]) {return 0;}
extern "C" int ResourceMgr_GetDinSwordGiProfileForGame(const char*,const char*) {return 0;}
extern "C" Color_RGBA8 CosmeticEditor_GetChangedColor(u8,u8,u8,u8,const char*) {
  assert(false && "native NEI fixture selected an absent Din layer"); return {};
}
void DrawOotSlateRuneFlame(u8,u8,u8) {assert(false && "concrete non-Somaria GI must not borrow a flame");}
#include "mm_song_draw.inc"
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
  alignas(16) static Gfx opa[0x6700], xlu[0x1000], overlay[0x800];
  play.state.gfxCtx = &gfx;
  gPlayState=&play;
  play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
  auto reset = [&]() {
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = std::end(opa);
    gfx.polyXlu.p = xlu;
    gfx.polyXlu.d = std::end(xlu);
    gfx.overlay.p = overlay;
    gfx.overlay.d = std::end(overlay);
    nativeDisplayLists.clear();
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
  };
  const auto packedEffects = [&]() {
    std::vector<Vtx> vertices;
    for(Gfx* cmd=xlu;cmd<gfx.polyXlu.p;++cmd)if(cmd->words.w0>>24==G_VTX) {
      const size_t count=(cmd->words.w0>>12)&255;
      const auto* batch=reinterpret_cast<const Vtx*>(cmd->words.w1);
      vertices.insert(vertices.end(),batch,batch+count);
    }
    return vertices;
  };
  struct Song {RandoItemId item;int profile;uint32_t hue;};
  const Song songs[]={
    {RI_OOT_SONG_MINUET_OF_FOREST,CW_SONG_OOT_MINUET,0x62FF62},
    {RI_OOT_SONG_BOLERO_OF_FIRE,CW_SONG_OOT_BOLERO,0xFF3C00},
    {RI_OOT_SONG_SERENADE_OF_WATER,CW_SONG_OOT_SERENADE,0x55B4DF},
    {RI_OOT_SONG_REQUIEM_OF_SPIRIT,CW_SONG_OOT_REQUIEM,0xDE9E2F},
    {RI_OOT_SONG_NOCTURNE_OF_SHADOW,CW_SONG_OOT_NOCTURNE,0xA028D2},
    {RI_OOT_SONG_PRELUDE_OF_LIGHT,CW_SONG_OOT_PRELUDE,0xEDE73E},
    {RI_SONG_EPONA,CW_SONG_EPONA,0xD96E30},{RI_SONG_SUN,CW_SONG_SUN,0xEDE73E}};
  for(const auto& song:songs)for(bool effects:{false,true})for(bool alt:{false,true})for(bool donor:{false,true}) {
    reset();play.gameplayFrames=42;itemEffects=effects;mmAltEnabled=alt;ownerRegistered=donor;
    const uint8_t color[]={uint8_t(song.hue>>16),uint8_t(song.hue>>8),uint8_t(song.hue),255};
    NeiGi_DrawShimmerOverlay(&play,color,nullptr);
    const auto expected=packedEffects();assert(!expected.empty());
    reset();const int descriptionsBefore=descriptorCalls;
    assert(MM_TryDrawNeiGi(song.item) && descriptorCalls==descriptionsBefore);
    const auto shown=packedEffects();
    assert(shown.size()>=expected.size() && !memcmp(shown.data(),expected.data(),expected.size()*sizeof(Vtx)) &&
           "native MM songs must submit the matching shared shimmer even without optional effects or donor/mod resources");
    if(song.profile==CW_SONG_EPONA || song.profile==CW_SONG_SUN)assert(shown.size()==expected.size());
    if(song.profile==CW_SONG_OOT_BOLERO) {
      bool hotCore=false, warmBody=false;
      for(const auto& vertex:shown) {
        const auto* rgba=vertex.v.cn;
        if(!rgba[3])continue;
        hotCore|=rgba[0]==255 && rgba[1]==242 && rgba[2]==173;
        warmBody|=rgba[0]==255 && rgba[1]==175 && rgba[2]==54;
      }
      assert(hotCore && warmBody && "native MM Bolero must submit the visible flame body and hot core in addition to red shimmer");
    }
    assert(nativeDisplayLists.size()==1 && !strcmp(reinterpret_cast<const char*>(nativeDisplayLists.front()),
                                                  "__OTR__objects/object_gi_melody/gGiSongNoteDL") && matrices.empty());
  }
  reset();
  assert(MM_TryDrawNeiGi(RI_SONG_STORMS));
  const auto rainOnly=packedEffects();
  reset();NeiGi_DrawMesh(&play,NeiGi::SampleSeason(42,1,NeiGi_CameraBasis(&play)));
  const auto expectedRain=packedEffects();
  assert(rainOnly.size()==expectedRain.size() && !memcmp(rainOnly.data(),expectedRain.data(),expectedRain.size()*sizeof(Vtx)));
  for(RandoItemId song:{RI_SONG_HEALING,RI_SONG_SOARING,RI_SONG_TIME,RI_SONG_SONATA,RI_SONG_NOVA,
                       RI_SONG_LULLABY,RI_SONG_LULLABY_INTRO,RI_SONG_ELEGY,RI_SONG_OATH,RI_SONG_DOUBLE_TIME,RI_SONG_INVERTED_TIME}) {
    reset();assert(MM_TryDrawNeiGi(song));
    assert(packedEffects().empty() && matrices.empty() && "regular MM songs must keep only their original note");
  }
  ownerRegistered=true;mmAltEnabled=false;itemEffects=false;
  std::cout << "PASS real native MM song submission: mandatory colored shimmer, original note, rain-only Storms, plain regular songs and donor/Alt independence\n";
  const auto firstOpaquePath = [&]() {
    for (Gfx *cmd = opa; cmd < gfx.polyOpa.p; ++cmd)
      if (cmd->words.w0 >> 24 == G_DL_OTR_FILEPATH)
        return reinterpret_cast<const char *>(cmd->words.w1);
    assert(false && "missing deferred opaque path");
    return static_cast<const char *>(nullptr);
  };
  const NeiGi::TextureMaterial guardedMaterial{
      "__OTR__objects/nei_used_magic/ice_fracture", true, true};
  ownerBase.insert(guardedMaterial.path);
  for (int draw = 0; draw < 3; ++draw)
    for (int shortArena = 0; shortArena < 4; ++shortArena) {
      reset();
      if (shortArena == 0)
        gfx.polyOpa.d = opa + 1;
      if (shortArena == 1)
        gfx.polyXlu.d = xlu + 1;
      if (shortArena == 2)
        gfx.overlay.d = overlay + 1;
      if (shortArena == 3)
        gfx.polyOpa.p = opa + 2, gfx.polyOpa.d = opa + 1;
      const auto opaHead = gfx.polyOpa.p, xluHead = gfx.polyXlu.p;
      const auto opaTail = gfx.polyOpa.d, xluTail = gfx.polyXlu.d;
      const auto mesh = NeiGi::SampleSong(CW_SONG_OOT_ZELDA, 42);
      if (draw == 0)
        NeiGi_DrawMesh(&play, mesh);
      if (draw == 1)
        assert(!NeiGi_DrawTexturedMesh(&play, mesh, guardedMaterial));
      if (draw == 2)
        NeiGi_DrawSongOverlay(&play, CW_SONG_OOT_ZELDA, "oot");
      assert(gfx.polyOpa.p == opaHead && gfx.polyOpa.d == opaTail &&
             gfx.polyXlu.p == xluHead && gfx.polyXlu.d == xluTail &&
             gfx.overlay.p == overlay && matrices.empty());
    }
  reset();
  assert(NeiGi_DrawTexturedMesh(&play, NeiGi::SampleSong(CW_SONG_OOT_ZELDA, 42),
                                guardedMaterial));
  int ownerDepth = 0;
  bool texturedVertices = false;
  for (Gfx *cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
    const unsigned op = cmd->words.w0 >> 24;
    if (op == G_COMBO_RM_PUSH)
      ++ownerDepth;
    if (op == G_VTX)
      assert(ownerDepth == 1), texturedVertices = true;
    if (op == G_COMBO_RM_POP)
      --ownerDepth;
    assert(ownerDepth >= 0);
  }
  assert(texturedVertices && ownerDepth == 0 &&
         gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d &&
         matrices.empty());
  std::cout << "PASS actual MM GRAPH_ALLOC/matrix/debug/setup arena guards, "
               "native textured owner balance and no commands on decline\n";
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
    const bool sword =
        id == RI_SWORD_KOKIRI || id == RI_SWORD_RAZOR ||
        id == RI_SWORD_GILDED || id == RI_GREAT_FAIRY_SWORD ||
        id == RI_OOT_MASTER_SWORD || id == RI_OOT_TRUE_MASTER_SWORD ||
        id == RI_OOT_BIGGORON_SWORD || id == RI_OOT_IRON_KNUCKLE_AXE ||
        id == RI_OOT_EXT_FOUR_SWORD;
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
  assert(std::string(firstOpaquePath()) ==
         "__OTR__@oot:objects/nei_held_redesign/fire_rod/gi_dl");
  const char *retained = firstOpaquePath();
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
    assert(std::string(firstOpaquePath()) ==
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
