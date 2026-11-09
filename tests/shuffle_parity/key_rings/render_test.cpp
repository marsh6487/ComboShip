// Actual host types, GBI, matrix/graph/setup bodies. GPU, resource and matrix
// allocation services are controlled boundaries, not configured gameplay proof.
#include "global.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifndef KEY_RING_TEST_MM
#define COMBO_MM_KEY_RING_HOST_OOT
#endif
static MtxF matrixStack[20];
MtxF* sMatrixStack = matrixStack;
MtxF* sCurrentMatrix = matrixStack;
static std::array<Mtx,64> matrixArena;
static size_t allocations, models, vertexReports, requestedMatrices, requestedOpa;
static bool available = true;
static int openDepth;
#ifdef KEY_RING_TEST_MM
Gfx gSetupDLs[SETUPDL_MAX][6];
#else
static Gfx sSetupDL[80][6];
#endif
static GraphicsContext graphics{};
static PlayState play{};
alignas(16) static Gfx opaque[2048], translucent[32], overlay[32];
extern "C" {
#define FrameInterpolation_RecordMatrixPush(...) ((void)0)
#define FrameInterpolation_RecordMatrixPop(...) ((void)0)
#define FrameInterpolation_RecordMatrixTranslate(...) ((void)0)
#define FrameInterpolation_RecordMatrixScale(...) ((void)0)
#define FrameInterpolation_RecordMatrixRotateAxis(...) ((void)0)
#include "key_ring_matrix.inc"
void FrameInterpolation_RecordOpenChild(const void*,int) { ++openDepth; }
void FrameInterpolation_RecordCloseChild() { assert(openDepth>0); --openDepth; }
void SkinMatrix_SetTranslate(MtxF* out,float x,float y,float z) {
    *out={}; out->xx=out->yy=out->zz=out->ww=1;
    out->xw=x;out->yw=y;out->zw=z;
}
void SkinMatrix_SetScale(MtxF* out,float x,float y,float z) {
    *out={};out->xx=x;out->yy=y;out->zz=z;out->ww=1;
}
void gSPDisplayList(Gfx* cmd,Gfx* list) {
    if (!std::strncmp(reinterpret_cast<const char*>(list),"__OTR__",7)) {
        cmd->words.w0=G_DL_OTR_FILEPATH<<24;cmd->words.w1=reinterpret_cast<uintptr_t>(list);
    } else { __gSPDisplayList(cmd,list); }
}
void gSPVertex(Gfx* cmd,uintptr_t address,int count,int start) { __gSPVertex(cmd,address,count,start); }
#ifdef KEY_RING_TEST_MM
Mtx* Matrix_Finalize(GraphicsContext*) {
#else
Mtx* Matrix_NewMtx(GraphicsContext*,char*,s32) {
#endif
    assert(allocations<matrixArena.size());auto* result=&matrixArena[allocations++];
    static_assert(sizeof(Mtx)==sizeof(MtxF));
    std::memcpy(result,sCurrentMatrix,sizeof(*result));return result;
}
int ResourceMgr_OTRSigCheck(char* path) { return path&&!std::strncmp(path,"__OTR__",7); }
bool NeiGi_CanDrawLayers(PlayState* state,size_t matrices,size_t opa,size_t xlu) {
    assert(state==&play&&xlu==2);requestedMatrices=matrices;requestedOpa=opa;
    return available&&size_t(static_cast<Gfx*>(graphics.polyOpa.d)-graphics.polyOpa.p)>=opa&&
           size_t(static_cast<Gfx*>(graphics.polyXlu.d)-graphics.polyXlu.p)>=xlu&&
           size_t(static_cast<Gfx*>(graphics.overlay.d)-graphics.overlay.p)>=2;
}
void NeiGi_ReportBlackMarketModel(const char* owner,const char* metal,const char*) {
    assert(!std::strcmp(owner,"mm")&&metal);++models;
}
void NeiGi_ReportBlackMarketVertices(const Vtx* vertices,size_t count) {
    assert(vertices&&count==120);++vertexReports;
}
#include "key_ring_graph.inc"
#include "key_ring_setup.inc"
}
#include "combo/ComboMmKeyRingDraw.h"
static void Reset() {
    sCurrentMatrix=sMatrixStack;*sCurrentMatrix={};
    sCurrentMatrix->xx=sCurrentMatrix->yy=sCurrentMatrix->zz=sCurrentMatrix->ww=1;
    Matrix_Translate(7,11,-13,MTXMODE_APPLY);
    graphics.polyOpa.p=opaque;graphics.polyOpa.d=std::end(opaque);
    graphics.polyXlu.p=translucent;graphics.polyXlu.d=std::end(translucent);
    graphics.overlay.p=overlay;graphics.overlay.d=std::end(overlay);
    play.state.gfxCtx=&graphics;allocations=models=vertexReports=requestedMatrices=requestedOpa=0;
    openDepth=0;available=true;
}
static void CheckGeometry() {
    const auto& vertices=ComboMmKeyRing::Connector();
    for(int batch=0;batch<6;++batch) {
        for(int side=0;side<4;++side) {
            const auto& last=vertices[batch*20+16+side];const auto& next=vertices[((batch+1)%6)*20+side];
            assert(!std::memcmp(last.v.ob,next.v.ob,sizeof(last.v.ob))&&last.v.cn[3]==255);
        }
        for(int row=0;row<4;++row)for(int side=0;side<4;++side) {
            const auto& a=vertices[batch*20+row*4+side].v;
            const auto& b=vertices[batch*20+row*4+(side+1)%4].v;
            const auto& c=vertices[batch*20+(row+1)*4+(side+1)%4].v;
            double u[3],v[3];for(int k=0;k<3;++k){u[k]=b.ob[k]-a.ob[k];v[k]=c.ob[k]-a.ob[k];}
            const double cross[]={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
            assert(cross[0]*cross[0]+cross[1]*cross[1]+cross[2]*cross[2]>1);
        }
    }
}
int main() {
    CheckGeometry();const int counts[]={1,3,1,4};
    const char metal[]="__OTR__objects/metal",emblem[]="__OTR__objects/emblem",ring[]="__OTR__objects/ring";
    for(int owner=0;owner<4;++owner)for(bool changed:{false,true}) {
        Reset();const auto before=*sCurrentMatrix;
        const uint8_t steel[]={uint8_t(changed?20:213),224,236,255};
        const uint8_t badge[]={201,uint8_t(changed?230:38),41,255},hoop[]={100,110,120,255};
        assert(ComboMmKeyRing::Draw(&play,owner,metal,emblem,ring,steel,badge,hoop));
        assert(sCurrentMatrix==sMatrixStack&&!std::memcmp(&before,sCurrentMatrix,sizeof(before))&&!openDepth);
        const int count=counts[owner];
        assert(allocations==size_t(1+2*count)&&requestedMatrices==allocations);
        assert(models==size_t(1+count)&&vertexReports==size_t(count));
        assert(size_t(graphics.polyOpa.p-opaque)<requestedOpa&&graphics.polyOpa.p<=graphics.polyOpa.d);
        assert(graphics.polyXlu.p==translucent&&graphics.overlay.p==overlay);
        size_t keys=0,emblems=0,rings=0,vertices=0,triangles=0;uint32_t tint=0;MtxF active{};bool loaded=false;
        for(const Gfx* cmd=opaque;cmd<graphics.polyOpa.p;++cmd) {
            const auto op=cmd->words.w0>>24;
            if(op==G_SETENVCOLOR)tint=cmd->words.w1;
            if(op==G_MTX){std::memcpy(&active,reinterpret_cast<const void*>(cmd->words.w1),sizeof(active));loaded=true;}
            if(op==G_VTX){++vertices;assert(((cmd->words.w0>>12)&255)==20);}
            if(op==G_TRI2)++triangles;
            if(op!=G_DL_OTR_FILEPATH)continue;
            const auto* path=reinterpret_cast<const char*>(cmd->words.w1);
            if(!std::strcmp(path,ring)){++rings;assert(tint==0x646e78ff);}
            if(!std::strcmp(path,emblem)){++emblems;assert(tint==(0xc90029ff|uint32_t(badge[1])<<16));}
            if(std::strcmp(path,metal))continue;
            ++keys;assert(loaded&&tint==(uint32_t(steel[0])<<24|0xe0ecff));
            assert(std::abs(std::sqrt(active.xx*active.xx+active.yx*active.yx+active.zx*active.zx)-.125)<1e-6);
            const double anchors[4][2]={{24.5,24.5},{57,31},{37.5,61.75},{21,21.25}};
            const double pivot[]={-2.012,16.440,-4.871},a[]={.04550093073105725,-.30200617759954484,.9522194778486311};
            const double offset[]={24.87564391098553-pivot[0],3.198162796904046-pivot[1],-3.2210231606525594-pivot[2]};
            const double angle=(double(keys-1)-(count-1)*.5)*((owner==3?47:40)*3.14159265358979323846/180);
            const double cross[]={a[1]*offset[2]-a[2]*offset[1],a[2]*offset[0]-a[0]*offset[2],a[0]*offset[1]-a[1]*offset[0]};
            const double dot=a[0]*offset[0]+a[1]*offset[1]+a[2]*offset[2],translation[]={7,11,-13};
            double expected[3];for(int k=0;k<3;++k)expected[k]=translation[k]+pivot[k]+offset[k]*std::cos(angle)+cross[k]*std::sin(angle)+a[k]*dot*(1-std::cos(angle));
            const double x=anchors[owner][0],y=anchors[owner][1];
            assert(std::abs(active.xx*x+active.xy*y+active.xw-expected[0])<1e-4);
            assert(std::abs(active.yx*x+active.yy*y+active.yw-expected[1])<1e-4);
            assert(std::abs(active.zx*x+active.zy*y+active.zw-expected[2])<1e-4);
        }
        assert(keys==size_t(count)&&emblems==keys&&rings==1);
        assert(vertices==size_t(6*count)&&triangles==size_t(96*count)&&tint==0xffffffff);
    }
    const uint8_t white[]={255,255,255,255};
    for(int owner:{-1,4,999}) {
        Reset();assert(!ComboMmKeyRing::Draw(&play,owner,metal,emblem,ring,white,white,white));
        assert(!allocations&&graphics.polyOpa.p==opaque);
    }
    for(int scarce=0;scarce<6;++scarce) {
        Reset();if(scarce==0)available=false;if(scarce==1)graphics.polyOpa.d=opaque+1;
        if(scarce==2)graphics.polyXlu.d=translucent;if(scarce==3)graphics.polyXlu.d=translucent+1;
        if(scarce==4)graphics.overlay.d=overlay;if(scarce==5)graphics.overlay.d=overlay+1;
        assert(!ComboMmKeyRing::Draw(&play,3,metal,emblem,ring,white,white,white));
        assert(!allocations&&!models&&graphics.polyOpa.p==opaque&&!openDepth);
    }
    std::puts("PASS native ring renderer: counts, palettes, real matrices/GBI/setup, seams, arena and pose restore");
    return 0;
}
