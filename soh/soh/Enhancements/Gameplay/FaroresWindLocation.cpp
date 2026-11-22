#include "global.h"
#include "gameplay/FaroresWindLocation.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/unbound/SceneDB.h"
#include "soh/util.h"
#ifdef COMBO_BUILD
#include "ComboExport.h"
#endif

extern "C" f32 sFontWidths[144];

namespace {
constexpr uint16_t kFaroresWindTextId = 0x003B;

const char* GetLocationName(int32_t entrance) {
    if (!EntranceDB_IsIndexValid(entrance, 0))
        return nullptr;
    const int32_t scene = gEntranceTable[entrance].scene;
    return SceneDB_IsValid(scene) ? SohUtils::GetSceneName(scene).c_str() : nullptr;
}

void ShowDestination(uint16_t* textId, bool* loadFromMessageTable) {
    if (!*loadFromMessageTable)
        return;

    std::string_view prefix = "Return point: ";
    if (gSaveContext.language == LANGUAGE_GER)
        prefix = "Zielpunkt: ";
    else if (gSaveContext.language == LANGUAGE_FRA)
        prefix = "Point de retour : ";

    const std::string heading = FaroresWind::Heading(0, gSaveContext.fw.set ? gSaveContext.fw.entranceIndex : -1,
                                                     prefix, GetLocationName, [](const std::string& text) {
                                                         float width = 0.0f;
                                                         for (unsigned char character : text)
                                                             width += sFontWidths[character - ' '] * 0.75f;
                                                         return width <= 216.0f;
                                                     });
    if (heading.empty())
        return;

    const CustomMessage native = CustomMessage::LoadVanillaMessageTableEntry(*textId);
    std::string body = native.GetForCurrentLanguage(MF_RAW);
    const size_t choice = body.find(CustomMessage::THREE_WAY_CHOICE());
    if (choice == std::string::npos)
        return;
    const size_t newline = body.rfind(CustomMessage::NEWLINE(), choice);
    if (newline == std::string::npos)
        return;

    body.replace(0, newline, heading);
    CustomMessage(body, native.GetTextBoxType(), native.GetTextBoxPosition()).LoadIntoFont();
    *loadFromMessageTable = false;
}

void Register() {
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(kFaroresWindTextId, ShowDestination);
}

RegisterShipInitFunc init(Register);
} // namespace

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT const char* SOH_GetFwLocationName(int32_t entrance) {
    return GetLocationName(entrance);
}
#endif
