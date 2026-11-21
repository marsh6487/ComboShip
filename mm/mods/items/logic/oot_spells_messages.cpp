#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/ShipUtils.h"
#include "global.h"
#include "mods/nei_save.h"
#include "gameplay/FaroresWindLocation.h"
#include <utility>
#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "entrances/BlackMarketEntrance.h"
#endif

extern "C" {
extern s32 gSceneEntranceTableCount;
extern SceneEntranceTableEntry sSceneEntranceTable[];
}

namespace {
const char* GetLocationName(int32_t entrance) {
    if (entrance < 0 || entrance > UINT16_MAX)
        return nullptr;
    const int entranceScene = entrance >> 9;
    const int spawn = (entrance >> 4) & 0x1F;
    if (entranceScene >= gSceneEntranceTableCount)
        return nullptr;
    const auto& sceneTable = sSceneEntranceTable[entranceScene];
    if (!sceneTable.table || spawn >= sceneTable.tableCount || !sceneTable.table[spawn])
        return nullptr;

    // Use the ordinary scene entry. Layer-specific arrays can be shorter than
    // the four-bit cutscene selector and share the same destination name.
    const int scene = ABS_ALT(sceneTable.table[spawn][0].sceneId);
#ifdef COMBO_BUILD
    if (scene == COMBO_BLACK_MARKET_SCENE)
        return "Skiejer's Black Market";
#endif
    return Ship_GetSceneName(scene);
}
} // namespace

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT const char* MM_GetFwLocationName(int32_t entrance) {
    return GetLocationName(entrance);
}
#endif

extern "C" void OotSpells_OpenFwTextbox(void) {
    const auto* fw = Nei_Save();
    const std::string heading = FaroresWind::Heading(1, fw->fwSet ? fw->fwEntrance : -1,
                                                     "Return point: ", GetLocationName, [](const std::string& text) {
                                                         float width = 0.0f;
                                                         for (unsigned char character : text)
                                                             width += Ship_GetCharFontWidthNES(character);
                                                         return width <= 300.0f;
                                                     });

    CustomMessage::Entry entry;
    entry.autoFormat = false;
    // MM's four-row textbox needs one heading and three choice rows. Encode
    // the existing return/dispel/exit choices without wrapping those rows.
    std::string body = heading.empty() ? "You cast Farore's Wind!" : heading;
    body += "\002\021\303Return to the Warp Point\021Dispel the Warp Point\021Exit\277";
    CustomMessage::StartTextbox(std::move(body), entry);
}
