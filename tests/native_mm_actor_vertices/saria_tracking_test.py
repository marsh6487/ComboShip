"""Exercise current Saria/Kokiri/Fado head callbacks with an affine-matrix fixture.

The production regression is a native-only pre-translation tracking pivot moving
the fitted head's neck anchor. This compiles the exact callback bodies and real
model-presence predicates; graphics submission and game/actor layout are fixture
boundaries. It is a draw-math test, not a complete actor build or runtime proof.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def method(root, path, name):
    source = (root / path).read_text()
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Production callback not found: {name}")
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


CALLBACKS = [
    ("soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c", "EnViewer_StaticSariaOverrideLimbDraw"),
    ("soh/src/overlays/actors/ovl_En_Viewer/static_story_kokiri.c", "StaticStoryKokiri_OverrideLimbDraw"),
    ("soh/src/overlays/actors/ovl_En_Sa/z_en_sa.c", "EnSa_OverrideLimbDraw"),
    ("soh/src/overlays/actors/ovl_En_Ko/z_en_ko.c", "EnKo_OverrideLimbDraw"),
]

GLOBAL = r"""
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint8_t u8;
typedef int32_t s32;
typedef union { struct { uintptr_t w0, w1; } words; } Gfx;
"""

ANIMATION = r"""
#pragma once
#include "global.h"
typedef struct AnimationHeader AnimationHeader;
typedef struct SkelAnime SkelAnime;
typedef struct MtxF MtxF;
"""

RESOURCES = r"""
#pragma once
#include "global.h"
bool ResourceMgr_IsAltAssetsEnabled(void);
uint8_t ResourceMgr_FileExists(const char*);
uint8_t ResourceMgr_FileAltExists(const char*);
"""

FIXTURE = r"""
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
typedef int16_t s16;
typedef float f32;
typedef struct { float x,y,z; } Vec3f;
typedef struct { int16_t x,y,z; } Vec3s;
typedef struct SkelAnime { int placeholder; } SkelAnime;
typedef struct { Vec3s headRot,torsoRot; } NpcInteractInfo;
typedef struct { int params; } Actor;
typedef struct {
    int type,pose,eyeIndex,talking,objectSlots[4];
    NpcInteractInfo interactInfo;
} StaticState;
typedef struct { struct { SkelAnime skelAnime; } skin; StaticState staticState; } EnViewer;
typedef struct {
    SkelAnime skelAnime;
    int leftEyeIndex,rightEyeIndex,mouthIndex;
    NpcInteractInfo interactInfo;
} EnSa;
typedef struct {
    SkelAnime skelAnime;
    Actor actor;
    int headObjectBankIdx,legsObjectBankIdx,eyeTextureIndex;
    NpcInteractInfo interactInfo;
    int fidgetTableY[20],fidgetTableZ[20];
} EnKo;
typedef struct {
    int sceneNum;
    struct { struct { void* segment; } status[4]; } objectCtx;
} PlayState;
#include "soh/Enhancements/cosmetics/NativeNpcModels.h"
#include "soh/Enhancements/cosmetics/YoungFadoNpc.h"
#define MTXMODE_APPLY 1
#define BINANG_TO_RAD(x) ((x)*(M_PI/32768.0))
#define VIRTUAL_TO_PHYSICAL(x) ((uintptr_t)(x))
#define SEGMENTED_TO_VIRTUAL(x) (x)
#define STATIC_STORY_ACTOR_SARIA 1
#define STATIC_STORY_ACTOR_KOKIRI_GIRL 2
#define STATIC_STORY_ACTOR_FADO 3
#define STATIC_POSE_FLAG_OCARINA 1
#define SCENE_SACRED_FOREST_MEADOW 999
#define ENKO_TYPE (this->actor.params)
#define ENKO_TYPE_CHILD_FADO 12
#define gSPSegment(g,s,v) ((void)(g),(void)(s),(void)(v))
uintptr_t gSegments[16];
static unsigned sFlags;
static bool sTrack = true;
static double matrix[4][4];

