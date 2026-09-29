#define MM_REAL_RENDERER
#include "../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "2s2h/Rando/NeiLanternPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"
#include "2s2h/Rando/NeiUsedMagicPresentation.h"
#include "rod_runtime_test.cpp"
#include <set>
std::set<std::string> ownerBase, ownerAlt;
bool ownerAltEnabled = false, mmAltEnabled = false, ownerRegistered = true;
extern "C" __attribute__((visibility("default"))) int32_t
OOT_NeiResourceExists(const char *path) {
  return ownerRegistered && (ownerBase.contains(path) ||
                             (ownerAltEnabled && ownerAlt.contains(path)));
}
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
  play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
  auto reset = [&]() {
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + 100000;
    gfx.polyXlu.p = xlu;
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
  };
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
