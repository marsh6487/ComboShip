#pragma once

// Both hosts ship this authored GI; it never borrows the MM tower clock.
#include "../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "functions.h"
#include <cstring>
#ifdef COMBO_BUILD
#include "menu/ComboItemDrawABI.h"
#endif

namespace NeiClockGi {
inline constexpr char Opaque[] = "__OTR__objects/object_nei_clock/gNeiClockDL";
inline constexpr char Glass[] = "__OTR__objects/object_nei_clock/gNeiClockGlassDL";
inline constexpr char GlassTexture[] = "__OTR__objects/object_nei_clock/gNeiClockGlassTexDL";
inline constexpr float Scale = .04f; // 1000 authored units -> 40 GI units, dial at the origin.
inline constexpr auto Shimmer = NeiGi::Kind::Hourglass;

#ifdef COMBO_BUILD
inline constexpr char OpaqueMM[] = "__OTR__@mm:objects/object_nei_clock/gNeiClockDL";
inline constexpr char GlassMM[] = "__OTR__@mm:objects/object_nei_clock/gNeiClockGlassDL";
inline constexpr char GlassTextureMM[] = "__OTR__@mm:objects/object_nei_clock/gNeiClockGlassTexDL";
inline constexpr char GlassOOT[] = "__OTR__@oot:objects/object_nei_clock/gNeiClockGlassDL";
inline constexpr char GlassTextureOOT[] = "__OTR__@oot:objects/object_nei_clock/gNeiClockGlassTexDL";
#endif

inline const char* TextureForGlass(const char* path) {
    if (!path)
        return nullptr;
    if (!std::strcmp(path, Glass))
        return GlassTexture;
#ifdef COMBO_BUILD
    if (!std::strcmp(path, GlassMM))
        return GlassTextureMM;
    if (!std::strcmp(path, GlassOOT))
        return GlassTextureOOT;
#endif
    return nullptr;
}

// Paired entrypoints keep the dynamic tile operation between the texture load
// and lens geometry. Never borrow a scene segment: its GPU binding can differ
// from gSegments, so restoring the CPU value would overwrite unrelated state.
// The caller reserves seven commands, or nine when supplying a native owner.
inline void PrepareGlass(Gfx*& head, uint32_t frame, const char* glass, const char* owner = nullptr) {
    const char* texture = TextureForGlass(glass);
    if (!texture)
        return;
    if (owner)
        gSPComboRMPush(head++, owner);
    gDma1p(head++, G_DL_OTR_FILEPATH, texture, 0, G_DL_PUSH);
    gDPTileSync(head++);
    // A 32-pixel repeat is 128 quarter-texels. Keep the next endpoint unwrapped
    // at 127 -> 128; interpolating toward zero would sweep backward at the seam.
    const float phase = float(frame & 127u);
    gDPSetTileSizeLerp(head, G_TX_RENDERTILE, phase, 0.f, phase + 124.f, 124.f, phase + 1.f, 0.f, phase + 125.f, 124.f);
    head += 5;
    if (owner)
        gSPComboRMPop(head++);
}

inline void Draw(PlayState* play, bool effects, const char* owner) {
    // ExternalPresentation reserves its existing two matrices/16 commands per
    // stream after this nine-command preparation. Its child shimmer then keeps
    // that reservation; no unreserved cleanup is emitted after it returns.
    const bool glass = NeiGi_CanDrawLayers(play, 2, 16, 25);
    if (glass)
        PrepareGlass(play->state.gfxCtx->polyXlu.p, play->gameplayFrames, Glass, owner);
    // A tight arena retains the accepted opaque presentation when it still fits.
    NeiGi_DrawExternalPresentation(play, Opaque, glass ? Glass : nullptr, Scale, static_cast<int>(Shimmer), effects,
                                   owner, false, 0.f, false);
}

#ifdef COMBO_BUILD
// Only Clock's paired glass lists add commands to the generic custom drawer.
// Preflight its complete matrix/command budget before any partial body draw.
inline bool CanDrawForeign(PlayState* play, int count, const char* const* paths) {
    int glassLayers = 0;
    for (int i = 0; i < count; ++i)
        glassLayers += TextureForGlass(paths[i]) != nullptr;
    return !glassLayers || NeiGi_CanDrawLayers(play, 4, 32, 32 + 7 * glassLayers);
}

// The lens is intrinsic; only the existing gold particles follow ItemEffects.
// Appearance refresh retains the established live foreign toggle behavior.
inline int Describe(CwItemDrawInfo* out, const char* opaque, bool effects) {
    out->drawKind = CW_DRAW_KIND_CUSTOM_GI;
    out->dlistCount = 2;
    out->xluStartIndex = 1;
    out->dlists[0] = opaque;
    out->dlists[1] = !std::strcmp(opaque, OpaqueMM) ? GlassMM : Glass;
    out->scale = Scale;
    out->neiShimmer = static_cast<int>(Shimmer) + 1;
    out->itemShimmer = effects;
    out->stateDependent = 2;
    return 1;
}
#endif
} // namespace NeiClockGi
