#pragma once
#include "ComboItemDrawABI.h"
#include "Rando/Types.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
void MM_DrawNeiGi(const CwItemDrawInfo& info);
bool MM_DescribeNeiGi(RandoItemId item, CwItemDrawInfo* out);
bool MM_TryDrawNeiGi(RandoItemId item);

// Wrap only the legacy fallback after the authored presentation was declined.
// Its model may change the matrix; shimmer uses the incoming GI pose.
class MM_NeiGiFallbackShimmer {
  public:
    explicit MM_NeiGiFallbackShimmer(RandoItemId item);
    ~MM_NeiGiFallbackShimmer();
    MM_NeiGiFallbackShimmer(const MM_NeiGiFallbackShimmer&) = delete;
    MM_NeiGiFallbackShimmer& operator=(const MM_NeiGiFallbackShimmer&) = delete;

  private:
    NeiGi::Kind mKind;
    bool mEnabled;
};
