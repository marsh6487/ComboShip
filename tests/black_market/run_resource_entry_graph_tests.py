#!/usr/bin/env python3
"""Compile the real entry-fixture graph importer against the production patcher.

The entry fixture intentionally retains serialized native v0 vertex bytes. Its
resource view must describe that 16-byte storage, including actual payload
bounds, rather than claiming the game's decoded Vtx stride.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def block(source, marker, semicolon=False):
    start = source.index(marker)
    brace = source.index("{", start)
    end, depth = brace + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + (";" if semicolon else "") + "\n"


PREFIX = r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "soh/mods/transformation_masks/assets/mm_display_list_patch.h"
static std::map<std::string,std::string> activeFiles,baseFiles;
static std::set<std::string> hiddenFiles;
static std::map<std::string,std::vector<MmDisplayListCommand>> importedGraphs;
static std::map<std::string,std::vector<uint8_t>> importedPayloads;
static std::map<std::string,std::string> importedTextureAliases;
static std::set<std::string> graphInProgress;
static std::unordered_map<uint64_t,std::string> resourceHashes;
static int graphPatches,graphVertices,graphTextures;
static MmDisplayListCommand opaqueRenderMode[4]={{0xDF000000,0},{0xDF000000,0},{0xDF000000,0},{0xDF000000,0}};
"""

