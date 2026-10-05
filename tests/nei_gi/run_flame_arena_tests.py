from pathlib import Path
import os,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
import tempfile
_tmp=tempfile.TemporaryDirectory(prefix='nei-flame-arena-');OUT=Path(_tmp.name)
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
flags=['-std=c++20','-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0']+['-I'+str(ROOT/p) for p in ('soh','soh/include','soh/src','soh/assets','soh/mods','libultraship/include','combo/menu')]
for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
 for k,v in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text()): flags.append(f'-D{k}="{v}"')
graph=functions((ROOT/'soh/src/code/graph.c').read_text())
(OUT/'nei_gi_graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
draw=functions((ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text())
rcp=functions((ROOT/'soh/src/code/z_rcp.c').read_text())
(OUT/'nei_gi_dispatch.inc').write_text(draw['Randomizer_DrawCaneSomariaUpgradeFlame']+'\n'+draw['Randomizer_DrawTrueMasterSwordFlame'])
f=(ROOT/'tests/nei_gi/presentation_test.cpp').read_text()
f='#define NEI_GI_FIXTURE_BOUNDARY_ONLY\n#include "ComboItemDrawABI.h"\n#include "objects/object_gi_fire/object_gi_fire.h"\n'+f
f=f.replace('static void DrawWeaponFlameOverlay(PlayState *, u8 r, u8 g, u8 b) {\n  Fixture::flameColors.push_back({r, g, b});\n}', 'void gSPSegment(void *p, int n, uintptr_t a) { static_cast<Gfx*>(p)->words.w1=a; }\n'+rcp['Gfx_TwoTexScrollEx'].replace('= Graph_Alloc(', '= (Gfx*)Graph_Alloc(')+'\n'+functions((ROOT/'soh/soh/GbiWrap.cpp').read_text().replace('extern "C" ',''))['gDPSetTileSizeLerp']+'\n'+draw['DrawWeaponFlameOverlay'])
f=f.replace('void gSPDisplayList(Gfx *, Gfx *) {\n  assert(false &&\n         "GI paths must be deferred, not resolved through the legacy wrapper");\n}', 'void gSPDisplayList(Gfx *p, Gfx *d) { __gSPDisplayList(p,d); }')
f=f.replace('  assert(tail >= reinterpret_cast<uintptr_t>(context->polyOpa.p)', '  if(tail < reinterpret_cast<uintptr_t>(context->polyOpa.p)) std::cerr << "allocation bytes="<<size<<" head="<<(context->polyOpa.p-Fixture::opa)<<" proposedTail="<<((reinterpret_cast<Gfx*>(tail))-Fixture::opa)<<"\\n";\n  assert(tail >= reinterpret_cast<uintptr_t>(context->polyOpa.p)')
mm=functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())
mmgi=functions((ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text())
f += '\nextern "C" { PlayState* gPlayState = &Fixture::play; }\n'
f += 'void* OotAssets_LoadGfxDirect(const char*) { static Gfx flame[1]; return flame; }\n'
f += '#define Gfx_SetupDL25_Xlu Gfx_SetupDL_25Xlu\n'
f += '#define MATRIX_FINALIZE_AND_LOAD(p, g) gSPMatrix(p, Matrix_NewMtx(g, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH)\n'
f += mm['DrawOotSlateRuneFlame'] + '\n' + mmgi['MM_DrawNeiGi']
f += r'''
int main() {
  using namespace Fixture;
  for (bool sword : {false, true}) {
    auto prepare = [&] {
      Reset();
      if (sword) {
        alt = 1;
        files.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
      } else {
        files.insert("__OTR__objects/nei_gi_redesign/cane_of_somaria/gi_dl");
      }
    };
    GetItemEntry e{};
    e.drawFunc = sword ? Randomizer_DrawTrueMasterSword : Randomizer_DrawCaneSomariaUpgrade;
    for (int capacity = 0; capacity < 40; ++capacity) {
      prepare();
      gfx.polyOpa.d = opa + capacity;
      assert(NeiGi_Draw(&play, &e));
      assert(gfx.polyOpa.p == opa && gfx.polyOpa.d == opa + capacity);
      assert(gfx.polyXlu.p == xlu && gfx.overlay.p == overlay);
      assert(allocations == 0 && arena.empty() && stack.empty() && Drawn().empty());
    }
    prepare();
    gfx.polyXlu.d = xlu + 31;
    assert(NeiGi_Draw(&play, &e));
    assert(allocations == 0 && gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
    prepare();
    assert(NeiGi_Draw(&play, &e));
    assert(allocations >= 2 && !Drawn().empty() && fallback == 0);
    assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d);
    assert(stack.empty() && matrix == 1);
  }
  CwItemDrawInfo info{};
  info.drawKind = CW_DRAW_KIND_NEI_GI;
  info.dlists[0] = "__OTR__objects/nei_gi_redesign/cane_of_somaria/gi_dl";
  info.dlistCount = 1;
  info.xluStartIndex = -1;
  GetItemEntry e{}; e.drawFunc = Randomizer_DrawCaneSomariaUpgrade;
  info.scale = FindPresentation(&e)->scale;
  info.neiEffect = static_cast<int>(FindPresentation(&e)->effect);
  info.neiSomariaUpgrade = true;
  for (bool shop : {false, true}) {
    for (int capacity = 0; capacity < 48; ++capacity) {
      Reset(); files.insert(info.dlists[0]); gfx.polyOpa.d = opa + capacity;
      MM_DrawNeiGi(info, shop);
      assert(gfx.polyOpa.p == opa && gfx.polyOpa.d == opa + capacity);
      assert(allocations == 0 && gfx.polyXlu.p == xlu && stack.empty());
    }
    Reset(); files.insert(info.dlists[0]);
    MM_DrawNeiGi(info, shop);
    assert(allocations >= 2 && Drawn() == std::vector<std::string>{info.dlists[0]});
    assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d);
    assert(stack.empty() && matrix == 1);
  }
}
'''
(OUT/'flame.cpp').write_text(f)
subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(OUT),'-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections',str(OUT/'flame.cpp'),'-Wl,--gc-sections','-o',str(OUT/'flame')],check=True)
# This sandbox cannot perform LeakSanitizer's process inspection. Keep ASan
# memory bounds and UBSan enabled, as in the other native graphics fixtures.
environment = {**os.environ, 'ASAN_OPTIONS': os.environ.get('ASAN_OPTIONS', '') + ':detect_leaks=0'}
subprocess.run([str(OUT/'flame')],check=True,env=environment)
print('PASS production OoT/MM Somaria and selected sword flames: exhausted arenas skip untouched; full arenas submit scroll, flame and model')
