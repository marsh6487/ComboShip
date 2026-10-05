#pragma once

#include "../soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
#include "DinSwordGiResources.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Vertex.h>
#include <cmath>
#include <limits>
#include <memory>

namespace NeiGi {
// Read the selected resource graph, including XML filepath and binary hash
// calls. This deliberately does not substitute the authored GI catalog bounds.
// Dynamic segment geometry and embedded matrices cannot establish static bounds.
template <class Load> class ModelBoundsReader {
  public:
    ModelBoundsReader(Load& load, float tilt) : mLoad(load), mSin(std::sin(tilt)), mCos(std::cos(tilt)) {}

    bool Read(const char* path, FrameBounds& out) {
        const auto root = std::dynamic_pointer_cast<Fast::DisplayList>(mLoad(path));
        if (!Walk(root, 0) || !mVertices)
            return false;
        out = { "selected_sword", { 0, mLow, 0 }, { 0, mHigh, 0 }, 2.f * mRadius, Kind::Neutral, {} };
        return mHigh > mLow && mRadius > 0.f;
    }

  private:
    bool Vertices(const std::shared_ptr<Ship::IResource>& resource, size_t offset, size_t count) {
        const auto vertices = std::dynamic_pointer_cast<Fast::Vertex>(resource);
        if (!vertices || count == 0 || offset > vertices->VertexList.size() ||
            count > vertices->VertexList.size() - offset || (mVertices += count) > 65536)
            return false;
        for (size_t i = offset; i < offset + count; ++i) {
            const auto& p = vertices->VertexList[i].v.ob;
            const float x = p[0] * mCos - p[1] * mSin, y = p[0] * mSin + p[1] * mCos;
            mLow = std::min(mLow, y);
            mHigh = std::max(mHigh, y);
            mRadius = std::max(mRadius, std::hypot(x, float(p[2])));
        }
        return true;
    }

    bool Walk(const std::shared_ptr<Fast::DisplayList>& list, unsigned depth) {
        if (!list || list->UCode != ucode_f3dex2 || depth >= 32)
            return false;
        const auto& commands = list->Instructions;
        for (size_t i = 0; i < commands.size(); ++i) {
            if (++mCommands > 16384)
                return false;
            const auto& command = commands[i];
            const uint32_t op = command.words.w0 >> 24;
            if (op == G_ENDDL)
                return true;
            if (op == G_DL_OTR_FILEPATH) {
                if (!Walk(std::dynamic_pointer_cast<Fast::DisplayList>(
                              mLoad(reinterpret_cast<const char*>(command.words.w1))), depth + 1))
                    return false;
                if ((command.words.w0 >> 16) & 1)
                    return true;
            } else if (op == G_VTX_OTR_FILEPATH) {
                if (++i == commands.size())
                    return false;
                const auto& args = commands[i];
                if (!Vertices(mLoad(reinterpret_cast<const char*>(command.words.w1)),
                              args.words.w1 & 0xffffu, args.words.w0))
                    return false;
            } else if (op == G_DL_OTR_HASH || op == G_VTX_OTR_HASH) {
                if (++i == commands.size())
                    return false;
                const uint64_t hash = (uint64_t(commands[i].words.w0) << 32) | uint32_t(commands[i].words.w1);
                const auto resource = mLoad(hash);
                if (op == G_DL_OTR_HASH) {
                    if (!Walk(std::dynamic_pointer_cast<Fast::DisplayList>(resource), depth + 1))
                        return false;
                    if ((command.words.w0 >> 16) & 1)
                        return true;
                } else {
                    const auto vertex = std::dynamic_pointer_cast<Fast::Vertex>(resource);
                    if (!vertex)
                        return false;
                    size_t offset = command.words.w1;
                    // The interpreter caches a vertex pointer in the command.
                    // Recover the offset against this exact selected resource.
                    if (offset > 0xfffffu) {
                        const uintptr_t base = reinterpret_cast<uintptr_t>(vertex->GetPointer());
                        if (offset < base || offset - base >= vertex->GetPointerSize())
                            return false;
                        offset -= base;
                    }
                    if (offset % sizeof(Vtx) ||
                        !Vertices(resource, offset / sizeof(Vtx), (command.words.w0 >> 12) & 0xff))
                        return false;
                }
            } else if (op == G_MTX || op == G_POPMTX || op == G_MTX_OTR ||
                       op == G_MTX_OTR_FILEPATH || op == G_VTX || op == G_DL ||
                       op == G_BRANCH_Z_OTR || op == G_BRANCH_Z) {
                return false;
            } else if (op == G_SETTIMG_OTR_HASH || op == G_MARKER || op == G_MOVEMEM_OTR) {
                if (++i == commands.size())
                    return false;
            }
        }
        return false;
    }

    Load& mLoad;
    float mSin, mCos, mLow = std::numeric_limits<float>::max(), mHigh = -std::numeric_limits<float>::max();
    float mRadius = 0;
    size_t mVertices = 0, mCommands = 0;
};

template <class Load>
bool SelectedModelFit(Load& load, const char* path, float scale, float tilt, bool shop, int dinProfile, ShopFit& out) {
    FrameBounds bounds{};
    ModelBoundsReader<Load> reader(load, tilt);
    if (!reader.Read(path, bounds))
        return false;
    if (dinProfile >= 1 && dinProfile <= 3) {
        const auto& layers = DinSwordGi::profiles[dinProfile - 1];
        for (const auto* layer : { layers.core, layers.flame }) {
            FrameBounds layerBounds{};
            ModelBoundsReader<Load> layerReader(load, tilt);
            if (!layerReader.Read(layer, layerBounds))
                return false; // Preserve the draw rather than claim a partial fit.
            bounds.minimum.y = std::min(bounds.minimum.y, layerBounds.minimum.y);
            bounds.maximum.y = std::max(bounds.maximum.y, layerBounds.maximum.y);
            bounds.spinningWidth = std::max(bounds.spinningWidth, layerBounds.spinningWidth);
        }
    }
    out = FrameFit(bounds, scale, shop);
    return true;
}
} // namespace NeiGi
