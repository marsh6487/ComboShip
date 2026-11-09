"""Build real Saria message-policy/resolver callbacks around controlled storage."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/cross_hints'))
from run_item_hint_export_tests import block


def fixture():
    custom = ROOT / 'soh/soh/Enhancements/custom-message'
    rando = ROOT / 'soh/soh/Enhancements/randomizer'
    header = (custom / 'CustomMessageManager.h').read_text()
    messages = (custom / 'CustomMessageManager.cpp').read_text()
    hints = (rando / '3drando/hints.cpp').read_text()
    runtime = (rando / 'Messages/StaticHints.cpp').read_text()
    hint_type = (rando / 'hint.cpp').read_text()
    text_types = (custom / 'CustomMessageTypes.h').read_text()
    rows = '\n'.join(line for line in runtime.splitlines()
                     if 'COND_ID_HOOK(OnOpenText, TEXT_SARIA' in line)
    ids = re.findall(r'(TEXT_SARIA\w*\s*=\s*0x[0-9A-Fa-f]+)', text_types)
    prefix = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "rando/CrossForeign.h"
#include "rando/SariaHintPolicy.h"
#include "soh/Enhancements/randomizer/EnemyFirstDrops.h"
using namespace std::literals::string_literals;
struct CwItemReceiptPresentation {};
class Text;
enum TextBoxType { TEXTBOX_TYPE_BLACK, TEXTBOX_TYPE_BLUE };
enum TextBoxPosition { TEXTBOX_POS_BOTTOM };
using ItemID = int;
enum { LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA, LANGUAGE_JPN, LANGUAGE_MAX };

#define TODO_TRANSLATE "TranslateThis"
#define SPDLOG_DEBUG(...) ((void)0)
extern "C" int SariaFixture_GetClassic(void);
extern "C" int SariaFixture_GetStat(void);
extern "C" void SariaFixture_SetOptions(int classic, int stat);
extern "C" int SariaFixture_IsRandomizer(void);
extern "C" void SariaFixture_SetRandomizer(int enabled);
#define IS_RANDO SariaFixture_IsRandomizer()
static uint64_t rando_state = 1234567;
static unsigned revealed;
static std::string loadedEn, loadedDe, loadedFr;
static TextBoxType loadedType;
'''
    parts = [prefix, header[header.index('#define QM_WHITE'):header.index('/**')],
             block(header, 'class CustomMessage {', True)]
    for marker in ('static const std::unordered_map<std::string, char> textBoxSpecialCharacters',
                   'static const std::unordered_map<std::string, std::string> percentColors',
                   'static const std::unordered_map<std::string, std::string> colorToPercent',
                   'static std::map<std::string, int> pixelWidthTable'):
        parts.append(block(messages, marker, True))
    parts.append('static const std::unordered_map<std::string, ItemID> altarIcons;\n')
    for marker in ('CustomMessage::CustomMessage(std::string english_, std::string german_, std::string french_, TextBoxType',
                   'CustomMessage::CustomMessage(std::string english_, std::string german_, std::string french_,\n',
                   'CustomMessage::CustomMessage(std::string english_, TextBoxType',
                   'const std::string CustomMessage::GetEnglish(', 'const std::string CustomMessage::GetGerman(',
                   'const std::string CustomMessage::GetFrench(', 'const std::string CustomMessage::GetForLanguage(',
                   'void CustomMessage::InsertNames(', 'void CustomMessage::Capitalize(',
                   'void CustomMessage::Replace(std::string&& oldStr, CustomMessage',
                   'const TextBoxType& CustomMessage::GetTextBoxType(',
                   'const TextBoxPosition& CustomMessage::GetTextBoxPosition(',
                   'void CustomMessage::SetTextBoxType(',
                   'void CustomMessage::operator+=(const std::string&',
                   'static size_t NextLineLength(', 'size_t CustomMessage::FindNEWLINE(',
                   'bool CustomMessage::AddBreakString(', 'void CustomMessage::AutoFormatString(',
                   'void CustomMessage::AutoFormat()', 'void CustomMessage::EncodeColors(',
                   'void CustomMessage::ReplaceColors(', 'void CustomMessage::ReplaceSpecialCharacters(',
                   'void CustomMessage::ReplaceAltarIcons(', 'void CustomMessage::ProcessMessageFormat(',
                   'std::string CustomMessage::MESSAGE_END()', 'std::string CustomMessage::NEWLINE()',
                   'std::string CustomMessage::COLOR(', 'std::string CustomMessage::WAIT_FOR_INPUT()',
                   'std::string CustomMessage::PLAYER_NAME()', 'std::string CustomMessage::ITEM_OBTAINED('):
        parts.append(block(messages, marker))
    parts.append(r'''
// Native insertion/color encoding/autoformatting remain real. Only GPU/font
// storage and unused formatting modes are service boundaries.
void CustomMessage::FormatString(std::string&) const { throw std::runtime_error("expected autoformat or raw"); }
void CustomMessage::CleanString(std::string&) const { throw std::runtime_error("expected autoformat or raw"); }
void CustomMessage::LoadIntoFont() {
    loadedEn = GetEnglish(MF_RAW); loadedDe = GetGerman(MF_RAW); loadedFr = GetFrench(MF_RAW);
    loadedType = GetTextBoxType();
}
''')
    parts.append(block((rando / '3drando/hints.hpp').read_text(), 'class HintText', True))
    parts.append(block(hints, 'HintText::HintText('))
    parts.append(block(hints, 'const CustomMessage& HintText::GetClear()'))
    parts.append('using oJson = nlohmann::ordered_json;\nnamespace Rando {\n')
    parts.append(block((rando / 'hint.h').read_text(), 'class Hint {', True))
    parts.append(block(hint_type, 'Hint::Hint()'))
    parts.append(block(hint_type, 'Hint::Hint(RandomizerHint ownKey_, std::vector<CustomMessage>'))
    parts.append(block(hint_type, 'bool Hint::IsEnabled()'))
    parts.append(r'''
const CustomMessage Hint::GetHintMessage(MessageFormat format, size_t id) const {
    CustomMessage hintText = messages.at(id);
    /* NATIVE SONG TEXTBOX POLICY */
    if (format == MF_AUTO_FORMAT)
        hintText.AutoFormat();
    return hintText;
}
struct Option { int value; int Get() const { return value; } explicit operator bool() const { return value; } };
struct ItemLocation {
    int item = 0;
    bool hinted = false, accessible = false;
    int GetPlacedRandomizerGet() const { return item; }
};
struct Location { std::string name; int home; const std::string& GetName() const { return name; } int GetArea() const { return home; } };
namespace StaticData {
std::array<Location, RC_MAX> locations{{{"sentinel", -1}, {"Z upgrade", 1}, {"A upgrade", 2}, {"Not magic", 3}}};
Location* GetLocation(int check) { return &locations.at(check); }
std::map<int, HintText> hintTextTable;
}
struct Context {
    std::array<ItemLocation, RC_MAX> placements;
    Hint hint;
    unsigned added = 0;
    unsigned seed = 42;
    static Context* GetInstance() { static Context ctx; return &ctx; }
    Hint* GetHint(int) { return &hint; }
    Option GetOption(int key) { return { key == RSK_SARIA_HINT ? SariaFixture_GetClassic() : SariaFixture_GetStat() }; }
    ItemLocation* GetItemLocation(int check) { return &placements.at(check); }
    unsigned GetSeed() const { return seed; }
    void AddHint(int, Hint value) { hint = std::move(value); ++added; }
};
}
using namespace Rando;
namespace RandomizerCheckObjects { std::string GetRCAreaName(int home) { return home == 1 ? "Kokiri Forest" : "Hyrule Field"; } }
static RandomizerCheck sEarlySariaMagicCheck = RC_UNKNOWN_CHECK;
static uint32_t sEarlySariaMagicSeed;
static bool sSariaMagicReachabilityVerified;
struct GameInteractor {
    struct OnRandoHintRevealed {};
    static GameInteractor* Instance;
    template<typename Hook> void ExecuteHooks(int key) { assert(key == RH_SARIA_HINT); ++revealed; }
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
#define RAND_GET_HINT(key) Context::GetInstance()->GetHint(key)
#define RAND_GET_OPTION(key) Context::GetInstance()->GetOption(key)
''')
    parts.append('enum { ' + ','.join(ids) + ' };\n')
    parts.append(block(hints, 'bool EnsureSariaMagicHint('))
    parts.append(block(runtime, 'static CustomMessage ReadHintMessage('))
    parts.append(block(runtime, 'void BuildSariaMessage('))
    parts.append(block(runtime, 'static bool SariaHintEnabled('))
    parts.append(r'''
using Callback = void(*)(uint16_t*, bool*);
static std::map<int, Callback> registered;
#define COND_ID_HOOK(hook, id, enabled, callback) if (enabled) registered[id] = callback
void RegisterSariaCallbacks() {
''')
    parts.append(rows + '\n}\nvoid InitTemplates() {\n')
    native_templates = (rando / '3drando/hint_list.cpp').read_text()
    for key in ('RHT_SARIA_TALK_HINT', 'RHT_SARIA_SONG_HINT'):
        start = native_templates.index('hintTextTable[' + key + '] =')
        end = native_templates.index('));', start) + 3
        parts.append('StaticData::' + native_templates[start:end])
    parts.append('}\n')
    parts.append((ROOT / 'tests/saria_magic/runtime_hint_cases.inc').read_text())
    song_style = block(hint_type, 'if (ownKey == RH_SARIA_HINT && id == 1)')
    return '\n'.join(parts).replace('/* NATIVE SONG TEXTBOX POLICY */', song_style)
