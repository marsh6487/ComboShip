#include <libultraship/libultra.h>
#include <fast/types.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <vector>


using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using s16 = int16_t;
using s32 = int32_t;
#include "combo/PakNativeData.h"
#define PAK_LOG(...) ((void)0)
#include "translator.inc"

static void Put16(std::vector<u8>& data, size_t offset, uint16_t value) {
    data.at(offset) = value >> 8;
    data.at(offset + 1) = value;
}
static void Put32(std::vector<u8>& data, size_t offset, uint32_t value) {
    Put16(data, offset, value >> 16);
    Put16(data, offset + 2, value);
}
static void Command(std::vector<u8>& data, size_t offset, uint32_t w0, uint32_t w1) {
    Put32(data, offset, w0);
    Put32(data, offset + 4, w1);
}
static void Vertex(std::vector<u8>& data, size_t offset, int index) {
    Put16(data, offset, int16_t(-1234 - index * 17));
    Put16(data, offset + 2, int16_t(2345 + index * 13));
    Put16(data, offset + 4, int16_t(-32760 + index));
    Put16(data, offset + 6, 0x3a00 + index);
    Put16(data, offset + 8, int16_t(-456 - index));
    Put16(data, offset + 10, int16_t(789 + index));
    for (int c = 0; c < 4; ++c)
        data.at(offset + 12 + c) = uint8_t(31 + index * 4 + c);
}
static void CheckVertex(const Vtx& value, int index) {
    assert(value.v.ob[0] == -1234 - index * 17);
    assert(value.v.ob[1] == 2345 + index * 13);
    assert(value.v.ob[2] == -32760 + index);
    assert(value.v.flag == 0x3a00 + index);
    assert(value.v.tc[0] == -456 - index);
    assert(value.v.tc[1] == 789 + index);
    for (int c = 0; c < 4; ++c)
        assert(value.v.cn[c] == 31 + index * 4 + c);
}

int main() {
    std::vector<u8> bytes(0x200);
    Command(bytes, 0x20, 0x01002004, 0x06000100); // two packed vertices
    Command(bytes, 0x28, 0xDA380003, 0x06000180); // embedded fixed-point matrix
    Command(bytes, 0x30, 0xDE000000, 0x06000080); // nested equipment/body list
    Command(bytes, 0x38, 0xDA380003, 0x0D000040); // dynamic skeletal matrix stays segmented
    Command(bytes, 0x40, 0xDF000000, 0);
    Command(bytes, 0x80, 0x01002004, 0x06000110); // overlaps original load by one vertex
    Command(bytes, 0x88, 0xDF000000, 0);
    for (int i = 0; i < 3; ++i)
        Vertex(bytes, 0x100 + i * 16, i);
    // N64 matrix with 1.25, -2.5, negative translation and an affine 1.
    Put16(bytes, 0x180, 1);
    Put16(bytes, 0x182, uint16_t(-3));
    Put16(bytes, 0x198, uint16_t(-124));
    Put16(bytes, 0x19e, 1);
    Put16(bytes, 0x1a0, 0x4000);
    Put16(bytes, 0x1a2, 0x8000);
    Put16(bytes, 0x1b8, 0xc000);
    const auto original = bytes;
    std::map<u32, Gfx*> lists;
    Gfx* root;
    // Conversion contexts are temporary. The model retains their owned buffers
    // until shutdown, covering a deferred draw after parser return/model moves.
    std::shared_ptr<PakNativeData::Buffers> owner;
    {
        SwapContext context{};
        context.data = bytes.data();
        context.size = bytes.size();
        root = TranslateDL(context, 0x20, lists);
#ifdef PAK_CONVERTED_NATIVE_DATA
        owner = context.nativeBuffers;
#endif
    }
    assert(root && lists.size() == 2);
    auto* first = reinterpret_cast<Vtx*>(root[0].words.w1);
    CheckVertex(first[0], 0);
    CheckVertex(first[1], 1);
    auto* nested = reinterpret_cast<Gfx*>(root[2].words.w1);
    auto* overlap = reinterpret_cast<Vtx*>(nested[0].words.w1);
    CheckVertex(overlap[0], 1);
    CheckVertex(overlap[1], 2);
    auto* matrix = reinterpret_cast<Mtx*>(root[1].words.w1);
#if defined(GBI_FLOATS) || defined(GBI_FLOAT_MTX)
    assert(matrix->mf[0][0] == 1.25f);
    assert(matrix->mf[0][1] == -2.5f);
    assert(matrix->mf[3][0] == -123.25f);
    assert(matrix->mf[3][3] == 1.0f);
#else
    assert(uint32_t(matrix->m[0][0]) == 0x0001fffd);
    assert(uint32_t(matrix->m[2][0]) == 0x40008000);
#endif
    assert(root[3].words.w1 == 0x0D000041); // original matrix slot, tagged for SegAddr
    assert(bytes == original); // repeated/overlapping alias loads cannot double-swap
    PakNativeData::Buffers buffers;
    assert(!buffers.Vertices(bytes.data(), bytes.size(), 0x1f0, 2));
    assert(!buffers.Vertices(bytes.data(), bytes.size(), UINT32_MAX, 2));
    assert(!buffers.Matrix(bytes.data(), bytes.size(), 0x1d0));
    assert(!buffers.Matrix(bytes.data(), bytes.size(), UINT32_MAX));
    auto* cached = buffers.Vertices(bytes.data(), bytes.size(), 0x100, 2);
    assert(cached == buffers.Vertices(bytes.data(), bytes.size(), 0x100, 2));
    CheckVertex(cached[1], 1);
    // Invalid full ranges are ignored rather than yielding an out-of-bounds native pointer.
    Command(bytes, 0xa0, 0x01002004, 0x060001f0);
    Command(bytes, 0xa8, 0xDA380003, 0x060001d0);
    Command(bytes, 0xb0, 0xDF000000, 0);
    SwapContext invalid{};
    invalid.data = bytes.data();
    invalid.size = bytes.size();
    auto* bad = TranslateDL(invalid, 0xa0, lists);
#ifdef PAK_CONVERTED_NATIVE_DATA
    assert(bad[0].words.w0 == 0 && bad[0].words.w1 == 0);
    assert(bad[1].words.w0 == 0 && bad[1].words.w1 == 0);
#endif
    for (auto& [offset, list] : lists)
        free(list);
    std::puts("PAK native vertex/matrix boundary, overlap, lifetime and bounds passed");
}
