#!/usr/bin/env python3
"""Compile actual indexed backend entry/upload/draw methods against recording GPU APIs.

Only device/context/GL calls and old shader-state setup are substituted. These
tests check bytes, formats, counts, failure handling and the legacy entrypoint;
they are not Windows/OpenGL platform compilation or GPU/runtime proof.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^void ' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing indexed production function: ' + name)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                    lambda m: ' ' * len(m.group()), source)
    end, depth = match.end(), 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


COMMON = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>
using std::size_t;
constexpr size_t TRIANGLE_BATCH_MAX_TRIANGLES = 256;
constexpr size_t VBO_MAX_FLOATS_PER_VERTEX = 40;
'''

DX = r'''
using UINT=unsigned;
enum { D3D11_USAGE_DYNAMIC=1, D3D11_BIND_INDEX_BUFFER=2, D3D11_CPU_ACCESS_WRITE=4,
       D3D11_MAP_WRITE_DISCARD=8, DXGI_FORMAT_R16_UINT=16 };
struct D3D11_BUFFER_DESC {unsigned Usage, ByteWidth, BindFlags, CPUAccessFlags, MiscFlags;};
struct D3D11_MAPPED_SUBRESOURCE {void* pData;};
#define ZeroMemory(p,n) std::memset(p,0,n)
template<class... Args> void ThrowIfFailed(int result, Args...) {
    if(result != 0) throw std::runtime_error("GPU failure");
}
struct Buffer {std::vector<uint8_t> bytes;};
struct Ptr {Buffer* p=nullptr; Buffer* Get(){return p;} Buffer** GetAddressOf(){return &p;}};
struct Device {
    D3D11_BUFFER_DESC desc;
    std::vector<std::unique_ptr<Buffer>> owned;
    int CreateBuffer(const D3D11_BUFFER_DESC* d, void*, Buffer** out) {
        desc=*d; owned.push_back(std::make_unique<Buffer>());
        owned.back()->bytes.resize(d->ByteWidth); *out=owned.back().get(); return 0;
    }
};
struct Context {
    bool failMap=false; unsigned draws=0, indexedDraws=0, maps=0, unmaps=0;
    Buffer* bound=nullptr; std::vector<uint16_t> lastIndices;
    int Map(Buffer* buffer,unsigned sub,unsigned mode,unsigned flags,D3D11_MAPPED_SUBRESOURCE* result){
        assert(sub==0 && mode==D3D11_MAP_WRITE_DISCARD && flags==0); ++maps;
        if(failMap){failMap=false;return -1;} result->pData=buffer->bytes.data();return 0;
    }
    void Unmap(Buffer*,unsigned sub){assert(sub==0);++unmaps;}
    void IASetIndexBuffer(Buffer* b,unsigned format,unsigned offset){
        assert(format==DXGI_FORMAT_R16_UINT && offset==0);bound=b;
    }
    void DrawIndexed(unsigned count,unsigned start,int base){
        assert(start==0 && base==0 && bound);++indexedDraws;
        lastIndices.resize(count);std::memcpy(lastIndices.data(),bound->bytes.data(),count*sizeof(uint16_t));
    }
    void Draw(unsigned count,unsigned first){assert(count==6 && first==0);++draws;}
};
struct Window{int GetWindowHandle(){return 0;}};
struct GfxRenderingAPIDX11 {
    Device device; Context context; Window window;
    Device* mDevice=&device;Context* mContext=&context;Window* mWindowBackend=&window;
    Ptr mIndexBuffer,mVertexBuffer;Buffer vertex;
    size_t internalFloats=0;
    GfxRenderingAPIDX11(){vertex.bytes.resize(256*3*40*sizeof(float));mVertexBuffer.p=&vertex;}
    void InitializeTriangleIndexBuffer();
    void SubmitIndexedTriangles(const uint16_t*,size_t);
    void DrawTriangles(float*,size_t,size_t);
    void DrawIndexedTriangles(float*,size_t,const uint16_t*,size_t,size_t);
    void DrawTrianglesInternal(float* data,size_t length,size_t count,const uint16_t* indices){
        internalFloats=length;
        // The unchanged production vertex upload boundary is inserted here.
        /* VERTEX UPLOAD */
        if(indices)SubmitIndexedTriangles(indices,count*3);else mContext->Draw(count*3,0);
    }
};
/* METHODS */
int main(){
    GfxRenderingAPIDX11 g;g.InitializeTriangleIndexBuffer();
    assert(g.device.desc.ByteWidth==1536 && g.device.desc.Usage==D3D11_USAGE_DYNAMIC);
    assert(g.device.desc.BindFlags==D3D11_BIND_INDEX_BUFFER && g.device.desc.CPUAccessFlags==D3D11_CPU_ACCESS_WRITE);
    float vertices[28];for(unsigned i=0;i<28;++i)vertices[i]=float(i)/7;
    uint16_t indices[]={0,1,2,2,1,3};
    g.DrawIndexedTriangles(vertices,28,indices,6,7);
    assert(g.internalFloats==28 && g.context.indexedDraws==1 && g.context.draws==0);
    assert(g.context.lastIndices==std::vector<uint16_t>({0,1,2,2,1,3}));
    assert(std::memcmp(g.vertex.bytes.data(),vertices,sizeof(vertices))==0);
    const auto before=g.context.indexedDraws;
    g.context.failMap=true;
    try{g.SubmitIndexedTriangles(indices,6);assert(false);}catch(const std::runtime_error&){}
    assert(g.context.indexedDraws==before);
    g.DrawTriangles(vertices,28,2);assert(g.context.draws==1);
}
'''

GL = r'''
using GLuint=unsigned; using GLsizei=int;
enum {GL_ARRAY_BUFFER=1, GL_ELEMENT_ARRAY_BUFFER=2, GL_STREAM_DRAW=3, GL_TRIANGLES=4, GL_UNSIGNED_SHORT=5};
struct GLState{unsigned next=1,element=0,indexedDraws=0,draws=0;std::vector<uint8_t> vertices,indices;}gl;
void glGenBuffers(GLsizei count,GLuint* out){assert(count==1);*out=gl.next++;}
void glBindBuffer(unsigned target,GLuint id){assert(target==GL_ELEMENT_ARRAY_BUFFER);gl.element=id;}
void glBufferData(unsigned target,size_t count,const void* data,unsigned usage){
    assert(usage==GL_STREAM_DRAW);auto& out=target==GL_ARRAY_BUFFER?gl.vertices:gl.indices;
    out.resize(count);if(count)std::memcpy(out.data(),data,count);
}
void glDrawElements(unsigned mode,GLsizei count,unsigned type,const void* at){
    assert(mode==GL_TRIANGLES && type==GL_UNSIGNED_SHORT && at==nullptr && gl.element!=0 && count==6);++gl.indexedDraws;
}
void glDrawArrays(unsigned mode,int first,GLsizei count){assert(mode==GL_TRIANGLES && first==0 && count==6);++gl.draws;}
struct GfxRenderingAPIOGL {
    GLuint mOpenglIbo=0;
    size_t internalFloats=0;
    void InitializeTriangleIndexBuffer();
    void SubmitIndexedTriangles(const uint16_t*,size_t);
    void DrawTriangles(float*,size_t,size_t);
    void DrawIndexedTriangles(float*,size_t,const uint16_t*,size_t,size_t);
    void DrawTrianglesInternal(float* buf_vbo,size_t buf_vbo_len,size_t count,const uint16_t* indices){
        internalFloats=buf_vbo_len;
        /* VERTEX UPLOAD */
        if(indices)SubmitIndexedTriangles(indices,count*3);else glDrawArrays(GL_TRIANGLES,0,count*3);
    }
};
/* METHODS */
int main(){
    GfxRenderingAPIOGL g;g.InitializeTriangleIndexBuffer();assert(g.mOpenglIbo!=0);
    float vertices[28];for(unsigned i=0;i<28;++i)vertices[i]=float(i)/7;
    uint16_t indices[]={0,1,2,2,1,3};g.DrawIndexedTriangles(vertices,28,indices,6,7);
    assert(g.internalFloats==28 && gl.indexedDraws==1 && gl.draws==0 && gl.element==g.mOpenglIbo);
    assert(gl.vertices.size()==sizeof(vertices) && std::memcmp(gl.vertices.data(),vertices,sizeof(vertices))==0);
    assert(gl.indices.size()==sizeof(indices) && std::memcmp(gl.indices.data(),indices,sizeof(indices))==0);
    g.DrawTriangles(vertices,28,2);assert(gl.draws==1);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    backends = ROOT / 'libultraship/src/fast/backends'
    for name, filename, fixture in [('dx11', 'gfx_direct3d11.cpp', DX), ('opengl', 'gfx_opengl.cpp', GL)]:
        source = (backends / filename).read_text()
        cls = 'GfxRenderingAPIDX11' if name == 'dx11' else 'GfxRenderingAPIOGL'
        methods = '\n'.join(function(source, cls + '::' + method) for method in
                            ['InitializeTriangleIndexBuffer', 'SubmitIndexedTriangles', 'DrawTriangles', 'DrawIndexedTriangles'])
        if name == 'dx11':
            begin = source.index('    D3D11_MAPPED_SUBRESOURCE ms;', source.index('// Set vertex buffer data'))
            end = source.index('    uint32_t stride = ', begin)
            upload = source[begin:end].replace('buf_vbo_len', 'length').replace('buf_vbo', 'data')
        else:
            upload = next(line for line in source.splitlines() if 'glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buf_vbo_len' in line)
        unit = COMMON + fixture.replace('/* METHODS */', methods).replace('/* VERTEX UPLOAD */', upload)
        with tempfile.TemporaryDirectory(prefix='indexed-' + name + '-') as temporary:
            cpp, binary = Path(temporary) / 'test.cpp', Path(temporary) / 'test'
            cpp.write_text(unit)
            flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-pedantic']
            if args.sanitize:
                flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                          '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
        print('PASS actual ' + name + ' indexed entrypoint, unique vertex upload, uint16 index bytes/draw and legacy path')


if __name__ == '__main__':
    main()