static void Identity(void) {
    memset(matrix,0,sizeof(matrix));
    for (int i=0;i<4;i++) matrix[i][i]=1;
}
static void Multiply(double b[4][4]) {
    double result[4][4]={0};
    for (int i=0;i<4;i++)
        for (int j=0;j<4;j++)
            for (int k=0;k<4;k++)
                result[i][j]+=matrix[i][k]*b[k][j];
    memcpy(matrix,result,sizeof(result));
}
void Matrix_Translate(float x,float y,float z,int mode) {
    double t[4][4]={{1,0,0,x},{0,1,0,y},{0,0,1,z},{0,0,0,1}};
    Multiply(t);
}
void Matrix_RotateX(float angle,int mode) {
    double c=cos(angle),s=sin(angle);
    double r[4][4]={{1,0,0,0},{0,c,-s,0},{0,s,c,0},{0,0,0,1}};
    Multiply(r);
}
void Matrix_RotateZ(float angle,int mode) {
    double c=cos(angle),s=sin(angle);
    double r[4][4]={{c,-s,0,0},{s,c,0,0},{0,0,1,0},{0,0,0,1}};
    Multiply(r);
}
void Matrix_RotateY(float angle,int mode) {
    double c=cos(angle),s=sin(angle);
    double r[4][4]={{c,0,s,0},{0,1,0,0},{-s,0,c,0},{0,0,0,1}};
    Multiply(r);
}
static Vec3f Transform(Vec3f p) {
    double v[3]={p.x,p.y,p.z};
    Vec3f result;
    float* xyz=&result.x;
    for (int i=0;i<3;i++) {
        xyz[i]=matrix[i][3];
        for (int j=0;j<3;j++) xyz[i]+=matrix[i][j]*v[j];
    }
    return result;
}
static double Distance(Vec3f a,Vec3f b) {
    return sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));
}
bool ResourceMgr_IsAltAssetsEnabled(void) { return (sFlags&1)!=0; }
uint8_t ResourceMgr_FileExists(const char* path) { return (sFlags&2)!=0; }
uint8_t ResourceMgr_FileAltExists(const char* path) {
    if (strstr(path,"/Skel")) return (sFlags&4)!=0;
    if (strstr(path,"/Body/")) return (sFlags&8)!=0;
    return 1;
}
Gfx* NativeNpcPose_SelectLimb(const SkelAnime* a,s32 i,Gfx* display) { return display; }
float Math_SinS(int a) { return sin(BINANG_TO_RAD(a)); }
float Math_CosS(int a) { return cos(BINANG_TO_RAD(a)); }
typedef int StaticStoryActorType;
typedef struct { int flags; } StaticStoryPoseDescriptor;
static StaticStoryPoseDescriptor pose={0};
const StaticStoryPoseDescriptor* StaticStoryActor_ResolvePose(int type,int id) { return &pose; }
int StaticStoryActor_CanTrack(int type,int id) { return sTrack; }
u8 StaticStoryActor_GetInteractionEyeIndex(int t,int p,int e,int talking) { return e; }
Gfx gSariaRightHandAndOcarinaDL[1],gFaDL[1],object_kw1_DL_002C10[1];
static void* sFadoEyes[3],*sGirlEyes[3];
static struct { int headId; } sModelInfo[16];
static struct { Gfx* dList; void** eyeTextures; } sHead[16];
"""

MAIN = r"""
int main(void) {
    PlayState play={0};
    EnViewer viewer={0};
    EnSa saria={0};
    EnKo kokiri={0};
    const struct Case {
        const char* name;
        Vec3f neck;
        int type,ordinary,kokiriType;
        float nativePivot;
    } cases[]={
        {"Saria static",{878,0,0},STATIC_STORY_ACTOR_SARIA,0,0,900},
        {"Twins static",{1016,0,0},STATIC_STORY_ACTOR_KOKIRI_GIRL,0,1,1200},
        {"Fado static",{884,-49,7},STATIC_STORY_ACTOR_FADO,0,12,1200},
        {"Saria ordinary",{878,0,0},STATIC_STORY_ACTOR_SARIA,1,0,900},
        {"Twins ordinary",{1016,0,0},STATIC_STORY_ACTOR_KOKIRI_GIRL,1,1,1200},
        {"Fado ordinary",{884,-49,7},STATIC_STORY_ACTOR_FADO,1,12,1200},
        {"Kokiri boy control",{1350,0,0},STATIC_STORY_ACTOR_KOKIRI_GIRL,1,2,1200}
    };
    const Vec3s angles[]={
        {0,0,0},{4096,0,0},{-4096,0,0},{0,4096,0},{0,-4096,0},{4096,-4096,0}
    };
    unsigned checks=0,failures=0;
    for (unsigned route=0;route<sizeof(cases)/sizeof(cases[0]);route++)
        for (sFlags=0;sFlags<16;sFlags++)
            for (int extraAxes=0;extraAxes<2;extraAxes++)
                for (unsigned angle=0;angle<sizeof(angles)/sizeof(angles[0]);angle++)
                    for (int tracking=0;tracking<(cases[route].ordinary?1:2);tracking++) {
                        sTrack=tracking==0;
                        viewer.staticState.type=cases[route].type;
                        viewer.staticState.interactInfo.headRot=angles[angle];
                        saria.interactInfo.headRot=angles[angle];
                        kokiri.interactInfo.headRot=angles[angle];
                        kokiri.actor.params=cases[route].kokiriType;
                        Vec3f pos=cases[route].neck;
                        if (extraAxes) { pos.y+=80;pos.z-=45; }
                        Vec3f savedPos=pos;
                        Vec3s rotation={0},savedRotation=rotation;
                        bool authored=sFlags==15 && cases[route].kokiriType!=2;
                        Identity();
                        /* Independent invariant: the authored neck is its joint
                         * origin; native fallback retains the existing pivot. */
                        if (sTrack && !authored) {
                            Matrix_Translate(cases[route].nativePivot,0,0,MTXMODE_APPLY);
                            Matrix_RotateX(BINANG_TO_RAD(angles[angle].y),MTXMODE_APPLY);
                            Matrix_RotateZ(BINANG_TO_RAD(angles[angle].x),MTXMODE_APPLY);
                            Matrix_Translate(-cases[route].nativePivot,0,0,MTXMODE_APPLY);
                        }
                        Vec3f expected=Transform(pos);
                        Identity();
                        Gfx commands[8]={0},*display=commands,*out=commands;
                        if (cases[route].ordinary) {
                            if (cases[route].type==STATIC_STORY_ACTOR_SARIA)
                                EnSa_OverrideLimbDraw(&play,16,&display,&pos,&rotation,&saria,&out);
                            else
                                EnKo_OverrideLimbDraw(&play,15,&display,&pos,&rotation,&kokiri,&out);
                        } else {
                            if (cases[route].type==STATIC_STORY_ACTOR_SARIA)
                                EnViewer_StaticSariaOverrideLimbDraw(&play,16,&display,&pos,&rotation,&viewer);
                            else
                                StaticStoryKokiri_OverrideLimbDraw(&play,15,&display,&pos,&rotation,&viewer,&out);
                        }
                        Vec3f actual=Transform(pos);
                        double error=Distance(actual,expected);
                        if (error>.001) {
                            if (failures<8)
                                fprintf(stderr,"FAIL %s flags=%u tracking=%d neck=(%.0f,%.0f,%.0f)"
                                        " angle=(%d,%d): anchor displacement %.6f\n",
                                        cases[route].name,sFlags,sTrack,pos.x,pos.y,pos.z,
                                        angles[angle].x,angles[angle].y,error);
                            failures++;
                        }
                        /* Keeping the neck still must retain the native look
                         * direction. Verify an off-origin point separately. */
                        const Vec3f direction={170,80,-110};
                        double rx=sTrack?BINANG_TO_RAD(angles[angle].y):0;
                        double rz=sTrack?BINANG_TO_RAD(angles[angle].x):0;
                        double zx=cos(rz)*direction.x-sin(rz)*direction.y;
                        double zy=sin(rz)*direction.x+cos(rz)*direction.y;
                        Vec3f expectedDirection={zx,cos(rx)*zy-sin(rx)*direction.z,
                                                   sin(rx)*zy+cos(rx)*direction.z};
                        Vec3f probe={pos.x+direction.x,pos.y+direction.y,pos.z+direction.z};
                        Vec3f actualProbe=Transform(probe);
                        Vec3f actualDirection={actualProbe.x-actual.x,actualProbe.y-actual.y,
                                               actualProbe.z-actual.z};
                        if (Distance(actualDirection,expectedDirection)>.001 ||
                            memcmp(&savedPos,&pos,sizeof(pos)) ||
                            memcmp(&savedRotation,&rotation,sizeof(rotation)))
                            failures++;
                        checks++;
                    }
    printf("%s Saria/Kokiri/Fado exact callbacks: %u checks, %u failures;"
           " authored neck anchors, look direction, Alt/partial-pack fallback,"
           " boy isolation and static tracking policy.\n",
           failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    root = args.source_root.resolve()
    with tempfile.TemporaryDirectory(prefix="saria-tracking-") as directory:
        work = Path(directory)
        (work / "soh").mkdir()
        (work / "global.h").write_text(GLOBAL)
        (work / "z64animation.h").write_text(ANIMATION)
        (work / "soh/ResourceManagerHelpers.h").write_text(RESOURCES)
        source = work / "tracking.c"
        source.write_text(FIXTURE + "\n".join(method(root, *callback) for callback in CALLBACKS) + MAIN)
        binary = work / "tracking"
        command = [os.environ.get("CC", "cc"), "-std=gnu2x",
                   "-Werror=implicit-function-declaration",
                   "-I" + str(work), "-I" + str(root / "soh"),
                   "-I" + str(root / "soh/include")]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
        subprocess.run([*command, str(source), "-lm", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()

