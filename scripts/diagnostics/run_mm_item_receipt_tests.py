#!/usr/bin/env python3
"""Exercise MM's production queued receipt/grant boundary with engine seams replaced."""
from pathlib import Path
import os
import re
import sys
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def verify_windows_reward_export_contract():
    """MSVC requires the declaration and definition to agree on DLL linkage."""
    header = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.h').read_text()
    source = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
    declaration = re.search(r'extern "C"[^;]*MM_GetDungeonRewardName\([^;]*;', header)
    definition = re.search(r'extern "C"[^\{]*MM_GetDungeonRewardName\([^\{]*\{', source)
    assert declaration and definition
    assert 'COMBO_EXPORT' in declaration.group(0), 'MM reward declaration lacks Windows DLL export linkage'
    assert 'COMBO_EXPORT' in definition.group(0), 'MM reward definition lacks Windows DLL export linkage'


verify_windows_reward_export_contract()


def block(source, start):
    begin = source.index('{', source.index(start))
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]


source = (ROOT / 'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
give = block(source, '.giveItem =')
donor_catalog = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
donor_names = set(re.findall(r'itemTable\[RG_\w+\]\s*=\s*Item\(.*?Text\{\s*"([^"\n]*)"', donor_catalog))
concrete_names = set(re.findall(r'"([^"\n]*)"',
    block((ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text(), 'const char* ConcreteReceiptName')))
assert concrete_names <= donor_names, f'Unknown exact donor keys: {concrete_names - donor_names}'
preamble = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <iostream>
#include "mm/2s2h/Rando/Types.h"
struct Actor {}; struct PlayState {};
namespace CustomItem { constexpr int GIVE_ITEM_CUTSCENE=1; }
int flags=1, param=RC_UNKNOWN, param2=0;
#define CUSTOM_ITEM_FLAGS flags
#define CUSTOM_ITEM_PARAM param
#define CUSTOM_ITEM_PARAM2 param2
struct Check { RandoItemId randoItemId=RI_UNKNOWN; bool obtained=false,cycleObtained=false,eligible=true; };
std::map<int,Check> checks;
#define RANDO_SAVE_CHECKS checks
#define RANDO_SAVE_OPTIONS options
std::map<int,int> options;
struct { struct { struct { struct { int foundTriforcePieces=0; } rando; } shipSaveInfo; } save; } gSaveContext;
int gMMComboGoalRequired=0;
int (*gMMComboOtherTriforceCount)()=nullptr;
int CVarGetInteger(const char*,int fallback) { return fallback; }
int granted=0,delivered=0;
bool queued=true;
namespace CustomMessage {
struct Entry { uint8_t textboxType=0,textboxYPos=0,icon=0xFE; uint16_t nextMessageID=0xFFFF,firstItemCost=0xFFFF,secondItemCost=0xFFFF; bool autoFormat=true; std::string msg; };
Entry shown;
void SetActiveCustomMessage(std::string msg,Entry e) { e.msg=msg;shown=e; }
void StartTextbox(std::string msg,Entry e) { e.msg=msg;shown=e; }
std::string RemoveColorCodes(std::string s) { return s; }
}
namespace Notification { struct Info { const char* itemIcon=nullptr;std::string message,suffix; };void Emit(Info){} }
namespace ComboRando {
struct ForeignItem { int itemGame=0;std::string itemName="Deku Leaf",displayName="Deku Leaf (OOT)";bool advancement=true,trap=false;std::string category="major"; };
std::string ShownForeignName(const ForeignItem&,const char* p) {return std::string(p)+" (OOT)";}
}
ComboRando::ForeignItem foreign;
void MMAnchor_BroadcastCrossItem(int,const char*,const char*) {}
void SaveManager_SaveCurrentForCombo() {}
namespace Rando {
void AppendReceiptSource(CustomMessage::Entry&,const std::string&);
namespace StaticData {
struct Item {RandoItemType randoItemType=RITYPE_MAJOR;};
std::map<RandoItemId,Item> Items;
std::string GetItemName(RandoItemId,bool=true,RandoCheckId=RC_UNKNOWN) {return "the Deku Leaf";}
uint8_t GetIconForZMessage(RandoItemId) {return 0xF5;}
bool ShouldShowGetItemCutscene(RandoItemId) {return true;}
const char* GetIconTexturePath(RandoItemId) {return "icon";}
std::string GetCheckDisplayName(RandoCheckId) {return "test check";}
}
RandoItemId ConvertItem(RandoItemId i,RandoCheckId){return i;}
RandoItemId CurrentJunkItem(RandoCheckId){return RI_RUPEE_GREEN;}
void GiveItem(RandoItemId,RandoCheckId){++granted;}
void LatchComboForeign(RandoCheckId){}
const char* ComboForeignLatchedName(RandoCheckId){return "Deku Leaf";}
uint8_t ComboForeignMessageIcon(RandoCheckId){return 0xF5;}
namespace MiscBehavior {
std::string BankRewardSourceSuffix(RandoCheckId){return " (Bank reward)";}
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId){return &foreign;}
bool ShouldShowForeignCutscene(RandoCheckId){return true;}
void OfferTrapItem(){}
void SendForeignCheck(RandoCheckId){++delivered;}
void BroadcastCheckObtainedIfFirst(RandoCheckId,RandoItemId,bool){}
std::string GetTrapMessage(){return "A trap!";}
}
// This seam stands in for the engine/catalog lookup, not for receipt selection.
bool ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& e) {
 if(id!=RI_OOT_NEI_DEKU_LEAF) return false;
 e.msg="You got the Deku Leaf!\x10Use it to glide and blow gusts.";e.autoFormat=false;return true;
}
bool ApplyForeignItemReceiptText(const char* name,CustomMessage::Entry& e,RandoCheckId=RC_UNKNOWN) {
 if(std::string(name)!="Deku Leaf") return false;
 return ApplyItemReceiptText(RI_OOT_NEI_DEKU_LEAF,e);
}
}
'''
checks_source = r'''
int main() {
 Actor actor; PlayState play;
 checks[param].randoItemId=RI_OOT_NEI_DEKU_LEAF;
 Apply(&actor,&play);
 assert(CustomMessage::shown.msg.find("glide")!=std::string::npos && "major item lost its full description");
 assert(CustomMessage::shown.icon==0xF5);
 assert(CustomMessage::shown.msg.find("Bank reward")!=std::string::npos);
 assert(granted==1 && checks[RC_UNKNOWN].obtained); // message change cannot swallow the grant
 param=RC_UNKNOWN;flags=1;checks[param]={RI_COMBO_FOREIGN};
 Apply(&actor,&play);
 assert(CustomMessage::shown.msg.find("glide")!=std::string::npos && "foreign OoT item lost its description");
 assert(delivered==1 && granted==1);
 param=RC_UNKNOWN;flags=1;checks[param]={RI_COMBO_FOREIGN};foreign.trap=true;
 Apply(&actor,&play);
 assert(CustomMessage::shown.msg.find("glide")==std::string::npos && "trap revealed its disguise description");
 assert(delivered==1);
 std::cout<<"queued native/foreign receipts, icons, bank attribution, grants and traps passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-item-receipts-') as tmp:
    tmp = Path(tmp)
    compiler = os.environ.get('CXX', 'c++')
    extra = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitizers' in sys.argv else []
    icon = (ROOT / 'tests/item_receipts/icon_test.cpp').read_text()
    icon_source = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
    icon = icon.replace('/* ICON_SELECTOR */',
        'static int32_t OOT_FillItemIconInfo(RandomizerGet rg,CwItemIconInfo* out) ' +
        block(icon_source, 'static int32_t OOT_FillItemIconInfo'))
    tu = tmp / 'icon.cpp'
    tu.write_text(icon)
    exe = tmp / 'icon'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    foreign_icon = (ROOT / 'tests/item_receipts/foreign_icon_test.cpp').read_text()
    owner_header = ROOT / 'combo/menu/ComboItemIconOwnership.h'
    foreign_icon = foreign_icon.replace('/* ICON_OWNER_INCLUDE */',
        '#include "combo/menu/ComboItemIconOwnership.h"' if owner_header.exists() else '')
    foreign_icon = foreign_icon.replace('/* FOREIGN_ICON_SELECTOR */',
        'uint8_t Rando::ComboForeignMessageIcon(RandoCheckId check) ' +
        block((ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text(), 'uint8_t Rando::ComboForeignMessageIcon'))
    tu = tmp / 'foreign_icon.cpp'
    tu.write_text(foreign_icon)
    exe = tmp / 'foreign_icon'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    song = (ROOT / 'tests/item_receipts/song_icons_test.cpp').read_text()
    messages = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    song = song.replace('/* CUSTOM_ICON_FUNCTIONS */',
        'void LoadCustomItemIcon(bool displayAsEnglish) ' + block(messages, 'void LoadCustomItemIcon') + '\n' +
        'void DrawCustomItemIcon(Gfx** p) ' + block(messages, 'void DrawCustomItemIcon'))
    engine = (ROOT / 'soh/src/code/z_message_PAL.c').read_text()
    song = song.replace('/* ENGINE_ICON_DRAW */',
        'u16 Message_DrawItemIcon(PlayState* play,u16 itemId,Gfx** p,u16 i) ' +
        block(engine, 'u16 Message_DrawItemIcon'))
    mm_message = (ROOT / 'mm/src/code/z_message.c').read_text()
    song_palette = ''
    for channel in ('D_801CFE04', 'D_801CFE1C', 'D_801CFE34'):
        song_palette += 's16 ' + channel + '[] = ' + block(mm_message, 's16 ' + channel + '[]') + ';\n'
    song = song.replace('/* MM_SONG_PALETTE */', song_palette)
    native_load = block(mm_message, 'void Message_LoadItemIcon')
    stage_start = mm_message.index('#define MESSAGE_CUSTOM_ICON_ITEM')
    song = song.replace('/* MM_CUSTOM_STAGE */',
        mm_message[stage_start:mm_message.index('// #endregion', stage_start)])
    song = song.replace('/* MM_CUSTOM_LOAD */',
        block(native_load, 'if ((itemId == MESSAGE_CUSTOM_ICON_ITEM)').replace('return;', ''))
    song = song.replace('/* MM_SONG_LOAD */',
        block(native_load, '} else if ((itemId >= ITEM_SONG_SONATA)'))
    native_draw = block(mm_message, 'void Message_DrawItemIcon')
    song = song.replace('/* MM_CUSTOM_DRAW */',
        block(native_draw, '} else if ((msgCtx->itemId == MESSAGE_CUSTOM_ICON_ITEM)'))
    song = song.replace('/* MM_SONG_DRAW */',
        block(native_draw, '} else if ((msgCtx->itemId >= ITEM_SONG_SONATA)'))
    rect = native_draw.rindex('gSPTextureRectangle(')
    song = song.replace('/* MM_ICON_RECTANGLE */', native_draw[rect:native_draw.index(';', rect) + 1])
    tu = tmp / 'song.cpp'
    tu.write_text(song)
    exe = tmp / 'song'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    magic = (ROOT / 'tests/item_receipts/magic_test.cpp').read_text()
    item_messages = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    magic = magic.replace('/* MAGIC_BUILDER */',
        'void BuildMagicStatUpgradeMessage(CustomMessage& msg) ' +
        block(item_messages, 'void BuildMagicStatUpgradeMessage(CustomMessage& msg) {'))
    tu = tmp / 'magic.cpp'
    tu.write_text(magic)
    exe = tmp / 'magic'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    tu = tmp / 'queue.cpp'
    helpers = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
    append = block(helpers, 'void Rando::AppendReceiptSource')
    tu.write_text(preamble + '\nvoid Rando::AppendReceiptSource(CustomMessage::Entry& entry,const std::string& source) ' + append +
                  '\nnamespace Rando::MiscBehavior { void Apply(Actor* actor,PlayState* play) ' + give +
                  '\n}\nusing Rando::MiscBehavior::Apply;\n' + checks_source)
    exe = tmp / 'queue'
    subprocess.run([compiler, '-std=c++20', '-DCOMBO_BUILD', '-I', str(ROOT), *extra, str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    codec = tmp / 'codec'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), *extra,
                    str(ROOT / 'tests/item_receipts/codec_test.cpp'), '-o', str(codec)], check=True)
    subprocess.run([str(codec)], check=True)

    host = (ROOT / 'combo/menu/ComboForeignDrawMM.h').read_text()
    cache = host[host.index('struct ComboForeignDrawCacheOOT {'):host.index('} // namespace')]
    latch = (ROOT / 'tests/item_receipts/latch_test.cpp').read_text().replace('/* FOREIGN_CACHE */', cache)
    tu = tmp / 'latch.cpp'
    tu.write_text(latch)
    exe = tmp / 'latch'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), *extra,
                    str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

    # Compile the real MM builder with actual item/FC/vanilla-GI catalog data.
    items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
    macro = items[items.index('#define RI('):items.index('// clang-format off')]
    catalog = macro + '\nnamespace Rando::StaticData { std::map<RandoItemId,RandoStaticItem> Items = ' + \
        block(items, 'std::map<RandoItemId, RandoStaticItem> Items') + ';\n}\n#undef RI\n'
    message = (ROOT / 'mm/src/code/z_message.c').read_text()
    catalog += '#define MESSAGE_ITEM_NONE 9999\ns16 D_801CFF94[] = ' + block(message, 's16 D_801CFF94[]') + ';\n'
    for channel in ('D_801CFE04', 'D_801CFE1C', 'D_801CFE34'):
        catalog += 's16 ' + channel + '[] = ' + block(message, 's16 ' + channel + '[]') + ';\n'
    catalog += 'extern "C" void Message_StageCustomItemIcon(void*,s16);\n'
    catalog += 'extern "C" void Message_StageCustomItemIconTint(void*,s16,s16,u8,u8,u8,u8);\n'
    catalog += 'u8 Rando::StaticData::GetIconForZMessage(RandoItemId randoItemId) ' + \
        block(items, 'u8 GetIconForZMessage') + '\n'
    native = (ROOT / 'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    catalog += 'struct NativeGetItemEntry { u8 itemId,field; s8 gid; u8 textId; u16 objectId; };\n'
    catalog += native[native.index('#define GIFIELD_'):native.index('GetItemEntry sGetItemTable')]
    catalog += '#define GET_ITEM(itemId, objectId, drawId, textId, field, chestAnim) { itemId,0,0,textId,0 }\n'
    catalog += 'NativeGetItemEntry sGetItemTable[GI_MAX-1] = ' + block(native, 'GetItemEntry sGetItemTable') + ';\n#undef GET_ITEM\n'
    catalog += 'extern "C" u16 Player_GetItemReceiptTextId(s16 getItemId,s16 itemId) ' + \
        block(native, 'u16 Player_GetItemReceiptTextId').replace('GetItemEntry', 'NativeGetItemEntry') + '\n'
    glue = (ROOT / 'mm/2s2h/FleetShipCombo/FleetComboItemsGlue.cpp').read_text()
    catalog += 'namespace { const int sFcNative[] = ' + block(glue, 'const int sFcNative[]') + ';\n'
    catalog += 'std::unordered_map<int,int>& ReverseMap() ' + block(glue, 'ReverseMap()') + '\n}\n'
    catalog += 'extern "C" int FcCombo_ItemForNative(int nativeId) ' + block(glue, 'int FcCombo_ItemForNative') + '\n'
    registry = (ROOT / 'soh/mods/extended_player.c').read_text()
    leaf = re.search(r'RG_DEKU_LEAF,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', registry).group(1)
    catalog += 'const char* kDekuLeafMessage = ' + leaf + ';\n'
    mmregistry = (ROOT / 'mm/mods/extended_player.c').read_text()
    rows = block(mmregistry, 'static const NeiItem sNeiItems[]')[1:-1]
    catalog += 'const NeiItem sReceiptNeiItems[] = {\n'
    for row in re.findall(r'\{[^{}]*\}', rows, re.S):
        item = re.match(r'\{\s*(ITEM_\w+)', row)
        text = re.search(r'\b(RG_\w+|NEI_NO_RG),\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', row)
        if item and text:
            catalog += '{ .item = ' + item.group(1) + ', .rg = ' + text.group(1) + ', .nameEn = ' + text.group(2) + ' },\n'
    catalog += '};\n'
    inventory = (ROOT / 'mm/src/code/z_inventory.c').read_text()
    for declaration in ('u32 gUpgradeMasks', 'u8 gUpgradeShifts'):
        catalog += 'extern "C" ' + declaration + '[8] = ' + block(inventory, declaration) + ';\n'
    (tmp / 'receipt_catalogs.inc').write_text(catalog)
    pause_desc = (ROOT / 'mm/2s2h/CustomMessage/PauseItemDescriptions.cpp').read_text()
    (tmp / 'receipt_map_pause.inc').write_text('extern "C" const char* PauseItemDesc_GetMapInfo(s32 dungeon,u16 itemId) ' +
                                             block(pause_desc, 'const char* PauseItemDesc_GetMapInfo'))
    mmflags = ['-DCOMBO_BUILD', '-DMM_BUILD_DLL', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
               '-DNON_EQUIVALENT', '-DNON_MATCHING']
    mmflags += ['-I' + str(ROOT / p) for p in ('mm/include', 'mm/include/PR', 'mm/src', 'mm', 'mm/2s2h',
                 'mm/assets', 'libultraship/include', 'libultraship/src', 'combo', 'combo/menu')]
    linked = tmp / 'catalog'
    result = subprocess.run([compiler, '-std=c++20', *mmflags, *extra, '-I' + str(tmp),
                            str(ROOT / 'tests/item_receipts/catalog_test.cpp'),
                            str(ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp'), '-rdynamic', '-ldl', '-o', str(linked)],
                            capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(linked)], check=True)

    # Execute the actual exported donor builder, with the authored descriptions.
    donor = (ROOT / 'tests/item_receipts/donor_test.cpp').read_text()
    descriptions = 'const CustomItemMessageEntry receiptMessages[] = {\n'
    for rg in ('RG_CANE_OF_SOMARIA', 'RG_PROGRESSIVE_ROCS', 'RG_CANE_PACCI_FLIP',
               'RG_ROCS_CAPE', 'RG_QUARTZ_OF_MOTION', 'RG_DEKU_LEAF',
               'RG_MM_REMAINS_GOHT', 'RG_MM_SONG_LULLABY', 'RG_MM_SONG_LULLABY_INTRO', 'RG_MM_SONG_NOVA',
               'RG_MM_SONG_HEALING', 'RG_MM_SONG_STORMS'):
        text = re.search(r'\{\s*' + rg + r',.*?,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)',
                         (ROOT / 'soh/soh/Enhancements/randomizer/randomizer.cpp').read_text(), re.S)
        if not text:
            text = re.search(r'\b' + rg + r',\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', registry)
        descriptions += '{' + rg + ', 0, ' + text.group(1) + ', nullptr, nullptr},\n'
    descriptions += '};\n'
    donor = donor.replace('/* DONOR_MESSAGES */', descriptions)
    exported = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    context_builders = 'bool BuildDungeonItemReceiptMessage(RandomizerGet rg, CustomMessage& msg, bool received = true);\n'
    for signature in ('static bool DungeonInformationEnabled()',
                      'extern "C" COMBO_EXPORT int32_t OOT_MapCompassInfoEnabled(void)',
                      'static CustomMessage DungeonRewardName(RandomizerCheck check)',
                      'static int16_t DungeonEntranceDestination(int16_t entrance)',
                      'static std::string DungeonEntranceSource(int16_t dungeonEntrance)',
                      'bool BuildDungeonItemReceiptMessage(RandomizerGet rg, CustomMessage& msg, bool received)',
                      'bool BuildTokenReceiptMessage(RandomizerGet rg, CustomMessage& msg)',
                      'extern "C" uint16_t Randomizer_GetDungeonItemInfoTextId(uint16_t cursorItem)',
                      'void BuildDungeonPauseInfoMessage(uint16_t* textId, bool* loadFromMessageTable)',
                      'void BuildMapMessage(uint16_t* textId, bool* loadFromMessageTable)'):
        context_builders += signature + ' ' + block(exported, signature + ' {') + '\n'
    donor = donor.replace('/* CONTEXT_BUILDERS */', context_builders)
    donor = donor.replace('/* FOREIGN_BUILDER */',
        'void BuildComboForeignMessage(Player* player,CustomMessage& msg) ' +
        block(exported, 'void BuildComboForeignMessage'))
    donor = donor.replace('/* DONOR_EXPORT */',
        'extern "C" int32_t OOT_GetItemReceiptText(const char* itemName,char* buffer,uint32_t capacity) ' +
        block(exported, 'int32_t OOT_GetItemReceiptText'))
    tu = tmp / 'donor.cpp'
    tu.write_text(donor)
    exe = tmp / 'donor'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-I', str(ROOT), *extra,
                    str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

    information = (ROOT / 'tests/item_receipts/information_test.cpp').read_text()
    hint = (ROOT / 'soh/soh/Enhancements/randomizer/hint.cpp').read_text()
    save = (ROOT / 'soh/soh/SaveManager.cpp').read_text()
    save_header = (ROOT / 'soh/soh/SaveManager.h').read_text()
    otr = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text()
    information = information.replace('/* ALTAR_BUILDER */',
        'const CustomMessage Hint::GetHintMessage(MessageFormat format,size_t id) const ' + block(hint, 'const CustomMessage Hint::GetHintMessage'))
    information = information.replace('/* SETTINGS_RESTORE */',
        'extern "C" void SOH_RestoreRandoSettings(const char* json) ' + block(otr, 'void SOH_RestoreRandoSettings'))
    information = information.replace('/* ALTAR_EXPORT */',
        'extern "C" const char* SOH_DumpAltarHintMessages(void) ' + block(otr, 'const char* SOH_DumpAltarHintMessages'))
    information = information.replace('/* LOAD_DATA */', block(
        save_header[save_header.index('template <typename T> void LoadData'):], ') {\n        if (name == "")'))
    information = information.replace('/* LOAD_ARRAY */',
        'void SaveManager::LoadArray(const std::string& name,const size_t size,LoadArrayFunc func) ' + block(save, 'void SaveManager::LoadArray'))
    information = information.replace('/* LOAD_SETTINGS */',
        'SaveManager::Instance->LoadArray("randoSettings",RSK_MAX,[&](size_t i) ' + block(save, 'SaveManager::Instance->LoadArray("randoSettings"') + ');')
    settings = (ROOT / 'soh/soh/Enhancements/randomizer/settings.cpp').read_text()
    information = information.replace('/* SPOILER_SETTINGS */',
        block(settings, 'void Settings::ParseJson')[1:].split('    nlohmann::json jsonExcludedLocations')[0])
    tu = tmp / 'information.cpp'
    tu.write_text(information)
    exe = tmp / 'information'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

    tracker = (ROOT / 'tests/item_receipts/tracker_test.cpp').read_text()
    launcher = (ROOT / 'combo/ComboShip.cpp').read_text()
    tracker = tracker.replace('/* TRACKER_PUSH */', 'static void Combo_PushHintTrackerData(int slot) ' +
                              block(launcher, 'static void Combo_PushHintTrackerData'))
    tu = tmp / 'tracker.cpp'
    tu.write_text(tracker)
    exe = tmp / 'tracker'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', *extra,
                    '-I', str(ROOT), '-I', str(ROOT / 'combo'), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
