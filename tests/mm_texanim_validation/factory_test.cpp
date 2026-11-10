// Production MM importer, parameter structs and LUS binary readers; logging is the only stub.
#include "2s2h/resource/importer/TextureAnimationFactory.h"
#include "2s2h/resource/type/TextureAnimation.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); std::exit(1); } } while (0)
// Extracted unchanged native interpolation body exercises the final-frame read.
using s32 = int32_t; using u16 = uint16_t; using f32 = float;
using AnimatedMatColorParams = SOH::AnimatedMatColorParams;
using F3DPrimColor = SOH::F3DPrimColor; using F3DEnvColor = SOH::F3DEnvColor;
struct PlayState {};
static uint32_t sMatAnimStep;
static F3DPrimColor drawnPrim;
static F3DEnvColor drawnEnv;
template <class T> static T* Lib_SegmentedToVirtual(T* pointer) { return pointer; }
static void AnimatedMat_SetColor(PlayState*, int, F3DPrimColor* prim, F3DEnvColor* env) {
    drawnPrim = *prim; if (env) drawnEnv = *env;
}
#include "native_lerp.inc"
using Bytes = std::vector<char>;
static void put(Bytes& out,unsigned value,unsigned bytes) {
    for(unsigned i=0;i<bytes;++i) out.push_back(static_cast<char>(value>>(8*i)));
}
static Bytes color(int type,int length,int count,std::vector<unsigned> keys,int primCount,int envCount) {
    Bytes b;put(b,1,4);put(b,-1,1);put(b,type,2);put(b,length,2);put(b,count,2);put(b,keys.size(),2);
    for(auto k:keys) put(b,k,2);
    put(b,primCount,2);for(int i=0;i<primCount;++i)for(int c=0;c<5;++c)put(b,20+i+c,1);
    put(b,envCount,2);for(int i=0;i<envCount;++i)for(int c=0;c<4;++c)put(b,40+i+c,1);
    return b;
}
static Bytes cycle(int length,int textures,std::vector<unsigned> indices) {
    Bytes b;put(b,1,4);put(b,-1,1);put(b,5,2);put(b,length,2);put(b,textures,4);
    for(int i=0;i<textures;++i){std::string path="objects/control/tex"+std::to_string(i);put(b,path.size(),4);b.insert(b.end(),path.begin(),path.end());}
    for(auto i:indices)put(b,i,1);return b;
}
static std::shared_ptr<SOH::TextureAnimation> load(Bytes bytes,size_t offset=0) {
    auto file=std::make_shared<Ship::File>();file->Buffer=std::make_shared<Bytes>(std::move(bytes));file->BufferOffset=offset;
    auto reader=std::make_shared<Ship::BinaryReader>(std::make_shared<Ship::MemoryStream>(file->Buffer,offset));
    reader->SetEndianness(Ship::Endianness::Little);file->Reader=reader;
    auto init=std::make_shared<Ship::ResourceInitData>();init->Path="control/texanim";init->Format=RESOURCE_FORMAT_BINARY;
    SOH::ResourceFactoryBinaryTextureAnimationV0 factory;
    try{return std::dynamic_pointer_cast<SOH::TextureAnimation>(factory.ReadResource(file,init));}
    catch(...){CHECK(false && "factory must contain malformed-reader exceptions");return nullptr;}
}
static void reject(const Bytes& b) {CHECK(!load(b));}
int main(int argc,char** argv) {
    // Reviewer reproduction: declared two Lagrange keys/colors, allocated one each.
    reject(color(4,20,2,{0},1,0));
    reject(color(4,20,2,{0,19},1,0));
    reject(color(4,20,2,{0,19},2,1));
    reject(color(4,20,2,{0,0},2,0));
    reject(color(4,20,0,{},0,0));
    reject(color(4,0,2,{0,19},2,0));
    std::vector<unsigned> many(51);for(unsigned i=0;i<many.size();++i)many[i]=i;
    reject(color(4,51,51,many,51,0));
    reject(color(2,4,0,{},3,0));reject(color(2,4,0,{},4,3));reject(color(2,0,0,{},0,0));
    reject(color(3,20,2,{0},2,0));reject(color(3,20,2,{0,19},1,0));
    reject(color(3,20,2,{0,19},2,1));reject(color(3,20,2,{19,0},2,0));
    reject(color(3,20,2,{0,0},2,0));reject(color(3,20,1,{0},1,0));
    reject(cycle(2,1,{0,1}));reject(cycle(0,1,{}));reject(cycle(2,0,{0,0}));
    auto badEnd=color(2,1,0,{},1,0);badEnd[4]=1;reject(badEnd);
    badEnd[4]=0;reject(badEnd);badEnd[4]=-9;reject(badEnd);
    auto badType=color(2,1,0,{},1,0);badType[5]=7;reject(badType);
    auto nativeSegment=color(2,1,0,{},1,0);nativeSegment[4]=-8;CHECK(load(nativeSegment));
    Bytes empty;put(empty,1,4);put(empty,0,1);put(empty,6,2);CHECK(load(empty));
    std::puts("PASS malformed color sizes, duplicate/descending keys, zero length, interpolation limit and cycle indices");
    const auto controls=std::vector<Bytes>{color(2,4,0,{},4,0),color(2,4,0,{},4,4),color(3,20,2,{0,19},2,2),color(4,20,2,{0,19},2,0),color(4,20,2,{19,0},2,2),cycle(3,2,{0,1,0})};
    for(const auto& bytes:controls) {
        CHECK(load(bytes));
        // Every truncation must reject before the underlying MemoryStream memcpy can overread.
        for(size_t n=0;n<bytes.size();++n)reject(Bytes(bytes.begin(),bytes.begin()+n));
    }
    // Native type3 search can step to count at its final authored key. The factory
    // supplies a last-color sentinel without changing authored length/count.
    auto lerp=load(color(3,20,2,{0,19},2,2));auto* p=static_cast<SOH::AnimatedMatColorParams*>(lerp->anims[0].params);
    CHECK(p->keyFrameLength==20 && p->keyFrameCount==2 && p->keyFrames[2]==20);
    CHECK(p->primColors[2].r==p->primColors[1].r && p->envColors[2].r==p->envColors[1].r);
    PlayState play;
    for (sMatAnimStep=0;sMatAnimStep<20;++sMatAnimStep) AnimatedMat_DrawColorLerp(&play,8,p);
    CHECK(drawnPrim.r==p->primColors[1].r && drawnEnv.r==p->envColors[1].r);
    // An authored extra terminal interval must retain its colors and keys.
    auto extra=load(color(3,20,2,{0,19,20},3,3));
    auto* ep=static_cast<SOH::AnimatedMatColorParams*>(extra->anims[0].params);
    CHECK(ep->keyFrames[2]==20 && ep->primColors[2].r==22 && ep->envColors[2].r==42);
    reject(color(3,20,2,{0,19,19},3,3));reject(color(3,20,2,{0,19,20},2,2));
    // No sentinel is needed when the last key already lies beyond reachable frames.
    auto boundary=load(color(3,65535,2,{0,65535},2,0));
    auto* bp=static_cast<SOH::AnimatedMatColorParams*>(boundary->anims[0].params);
    sMatAnimStep=65534;AnimatedMat_DrawColorLerp(&play,8,bp);
    CHECK(bp->keyFrameLength==65535 && bp->keyFrames[1]==65535);
    Bytes two=controls[2];two[0]=2;Bytes second=controls.back();two.insert(two.end(),second.begin()+4,second.end());
    // First entry becomes nonterminal; truncated second entry must free the completed first.
    two[4]=1;for(size_t n=controls[2].size();n<two.size();++n)reject(Bytes(two.begin(),two.begin()+n));
    CHECK(load(two));
    std::puts("PASS valid controls, native LERP final-frame sentinel, every-byte truncation and multi-entry cleanup");
    for(int i=1;i<argc;++i) {
        std::ifstream in(argv[i],std::ios::binary);CHECK(in.good());Bytes bytes{std::istreambuf_iterator<char>(in),{}};
        auto native=load(std::move(bytes),64);CHECK(native);
        for (auto& material:native->anims) if (material.type==3) {
            auto* cp=static_cast<SOH::AnimatedMatColorParams*>(material.params);
            for (sMatAnimStep=0;sMatAnimStep<cp->keyFrameLength;++sMatAnimStep)
                AnimatedMat_DrawColorLerp(&play,8,cp);
        }
    }
    std::printf("PASS %d actual native material controls\n",argc-1);
}
