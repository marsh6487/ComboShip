#!/usr/bin/env python3
"""Compile the production native-head helper against a small resource/frame model.

This verifies command/lifetime boundaries, not the game's renderer or artwork.
"""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
source = (root / 'soh/mods/transformation_masks/mm_player_form.cpp').read_text()
start = source.index('static const char* sNativeBlinkHeadPaths')
end = source.index('// Action IDs (forward declaration for MmFormState)', start)
production = source[start:end]
prefix = r'''
#include <vector>
#include <memory>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <string>
#include "soh/mods/transformation_masks/mm_form_blink_pixels.h"
using s32 = int32_t; using u8 = uint8_t;
constexpr int MM_PLAYER_FORM_FIERCE_DEITY=0, MM_PLAYER_FORM_DEKU=3;
struct Gfx { struct { uintptr_t w0, w1; } words; };
struct PlayState { struct { void* gfxCtx; } state; };
static void* Graph_Alloc(void*, size_t size) { alignas(8) static uint8_t arena[65536]; assert(size < sizeof arena); return arena; }
static int ResourceMgr_OTRSigCheck(char* p) { return !strncmp(p,"__OTR__",7); }
namespace Ship { struct Resource { virtual ~Resource() = default; }; }
namespace Fast {
enum class TextureType { RGBA16bpp };
struct Texture : Ship::Resource {
    TextureType Type = TextureType::RGBA16bpp; int Flags=0, Width=64, Height=32;
    size_t ImageDataSize=MmFormBlink::Bytes; uint8_t* ImageData;
};
struct DisplayList : Ship::Resource { std::vector<Gfx> Instructions; };
}
namespace Ship {
struct Archive { const char* HashToCString(uint64_t hash) {
    return hash == 123 ? "objects/object_link_nuts/object_link_nuts_Tex_0048B0" : "objects/other/untouched";
} };
struct Manager {
    bool alt=false; std::shared_ptr<Fast::DisplayList> dl=std::make_shared<Fast::DisplayList>();
    std::shared_ptr<Fast::Texture> tex=std::make_shared<Fast::Texture>();
    bool IsAltAssetsEnabled() { return alt; }
    std::shared_ptr<Archive> GetArchiveManager() { return std::make_shared<Archive>(); }
    std::shared_ptr<Resource> LoadResourceProcess(const char* path, bool=false) {
        if (std::string(path).find("HeadDL") != std::string::npos) return dl;
        return tex;
    }
    std::shared_ptr<Resource> LoadResource(const char*) { return tex; }
};
struct Context { static Context* GetRawInstance() { static Context context; return &context; }
    std::shared_ptr<Manager> manager=std::make_shared<Manager>();
    std::shared_ptr<Manager> GetResourceManager() { return manager; }
};
}
'''
suffix = r'''
int main() {
    PlayState play{};
    auto manager=Ship::Context::GetRawInstance()->manager;
    alignas(8) uint8_t pixels[MmFormBlink::Bytes]; memset(pixels,255,sizeof pixels);
    pixels[1]=254; // actual cutout alpha is required
    manager->tex->ImageData=pixels;
    manager->dl->Instructions={{{0xE7000000,0}},{{0x20100000,0}},{{0,123}},{{0xDF000000,0}}};
    char head[]="__OTR__objects/object_link_nuts/gLinkDekuHeadDL";
    assert(!MmForm_NativeBlinkHead(&play,3,0,reinterpret_cast<Gfx*>(head)));
    Gfx* copy=MmForm_NativeBlinkHead(&play,3,1,reinterpret_cast<Gfx*>(head));
    assert(copy && copy[0].words.w0==0xE7000000 && copy[3].words.w0==0xDF000000);
    assert(copy[1].words.w0==0xFD100000 && copy[2].words.w0==0 && copy[2].words.w1==0);
    uintptr_t half=copy[1].words.w1; assert((half&1)==0);
    assert(manager->dl->Instructions[1].words.w0==0x20100000);
    assert(manager->dl->Instructions[2].words.w1==123);
    copy=MmForm_NativeBlinkHead(&play,3,2,reinterpret_cast<Gfx*>(head));
    assert(copy && copy[1].words.w1!=half);
    char custom[]="__OTR__objects/custom/HeadDL";
    assert(!MmForm_NativeBlinkHead(&play,3,1,reinterpret_cast<Gfx*>(custom)));
    manager->tex->Width=512;
    assert(!MmForm_NativeBlinkHead(&play,3,1,reinterpret_cast<Gfx*>(head)));
    manager->tex->Width=64;
    pixels[0]=0; // same-size custom art is also preserved
    assert(!MmForm_NativeBlinkHead(&play,3,1,reinterpret_cast<Gfx*>(head)));
    pixels[0]=255;
    manager->alt=true;
    manager->dl->Instructions={{{0xFD100000,0x08000000}},{{0xDF000000,0}}};
    assert(!MmForm_NativeBlinkHead(&play,3,1,reinterpret_cast<Gfx*>(head)));
    manager->alt=false;
    char texPath[]="objects/object_link_nuts/object_link_nuts_Tex_0048B0";
    manager->dl->Instructions={{{0x25100000,reinterpret_cast<uintptr_t>(texPath)}},{{0xDF000000,0}}};
    copy=MmForm_NativeBlinkHead(&play,3,1,reinterpret_cast<Gfx*>(head));
    assert(copy && copy[0].words.w0==0xFD100000 && copy[1].words.w0==0xDF000000);
    MmForm_ResetNativeBlinkHeads();
    assert(sNativeBlinkHeads[1].instructions.empty() && sNativeBlinkPixelsReady[1]);
}
'''
with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'test.cpp'; p.write_text(prefix+production+suffix)
    binary=Path(d)/'test'
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(root),str(p),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Native head helper: command preservation, identity, Alt, state and format boundaries passed')
