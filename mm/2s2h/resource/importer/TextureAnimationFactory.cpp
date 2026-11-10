#include "2s2h/resource/importer/TextureAnimationFactory.h"
#include "2s2h/resource/type/TextureAnimation.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace SOH {
namespace {
// MemoryStream checks the first byte of a read, but a multi-byte read can still
// straddle EOF. Bound every factory read before calling that shared reader.
class TextureAnimationReader {
  public:
    TextureAnimationReader(std::shared_ptr<Ship::BinaryReader> reader, size_t size) : reader(reader), size(size) {
    }
    void Require(size_t bytes) const {
        const size_t position = reader->GetBaseAddress();
        if (position > size || bytes > size - position)
            throw std::runtime_error("truncated animated material");
    }
    int8_t ReadSigned8() {
        Require(1);
        return reader->ReadInt8();
    }
    uint8_t ReadUnsigned8() {
        Require(1);
        return reader->ReadUByte();
    }
    int16_t ReadSigned16() {
        Require(2);
        return reader->ReadInt16();
    }
    uint16_t ReadUnsigned16() {
        Require(2);
        return reader->ReadUInt16();
    }
    uint32_t ReadUnsigned32() {
        Require(4);
        return reader->ReadUInt32();
    }
    std::string String() {
        const uint32_t length = ReadUnsigned32();
        Require(length);
        if (length > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
            throw std::runtime_error("invalid animated material path length");
        std::string value(length, '\0');
        if (length)
            reader->Read(value.data(), static_cast<int32_t>(length));
        return value;
    }

  private:
    std::shared_ptr<Ship::BinaryReader> reader;
    size_t size;
};

template <class T> T* AllocateParams(AnimatedMaterial& material, size_t count = 1) {
    // Attach a zero-initialized record immediately: resource destruction owns all
    // partially populated records and arrays if a later read/allocation throws.
    auto* params = static_cast<T*>(std::calloc(count, sizeof(T)));
    if (!params)
        throw std::bad_alloc();
    material.params = params;
    return params;
}

void ReadColors(TextureAnimationReader& reader, AnimatedMaterial& material) {
    auto* params = AllocateParams<AnimatedMatColorParams>(material);
    params->keyFrameLength = reader.ReadUnsigned16();
    params->keyFrameCount = reader.ReadUnsigned16();
    const size_t keyCount = reader.ReadUnsigned16();
    reader.Require(keyCount * sizeof(uint16_t));
    if (keyCount) {
        params->keyFrames = new uint16_t[keyCount];
        for (size_t i = 0; i < keyCount; ++i)
            params->keyFrames[i] = reader.ReadUnsigned16();
    }
    const size_t primCount = reader.ReadUnsigned16();
    reader.Require(primCount * 5);
    if (primCount) {
        params->primColors = new F3DPrimColor[primCount];
        for (size_t i = 0; i < primCount; ++i) {
            auto& color = params->primColors[i];
            color.r = reader.ReadUnsigned8();
            color.g = reader.ReadUnsigned8();
            color.b = reader.ReadUnsigned8();
            color.a = reader.ReadUnsigned8();
            color.lodFrac = reader.ReadUnsigned8();
        }
    }
    const size_t envCount = reader.ReadUnsigned16();
    reader.Require(envCount * 4);
    if (envCount) {
        params->envColors = new F3DEnvColor[envCount];
        for (size_t i = 0; i < envCount; ++i) {
            auto& color = params->envColors[i];
            color.r = reader.ReadUnsigned8();
            color.g = reader.ReadUnsigned8();
            color.b = reader.ReadUnsigned8();
            color.a = reader.ReadUnsigned8();
        }
    }

    const auto type = static_cast<TextureAnimationParamsType>(material.type);
    const size_t count = params->keyFrameCount;
    const size_t required = type == TextureAnimationParamsType::ColorChange ? params->keyFrameLength : count;
    if (!params->keyFrameLength || !required || primCount < required || (envCount && envCount < required))
        throw std::runtime_error("undersized animated color table");
    if (type == TextureAnimationParamsType::ColorChange)
        return; // Native type 2 never indexes keyFrames.
    if (keyCount < count || (type == TextureAnimationParamsType::ColorChangeLagrange && count > 50) ||
        (type == TextureAnimationParamsType::ColorChangeLERP && count < 2))
        throw std::runtime_error("invalid animated color key count");
    for (size_t i = 1; i < count; ++i) {
        if (type == TextureAnimationParamsType::ColorChangeLERP) {
            if (params->keyFrames[i] <= params->keyFrames[i - 1])
                throw std::runtime_error("unordered linear color keys");
        } else {
            // Lagrange ordering is arbitrary, but repeated x values divide by zero.
            for (size_t j = 0; j < i; ++j)
                if (params->keyFrames[i] == params->keyFrames[j])
                    throw std::runtime_error("duplicate Lagrange color keys");
        }
    }
    if (type != TextureAnimationParamsType::ColorChangeLERP || params->keyFrames[count - 1] >= params->keyFrameLength)
        return;

    // Native AnimatedMat_DrawColorLerp advances to index count at the final key.
    // Native resources end at length-1, so append a constant final-color interval
    // without changing the authored period/count. Preserve a supplied valid extra
    // terminal interval; reject incomplete/invalid extras rather than overwriting it.
    const bool extra = keyCount > count || primCount > count || envCount > count;
    if (extra) {
        if (keyCount <= count || primCount <= count || (envCount && envCount <= count) ||
            params->keyFrames[count] <= params->keyFrames[count - 1] ||
            params->keyFrames[count] < params->keyFrameLength - 1)
            throw std::runtime_error("invalid terminal linear color interval");
        return;
    }
    auto keys = std::make_unique<uint16_t[]>(count + 1);
    auto prim = std::make_unique<F3DPrimColor[]>(count + 1);
    std::unique_ptr<F3DEnvColor[]> env;
    std::copy_n(params->keyFrames, count, keys.get());
    std::copy_n(params->primColors, count, prim.get());
    keys[count] = params->keyFrameLength;
    prim[count] = prim[count - 1];
    if (envCount) {
        env = std::make_unique<F3DEnvColor[]>(count + 1);
        std::copy_n(params->envColors, count, env.get());
        env[count] = env[count - 1];
    }
    delete[] params->keyFrames;
    delete[] params->primColors;
    delete[] params->envColors;
    params->keyFrames = keys.release();
    params->primColors = prim.release();
    params->envColors = env.release();
}
} // namespace

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryTextureAnimationV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                      std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!file || !initData || !file->Buffer || !FileHasValidFormatAndReader(file, initData))
        return nullptr;
    try {
        auto rawReader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);
        if (!rawReader)
            return nullptr;
        TextureAnimationReader reader(rawReader, file->Buffer->size());
        auto animation = std::make_shared<TextureAnimation>(initData);
        const size_t count = reader.ReadUnsigned32();
        reader.Require(count * 3); // Each entry needs at least a segment and type.
        animation->anims.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            animation->anims.emplace_back(AnimatedMaterial{});
            auto& material = animation->anims.back();
            material.segment = reader.ReadSigned8();
            material.type = reader.ReadSigned16();
            const bool empty = material.type == static_cast<int16_t>(TextureAnimationParamsType::Empty);
            // Native dispatch walks until a negative segment, with a single
            // segment-zero Empty record as its documented no-op exception.
            if (empty) {
                if (count != 1 || material.segment != 0)
                    throw std::runtime_error("invalid empty animated material");
            } else if (material.segment < -8 || material.segment > 8 || material.segment == 0 ||
                       (material.segment < 0) != (i + 1 == count)) {
                throw std::runtime_error("invalid animated material terminator");
            }
            switch (static_cast<TextureAnimationParamsType>(material.type)) {
                case TextureAnimationParamsType::SingleScroll:
                case TextureAnimationParamsType::DualScroll: {
                    const size_t scrolls =
                        material.type == static_cast<int16_t>(TextureAnimationParamsType::DualScroll) ? 2 : 1;
                    reader.Require(scrolls * 4);
                    auto* params = AllocateParams<AnimatedMatTexScrollParams>(material, scrolls);
                    for (size_t j = 0; j < scrolls; ++j) {
                        params[j].xStep = reader.ReadSigned8();
                        params[j].yStep = reader.ReadSigned8();
                        params[j].width = reader.ReadUnsigned8();
                        params[j].height = reader.ReadUnsigned8();
                    }
                    break;
                }
                case TextureAnimationParamsType::ColorChange:
                case TextureAnimationParamsType::ColorChangeLERP:
                case TextureAnimationParamsType::ColorChangeLagrange:
                    ReadColors(reader, material);
                    break;
                case TextureAnimationParamsType::TextureCycle: {
                    auto* params = AllocateParams<AnimatedMatTexCycleParams>(material);
                    params->keyFrameLength = reader.ReadUnsigned16();
                    const size_t textures = reader.ReadUnsigned32();
                    if (!params->keyFrameLength || !textures)
                        throw std::runtime_error("empty animated texture cycle");
                    reader.Require(textures * 4); // Every texture has at least its string-length field.
                    params->textureList = new void*[textures]();
                    params->textureIndexList = new uint8_t[params->keyFrameLength];
                    for (size_t j = 0; j < textures; ++j) {
                        animation->textureCycleTextures.emplace_back("__OTR__" + reader.String());
                        params->textureList[j] = const_cast<char*>(animation->textureCycleTextures.back().c_str());
                    }
                    reader.Require(params->keyFrameLength);
                    for (size_t j = 0; j < params->keyFrameLength; ++j) {
                        const uint8_t index = reader.ReadUnsigned8();
                        if (index >= textures)
                            throw std::runtime_error("animated texture cycle index out of range");
                        params->textureIndexList[j] = index;
                    }
                    break;
                }
                case TextureAnimationParamsType::Empty:
                    break;
                default:
                    throw std::runtime_error("invalid animated material type");
            }
        }
        return animation;
    } catch (const std::exception& error) {
        SPDLOG_ERROR("Invalid animated material {}: {}", initData->Path, error.what());
        return nullptr;
    }
}
} // namespace SOH
