// The runner inserts unmodified production draw, native GBI wrappers and arena
// functions. Only resource availability, matrix state and GPU consumption are
// doubled; no production command-writing macro is replaced.
#include "global.h"
#include "ComboGameMode.h"
#include "entrances/LakeHyliaBridge.h"
#include "entrances/LostWoodsEntrance.h"
#include "render/RegionalBridgeMaterials.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>

SaveContext gSaveContext{};
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static bool replacements;
static float lakeWaterY;
static size_t openChildren;
static std::vector<Gfx> opa(8192), xlu(2048), overlay(64);
static int matrixDepth;
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) { ++openChildren; }
extern "C" void FrameInterpolation_RecordCloseChild() { assert(openChildren); --openChildren; }
extern "C" u8 ResourceMgr_FileExists(const char* path) {
    assert(std::strncmp(path, "textures/comboship/regional/", 27) == 0);
    return replacements;
}
extern "C" int ResourceMgr_OTRSigCheck(char* path) {
    return path && std::memcmp(path, "__OTR__", 7) == 0;
}
extern "C" Gfx* PakLoader_GetDLOverride(const char*) { assert(false); return nullptr; }
extern "C" Gfx* ResourceMgr_LoadGfxByName(char*) { assert(false); return nullptr; }
extern "C" Vtx* ResourceMgr_LoadVtxByName(char*) { assert(false); return nullptr; }
extern "C" void Matrix_Push() { ++matrixDepth; }
extern "C" void Matrix_Pop() { assert(matrixDepth); --matrixDepth; }
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) {
    assert(x == 0 && y == 0 && z == 0 && mode == MTXMODE_NEW);
}
extern "C" Mtx* Matrix_NewMtx(GraphicsContext* gfx, char*, s32) {
    auto* matrix = static_cast<Mtx*>(Graph_Alloc(gfx, sizeof(Mtx)));
    std::memset(matrix, 0, sizeof(*matrix));
    return matrix;
}
extern "C" s32 WaterBox_GetSurface1(PlayState*, CollisionContext*, f32 x, f32 z, f32* y, WaterBox**) {
    assert(x == -1900 && z == 3757);
    *y = lakeWaterY;
    return true;
}
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#undef HREG
#define HREG(index) 0
/* NATIVE_GRAPH */
/* NATIVE_GBI */
/* NATIVE_ARENA */
namespace {
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int);
extern "C" void FrameInterpolation_RecordCloseChild(void);
namespace Lake = ComboLakeHylia;
struct CaveActor { DynaPolyActor dyna; };
static CaveActor cave;
static CaveActor* sCave = &cave;
/* PRODUCTION_DRAW */
}

using F3DGfx = Gfx;
using F3DVtx = Vtx;
struct Interpreter {
    uintptr_t mSegmentPointers[16]{};
    size_t vertices = 0;
    const GraphicsContext* gfx = nullptr;
    void* SegAddr(uintptr_t);
    void GfxSpVertex(uint32_t count, uint32_t offset, const F3DVtx* pointer) {
        assert(count == 4 && offset == 0);
        const auto address = reinterpret_cast<uintptr_t>(pointer);
        const auto& rock = RockVertices();
        const auto rockBegin = reinterpret_cast<uintptr_t>(rock.data());
        const auto rockEnd = rockBegin + rock.size() * sizeof(Vtx);
        const auto tailBegin = reinterpret_cast<uintptr_t>(gfx->polyOpa.d);
        const auto tailEnd = reinterpret_cast<uintptr_t>(opa.data() + opa.size());
        assert((address >= rockBegin && address + count * sizeof(Vtx) <= rockEnd) ||
               (address >= tailBegin && address + count * sizeof(Vtx) <= tailEnd));
        for (size_t i = 0; i < count; ++i)
            assert(pointer[i].v.cn[3] == 255);
        vertices += count;
    }
};
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << (width)) - 1))
/* NATIVE_INTERPRETER */

