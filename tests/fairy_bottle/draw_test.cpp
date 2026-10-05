#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "combo/menu/ComboFairyBottle.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboSongDraw.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using u8 = uint8_t;
using f32 = float;
struct Gfx { int stream = 0; };
struct GraphicsContext {};
struct MtxF {
    float x = 0, y = 0, z = 0, sx = 1, sy = 1, sz = 1;
    bool billboard = false;
};
using Mtx = MtxF;
struct PlayState {
    struct { GraphicsContext* gfxCtx; uint32_t frames = 0; } state;
    uint32_t gameplayFrames = 0;
    MtxF billboardMtxF;
};
GraphicsContext gfx;
PlayState play{{&gfx}, 0, {}};
PlayState* gPlayState = &play;
char opaque[] = "opaque", glass[] = "glass", fairy[] = "fairy", matrixPath[] = "matrix";
char genericOpaque[] = "genericOpaque", genericGlass[] = "genericGlass";
char blueFire[] = "__OTR__objects/object_gi_fire/gGiBlueFireChamberstickDL";
const char* gGiFairyBottleTexAnim = "texAnim";
std::unordered_set<std::string> selectedMods;
int ResourceMgr_IsModAsset(const char* path) { return selectedMods.count(path) != 0; }
int ResourceMgr_IsModAssetForGame(const char* game, const char* path) {
    assert(!strcmp(game,"oot") && !strcmp(path,blueFire+7));
    return selectedMods.count(blueFire) != 0;
}

constexpr int MTXMODE_APPLY = 1, G_MTX_MODELVIEW = 2, G_MTX_LOAD = 4, G_TX_RENDERTILE = 0;
constexpr int GID_FAIRY = 0, GID_SONG_GENERIC = 0x75, kMaxMatEntries = 8;
MtxF current, initial, loaded[2];
MtxF nativePlacement{0, 3, 0};
std::vector<MtxF> stack;
struct Draw { int stream; std::string path; MtxF pose; };
std::vector<Draw> draws;
std::vector<std::string> events;
Gfx opa[64], xlu[64];
Gfx* opaPtr;
Gfx* xluPtr;
int foreignRestores = 0, hostFallbacks = 0, ownerScope = 1;
bool ownerPresent = true, matrixPresent = true, matrixPointerPresent = true, materialPresent = true;
bool fairyResources = false;
int animationFrame = -1, skeletonDraws = 0;
int shimmerDraws = 0;
uint8_t fairyPrim[4]{}, fairyEnv[4]{};

