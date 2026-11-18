#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <utility>

// ModLoader64 objects always store N64 records: 16-byte vertices and fixed-point
// matrices. They bypass the OTR resource factories, so decode them at the PAK
// boundary too. In particular, native Vtx is 24 bytes in the wide-world build.
// Keep the source bytes immutable: body/equipment DLs may load overlapping ranges.
namespace PakNativeData {
inline uint16_t Read16(const uint8_t* p) {
    return (uint16_t(p[0]) << 8) | p[1];
}
inline uint32_t Read32(const uint8_t* p) {
    return (uint32_t(Read16(p)) << 16) | Read16(p + 2);
}

struct Buffers {
    std::map<std::pair<uint32_t, uint32_t>, std::shared_ptr<Vtx>> vertices;
    std::map<uint32_t, std::shared_ptr<Mtx>> matrices;

    Vtx* Vertices(const uint8_t* data, uint32_t size, uint32_t offset, uint32_t count) {
        if (!count || offset > size || count > (size - offset) / 16)
            return nullptr;
        auto key = std::make_pair(offset, count);
        auto found = vertices.find(key);
        if (found != vertices.end())
            return found->second.get();
        std::shared_ptr<Vtx> output(new Vtx[count](), std::default_delete<Vtx[]>());
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t* source = data + offset + i * 16;
            for (unsigned axis = 0; axis < 3; ++axis)
                output.get()[i].v.ob[axis] = int16_t(Read16(source + axis * 2));
            output.get()[i].v.flag = Read16(source + 6);
            output.get()[i].v.tc[0] = int16_t(Read16(source + 8));
            output.get()[i].v.tc[1] = int16_t(Read16(source + 10));
            std::memcpy(output.get()[i].v.cn, source + 12, 4);
        }
        return vertices.emplace(key, std::move(output)).first->second.get();
    }

    Mtx* Matrix(const uint8_t* data, uint32_t size, uint32_t offset) {
        if (offset > size || size - offset < 64)
            return nullptr;
        auto found = matrices.find(offset);
        if (found != matrices.end())
            return found->second.get();
        auto output = std::make_shared<Mtx>();
#if defined(GBI_FLOATS) || defined(GBI_FLOAT_MTX)
        for (unsigned i = 0; i < 16; ++i) {
            const int16_t integer = int16_t(Read16(data + offset + i * 2));
            const uint16_t fraction = Read16(data + offset + 32 + i * 2);
            output->mf[i / 4][i % 4] = float(integer) + float(fraction) / 65536.0f;
        }
#else
        for (unsigned i = 0; i < 16; ++i)
            output->m[i / 4][i % 4] = int32_t(Read32(data + offset + i * 4));
#endif
        return matrices.emplace(offset, std::move(output)).first->second.get();
    }
};
} // namespace PakNativeData
