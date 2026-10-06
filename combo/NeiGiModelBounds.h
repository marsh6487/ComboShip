#pragma once

#include "../soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
#include "DinSwordGiResources.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Matrix.h>
#include <fast/resource/type/Vertex.h>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace NeiGi {
// Read the selected resource graph, including XML filepath and binary hash
// calls. This deliberately does not substitute the authored GI catalog bounds.
// Resource modelview matrices are part of the selected geometry. Dynamic
// segment geometry or matrices that replace the caller's pose remain unsupported.
template <class Load> class ModelBoundsReader {
  public:
    ModelBoundsReader(Load& load, float tilt) : mLoad(load), mSin(std::sin(tilt)), mCos(std::cos(tilt)) {
    }

    bool Read(const char* path, FrameBounds& out) {
        const auto root = std::dynamic_pointer_cast<Fast::DisplayList>(mLoad(path));
        if (!Walk(root, 0) || !mVertices || !mStack.empty())
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
            float transformed[3]{};
            for (int axis = 0; axis < 3; ++axis) {
                transformed[axis] = mMatrix[3][axis];
                for (int input = 0; input < 3; ++input)
                    transformed[axis] += p[input] * mMatrix[input][axis];
                if (!std::isfinite(transformed[axis]))
                    return false;
            }
            const float x = transformed[0] * mCos - transformed[1] * mSin;
            const float y = transformed[0] * mSin + transformed[1] * mCos;
            mLow = std::min(mLow, y);
            mHigh = std::max(mHigh, y);
            mRadius = std::max(mRadius, std::hypot(x, transformed[2]));
        }
        return true;
    }

    using Matrix = std::array<std::array<float, 4>, 4>;
    bool ApplyMatrix(const std::shared_ptr<Ship::IResource>& resource, uint32_t flags) {
        const auto matrix = std::dynamic_pointer_cast<Fast::Matrix>(resource);
        // LOAD/projection matrices replace the host pose; an outer GI fit
        // cannot reliably contain such geometry without changing the asset.
        if (!matrix || (flags & ~(uint32_t)G_MTX_PUSH) || mStack.size() >= 32)
            return false;
        Matrix decoded{};
#ifdef GBI_FLOATS
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                decoded[row][column] = matrix->Matrx.mf[row][column];
#else
        const auto* words = reinterpret_cast<const uint32_t*>(&matrix->Matrx);
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; column += 2) {
                const int index = row * 2 + column / 2;
                decoded[row][column] = int32_t((words[index] & 0xffff0000u) | (words[index + 8] >> 16)) / 65536.f;
                decoded[row][column + 1] = int32_t((words[index] << 16) | (words[index + 8] & 0xffffu)) / 65536.f;
            }
        }
#endif
        if (decoded[0][3] != 0 || decoded[1][3] != 0 || decoded[2][3] != 0 || decoded[3][3] != 1)
            return false;
        for (const auto& row : decoded)
            for (float value : row)
                if (!std::isfinite(value))
                    return false;
        if (flags & G_MTX_PUSH)
            mStack.push_back(mMatrix);
        Matrix combined{};
        // Match the interpreter's row-vector resource * current modelview.
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                for (int input = 0; input < 4; ++input)
                    combined[row][column] += decoded[row][input] * mMatrix[input][column];
        mMatrix = combined;
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
                              mLoad(reinterpret_cast<const char*>(command.words.w1))),
                          depth + 1))
                    return false;
                if ((command.words.w0 >> 16) & 1)
                    return true;
            } else if (op == G_VTX_OTR_FILEPATH) {
                if (++i == commands.size())
                    return false;
                const auto& args = commands[i];
                if (!Vertices(mLoad(reinterpret_cast<const char*>(command.words.w1)), args.words.w1 & 0xffffu,
                              args.words.w0))
                    return false;
            } else if (op == G_MTX_OTR_FILEPATH) {
                if (!ApplyMatrix(mLoad(reinterpret_cast<const char*>(command.words.w1)),
                                 (command.words.w0 & 0xffu) ^ G_MTX_PUSH))
                    return false;
            } else if (op == G_POPMTX) {
                const size_t count = command.words.w1 / 64;
                if (!count || command.words.w1 % 64 || count > mStack.size())
                    return false;
                mMatrix = mStack[mStack.size() - count];
                mStack.resize(mStack.size() - count);
            } else if (op == G_DL_OTR_HASH || op == G_VTX_OTR_HASH || op == G_MTX_OTR) {
                if (++i == commands.size())
                    return false;
                const uint64_t hash = (uint64_t(commands[i].words.w0) << 32) | uint32_t(commands[i].words.w1);
                const auto resource = mLoad(hash);
                if (op == G_DL_OTR_HASH) {
                    if (!Walk(std::dynamic_pointer_cast<Fast::DisplayList>(resource), depth + 1))
                        return false;
                    if ((command.words.w0 >> 16) & 1)
                        return true;
                } else if (op == G_MTX_OTR) {
                    if (!ApplyMatrix(resource, (command.words.w0 & 0xffu) ^ G_MTX_PUSH))
                        return false;
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
            } else if (op == G_MTX || op == G_VTX || op == G_DL || op == G_BRANCH_Z_OTR || op == G_BRANCH_Z) {
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
    Matrix mMatrix{ { { { 1, 0, 0, 0 } }, { { 0, 1, 0, 0 } }, { { 0, 0, 1, 0 } }, { { 0, 0, 0, 1 } } } };
    std::vector<Matrix> mStack;
};

template <class Load>
bool SelectedModelFit(Load& load, const char* path, float scale, float tilt, int presentation, int dinProfile,
                      ShopFit& out) {
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
    out = FrameFit(bounds, scale, presentation == 1, presentation >= 2 ? presentation - 1 : 0);
    return true;
}
} // namespace NeiGi
