// Executes the production MM skin renderer and native skin matrix math.
// Only allocation, display-list submission, CVars and the actor matrix boundary are replaced.
extern "C" {
#include "expansions/ssbb/ssbb_anim.h"
#include "expansions/ssbb/ssbb_skin.h"
#include "expansions/ssbb/characters/pikachu_ssbb_register.h"
}
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static float scaleOverride, submittedScale;
static int displayLists;
extern "C" {
int32_t CVarGetInteger(const char*, int32_t value) { return value; }
float CVarGetFloat(const char* name, float value) {
    return std::strcmp(name, "gExpansions.SSBB.SkinScale") == 0 ? scaleOverride : value;
}
void* ZeldaArena_Malloc(size_t size) { return std::calloc(1, size); }
void ZeldaArena_Free(void* p) { std::free(p); }
void* Graph_Alloc(GraphicsContext*, size_t size) { static Mtx m; assert(size == sizeof(m)); return &m; }
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Gfx_SetupDL25_Opa(GraphicsContext*) {}
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild(void) {}
void gSPSegment(void*, int, uintptr_t) {}
void gSPDisplayList(Gfx*, Gfx*) { ++displayLists; }
void Matrix_SetTranslateRotateYXZ(f32, f32, f32, Vec3s*) {}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode) { assert(x == y && y == z); submittedScale = x; }
Mtx* Matrix_ToMtx(Mtx* out) { return out; }
Gfx gCullBackDList[1];
AnimationHeader pikachu_ssbb_Wait1_anim{}, pikachu_ssbb_Wait3_anim{};
}
static void close(float a, float b) { if (std::fabs(a-b) > .002f) {
    std::fprintf(stderr, "FAIL skin pose: expected %.3f, got %.3f\n", b, a); std::exit(1);
} }
int main() {
    StandardLimb limbs[3]{};
    void* skeleton[] = {&limbs[0], &limbs[1], &limbs[2]};
    for (int i=0; i<3; ++i) { limbs[i].child = i==2 ? LIMB_DONE : i+1; limbs[i].sibling=LIMB_DONE; }
    SSBBBoneFrame frames[6]{};
    for (auto& f:frames) f.sx=f.sy=f.sz=1;
    frames[0].tx=10; frames[1].ty=20; frames[2].tx=30; frames[2].ty=5;
    frames[3]=frames[0]; frames[3].tx=30; frames[3].sx=2;
    frames[4]=frames[1]; frames[5]=frames[2];
    SSBBAnim anim{"fixture", 2, 3, 30, frames};
    SSBBSkinVertex vertex{2,0,0,127,0,0,32,64,255};
    SSBBSkinWeight weight{{0,0,0,0},{255,0,0,0}};
    MtxF inverse[3]{};
    for (auto& m:inverse) m.xx=m.yy=m.zz=m.ww=1;
    SSBBSkinMesh skin{};
    skin.vertexCount=1; skin.boneCount=3; skin.vertices=&vertex; skin.weights=&weight;
    skin.invBindMatrices=inverse; skin.daeToF64.xx=skin.daeToF64.yy=skin.daeToF64.zz=skin.daeToF64.ww=1;
    Gfx mesh[1]{}; skin.displayList=mesh;
    SSBBCharacterDef def{}; def.skinMesh=&skin; def.scale=.3f; def.numLimbs=3;
    SSBBCharacterInstance inst{}; inst.def=&def; inst.skeleton=skeleton; inst.initialized=1;
    inst.ssbbAnim=&anim; inst.curFrame=.5f;
    GraphicsContext gfx{}; Gfx commands[128]{}; PlayState play{}; play.state.gfxCtx=&gfx;
    Vec3f position{}; Vec3s rotation{};
    auto draw=[&] { gfx.polyOpa.p=commands; SSBBSkin_Draw(&inst,&play,&position,&rotation); };
    SSBBSkin_Init(&skin);
    // Existing Pikachu defaults: snap fractional frames, strip TopN/EyeYellowM motion,
    // and retain the shared scale override.
    scaleOverride=.7f; draw();
    close(skin.vtxBuf[skin.bufIndex^1][0].n.ob[0],2); close(submittedScale,.7f);
#ifdef MM_WOLF_SKIN_OPTIONS
    skin.preserveRootMotion=1; skin.interpolateFrames=1; skin.useDefinitionScale=1;
#endif
    draw();
    // The unmodified renderer fails here (2 instead of 23): Wolf's authored root
    // translation and fractional scale must reach the actual packed vertex.
    close(skin.vtxBuf[skin.bufIndex^1][0].n.ob[0],23); close(submittedScale,.3f);
#ifdef MM_WOLF_SKIN_OPTIONS
    Vec3f bone{}; assert(SSBBSkin_GetBoneWorldPos(&inst,2,&bone));
    close(bone.x,65); close(bone.y,25);
    assert(!SSBBSkin_GetBoneWorldPos(&inst,3,&bone));
    assert(!SSBBSkin_GetBoneWorldPos(&inst,-1,&bone));
    assert(!SSBBSkin_GetBoneWorldPos(&inst,0,nullptr));
    SSBBCharacterInstance other=inst;
    assert(!SSBBSkin_GetBoneWorldPos(&other,0,&bone));
    assert(SSBBSkin_ComputePose(&other));
    assert(!SSBBSkin_GetBoneWorldPos(&inst,0,&bone));
    assert(SSBBSkin_GetBoneWorldPos(&other,0,&bone)); close(bone.x,20);
    // Shortest rotation arc, including wrap across -180/180.
    frames[0].rz=170; frames[3].rz=-170; inst.curFrame=.5f; draw();
    close(skin.vtxBuf[skin.bufIndex^1][0].n.ob[0],17);
    inst.curFrame=1; draw(); close(submittedScale,.3f);
    SSBBSkin_Destroy(&skin);
    assert(!SSBBSkin_GetBoneWorldPos(&inst,0,&bone));
#else
    SSBBSkin_Destroy(&skin);
#endif
    // Actual production Pikachu has 48 skeleton nodes but only 47 weighted bones.
    // The trailing node is a rigid extra limb; it has no inverse bind or skin weights.
    SSBBBoneFrame pikaFrames[47]{};
    for (auto& f:pikaFrames) f.sx=f.sy=f.sz=1;
    SSBBAnim pikaAnim{"pikachu-fixture",1,47,30,pikaFrames};
    SSBBCharacterInstance pika{};
    pika.def=&pikachu_ssbb_def; pika.skeleton=(void**)pikachu_ssbb_skeleton.sh.segment;
    pika.initialized=1; pika.ssbbAnim=&pikaAnim;
    assert(pika.def->numLimbs==48 && pika.def->skinMesh->boneCount==47);
    SSBBSkin_Init(pika.def->skinMesh); gfx.polyOpa.p=commands;
    int before=displayLists;
    SSBBSkin_Draw(&pika,&play,&position,&rotation);
    if (displayLists == before) { std::fputs("FAIL actual Pikachu 48-node/47-bone metadata stopped rendering\n",stderr); return 1; }
    close(submittedScale,.7f);
#ifdef MM_WOLF_SKIN_OPTIONS
    assert(!SSBBSkin_GetBoneWorldPos(&pika,47,&position));
    assert(!SSBBSkin_GetBoneWorldPos(&pika,48,&position));
#endif
    SSBBSkin_Destroy(pika.def->skinMesh);
    std::puts("PASS production MM skin: Pikachu defaults, Wolf root/TRS interpolation, scale and owner-scoped bone queries");
}
