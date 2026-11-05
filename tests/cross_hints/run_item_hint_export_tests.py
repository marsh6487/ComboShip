#!/usr/bin/env python3
"""Compile the real item exporter, native text encoding, and downstream picker.

Requires a C++20 compiler and nlohmann/json.hpp (NLOHMANN_JSON_INCLUDE may
select its include directory). In a source slice, COMBOSHIP_NATIVE_SOURCE_ROOT
may point at a full native source tree of the same revision. No network access
or game/save writes occur. Engine UI formatting is deliberately unavailable;
the native MF_ENCODE path is compiled unchanged.
"""

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
NATIVE_ROOT = Path(os.environ.get("COMBOSHIP_NATIVE_SOURCE_ROOT", ROOT))
CUSTOM = "soh/soh/Enhancements/custom-message/"
RANDO = "soh/soh/Enhancements/randomizer/"


def source(path):
    local = ROOT / path
    return (local if local.is_file() else NATIVE_ROOT / path).read_text()


def block(text, marker, declaration=False):
    """Extract one native definition without rewriting its body."""
    start = text.index(marker)
    opening = text.index("{", start)
    depth = 1
    # Ignore braces in strings/comments, including the native diagnostic text.
    tokens = re.finditer(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]',
                         text[opening + 1:])
    for token in tokens:
        value = token.group()
        if value == "{":
            depth += 1
        elif value == "}":
            depth -= 1
            if depth == 0:
                end = opening + 1 + token.end()
                return text[start:end] + (";" if declaration else "") + "\n"
    raise ValueError(f"Unclosed definition: {marker}")


def fixture():
    messages_h = source(CUSTOM + "CustomMessageManager.h")
    messages = source(CUSTOM + "CustomMessageManager.cpp")
    text_cpp = source(CUSTOM + "text.cpp")
    hints = source(RANDO + "3drando/hints.cpp")
    item = source(RANDO + "item.cpp")
    exporter = source("soh/soh/OTRGlobals.cpp")
    cross = source("combo/rando/CrossHints.h")
    parts = [r'''
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
using namespace std::literals::string_literals;
// Engine-only declaration dependencies; no layout/ABI assertion is made here.
struct CwItemReceiptPresentation {};
enum TextBoxType { TEXTBOX_TYPE_BLACK };
enum TextBoxPosition { TEXTBOX_POS_BOTTOM };
using ItemID = int;
enum { LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA };
#define TODO_TRANSLATE "TranslateThis"
#define SPDLOG_DEBUG(...) ((void)0)
''']
    parts.append(messages_h[messages_h.index("#define QM_WHITE"):messages_h.index("/**")])
    parts.append(block(source(CUSTOM + "text.h"), "class Text", True))
    parts.append(block(messages_h, "class CustomMessage {", True))
    parts.append(block(source(RANDO + "3drando/hints.hpp"), "class HintText", True))
    parts.append(block(messages, "static const std::unordered_map<std::string, std::string> colorToPercent", True))
    parts.append("Text::Text() = default;\n")
    for marker in (
        "Text::Text(std::string english_, std::string french_, std::string german_)",
        "static bool IsMissing(", "const std::string& Text::GetEnglish()",
        "const std::string& Text::GetFrench()", "const std::string& Text::GetGerman()",
    ):
        parts.append(block(text_cpp, marker))
    for marker in (
        "CustomMessage::CustomMessage(std::string english_, std::string german_, std::string french_, TextBoxType",
        "CustomMessage::CustomMessage(std::string english_, std::string german_, std::string french_,\n",
        "const std::string CustomMessage::GetEnglish(", "const std::string CustomMessage::GetGerman(",
        "const std::string CustomMessage::GetFrench(", "const std::string CustomMessage::GetForLanguage(",
        "void CustomMessage::ProcessMessageFormat(", "void CustomMessage::EncodeColors(",
    ):
        parts.append(block(messages, marker))
    parts.append(r'''
void CustomMessage::FormatString(std::string&) const { throw std::runtime_error("expected MF_ENCODE"); }
void CustomMessage::CleanString(std::string&) const { throw std::runtime_error("expected MF_ENCODE"); }
void CustomMessage::AutoFormatString(std::string&) const { throw std::runtime_error("expected MF_ENCODE"); }
''')
    for marker in (
        "HintText::HintText(", "const CustomMessage& HintText::GetClear()",
        "const CustomMessage& HintText::GetObscure(size_t", "const CustomMessage& HintText::GetAmbiguous(size_t",
        "size_t HintText::GetAmbiguousSize()", "size_t HintText::GetObscureSize()",
    ):
        parts.append(block(hints, marker))
    parts.append(r'''
enum RandomizerHintTextKey { RHT_NONE, RHT_AUTHORED };
namespace Rando {
namespace StaticData { HintText hintTextTable[2]; }
// Narrow native Item boundary: use its real accessor implementations below.
class Item {
    Text name;
    RandomizerHintTextKey hintKey;
public:
    Item(Text value, RandomizerHintTextKey key) : name(std::move(value)), hintKey(key) {}
    const Text& GetName() const;
    RandomizerHintTextKey GetHintKey() const;
    const HintText& GetHint() const;
};
''')
    for marker in ("const Text& Item::GetName()", "RandomizerHintTextKey Item::GetHintKey()",
                   "const HintText& Item::GetHint()"):
        parts.append(block(item, marker))
    parts.append("}\n")
    parts.append(block(exporter, "static nlohmann::json Combo_CustomMessageToJson("))
    parts.append(block(exporter, "static nlohmann::json Combo_HintTextToJson("))
    if "static nlohmann::json Combo_ItemHintToJson(" in exporter:
        parts.append(block(exporter, "static nlohmann::json Combo_ItemHintToJson("))
    # Compile the caller too: an unconnected helper must not make this test green.
    caller = re.search(r'items\.push_back\(\{ \{ "name", name \}, \{ "hint", [^\n]+', exporter)
    if not caller:
        raise ValueError("Could not locate native per-item export call")
    parts.append('nlohmann::json exportItem(const Rando::Item& item) {\n'
                 'const auto& name = item.GetName().GetEnglish();\n'
                 'auto items = nlohmann::json::array();\n' + caller.group() +
                 '\nreturn items.at(0).at("hint");\n}\n')
    parts.append("namespace ComboRando {\n")
    parts.append(block(source("combo/rando/CrossWorldRando.h"), "struct CwRng", True))
    parts.append(block(cross, "struct Tri", True))
    parts.append(block(cross, "inline Tri FromJson("))
    parts.append(block(cross, "inline Tri PickTemplate("))
    parts.append("}\n")
    parts.append(TESTS)
    return "\n".join(parts)


