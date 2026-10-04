#!/usr/bin/env python3
"""Exercise MM's production queued receipt/grant boundary with engine seams replaced."""
from pathlib import Path
import os
import re
import sys
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


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
    tu = tmp / 'queue.cpp'
    helpers = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
    append = block(helpers, 'void Rando::AppendReceiptSource')
    tu.write_text(preamble + '\nvoid Rando::AppendReceiptSource(CustomMessage::Entry& entry,const std::string& source) ' + append +
                  '\nnamespace Rando::MiscBehavior { void Apply(Actor* actor,PlayState* play) ' + give +
                  '\n}\nusing Rando::MiscBehavior::Apply;\n' + checks_source)
    exe = tmp / 'queue'
    compiler = os.environ.get('CXX', 'c++')
    extra = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitizers' in sys.argv else []
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
               'RG_ROCS_CAPE', 'RG_QUARTZ_OF_MOTION', 'RG_DEKU_LEAF'):
        text = re.search(r'\{\s*' + rg + r',.*?,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)',
                         (ROOT / 'soh/soh/Enhancements/randomizer/randomizer.cpp').read_text(), re.S)
        if not text:
            text = re.search(r'\b' + rg + r',\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', registry)
        descriptions += '{' + rg + ', 0, ' + text.group(1) + ', nullptr, nullptr},\n'
    descriptions += '};\n'
    donor = donor.replace('/* DONOR_MESSAGES */', descriptions)
    exported = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    donor = donor.replace('/* DONOR_EXPORT */',
        'extern "C" int32_t OOT_GetItemReceiptText(const char* itemName,char* buffer,uint32_t capacity) ' +
        block(exported, 'int32_t OOT_GetItemReceiptText'))
    tu = tmp / 'donor.cpp'
    tu.write_text(donor)
    exe = tmp / 'donor'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), *extra,
                    str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