struct Counts { size_t vertices = 0, triangles = 0, textures = 0, pushes = 0, pops = 0; };
static Counts Walk(Interpreter& interpreter, Gfx* begin, Gfx* end, int depth = 0) {
    assert(depth < 2);
    Counts counts;
    for (auto* command = begin; command != end; ++command) {
        const auto opcode = command->words.w0 >> 24;
        switch (opcode) {
            case G_DL: {
                // The cave's only raw sublist is the engine's actual setup.
                auto* target = static_cast<Gfx*>(interpreter.SegAddr(command->words.w1));
                assert(target == sSetupDL[SETUPDL_25]);
                Walk(interpreter, target, target + 6, depth + 1);
                break;
            }
            case G_ENDDL:
                assert(depth == 1 && command + 1 == end);
                break;
            case G_VTX:
                gfx_vtx_handler_f3dex2(&interpreter, &command);
                counts.vertices += 4;
                break;
            case G_TRI2:
                assert(command->words.w0 == (uintptr_t(G_TRI2) << 24 | 0x000204));
                assert(command->words.w1 == 0x000406);
                counts.triangles += 2;
                break;
            case G_SETTIMG: {
                const auto* path = static_cast<const char*>(interpreter.SegAddr(command->words.w1));
                assert(ResourceMgr_OTRSigCheck(const_cast<char*>(path)));
                assert(std::strcmp(path, RegionalBridgeMaterials::Resolve(
                    counts.vertices ? RegionalBridgeMaterials::Material::LakeWater :
                                      RegionalBridgeMaterials::Material::LakeStone,
                    ResourceMgr_FileExists)) == 0 ||
                       std::strcmp(path, RegionalBridgeMaterials::Resolve(
                           RegionalBridgeMaterials::Material::LakeWater, ResourceMgr_FileExists)) == 0);
                ++counts.textures;
                break;
            }
            case G_COMBO_RM_PUSH:
                assert(std::strcmp(reinterpret_cast<const char*>(command->words.w1), "oot") == 0);
                ++counts.pushes;
                break;
            case G_COMBO_RM_POP:
                ++counts.pops;
                break;
            case G_MTX:
                assert(command->words.w1 >= reinterpret_cast<uintptr_t>(interpreter.gfx->polyOpa.d));
                assert(command->words.w1 + sizeof(Mtx) <= reinterpret_cast<uintptr_t>(opa.data() + opa.size()));
                break;
            case G_NOOP: case G_GEOMETRYMODE: case G_TEXTURE: case G_SETCOMBINE:
            case G_SETTILE: case G_LOADBLOCK: case G_RDPPIPESYNC: case G_RDPLOADSYNC:
            case G_SETTILESIZE: case G_SETPRIMCOLOR: case G_RDPSETOTHERMODE:
                break;
            default:
                std::fprintf(stderr, "invalid cave opcode %#zx at command %td\n", size_t(opcode), command - begin);
                assert(false);
        }
    }
    return counts;
}

int main() {
    GraphicsContext gfx{};
    PlayState play{};
    CollisionHeader collision{};
    play.state.gfxCtx = &gfx;
    play.sceneNum = SCENE_LAKE_HYLIA;
    play.colCtx.colHeader = &collision;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    for (bool alt : {false, true})
        for (float level : {Lake::kFullWaterY, Lake::kDrainedWaterY, Lake::kFullWaterY})
            for (u32 frame : {0u, 1u, 2047u, 2048u, 0xffffffffu}) {
                replacements = alt;
                lakeWaterY = level;
                play.gameplayFrames = frame;
                auto initialize = [](TwoHeadGfxArena& arena, std::vector<Gfx>& commands) {
                    std::memset(commands.data(), 0xA9, commands.size() * sizeof(Gfx));
                    arena.p = commands.data();
                    arena.d = commands.data() + commands.size();
                };
                initialize(gfx.polyOpa, opa);
                initialize(gfx.polyXlu, xlu);
                initialize(gfx.overlay, overlay);
                CaveDraw(&cave.dyna.actor, &play);
                assert(gfx.polyOpa.p > opa.data() && gfx.polyOpa.p <= gfx.polyOpa.d);
                assert(gfx.polyXlu.p <= gfx.polyXlu.d && gfx.overlay.p <= gfx.overlay.d);
                assert(openChildren == 0 && matrixDepth == 0);
                Interpreter interpreter;
                interpreter.gfx = &gfx;
                const auto rock = Walk(interpreter, opa.data(), gfx.polyOpa.p);
                const auto water = Walk(interpreter, xlu.data(), gfx.polyXlu.p);
                assert(rock.vertices == Geometry().solid.size() * 4);
                assert(rock.triangles == Geometry().solid.size() * 2);
                assert(rock.textures == 1 && rock.pushes == 1 && rock.pops == 1);
                const bool inlet = Lake::HasInlet(level);
                assert(water.vertices == (inlet ? Geometry().water.size() * 4 : 0));
                assert(water.triangles == (inlet ? Geometry().water.size() * 2 : 0));
                assert(water.textures == size_t(inlet) && water.pushes == size_t(inlet) && water.pops == size_t(inlet));
            }
    puts("PASS actual CaveDraw/GBI/native vertex dispatch: setup lists, command validity, static/frame vertex lifetime, native/replacement full/drained/refilled draws");
}