void Matrix_Push() { stack.push_back(current); }
void Matrix_Pop() { assert(!stack.empty()); current = stack.back(); stack.pop_back(); }
void Matrix_Translate(float x, float y, float z, int) { current.x += x; current.y += y; current.z += z; }
void Matrix_Scale(float x, float y, float z, int) { current.sx *= x; current.sy *= y; current.sz *= z; }
void Matrix_ReplaceRotation(MtxF*) { current.billboard = true; }
void Matrix_Mult(MtxF* mtx, int) { current.x += mtx->x; current.y += mtx->y; current.z += mtx->z; }
void Matrix_MtxToMtxF(Mtx* input, MtxF* output) { *output = *input; }
void LoadMatrix(Gfx* command, MtxF* pose) { loaded[command->stream] = *pose; }
void DrawList(Gfx* command, const void* path) {
    assert(ownerScope == 1 && "foreign load scope leaked into host drawing");
    draws.push_back({command->stream, static_cast<const char*>(path), loaded[command->stream]});
    events.push_back("draw");
}
void Segment(Gfx* command, int segment, uintptr_t) { assert(command->stream == 1 && segment == 8); }
template <class... T> uintptr_t Gfx_TwoTexScrollEx(T...) { return 1; }
template <class T> T Lib_SegmentedToVirtual(T path) { return path; }
Mtx* ResourceMgr_LoadMtxByName(void* path) { assert(path == matrixPath); return &nativePlacement; }
void AnimatedMat_Draw(PlayState*, const char* path) { assert(path == gGiFairyBottleTexAnim); events.push_back("mat"); }
void GetItem_Draw(PlayState*, int id);
struct Color_RGB8 { uint8_t r,g,b; };
int CVarGetInteger(const char*,int) { return 0; }
Color_RGB8 CVarGetColor24(const char*,Color_RGB8 color) { return color; }
bool ComboOotMaskShimmerColor(int,uint8_t*) { return false; }
void ComboMaskShimmerColor(int,uint8_t*) { assert(false); }
bool DinFireShield_DrawItem(PlayState*,int) { return false; }
#define CVAR_COSMETIC(x) x
void ComboDrawMaskShimmer(PlayState*,const char*,const uint8_t color[4],const char*) {
    assert(color[0]==255 && color[1]==160 && color[2]==235 && color[3]==255);
    ++shimmerDraws;
}
void OOT_RestoreForeignSegs(PlayState*, int32_t* segs, int count) { assert(count == 1 && segs[0] == 8); ++foreignRestores; }
void MM_RestoreForeignSegs(int32_t* segs, int count) { OOT_RestoreForeignSegs(&play, segs, count); }
bool ComboForeignTexAnim_Run(PlayState*, const char* owner, const char* path, bool opa, int32_t* segs, int32_t* count) {
    assert(ownerScope == 1 && !strcmp(owner, "mm") && opa);
    if (!materialPresent || !path) return false;
    assert(!strcmp(path, "texAnim"));
    segs[0] = 8; *count = 1; events.push_back("mat"); return true;
}
void ComboForeignTexAnim_Restore(PlayState* play, int32_t* segs, int count, bool opa) {
    assert(opa); OOT_RestoreForeignSegs(play, segs, count); events.push_back("restore");
}
namespace Ship {
struct Resource {
    void* GetRawPointer() { return matrixPointerPresent ? &nativePlacement : nullptr; }
};
struct ResourceManager {
    std::shared_ptr<Resource> LoadResource(const char* path) {
        assert(ownerScope == 2 && !strcmp(path, "matrix"));
        return matrixPresent ? std::make_shared<Resource>() : nullptr;
    }
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const char* owner) {
        assert(!strcmp(owner, "mm"));
        return ownerPresent ? std::make_shared<ResourceManager>() : nullptr;
    }
};
struct ResourceManagerScope {
    ResourceManagerScope(std::shared_ptr<ResourceManager>) { assert(ownerScope == 1); ownerScope = 2; }
    ~ResourceManagerScope() { ownerScope = 1; }
};
}
struct ComboForeignDrawInfo {
    int count = 4;
    const char* dls[4] = {opaque, glass, fairy, "__OTR__@mm:matrix"};
    const char* matAnimPath = "texAnim";
    int drawKind = CW_DRAW_KIND_MM_FAIRY_CONTAINER, xluStart = 1, neiEffect = 0, neiShimmer = 0;
    float scale = 0, neiEffectCenter[3]{};
    bool animOk = false, itemShimmer = true;
    uint8_t itemShimmerColor[4] = {255,160,235,255}, primColorXlu[4]{};
    CwItemAnimDrawInfo anim{};
};
using ComboForeignDrawInfoOOT = ComboForeignDrawInfo;
using RandomizerCheck = int;
constexpr int RC_UNKNOWN_CHECK = -1;
struct GetItemEntry { int comboForeignCheck = 1, gid = GID_FAIRY; };
const ComboForeignDrawInfo* selectedForeign = nullptr;
RandomizerCheck OOT_GetQueuedDrawCheck() { return 1; }
const ComboForeignDrawInfo* ComboResolveForeignDrawInfo(int) { return selectedForeign; }
bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo*,const char*,PlayState*) { return false; }
NeiGi::Basis NeiGi_CameraBasis(PlayState*) { return {}; }
template<class... T> void NeiGi_DrawPresentation(T...) { assert(false); }
template<class... T> void NeiGi_DrawMesh(T...) { assert(false); }
template<class... T> void NeiGi_DrawSeasonOverlay(T...) { assert(false); }
template<class... T> void NeiGi_DrawSongOverlay(T...) { assert(false); }
template<class... T> void ComboDrawSpinAttackGi(T...) { assert(false); }
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define POLY_OPA_DISP opaPtr
#define POLY_XLU_DISP xluPtr
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) events.push_back("opa")
#define Gfx_SetupDL_25Xlu(...) events.push_back("xlu")
#define Gfx_SetupDL25_Opa(...) events.push_back("opa")
#define Gfx_SetupDL25_Xlu(...) events.push_back("xlu")
#define OOT_FOREIGN_PIN_OPA() ((void)0)
#define OOT_FOREIGN_PIN_XLU() ((void)0)
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() ((void)0)
#define MATRIX_NEWMTX(...) (&current)
#define gSPMatrix(p,m,flags) LoadMatrix(p,m)
#define MATRIX_FINALIZE_AND_LOAD(p,ctx) LoadMatrix(p,&current)
#define COMBO_FOREIGN_MTX(p) LoadMatrix(p,&current)
#define gSPDisplayList(p,dl) DrawList(p,dl)
#define gSPSegment(p,seg,dl) Segment(p,seg,(uintptr_t)(dl))