TEST = r"""
static void Put32(std::vector<uint8_t>& bytes,size_t at,uint32_t value) {
    for (unsigned i=0;i<4;i++) bytes.at(at+i)=value>>(8*i);
}
static std::vector<uint8_t> Vertices(uint32_t count) {
    std::vector<uint8_t> bytes(72+size_t(count)*16);
    std::memcpy(bytes.data()+4,"RRAO",4);
    Put32(bytes,64,25); Put32(bytes,68,count);
    for (size_t i=72;i<bytes.size();i++) bytes[i]=uint8_t(i-72);
    return bytes;
}
static std::vector<uint8_t> Graph(std::initializer_list<uint32_t> words) {
    std::vector<uint8_t> bytes(72+words.size()*4);
    std::memcpy(bytes.data()+4,"TLDO",4); bytes[64]=4;
    size_t offset=72;
    for (auto word:words) { Put32(bytes,offset,word);offset+=4; }
    return bytes;
}
static void Write(const char* folder,const std::string& name,std::vector<uint8_t> bytes) {
    const std::string path=std::string(folder)+"/"+name;
    std::ofstream stream(path,std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    stream.close(); assert(stream);
    baseFiles[name]=path;
}
int main(int argc,char** argv) {
    assert(argc==2);
    // Compilation here catches the same generated-header ABI failure as CI.
    MmDisplayListResolveResource resolver=ResolveGraphResource;
    Write(argv[1],"vertices",Vertices(3)); resourceHashes[21]="vertices";
    auto truncated=Vertices(3);truncated.pop_back();
    Write(argv[1],"truncated",std::move(truncated));resourceHashes[22]="truncated";
    auto wrongType=Vertices(3);Put32(wrongType,64,16);
    Write(argv[1],"scalar",std::move(wrongType));resourceHashes[23]="scalar";
    auto wide=Vertices(3);Put32(wide,8,1);
    Write(argv[1],"unsupported-wide",std::move(wide));resourceHashes[24]="unsupported-wide";
    auto bigEndian=Vertices(3);bigEndian[0]=1;
    Write(argv[1],"unsupported-endian",std::move(bigEndian));resourceHashes[25]="unsupported-endian";
    auto wrongCount=Vertices(3);Put32(wrongCount,68,UINT32_MAX);
    Write(argv[1],"wrong-count",std::move(wrongCount));resourceHashes[26]="wrong-count";
    std::vector<uint8_t> texture(96);std::memcpy(texture.data()+4,"XETO",4);
    Write(argv[1],"texture",std::move(texture));resourceHashes[31]="texture";
    Write(argv[1],"nested",Graph({0x32001002,32,0,21,0xDF000000,0}));resourceHashes[41]="nested";
    Write(argv[1],"root",Graph({0x31000000,0,0,41,0x20000000,0,0,31,0xDF000000,0}));

    MmDisplayListVertexResourceView view{};
    const auto pointer=resolver(nullptr,MM_DISPLAY_LIST_REFERENCE_VERTEX,21,&view);
    assert(pointer && pointer==view.pointer);
    assert(view.size==48 && view.recordSize==16 && view.stride==16);
    assert(pointer==reinterpret_cast<uintptr_t>(importedPayloads.at("vertices").data()+72));
    assert(reinterpret_cast<const uint8_t*>(pointer)[32]==32);

    MmDisplayListPatchStats stats{};
    MmDisplayListCommand finalVertex[]={{0x32001002,32},{0,21},{0xDF000000,0}};
    assert(MmDisplayList_PatchCommands(finalVertex,3,resolver,nullptr,&stats));
    assert(stats.verticesPatched==1 && stats.malformed==0 && stats.unresolved==0);
    assert(finalVertex[0].w1==reinterpret_cast<uintptr_t>(importedPayloads.at("vertices").data()+72+32));
    assert(finalVertex[0].w0==0x01001002);

    for (const uintptr_t offset:{uintptr_t(1),uintptr_t(48)}) {
        MmDisplayListCommand invalid[]={{0x32001002,offset},{0,21},{0xDF000000,0}};
        assert(!MmDisplayList_PatchCommands(invalid,3,resolver,nullptr,&stats));
        assert(stats.malformed==1 && stats.verticesPatched==0);
    }
    for (uint64_t hash:{UINT64_C(22),UINT64_C(23),UINT64_C(24),UINT64_C(25),UINT64_C(26),UINT64_C(999)}) {
        view={123,456,22,24};
        assert(!resolver(nullptr,MM_DISPLAY_LIST_REFERENCE_VERTEX,hash,&view));
        assert(view.pointer==0 && view.size==0);
    }

    view={123,456,22,24};
    assert(resolver(nullptr,MM_DISPLAY_LIST_REFERENCE_RENDER_MODE,2,&view)==
           reinterpret_cast<uintptr_t>(&opaqueRenderMode[2]));
    assert(view.pointer==0 && view.size==0);
    assert(!resolver(nullptr,MM_DISPLAY_LIST_REFERENCE_RENDER_MODE,3,&view));
    assert(resolver(nullptr,MM_DISPLAY_LIST_REFERENCE_TEXTURE,31,&view));
    assert(importedPayloads.at("texture").size()==96);
    assert(importedTextureAliases.at("texture")=="__OTR__texture");

    auto* root=ImportGraph("root");
    assert(root && graphPatches==1 && graphVertices==1 && graphTextures==1);
    assert(root[0].w1==reinterpret_cast<uintptr_t>(importedGraphs.at("nested").data()));
    assert(importedGraphs.at("nested")[0].w1==
           reinterpret_cast<uintptr_t>(importedPayloads.at("vertices").data()+72+32));
    assert(root[2].w1==reinterpret_cast<uintptr_t>(importedTextureAliases.at("texture").c_str()));

    hiddenFiles.insert("vertices");view={123,456,22,24};
    assert(!resolver(nullptr,MM_DISPLAY_LIST_REFERENCE_VERTEX,21,&view));
    assert(view.pointer==0 && view.size==0);
    std::puts("PASS entry graph fixture: current resolver ABI, raw-v0 stride/storage bounds,"
              " final vertex, malformed resources, nested graph, texture alias and hidden-resource fallback");
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--fixture", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    source = (args.fixture or root / "tests/black_market/resource_entry_fixture.h").read_text()
    generated = PREFIX + block(source, "inline std::vector<uint8_t> ReadResourceBytes(")
    generated += block(source, "struct ResourceReader {", semicolon=True)
    generated += "inline MmDisplayListCommand* ImportGraph(const std::string& name);\n"
    generated += block(source, "inline uintptr_t ResolveGraphResource(")
    generated += block(source, "inline MmDisplayListCommand* ImportGraph(const std::string& name) {") + TEST
    with tempfile.TemporaryDirectory(prefix="resource-entry-graph-") as directory:
        work = Path(directory)
        path = work / "fixture.cpp"
        path.write_text(generated)
        binary = work / "test"
        flags = [os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                 "-I" + str(root)]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
        subprocess.run([*flags,str(path),
                        str(root / "soh/mods/transformation_masks/assets/mm_display_list_patch.cpp"),
                        "-o",str(binary)],check=True)
        subprocess.run([str(binary),str(work)],check=True,
                       env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})


if __name__ == "__main__":
    main()

