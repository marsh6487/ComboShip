#!/usr/bin/env python3
"""Expand all three production shader templates with the host-pinned Prism processor.

Usage: python3 run_shader_tests.py /path/to/prism-processor [output-directory]
The Prism checkout must match libultraship's configured dependency. Native rendering
and platform shader compilation remain separate checks.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BACKENDS = ROOT/'libultraship/src/fast/backends'
PRISM = Path(sys.argv[1]).resolve()


def function(source, signature):
    start = source.index(signature)
    at = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[at] == '{') - (source[at] == '}')
        at += 1
    return source[start:at]


def context(source, signature):
    text = function(source, signature)
    at = text.index('    prism::ContextItems ')
    return text[at:text.index('    processor.populate(', at)]


gl = (BACKENDS/'gfx_opengl.cpp').read_text()
dx = (BACKENDS/'gfx_direct3d11.cpp').read_text()
metal = (BACKENDS/'gfx_metal_shader.cpp').read_text()
interpreter = (ROOT/'libultraship/include/fast/interpreter.h').read_text()
api = (ROOT/'libultraship/include/fast/backends/gfx_rendering_api.h').read_text()
enum = function(interpreter, 'enum {\n    SHADER_0')+';'
features = function(interpreter, 'struct CCFeatures {')+';'
layout = api[api.index('struct ShaderVertexLayout'):api.index('// A hash function')]
source = r'''
#include <prism/processor.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <regex>
using namespace prism;
using std::size_t;
namespace Fast {enum FilteringMode {FILTER_THREE_POINT,FILTER_LINEAR,FILTER_NONE};}
using namespace Fast;
'''+enum+'\n'+features+'\n'+layout+r'''
std::string read(const std::filesystem::path& path) {std::ifstream f(path); assert(f.good());
 return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
std::string neutral(std::string x,bool isMetal) {
 x=std::regex_replace(x,std::regex(R"(//[^\n]*\n)"),"\n");
 if(isMetal) {auto at=x.find("struct DrawUniforms");assert(at!=std::string::npos);
 auto end=x.find("};",at);assert(end!=std::string::npos);x.erase(at,end+2-at);}
 return std::regex_replace(x,std::regex(R"(\s+)"),"");
}
'''
# Production combiner formula callbacks are used unchanged.
for name, text, helper_end, ctx, vs in [
    ('GL', gl, 'std::optional<std::string> opengl_include_fs',
     context(gl, 'std::string GfxRenderingAPIOGL::BuildFsShader('),
     context(gl, 'static std::string BuildVsShader(')),
    ('DX', dx, 'static size_t raw_numFloats',
     context(dx, 'std::string gfx_direct3d_common_build_shader('), None),
    ('Metal', metal, 'static int vertex_index',
     context(metal, 'MTL::VertexDescriptor* gfx_metal_build_shader('), None),
]:
    helpers = text[text.index('#define RAND_NOISE'):text.index(helper_end)]
    source += '\nnamespace '+name+' {\n'+helpers+'\n#undef RAND_NOISE\n'
    source += r'''
size_t numFloats,raw_numFloats,vertex_index;
prism::ContextTypes* UpdateFloats(prism::ContextTypes*,prism::ContextTypes* n) {
 numFloats+=std::get<int>(*n);return nullptr;}
prism::ContextTypes* update_raw_floats(prism::ContextTypes*,prism::ContextTypes* n) {
 raw_numFloats+=std::get<int>(*n);++vertex_index;return nullptr;}
prism::ContextTypes* get_vertex_index() {return new prism::ContextTypes{int(vertex_index)};}
std::string expand(const std::string& input,const CCFeatures& cc_features,bool vertex=false) {
 numFloats=4;raw_numFloats='''+('0' if name=='Metal' else '4')+r''';vertex_index=0;
 bool mSrgbMode=false,use_srgb=false,include_root_signature=false,three_point_filtering=true;
 int mCurrentFilterMode=FILTER_THREE_POINT;
 prism::Processor processor;
'''
    if vs:
        source += 'if(vertex) {\n'+vs+'\nprocessor.populate(mContext);\n} else {\n'+ctx+'\nprocessor.populate(mContext);\n}\n'
    else:
        source += ctx+'\nprocessor.populate('+('context' if name=='Metal' else 'mContext')+');\n'
    source += 'processor.load(input);return processor.process();\n}\n}\n'
source += r'''
int main(int argc,char** argv) {
 assert(argc==4);std::filesystem::path root=argv[1],baseline=argv[2],output=argv[3];
 std::filesystem::create_directories(output);
 std::string names[3]={"opengl/default.shader.glsl","directx/default.shader.hlsl","metal/default.shader.metal"};
 std::string current[3],old[3];
 for(int i=0;i<3;++i) {current[i]=read(root/"libultraship/src/fast/shaders"/names[i]);old[i]=read(baseline/names[i]);}
 size_t count=0;
 for(unsigned flags=0;flags<128;++flags) {
  CCFeatures f{};f.numInputs=flags%5;
  f.usedTextures[0]=flags&1;f.usedTextures[1]=flags&2;
  f.opt_fog=flags&4;f.opt_grayscale=flags&8;f.opt_alpha=flags&16;
  f.opt_prim_depth=flags&32;f.opt_2cyc=flags&64;
  f.opt_noise=flags&16;f.opt_texture_edge=flags&4;f.opt_alpha_threshold=flags&8;
  for(int t=0;t<2;++t) {
   f.clamp[t][0]=f.usedTextures[t]&&bool(flags&16);f.clamp[t][1]=f.usedTextures[t]&&bool(flags&32);
   f.used_masks[t]=f.usedTextures[t]&&bool(flags&64);f.used_blend[t]=f.used_masks[t]&&bool(flags&8);
   f.do_single[t][0]=f.do_single[t][1]=true;f.color_alpha_same[t]=true;
   for(int channel=0;channel<2;++channel) f.c[t][channel][3]=f.numInputs?SHADER_INPUT_1:SHADER_1;
  }
  for(int toon=0;toon<2;++toon) {
   f.opt_toon=toon;
   auto vertex=GL::expand(current[0],f,true);auto fragment=GL::expand(current[0],f);
   auto hlsl=DX::expand(current[1],f);auto msl=Metal::expand(current[2],f);
   const auto expected=GetShaderVertexLayout(f).numFloats;
   // GL count is overwritten only by the vertex expansion; fragment expansion does not evaluate update_floats.
   GL::expand(current[0],f,true);
   assert(GL::numFloats==expected&&DX::raw_numFloats==expected&&Metal::raw_numFloats==expected);
   assert((vertex.find("aNormal")!=std::string::npos)==bool(toon));
   assert((fragment.find("toonNL")!=std::string::npos)==bool(toon));
   assert((hlsl.find("cbuffer PerToonCB : register(b3)")!=std::string::npos)==bool(toon));
   assert((msl.find("float3 normal")!=std::string::npos)==bool(toon));
   if(!toon) {
    assert(neutral(vertex,false)==neutral(GL::expand(old[0],f,true),false));
    assert(neutral(fragment,false)==neutral(GL::expand(old[0],f),false));
    assert(neutral(hlsl,false)==neutral(DX::expand(old[1],f),false));
    assert(neutral(msl,true)==neutral(Metal::expand(old[2],f),true));
   }
   std::string id=std::to_string(flags)+"-"+std::to_string(toon);
   std::ofstream(output/(id+".vert"))<<vertex;
   std::ofstream(output/(id+".frag"))<<fragment;
   ++count;
  }
 }
 std::cout<<"PASS "<<count<<" Prism variants per backend; neutral shader output matches host baseline\n";
}
'''
with tempfile.TemporaryDirectory(prefix='cel-prism-') as temp:
    directory = Path(temp)
    baseline = directory/'baseline'
    for name in ['opengl/default.shader.glsl', 'directx/default.shader.hlsl', 'metal/default.shader.metal']:
        path = baseline/name
        path.parent.mkdir(parents=True, exist_ok=True)
        result = subprocess.run(['git', 'show', 'badc4e09a5176b5d423926eed21d5d19a67ee87e:libultraship/src/fast/shaders/'+name],
                                cwd=ROOT, env={**os.environ, 'GIT_NO_LAZY_FETCH': '1'}, check=True, capture_output=True)
        path.write_bytes(result.stdout)
    stub = directory/'include/spdlog/spdlog.h'
    stub.parent.mkdir(parents=True)
    stub.write_text('#pragma once\n#define SPDLOG_DEBUG(...) ((void)0)\n')
    cpp = directory/'test.cpp'
    cpp.write_text(source)
    invoke = directory/'invoke.o'
    subprocess.run([os.environ.get('CC', 'gcc'), '-c', str(PRISM/'src/prism/utils/invoke.c'), '-o', str(invoke)], check=True)
    exe = directory/'test'
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-DUSE_OPENGLES', '-w',
                    '-I'+str(PRISM/'src'), '-I'+str(directory/'include'), str(cpp),
                    *(str(PRISM/file) for file in ['src/prism/processor.cpp', 'src/prism/ast.cpp',
                                                  'src/prism/lexer.cpp', 'src/prism/utils/gv.cpp']),
                    str(invoke), '-o', str(exe)], check=True)
    output = Path(sys.argv[2]).resolve() if len(sys.argv)>2 else directory/'output'
    subprocess.run([str(exe), str(ROOT), str(baseline), str(output)], check=True)