struct Actor {};
struct Vec3f { float x,y,z; };
struct Vec3s { s16 x,y,z; };
constexpr int SKELANIME_TYPE_FLEX = 1;
struct SkeletonHeader { int limbCount; int skeletonType = 0; };
struct FlexSkeletonHeader { SkeletonHeader sh; int dListCount; };
struct AnimationHeader { int lastFrame = 29; };
using AnimationHeaderCommon = AnimationHeader;
struct SkelAnime { void** skeleton; Vec3s* jointTable; };
#ifdef COMBO_FAIRY_HOST_MM
SkeletonHeader fairySkeleton{6};
#else
SkeletonHeader fairySkeleton{14};
#endif
AnimationHeader fairyAnimation;
extern "C" uint8_t ResourceMgr_FileExists(const char*) { return fairyResources; }
extern "C" uint8_t ResourceMgr_FileAltExists(const char*) { return 0; }
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return false; }
extern "C" SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* registered) {
    assert(!registered && strstr(path,"gameplay_keep"));
    return &fairySkeleton;
}
extern "C" AnimationHeader* ResourceMgr_LoadAnimByName(const char* path) { assert(strstr(path,"gameplay_keep")); return &fairyAnimation; }
s16 Animation_GetLastFrame(AnimationHeader* animation) { return animation->lastFrame; }
void SkelAnime_Init(PlayState*,SkelAnime* skel,SkeletonHeader* skeleton,AnimationHeader*,Vec3s* joints,Vec3s*,s32 count) {
    assert(skeleton->limbCount+1 == count); skel->jointTable = joints;
}
void Animation_Change(SkelAnime*,AnimationHeader*,float speed,float start,float,int,float) {
    assert(speed == 0); animationFrame = (int)start;
}
template <typename Override>
Gfx* SkelAnime_Draw(PlayState* play,void**,Vec3s*,Override limb,void*,void*,Gfx* gfx) {
    ++skeletonDraws;
    Matrix_Push();
#ifdef COMBO_FAIRY_HOST_MM
    limb(play,6,nullptr,nullptr,nullptr,nullptr,&gfx);
#else
    limb(play,8,nullptr,nullptr,nullptr,nullptr,&gfx);
#endif
    assert(current.billboard);
    Matrix_Pop();
    LoadMatrix(gfx,&current); DrawList(gfx,"fairy-vfx");
    return gfx+1;
}
template <typename Override>
Gfx* SkelAnime_DrawFlex(PlayState* play,void** skeleton,Vec3s* joints,int,Override limb,void*,void*,Gfx* gfx) {
    return SkelAnime_Draw(play,skeleton,joints,limb,nullptr,nullptr,gfx);
}
Gfx materialCommands[5];
void* Graph_Alloc(GraphicsContext*,size_t size) { assert(size == sizeof(materialCommands)); return materialCommands; }
constexpr int ANIMMODE_LOOP=0,G_RM_PASS=0,G_RM_ZB_CLD_SURF2=0;
#define GRAPH_ALLOC(ctx,size) Graph_Alloc(ctx,size)
#define gDPPipeSync(...) ((void)0)
#define gDPSetPrimColor(p,lod,min,r,g,b,a) (fairyPrim[0]=r,fairyPrim[1]=g,fairyPrim[2]=b,fairyPrim[3]=a)
#define gDPSetEnvColor(p,r,g,b,a) ((void)(p),fairyEnv[0]=r,fairyEnv[1]=g,fairyEnv[2]=b,fairyEnv[3]=a)
#define gDPSetRenderMode(...) ((void)0)
#define gSPEndDisplayList(...) ((void)0)
#define Gfx_SetupDL27_Xlu(...) events.push_back("fairy-setup")
#define Gfx_SetupDL_27Xlu(...) events.push_back("fairy-setup")
#include "combo/menu/ComboFairyBottleDraw.h"
#include "fairy_production.inc"

