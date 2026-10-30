#!/usr/bin/env python3
"""Compile production transport/layout and GL/D3D11 stencil code with deterministic API providers.

These are native CPU/state tests. They do not claim GPU driver or game runtime validation.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BACKENDS = ROOT / 'libultraship/src/fast/backends'
API = (ROOT / 'libultraship/include/fast/backends/gfx_rendering_api.h').read_text()


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        if source[end] == '{':
            depth += 1
        elif source[end] == '}':
            depth -= 1
        end += 1
    return source[start:end]


def run(name, source):
    with tempfile.TemporaryDirectory(prefix=f'cel-{name}-') as directory:
        cpp = Path(directory) / 'test.cpp'
        cpp.write_text(source)
        binary = Path(directory) / 'test'
        subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                        str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print(f'PASS {name}', flush=True)


header = ROOT / 'libultraship/include/fast/toon_shading.h'
toon = header.read_text()
# Use production mode values and ramp defaults, not a duplicate test enum.
enum = function(toon, 'enum class StencilMode') + ';'
constants = '\n'.join(line for line in toon.splitlines() if line.startswith('#define TOON_SHADING_DEFAULT_'))
layout = API[API.index('struct ShaderVertexLayout'):API.index('// A hash function')]
lighting_methods = '\n'.join(function(API, signature) for signature in
                            ['virtual void SetToonLighting(', 'virtual void SetToonRamp(', 'virtual void SetStencilMode('])
fields = API[API.index('    float mToonLightDir'):API.index('    bool mSrgbMode')]
common = '''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <algorithm>
using std::size_t;
'''+enum+'\n'+constants+'\n'+layout+'\n'
run('layout-and-neutral-transport', common + '''
struct Features {
    bool usedTextures[2]{};
    bool clamp[2][2]{};
    bool opt_fog = false, opt_grayscale = false, opt_toon = false, opt_alpha = false;
    uint8_t numInputs = 0;
};
struct Transport {
'''+lighting_methods+'\n'+fields+'''
};
int main() {
    Transport t;
    assert(t.mStencilMode == 0 && t.mToonDebug == 0);
    float d[3] = {1, 2, 3}, c[3] = {.1f, .2f, .3f}, a[3] = {.4f, .5f, .6f};
    t.SetToonLighting(d, c, a);
    assert(std::memcmp(t.mToonLightDir, d, sizeof(d)) == 0);
    assert(std::memcmp(t.mToonLightColor, c, sizeof(c)) == 0);
    assert(std::memcmp(t.mToonAmbient, a, sizeof(a)) == 0);
    t.SetToonRamp(.2f, .3f, .4f, .5f, 1);
    assert(t.mToonRampCenter == .2f && t.mToonRampSoftness == .3f);
    assert(t.mToonHighlightIntensity == .4f && t.mToonShadowIntensity == .5f && t.mToonDebug == 1);
    for (int invalid : {-1, 5, 8, 255}) { t.SetStencilMode(invalid); assert(t.mStencilMode == 0); }
    for (int mode = 0; mode <= 4; ++mode) { t.SetStencilMode(mode); assert(t.mStencilMode == mode); }
    size_t count = 0;
    for (unsigned flags = 0; flags < 1024; ++flags) {
        Features f;
        f.usedTextures[0] = flags & 1; f.usedTextures[1] = flags & 2;
        for (int tex = 0; tex < 2; ++tex)
            for (int dim = 0; dim < 2; ++dim) f.clamp[tex][dim] = flags & (4u << (tex * 2 + dim));
        f.opt_fog = flags & 64; f.opt_grayscale = flags & 128;
        f.opt_alpha = flags & 256; f.opt_toon = flags & 512;
        for (unsigned inputs = 0; inputs <= 4; ++inputs) {
            f.numInputs = inputs;
            auto actual = GetShaderVertexLayout(f);
            // Build a legacy template's physical fields with recognizable scalar values.
            std::vector<std::pair<size_t, size_t>> attributes;
            std::vector<float> packed;
            auto append = [&](size_t n) {
                attributes.push_back({packed.size(), n});
                for (size_t i = 0; i < n; ++i) packed.push_back(float(1000 + packed.size()));
            };
            append(4);
            for (int tex = 0; tex < 2; ++tex) if (f.usedTextures[tex]) {
                append(2); for (int dim = 0; dim < 2; ++dim) if (f.clamp[tex][dim]) append(1);
            }
            if (f.opt_fog) append(4);
            if (f.opt_grayscale) append(4);
            assert(actual.normalOffset == packed.size());
            for (unsigned i = 0; i < inputs; ++i) append(f.opt_alpha ? 4 : 3);
            auto cpu = packed;
            if (f.opt_toon) cpu.insert(cpu.begin() + actual.normalOffset, {-.25f, .5f, .75f});
            assert(actual.numFloats == cpu.size() && actual.numFloats <= 40);
            // The shader has no normal attribute: descriptor indices stay unchanged,
            // offsets skip the CPU normal and all UV/fog/input scalars remain exact.
            for (auto [offset, size] : attributes) {
                auto input = GetShaderVertexAttributeOffset(offset, actual, f.opt_toon);
                for (size_t n = 0; n < size; ++n) assert(cpu[input + n] == packed[offset + n]);
                assert(GetShaderVertexAttributeOffset(offset, actual, false) == offset);
            }
            ++count;
        }
    }
    assert(count == 5120);
}
''')

ogl = (BACKENDS / 'gfx_opengl.cpp').read_text()
gl_methods = '\n'.join(function(ogl, signature) for signature in [
    'void GfxRenderingAPIOGL::DrawTriangles(', 'void GfxRenderingAPIOGL::StartFrame(',
    'void GfxRenderingAPIOGL::ClearFramebuffer(', 'void GfxRenderingAPIOGL::ClearDepthRegion('])
if 'void GfxRenderingAPIOGL::DrawTrianglesInternal(' in ogl:
    gl_methods += '\n' + function(ogl, 'void GfxRenderingAPIOGL::DrawTrianglesInternal(')
run('opengl-stencil-and-clear', common + r'''
using GLint = int; using GLfloat = float; using GLboolean = bool;
constexpr int GL_TRUE=1, GL_FALSE=0, GL_DEPTH_TEST=2, GL_STENCIL_TEST=3,
 GL_LEQUAL=4, GL_LESS=5, GL_ALWAYS=6, GL_NOTEQUAL=7, GL_KEEP=8, GL_ZERO=9,
 GL_BACK=10, GL_FRONT=11, GL_INCR_WRAP=12, GL_DECR_WRAP=13, GL_INCR=14, GL_DECR=15,
 GL_POLYGON_OFFSET_FILL=16, GL_ARRAY_BUFFER=17, GL_STREAM_DRAW=18, GL_TRIANGLES=19,
 GL_SCISSOR_TEST=20, GL_SCISSOR_BOX=21, GL_COLOR_BUFFER_BIT=0x100, GL_DEPTH_BUFFER_BIT=0x200, GL_STENCIL_BUFFER_BIT=0x400;
#define CVAR_Z_FIGHTING_MODE "z-fighting"
struct Vars { int GetInteger(const char*, int n) { return n; } };
namespace Ship { struct Context { static Context* GetRawInstance() { static Context x; return &x; }
 Vars* GetConsoleVariables() { static Vars v; return &v; } }; }
struct GLState {
 bool stencil=false, depth=false, scissor=false; unsigned mask=0, clearMask=0;
 int func=GL_ALWAYS, reference=0, frontFail=GL_KEEP, backFail=GL_KEEP, frontPass=GL_KEEP, backPass=GL_KEEP;
 int clearValue=99, depthFunc=0, transitions=0; bool depthMask=false;
} gl;
void glEnable(int cap) { if(cap==GL_STENCIL_TEST) {gl.stencil=true; ++gl.transitions;}
 if(cap==GL_DEPTH_TEST) {gl.depth=true;} if(cap==GL_SCISSOR_TEST) {gl.scissor=true;} }
void glDisable(int cap) { if(cap==GL_STENCIL_TEST) {gl.stencil=false; ++gl.transitions;}
 if(cap==GL_DEPTH_TEST) {gl.depth=false;} if(cap==GL_SCISSOR_TEST) {gl.scissor=false;} }
void glDepthMask(bool b) {gl.depthMask=b;} void glDepthFunc(int f) {gl.depthFunc=f;}
void glStencilMask(unsigned m) {gl.mask=m;} void glStencilFunc(int f,int r,unsigned) {gl.func=f;gl.reference=r;}
void glStencilOp(int,int f,int p) {gl.frontFail=gl.backFail=f;gl.frontPass=gl.backPass=p;}
void glStencilOpSeparate(int face,int,int f,int p) {
 if(face==GL_FRONT) {gl.frontFail=f;gl.frontPass=p;} else {gl.backFail=f;gl.backPass=p;} }
void glPolygonOffset(float,float) {} void glBufferData(int,size_t,float*,int) {} void glDrawArrays(int,int,size_t) {}
void glClearStencil(int i) {gl.clearValue=i;} void glClearColor(float,float,float,float) {}
bool glIsEnabled(int) {return gl.scissor;} void glGetIntegerv(int,int* out) {std::fill(out,out+4,0);}
void glClear(unsigned mask) {gl.clearMask=mask;} void glScissor(int,int,int,int) {}
struct GfxRenderingAPIOGL {
''' + lighting_methods + '\n'+ fields + r'''
 int mLastStencilMode=-1, mLastScissorEnabled=-1;
 unsigned mFrameCount=0; size_t mCurrentFrameBuffer=0;
 struct FB {int height=240;}; std::vector<FB> mFrameBuffers{FB{}};
 void SetPerDrawUniforms() {}
 void DrawTriangles(float*,size_t,size_t); void StartFrame();
 void DrawTrianglesInternal(float*,size_t,size_t,const uint16_t*);
 void SubmitIndexedTriangles(const uint16_t*,size_t) {assert(false && "legacy stencil fixture uses direct draws");}
 void ClearFramebuffer(bool,bool); void ClearDepthRegion(int,int,int,int);
};
''' + gl_methods + r'''
int main() {
 GfxRenderingAPIOGL g; float data[12]{};
 auto draw=[&] {g.DrawTriangles(data,12,1);};
 g.mCurrentDepthTest=1; g.mCurrentDepthMask=0;
 g.StartFrame(); draw(); assert(!gl.stencil && gl.depth && gl.depthFunc==GL_LESS);
 int transitions=gl.transitions; draw(); assert(gl.transitions==transitions);
 g.SetStencilMode((int)StencilMode::VolumeIncrDecr); draw();
 assert(gl.stencil && gl.mask==255 && gl.func==GL_ALWAYS && gl.reference==0);
 assert(gl.frontFail==GL_DECR_WRAP && gl.backFail==GL_INCR_WRAP);
 assert(gl.frontPass==GL_KEEP && gl.backPass==GL_KEEP);
 // A front-first z-fail on a zero stencil value must wrap, then cancel with its back face.
 unsigned stencil=0; stencil=(stencil-1)&255; assert(stencil!=0); stencil=(stencil+1)&255; assert(stencil==0);
 g.SetStencilMode((int)StencilMode::Composite); draw();
 assert(gl.func==GL_NOTEQUAL && gl.frontPass==GL_ZERO && gl.backPass==GL_ZERO);
 assert(gl.frontFail==GL_KEEP && gl.backFail==GL_KEEP);
 g.SetStencilMode((int)StencilMode::VolumeIncr); draw(); assert(gl.frontFail==GL_INCR && gl.backFail==GL_INCR);
 g.SetStencilMode((int)StencilMode::VolumeDecr); draw(); assert(gl.frontFail==GL_DECR && gl.backFail==GL_DECR);
 g.mCurrentZmodeDecal=1; draw(); assert(gl.depthFunc==GL_LEQUAL);
 g.ClearFramebuffer(false,true); assert(gl.clearMask==(GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT));
 assert(gl.clearValue==0 && gl.mask==255 && !gl.depthMask);
 g.ClearFramebuffer(true,false); assert(gl.clearMask==GL_COLOR_BUFFER_BIT);
 g.ClearDepthRegion(1,2,3,4); assert(gl.clearMask==(GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT));
 // OoT ends in a non-neutral mode; an MM frame with no new commands still returns to Off.
 g.StartFrame(); draw(); assert(g.mStencilMode==0 && !gl.stencil);
 g.SetStencilMode(99); draw(); assert(!gl.stencil);
}
''')

dx = (BACKENDS / 'gfx_direct3d11.cpp').read_text()
dx_internal = 'void GfxRenderingAPIDX11::DrawTrianglesInternal('
state = function(dx, dx_internal if dx_internal in dx else 'void GfxRenderingAPIDX11::DrawTriangles(')
state = state[:state.index('    if (mLastZmodeDecal != mCurrentZmodeDecal)')] + '\n}\n'
if dx_internal in dx:
    state = function(dx, 'void GfxRenderingAPIDX11::DrawTriangles(') + '\n' + state
run('direct3d-stencil-state-cache', common + r'''
constexpr int D3D11_DEPTH_WRITE_MASK_ALL=1, D3D11_DEPTH_WRITE_MASK_ZERO=0,
 D3D11_COMPARISON_LESS_EQUAL=2, D3D11_COMPARISON_LESS=3, D3D11_COMPARISON_ALWAYS=4,
 D3D11_COMPARISON_NOT_EQUAL=5, D3D11_STENCIL_OP_KEEP=6, D3D11_STENCIL_OP_INCR_SAT=7,
 D3D11_STENCIL_OP_DECR_SAT=8, D3D11_STENCIL_OP_INCR=9, D3D11_STENCIL_OP_DECR=10, D3D11_STENCIL_OP_ZERO=11;
struct D3D11_DEPTH_STENCILOP_DESC {int StencilFailOp,StencilDepthFailOp,StencilPassOp,StencilFunc;};
struct D3D11_DEPTH_STENCIL_DESC {int DepthEnable,DepthWriteMask,DepthFunc,StencilEnable;
 unsigned StencilReadMask,StencilWriteMask; D3D11_DEPTH_STENCILOP_DESC FrontFace,BackFace;};
#define ZeroMemory(p,n) std::memset(p,0,n)
void ThrowIfFailed(int result) {assert(result==0);}
struct State {D3D11_DEPTH_STENCIL_DESC desc;};
struct Ptr {State* value=nullptr; State* Get() {return value;} State** GetAddressOf() {return &value;}
 bool operator==(std::nullptr_t) {return value==nullptr;} ~Ptr() {delete value;} };
struct Device {int creates=0; int CreateDepthStencilState(D3D11_DEPTH_STENCIL_DESC* d, State** out) {
 ++creates; *out=new State{*d}; return 0;}};
struct Context {State* bound=nullptr; int binds=0; void OMSetDepthStencilState(State* p,int ref) {
 assert(ref==0); bound=p; ++binds; }};
struct GfxRenderingAPIDX11 {
''' + lighting_methods+'\n'+fields+r'''
 int mLastStencilMode=-1; Ptr mDepthStencilStates[64];
 Device device; Context context; Device* mDevice=&device; Context* mContext=&context;
 void DrawTriangles(float*,size_t,size_t);
 void DrawTrianglesInternal(float*,size_t,size_t,const uint16_t*);
};
'''+state+r'''
int main() {
 GfxRenderingAPIDX11 g; float data[1]{};
 auto draw=[&] {g.DrawTriangles(data,0,0); g.mLastZmodeDecal=g.mCurrentZmodeDecal;};
 draw(); assert(!g.context.bound->desc.StencilEnable);
 int binds=g.context.binds; draw(); assert(g.context.binds==binds);
 g.SetStencilMode((int)StencilMode::VolumeIncrDecr); draw();
 auto d=g.context.bound->desc;
 assert(d.StencilEnable && d.StencilReadMask==255 && d.StencilWriteMask==255);
 assert(d.FrontFace.StencilDepthFailOp==D3D11_STENCIL_OP_DECR);
 assert(d.BackFace.StencilDepthFailOp==D3D11_STENCIL_OP_INCR);
 g.SetStencilMode((int)StencilMode::Composite); draw(); d=g.context.bound->desc;
 assert(d.FrontFace.StencilFunc==D3D11_COMPARISON_NOT_EQUAL);
 assert(d.FrontFace.StencilPassOp==D3D11_STENCIL_OP_ZERO && d.BackFace.StencilPassOp==D3D11_STENCIL_OP_ZERO);
 int creates=g.device.creates;
 g.SetStencilMode((int)StencilMode::VolumeIncrDecr); draw(); assert(g.device.creates==creates);
 g.SetStencilMode((int)StencilMode::Off); draw(); assert(g.device.creates==creates && !g.context.bound->desc.StencilEnable);
 for(int depth=0;depth<2;++depth) for(int write=0;write<2;++write) for(int decal=0;decal<2;++decal)
  for(int mode=0;mode<=4;++mode) {
   g.mCurrentDepthTest=depth;g.mCurrentDepthMask=write;g.mCurrentZmodeDecal=decal;g.SetStencilMode(mode);draw();
   d=g.context.bound->desc;
   assert(d.DepthEnable==(depth||write));assert(d.DepthWriteMask==write);
   assert(d.DepthFunc==(depth?(decal?D3D11_COMPARISON_LESS_EQUAL:D3D11_COMPARISON_LESS):D3D11_COMPARISON_ALWAYS));
   assert(d.StencilEnable==(mode!=0));
  }
 assert(g.device.creates==40);
}
''')


metal = (BACKENDS / 'gfx_metal.cpp').read_text()
mstate = function(metal, 'void GfxRenderingAPIMetal::DrawTriangles(')
mstate = mstate[:mstate.index('    if (current_framebuffer.mLastZmodeDecal != mCurrentZmodeDecal)')] + '\n}\n'
mstate = mstate.replace('    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();\n', '')
mstate = mstate.replace('    bool textures_changed = false;\n', '')
run('metal-stencil-state-cache', common + r'''
namespace MTL {
enum {CompareFunctionAlways=1, CompareFunctionLess=2, CompareFunctionLessEqual=3, CompareFunctionNotEqual=4,
 StencilOperationKeep=5, StencilOperationZero=6, StencilOperationIncrementClamp=7, StencilOperationDecrementClamp=8,
 StencilOperationIncrementWrap=9, StencilOperationDecrementWrap=10};
struct StencilDescriptor {
 unsigned read=0,write=0; int fail=0,depthFail=0,pass=0,func=0;
 static StencilDescriptor* alloc() {return new StencilDescriptor;}
 StencilDescriptor* init() {return this;} void release() {delete this;}
 void setReadMask(unsigned x) {read=x;} void setWriteMask(unsigned x) {write=x;}
 void setStencilFailureOperation(int x) {fail=x;} void setDepthFailureOperation(int x) {depthFail=x;}
 void setDepthStencilPassOperation(int x) {pass=x;} void setStencilCompareFunction(int x) {func=x;}
};
struct DepthStencilDescriptor {
 bool write=false,stencil=false; int depth=0; StencilDescriptor front,back;
 static DepthStencilDescriptor* alloc() {return new DepthStencilDescriptor;}
 DepthStencilDescriptor* init() {return this;} void release() {delete this;}
 void setDepthWriteEnabled(bool x) {write=x;} void setDepthCompareFunction(int x) {depth=x;}
 void setFrontFaceStencil(StencilDescriptor* x) {stencil=true;front=*x;}
 void setBackFaceStencil(StencilDescriptor* x) {stencil=true;back=*x;}
};
struct DepthStencilState {DepthStencilDescriptor desc;};
struct Device {int creates=0; DepthStencilState* newDepthStencilState(DepthStencilDescriptor* d) {
 ++creates;return new DepthStencilState{*d};}};
struct Encoder {DepthStencilState* state=nullptr; int binds=0;
 void setDepthStencilState(DepthStencilState* s) {state=s;++binds;}
 void setStencilReferenceValue(int x) {assert(x==0);}
};
}
struct GfxRenderingAPIMetal {
''' + lighting_methods+'\n'+fields+r'''
 struct FB {int mLastDepthTest=-1,mLastDepthMask=-1,mLastZmodeDecal=-1,mLastStencilMode=-1;
 MTL::Encoder encoder; MTL::Encoder* mCommandEncoder=&encoder;};
 FB mFramebuffers[1]; size_t mCurrentFramebuffer=0;
 MTL::DepthStencilState* mDepthStencilStates[64]{}; MTL::Device device; MTL::Device* mDevice=&device;
 ~GfxRenderingAPIMetal() {for(auto* s:mDepthStencilStates) delete s;}
 void DrawTriangles(float*,size_t,size_t);
};
'''+mstate+r'''
int main() {
 GfxRenderingAPIMetal g; float data[1]{};
 auto& fb=g.mFramebuffers[0];
 auto draw=[&] {g.DrawTriangles(data,0,0);fb.mLastZmodeDecal=g.mCurrentZmodeDecal;};
 draw();assert(!fb.encoder.state->desc.stencil); int binds=fb.encoder.binds;
 draw();assert(fb.encoder.binds==binds);
 g.SetStencilMode((int)StencilMode::VolumeIncrDecr);draw();auto d=fb.encoder.state->desc;
 assert(d.stencil&&d.front.read==255&&d.back.write==255);
 assert(d.front.depthFail==MTL::StencilOperationDecrementWrap&&d.back.depthFail==MTL::StencilOperationIncrementWrap);
 assert(d.front.fail==MTL::StencilOperationKeep&&d.front.pass==MTL::StencilOperationKeep);
 g.SetStencilMode((int)StencilMode::Composite);draw();d=fb.encoder.state->desc;
 assert(d.front.func==MTL::CompareFunctionNotEqual&&d.front.pass==MTL::StencilOperationZero);
 assert(d.back.func==MTL::CompareFunctionNotEqual&&d.back.pass==MTL::StencilOperationZero);
 int creates=g.device.creates;
 g.SetStencilMode((int)StencilMode::VolumeIncrDecr);draw();assert(g.device.creates==creates);
 g.SetStencilMode((int)StencilMode::Off);draw();assert(g.device.creates==creates&&!fb.encoder.state->desc.stencil);
 for(int depth=0;depth<2;++depth) for(int write=0;write<2;++write) for(int decal=0;decal<2;++decal)
  for(int mode=0;mode<=4;++mode) {
   g.mCurrentDepthTest=depth;g.mCurrentDepthMask=write;g.mCurrentZmodeDecal=decal;g.SetStencilMode(mode);draw();
   d=fb.encoder.state->desc;
   assert(d.write==bool(write));assert(d.stencil==(mode!=0));
   assert(d.depth==(depth?(decal?MTL::CompareFunctionLessEqual:MTL::CompareFunctionLess):MTL::CompareFunctionAlways));
  }
 assert(g.device.creates==40);
}
''')


metal_clear = function(metal, 'void GfxRenderingAPIMetal::ClearFramebuffer(')
run('metal-encoder-stencil-clear-lifetime', common + r'''
#include <string>
namespace fmt {template<class... A> std::string format(const char*,A...) {return {};}}
namespace NS {constexpr int UTF8StringEncoding=0; struct String {static void* string(const char*,int) {return nullptr;}};}
namespace MTL {
enum LoadAction {LoadActionDontCare,LoadActionLoad,LoadActionClear}; constexpr int DepthClipModeClamp=1;
struct RenderPassColorAttachmentDescriptor {LoadAction load=LoadActionLoad;
 LoadAction loadAction() {return load;} void setLoadAction(LoadAction x) {load=x;}};
using RenderPassDepthAttachmentDescriptor=RenderPassColorAttachmentDescriptor;
struct StencilAttachment : RenderPassColorAttachmentDescriptor {unsigned clear=77;void setClearStencil(unsigned x) {clear=x;}};
struct Colors {RenderPassColorAttachmentDescriptor color;auto* object(int) {return &color;}};
struct Pass {Colors colors;RenderPassDepthAttachmentDescriptor depth;StencilAttachment stencil;
 Colors* colorAttachments() {return &colors;} auto* depthAttachment() {return &depth;}
 auto* stencilAttachment() {return &stencil;}};
struct Viewport {};struct ScissorRect {};
struct Encoder {int ends=0;void endEncoding() {++ends;} void setLabel(void*) {} void setDepthClipMode(int) {}
 void setViewport(const Viewport&) {} void setScissorRect(const ScissorRect&) {}};
struct CommandBuffer {Encoder encoder;int begins=0; LoadAction color,depth,stencil;unsigned clear;
 Encoder* renderCommandEncoder(Pass* p) {++begins;color=p->colors.color.load;depth=p->depth.load;
 stencil=p->stencil.load;clear=p->stencil.clear;return &encoder;}};
}
constexpr int SHADER_MAX_TEXTURES=6;
struct GfxRenderingAPIMetal {
 struct FB {MTL::Pass pass;MTL::CommandBuffer buffer;MTL::Encoder initial;
 MTL::Pass* mRenderPassDescriptor=&pass;MTL::CommandBuffer* mCommandBuffer=&buffer;
 MTL::Encoder* mCommandEncoder=&initial;bool mHasDepthBuffer=true;
 MTL::Viewport viewport;MTL::ScissorRect scissor;MTL::Viewport* mViewport=&viewport;MTL::ScissorRect* mScissorRect=&scissor;
 bool mHasBoundVertexShader=true,mHasBoundFragShader=true;void* mLastShaderProgram=(void*)1;
 void* mLastBoundTextures[6]{};void* mLastBoundSamplers[6]{};
 int mLastDepthTest=1,mLastDepthMask=1,mLastZmodeDecal=1,mLastStencilMode=4;
 } mFramebuffers[1];size_t mCurrentFramebuffer=0;
 void ClearFramebuffer(bool,bool);
};
'''+metal_clear+r'''
int main() {
 GfxRenderingAPIMetal g;auto& fb=g.mFramebuffers[0];
 g.ClearFramebuffer(false,false);assert(fb.buffer.begins==0);
 g.ClearFramebuffer(true,false);
 assert(fb.buffer.color==MTL::LoadActionClear&&fb.buffer.depth==MTL::LoadActionLoad);
 assert(fb.buffer.stencil==MTL::LoadActionLoad); // color clear preserves a pending volume mask
 assert(fb.pass.colors.color.load==MTL::LoadActionLoad&&fb.pass.stencil.load==MTL::LoadActionLoad);
 assert(fb.mLastStencilMode==-1&&!fb.mHasBoundVertexShader&&!fb.mHasBoundFragShader&&fb.mLastShaderProgram==nullptr);
 g.ClearFramebuffer(false,true);
 assert(fb.buffer.color==MTL::LoadActionLoad&&fb.buffer.depth==MTL::LoadActionClear);
 assert(fb.buffer.stencil==MTL::LoadActionClear&&fb.buffer.clear==0);
 assert(fb.pass.depth.load==MTL::LoadActionLoad&&fb.pass.stencil.load==MTL::LoadActionLoad);
 fb.mHasDepthBuffer=false;g.ClearFramebuffer(false,true);
 assert(fb.buffer.stencil==MTL::LoadActionLoad&&fb.buffer.depth==MTL::LoadActionLoad);
}
''')

# These source contracts complement executable state tests where platform SDKs are unavailable.
for name in ['gfx_opengl.h', 'gfx_direct3d_common.h', 'gfx_metal.h']:
    text = (ROOT/'libultraship/include/fast/backends'/name).read_text()
    assert 'std::pair<uint64_t, uint32_t>' not in text, name+' truncates the toon shader key'
assert 'cbuffer PerPrimDepthCB : register(b2)' in (ROOT/'libultraship/src/fast/shaders/directx/default.shader.hlsl').read_text()
assert 'cbuffer PerToonCB : register(b3)' in (ROOT/'libultraship/src/fast/shaders/directx/default.shader.hlsl').read_text()
assert 'texture_desc.Format = DXGI_FORMAT_R24G8_TYPELESS' in dx
assert 'view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT' in dx
assert 'srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS' in dx
assert 'D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL' in dx
for backend in ['gfx_opengl.cpp', 'gfx_direct3d11.cpp', 'gfx_metal_shader.cpp']:
    assert '{ "o_toon", cc_features.opt_toon }' in (BACKENDS/backend).read_text()
    assert 'LoadResource(path, true, init)' in (BACKENDS/backend).read_text()
print('PASS shader-key, primitive-depth register and depth/stencil format source contracts')
