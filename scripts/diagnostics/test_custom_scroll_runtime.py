"""Compile the actual patched scroll handler without either game DLL.

Catches unresolved game callbacks, cross-interpreter tick leakage, signed
scroll wrapping, interpolation drift, and overflow on long-running clocks.
"""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
patch = (ROOT / 'patches/libultraship-custom-tex-scroll-e3cd6591.patch').read_text()
section = patch.split('diff --git a/src/fast/interpreter.cpp')[1].split('diff --git')[0]
added = '\n'.join(line[1:] for line in section.splitlines() if line.startswith('+') and not line.startswith('+++'))

def function(name):
    start = added.index(name)
    start = added.rfind('\n', 0, start) + 1
    body = added.index('{', start)
    depth = 1
    end = body + 1
    while depth:
        depth += (added[end] == '{') - (added[end] == '}')
        end += 1
    return added[start:end]

source = r'''
#include <cstdint>
#include <cmath>
#include <memory>
#include <cassert>
constexpr int G_TX_MIRROR = 1;
struct Tile { uint8_t masks=0, maskt=0, cms=0, cmt=0, shifts=0, shiftt=0;
    float uls=0, ult=0, lrs=124, lrt=124; };
struct RDP { Tile texture_tile[8]; bool textures_changed[2]{}; };
struct Interpreter { RDP storage; RDP* mRdp=&storage; uint32_t mGameTick=0; float mInterpolationT=0; };
struct F3DGfx { struct { uintptr_t w0, w1; } words; };
static std::weak_ptr<Interpreter> mInstance;
static uint32_t GetTileSizeFromCoordinates(float low, float high) {
    return static_cast<uint32_t>(lroundf((high-low+4)/4));
}
'''
source += '\n'.join(line for line in added.splitlines() if line.startswith('extern "C"'))
for name in ('GetRepeatPeriodFromTile(', 'WrapScrollCoordinate(', 'gfx_scroll_texture_handler_custom('):
    source += '\n' + function(name)
source += r'''
int main() {
    auto oot=std::make_shared<Interpreter>();
    auto mm=std::make_shared<Interpreter>();
    F3DGfx command{{0, (uint32_t(4)<<16)|uint16_t(-4)}};
    F3DGfx* ptr=&command;
    auto run=[&](std::shared_ptr<Interpreter> game, uint32_t tick, float t) {
        game->storage=RDP{}; game->mGameTick=tick; game->mInterpolationT=t;
        mInstance=game; assert(!gfx_scroll_texture_handler_custom(&ptr));
        assert(game->storage.textures_changed[0] && game->storage.textures_changed[1]);
    };
    run(oot, 3, 0); assert(oot->storage.texture_tile[0].uls==12);
    assert(oot->storage.texture_tile[0].ult==116);
    run(mm, 10, 0); assert(mm->storage.texture_tile[0].uls==40);
    run(oot, 3, 0.5f); assert(oot->storage.texture_tile[0].uls==14);
    run(oot, 3, 1); assert(oot->storage.texture_tile[0].uls==16);
    run(oot, 3, 0); assert(oot->storage.texture_tile[0].uls==12);
    run(mm, 0, 0); assert(mm->storage.texture_tile[0].uls==0);
    run(oot, UINT32_MAX, 0); assert(oot->storage.texture_tile[0].uls==124);
    assert(oot->storage.texture_tile[0].ult==4);
}
'''
with tempfile.TemporaryDirectory(prefix='combo-scroll-test-') as temp:
    test = pathlib.Path(temp) / 'scroll.cpp'
    test.write_text(source)
    exe = pathlib.Path(temp) / 'scroll'
    subprocess.run(['g++', '-std=c++20', '-fsanitize=undefined', '-fno-sanitize-recover=all', str(test), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS actual custom scroll handler: standalone link, independent clocks, interpolation, wrapping, overflow')