bool Near(float x, float y) { return std::abs(x - y) < .0001f; }
bool Same(const MtxF& a, const MtxF& b) {
    return Near(a.x,b.x) && Near(a.y,b.y) && Near(a.z,b.z) && Near(a.sx,b.sx) && Near(a.sy,b.sy) &&
           Near(a.sz,b.sz) && a.billboard == b.billboard;
}
void Reset(uint32_t frame) {
    play.gameplayFrames = frame; play.state.frames = 999; // dormant-owner clock cannot drive the motion
    current = initial = {10,20,30,2,2,2};
    draws.clear(); events.clear(); stack.clear(); foreignRestores = hostFallbacks = skeletonDraws = shimmerDraws = 0;
    opaPtr = opa; xluPtr = xlu;
    for (auto& command : opa) command.stream = 0;
    for (auto& command : xlu) command.stream = 1;
}
void CheckDraw(float contentsY, bool foreign) {
    assert(draws.size() == 3 && draws[0].stream == 0 && draws[1].stream == 1 && draws[2].stream == 1);
    assert(draws[0].path == (selectedMods.count(genericGlass) ? genericOpaque : opaque));
    assert(draws[1].path == (selectedMods.count(genericGlass) ? genericGlass : glass));
    assert(draws[2].path == fairy && Same(draws[0].pose, initial) && Same(draws[1].pose, initial));
    auto expected = ComboFairyBottle_Sample(play.gameplayFrames);
    const auto& content = draws[2].pose;
    assert(Near(content.x, initial.x + expected.x) && Near(content.y, initial.y + contentsY + expected.y));
    assert(Near(content.z, initial.z + expected.z) && content.billboard);
    assert(Near(content.sx, 2 * expected.scaleX) && Near(content.sy, 2 * expected.scaleY));
    assert(Near(content.sz, 2 * expected.scaleZ) && Same(current, initial) && stack.empty());
    assert(foreignRestores == (foreign ? 1 : 0) && hostFallbacks == 0);
}

