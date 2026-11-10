#include "particle_cmc.h"
#include "combo/rando/ContainerCategoryColors.h"

static_assert(ITEM_CATEGORY_JUNK == 0 && ITEM_CATEGORY_LESSER == 1 && ITEM_CATEGORY_HEALTH == 2 &&
              ITEM_CATEGORY_BOSS_KEY == 3 && ITEM_CATEGORY_SMALL_KEY == 4 && ITEM_CATEGORY_SKULLTULA_TOKEN == 5 &&
              ITEM_CATEGORY_MAJOR == 6);

Color_RGBA8 Randomizer_GetParticleCMCColor(GetItemCategory colorIndex, ParticleCMCColorType colorType) {
    const auto color = ComboRando::ContainerCategoryColors::Get(
        static_cast<ComboRando::ContainerCategoryColors::Category>(colorIndex), colorType);
    return { color.r, color.g, color.b, color.a };
}