TESTS = r'''
using nlohmann::json;
using namespace ComboRando;
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
bool equal(const Tri& a, const Tri& b) { return a.en == b.en && a.de == b.de && a.fr == b.fr; }
int main() {
    Rando::StaticData::hintTextTable[RHT_NONE] = HintText(CustomMessage("No Hint", "Kein Hinweis", "Pas d'Indice"));
    const HintText authored(
        CustomMessage("#Clear# [[1]]", "#Klar# [[1]]", "#Clair# [[1]]", {QM_GREEN}),
        {CustomMessage("Amb A", "Mehr A", "Ambigu A"), CustomMessage("Amb B", "Mehr B", "Ambigu B")},
        {CustomMessage("Obs A", "Dunkel A", "Obscur A"), CustomMessage("Obs B", "Dunkel B", "Obscur B"),
         CustomMessage("Obs C", "Dunkel C", "Obscur C")});
    Rando::StaticData::hintTextTable[RHT_AUTHORED] = authored;
    // Text is EN/FR/DE, whereas CustomMessage is EN/DE/FR.
    const Rando::Item missing(Text("Hylia's Grace", "Grâce d'Hylia", "Hylias Gnade"), RHT_NONE);
    const json replacement = exportItem(missing);
    const json expected = json::parse(R"({"clear":{"en":"Hylia's Grace","de":"Hylias Gnade","fr":"Grâce d'Hylia"},"ambiguous":[],"obscure":[]})");
    require(replacement == expected, "RHT_NONE exports actual EN/DE/FR names with empty variant arrays");
    std::cout << "PASS: translated RHT_NONE names and empty variant shape\n";
    const Rando::Item real(Text("Other name", "Autre nom", "Anderer Name"), RHT_AUTHORED);
    const json authoredExport = exportItem(real);
    const json authoredExpected = json::parse(R"({"clear":{"en":"%gClear%w [[1]]","de":"%gKlar%w [[1]]","fr":"%gClair%w [[1]]"},"ambiguous":[{"en":"Amb A","de":"Mehr A","fr":"Ambigu A"},{"en":"Amb B","de":"Mehr B","fr":"Ambigu B"}],"obscure":[{"en":"Obs A","de":"Dunkel A","fr":"Obscur A"},{"en":"Obs B","de":"Dunkel B","fr":"Obscur B"},{"en":"Obs C","de":"Dunkel C","fr":"Obscur C"}]})");
    require(authoredExport == authoredExpected, "authored multilingual colors/placeholders/variants unchanged");
    require(authoredExport == Combo_HintTextToJson(authored), "authored export matches existing serializer byte for byte");
    std::cout << "PASS: authored hints preserve encoded colors, placeholders and every variant\n";
    const Rando::Item encoded(Text("%g#Rune# [[1]]&€", "%r#Rune FR# [[2]]^é", "%b#Rune DE# [[3]]@ä"), RHT_NONE);
    require(exportItem(encoded) == json::parse(R"({"clear":{"en":"%gRune [[1]]&€","de":"%bRune DE [[3]]@ä","fr":"%rRune FR [[2]]^é"},"ambiguous":[],"obscure":[]})"),
            "fallback names use native MF_ENCODE; percent codes, markers and UTF-8 survive");
    require(exportItem(Rando::Item(Text("Fallback", "", "TranslateThis"), RHT_NONE)).at("clear") ==
            json::parse(R"({"en":"Fallback","de":"Fallback","fr":"Fallback"})"), "native missing-translation fallback retained");
    std::cout << "PASS: native encoded names and missing-translation behavior\n";
    const auto sentinel = Combo_HintTextToJson(Rando::StaticData::hintTextTable[RHT_NONE]);
    require(sentinel.at("clear").at("en") == "No Hint", "shared location/template sentinel remains untouched");
    for (int clarity = 0; clarity < 3; ++clarity) {
        for (uint64_t seed = 1; seed <= 64; ++seed) {
            CwRng oldStream(seed), newStream(seed);
            const auto text = PickTemplate(replacement, clarity, newStream);
            PickTemplate(sentinel, clarity, oldStream);
            require(equal(text, {"Hylia's Grace", "Hylias Gnade", "Grâce d'Hylia"}), "every clarity renders actual translated item");
            require(oldStream.s == seed && newStream.s == seed, "fallback has zero variant RNG draws");
            // A later ordinary multivariant selection must remain identical.
            for (int later = 0; later < 12; ++later) {
                require(equal(PickTemplate(authoredExport, clarity, oldStream), PickTemplate(authoredExport, clarity, newStream)),
                        "downstream multivariant hint selections unchanged");
            }
            require(oldStream.s == newStream.s, "downstream RNG state unchanged");
            // Trap selection draws from true authored item, then renders fallback disguise.
            CwRng oldTrap(seed), newTrap(seed);
            PickTemplate(authoredExport, clarity, oldTrap, &sentinel);
            const auto disguised = PickTemplate(authoredExport, clarity, newTrap, &replacement);
            require(equal(disguised, text), "trap renders fallback disguise name at every clarity");
            require(oldTrap.s == newTrap.s, "trap preserves true-item variant draws");
            require(equal(PickTemplate(authoredExport, clarity, oldTrap), PickTemplate(authoredExport, clarity, newTrap)),
                    "later selections remain identical after trap fallback");
        }
    }
    std::cout << "PASS: all clarity tiers, 64 seeds, downstream selections and trap RNG continuity\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="comboship-item-hint-") as temp:
        cpp = Path(temp) / "item_hint_export.cpp"
        binary = Path(temp) / "item_hint_export"
        cpp.write_text(fixture())
        command = shlex.split(os.environ.get("CXX", "c++"))
        command += ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1"]
        if os.environ.get("NLOHMANN_JSON_INCLUDE"):
            command += ["-I", os.environ["NLOHMANN_JSON_INCLUDE"]]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        subprocess.run(command + [str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("Item hint export fixture passed" + (" (ASan/UBSan)" if args.sanitize else ""))


if __name__ == "__main__":
    main()