int main() {
    for (int mask = 0; mask < 32; ++mask) {
        selectedMods.clear();
        const char* paths[] = {opaque, glass, genericOpaque, genericGlass};
        for (int i = 0; i < 4; ++i) if (mask & (1 << i)) selectedMods.insert(paths[i]);
        auto shell = ComboFairyBottle_SelectShell(opaque,glass,genericOpaque,genericGlass,ResourceMgr_IsModAsset,blueFire,mask&16);
        bool blue = !(mask & 3) && (mask & 16), generic = !(mask & 3) && !blue && (mask & 12);
        assert(shell.opaque == (blue ? blueFire : generic ? genericOpaque : opaque));
        assert(shell.glass == (blue ? blueFire : generic ? genericGlass : glass));
    }
    auto shell = ComboFairyBottle_SelectShell(opaque,glass,genericOpaque,genericGlass,nullptr,blueFire,1);
    assert(shell.opaque == opaque && shell.glass == glass);
    selectedMods.clear();
    for (uint32_t frame = 0; frame < 360; ++frame) {
        auto motion = ComboFairyBottle_Sample(frame), repeat = ComboFairyBottle_Sample(frame + 360);
        assert(std::abs(motion.x) <= .65001f && std::abs(motion.y) <= 1.15001f && std::abs(motion.z) <= .35001f);
        assert(motion.scaleX >= .73999f && motion.scaleX <= .82001f && Near(motion.scaleY,.78f) && Near(motion.scaleZ,.78f));
        assert(Near(motion.x,repeat.x) && Near(motion.y,repeat.y) && Near(motion.z,repeat.z));
    }
    assert(!Near(ComboFairyBottle_Sample(0).x, ComboFairyBottle_Sample(27).x));
    assert(!Near(ComboFairyBottle_Sample(0).scaleX, ComboFairyBottle_Sample(1).scaleX));
    puts("PASS fairy/TP BlueFire/generic shell precedence (all 32 selected-mod combinations) and full 360-frame motion envelope");
    for (uint32_t frame : {0u,1u,27u}) {
        auto motion = ComboFairyBottle_Sample(frame);
        printf("fairy frame=%u offset=(%.5f,%.5f,%.5f) scale=(%.5f,%.5f,%.5f)\n", frame,
               motion.x,motion.y,motion.z,motion.scaleX,motion.scaleY,motion.scaleZ);
    }

    void* resources[8]{};
    int xluStart = -1, scroll = 0, kind = -1; float scale = 0; u8 colors[16]{};
    for (int generic = 0; generic < 2; ++generic) {
        selectedMods.clear(); if (generic) selectedMods.insert(genericGlass);
        assert(oot::GetItem_GetDrawTableEntry(0,resources,8,&xluStart,&scale,&kind,colors) == 3);
        assert(kind == CW_DRAW_KIND_FAIRY && xluStart == 1 && resources[2] == fairy);
        assert(resources[0] == (generic ? genericOpaque : opaque));
        for (int capacity = 0; capacity < 3; ++capacity)
            assert(oot::GetItem_GetDrawTableEntry(0,resources,capacity,&xluStart,&scale,&kind,colors) == 0);
        assert(mm::GetItem_GetDrawTableEntry(0,resources,8,&xluStart,&scale,&scroll,&kind) == 4);
        assert(kind == CW_DRAW_KIND_MM_FAIRY_CONTAINER && xluStart == 1 && resources[2] == fairy && resources[3] == matrixPath);
        assert(resources[0] == (generic ? genericOpaque : opaque));
        for (int capacity = 0; capacity < 4; ++capacity)
            assert(mm::GetItem_GetDrawTableEntry(0,resources,capacity,&xluStart,&scale,&scroll,&kind) == 0);
        for (uint32_t frame : {0u,27u}) {
            Reset(frame); oot::GetItem_DrawFairy(&play,0); CheckDraw(0,false);
            Reset(frame); mm::GetItem_DrawFairyBottle(&play,1); CheckDraw(0,false);
            Reset(frame); mm::GetItem_DrawFairyContainer(&play,0); CheckDraw(nativePlacement.y,false);
        }
    }
    assert(CwMinDlistsForKind(CW_DRAW_KIND_MM_FAIRY_CONTAINER) == 4);
    puts("PASS native OoT/MM shell exports, complete MM contents+matrix recipe, stable shell and moving fairy command streams");

    selectedMods.clear(); ComboForeignDrawInfo info;
    for (uint32_t frame : {0u,27u}) {
        Reset(frame); OOT_DrawForeignFairyBottle(&play,&info); CheckDraw(0,true);
        Reset(frame); MM_DrawForeignFairy(&info); CheckDraw(0,true);
        Reset(frame); OOT_DrawForeignFairyContainer(&play,&info); CheckDraw(nativePlacement.y,true);
        assert(events.front() == "mat" && events.back() == "restore");
    }
    puts("PASS both foreign directions: active-host motion, OPA shell/XLU glass+fairy order, native placement and state restoration");
    fairyResources = true;
    selectedMods = {blueFire};
    for (uint32_t frame : {0u,1u,27u}) {
        Reset(frame); oot::GetItem_DrawFairy(&play,0);
        assert(draws.size() == 2 && draws[0].path == blueFire && draws[1].path == "fairy-vfx");
        assert(skeletonDraws == 1 && animationFrame == (int)frame && Same(current,initial) && stack.empty());
        assert(fairyPrim[0] == 255 && fairyPrim[1] == 160 && fairyPrim[2] == 235);
        assert(fairyEnv[0] == 255 && fairyEnv[1] == 160 && fairyEnv[2] == 235);
        auto motion = ComboFairyBottle_Sample(frame);
        assert(Near(draws[1].pose.x,initial.x-8+motion.x) && Near(draws[1].pose.y,initial.y-2+motion.y));
        assert(Near(draws[1].pose.sx,2*motion.scaleX*.004f));
        Reset(frame); mm::GetItem_DrawFairyContainer(&play,0);
        assert(draws.size() == 2 && ComboFairyBottle_IsBlueFireShell(draws[0].path.c_str()) && draws[1].path == "fairy-vfx");
        info.dls[0]=info.dls[1]=blueFire;
        Reset(frame); OOT_DrawForeignFairyContainer(&play,&info);
        assert(draws.size() == 2 && draws[0].path == blueFire && draws[1].path == "fairy-vfx" && foreignRestores == 1);
        Reset(frame); MM_DrawForeignFairy(&info);
        assert(draws.size() == 2 && draws[0].path == blueFire && draws[1].path == "fairy-vfx" && foreignRestores == 1);
    }
    fairyResources = false; Reset(27); oot::GetItem_DrawFairy(&play,0);
    assert(draws.size() == 2 && draws[0].path == blueFire && draws[1].path == fairy && skeletonDraws == 0);
    selectedMods = {blueFire,glass};
    assert(oot::GetItem_FairyBottleShell(0).opaque == opaque && mm::GetItem_FairyBottleShell(0).opaque == opaque);
    puts("PASS selected TP BlueFire shell once, animated host-local fairy wings/glow in pink, no blue flame, native contents fallback");
    selectedMods.clear(); info.dls[0]=opaque; info.dls[1]=glass;
    selectedForeign = &info; GetItemEntry entry;
    for (int missing = 0; missing < 6; ++missing) {
        Reset(27);
        ownerPresent = missing != 0; matrixPresent = missing != 1; matrixPointerPresent = missing != 2;
        materialPresent = missing != 3;
        info.dls[3] = missing == 4 ? "__OTR__@oot:matrix" : "__OTR__@mm:matrix";
        info.count = missing == 5 ? 3 : 4;
        OOT_DrawForeignFairyContainer(&play,&info);
        assert(hostFallbacks == 1 && draws.size()==3 && shimmerDraws==1 && stack.empty() && Same(current,initial) && ownerScope == 1);
        for (bool overlay : {true,false}) {
            info.itemShimmer=overlay;
            Reset(27); OOT_DrawComboForeign(&play,&entry);
            assert(hostFallbacks == 1 && draws.size()==3 && stack.empty() && Same(current,initial));
            assert(shimmerDraws==1 && "outer foreign dispatcher duplicated the native fallback's pink overlay");
        }
    }
    ownerPresent=matrixPresent=matrixPointerPresent=materialPresent=true;
    info.dls[3]="__OTR__@mm:matrix"; info.count=4; info.itemShimmer=true;
    Reset(27); OOT_DrawComboForeign(&play,&entry);
    assert(hostFallbacks==0 && shimmerDraws==1 && draws.size()==3 && stack.empty() && Same(current,initial));
    puts("PASS real foreign dispatcher + native fallback: exactly one pink overlay for missing resources and complete recipes");
}
