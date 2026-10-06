#pragma once

#include "NeiGiEffectPolicy.h"
#include "NeiGiFrameFit.h"
#include "NeiGiSongEffectPolicy.h"
#include "z64.h"
namespace NeiGi {
// Private RGBA32 resources use a 32x32 logical tile; resource metadata owns
// the high-resolution scale. Path and material must have static lifetime.
struct TextureMaterial {
    const char* path;
    bool repeatS = false;
    bool repeatT = false;
};
} // namespace NeiGi
extern "C" {

// Shared presentation primitives. The caller owns pose, origin and scale;
// these functions do not add GI rotation or optional pickup shimmer.
NeiGi::Basis NeiGi_CameraBasis(PlayState* play);
void NeiGi_DrawMesh(PlayState* play, const NeiGi::Mesh& mesh, NeiGi::Kind orb = NeiGi::Kind::Neutral);
// False queues nothing, so the caller can keep its geometry-only fallback.
bool NeiGi_DrawTexturedMesh(PlayState* play, const NeiGi::Mesh& mesh, const NeiGi::TextureMaterial& material);
// Intrinsic weather; profiles 1..4 are individual seasons, 5 cycles on the actual rod.
void NeiGi_DrawSeasonOverlay(PlayState* play, int profile, const char* owner);
void NeiGi_DrawSongOverlay(PlayState* play, int song, const char* owner);
// Selected external geometry uses its resource bounds for MM receipts and
// swords. The item's identity still controls its optional particles.
void NeiGi_DrawExternalPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int shimmerKind,
                                    bool shimmer, const char* owner, bool shop = false, float tilt = 0.f,
                                    bool fit = true, int mmPickup = 0);
// Owner-aware mesh/effect composition used by both foreign GI directions.
void NeiGi_DrawPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int effect,
                            const float center[3], bool shimmer, const char* owner, bool shop = false,
                            int mmPickup = 0);
}
