#!/usr/bin/env python3
"""Reproduce soul icon texture/receipt ABI mismatches from real asset metadata."""
import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]

def body(source, signature):
    first = source.index('{', source.index(signature))
    depth, end = 1, first + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[first:end]

def oot_source():
    # Reuse the established native renderer seam, with every soul's actual
    # catalog icon rather than a hand-authored icon-size assumption.
    code = (ROOT / 'tests/item_receipts/song_icons_test.cpp').read_text().split('// These are the selected native MM branches')[0]
    code = '#include <memory>\n' + code
    code = code.replace('namespace Rando::StaticData {', 'struct GetItemEntry { int itemId = 3; };\nnamespace Rando::StaticData {')
    code = code.replace('struct Item {', '''struct Item {
    std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet*) const { return std::make_shared<GetItemEntry>(); }
    std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const { return std::make_shared<GetItemEntry>(); }
    bool HasCustomIcon() const { return true; }
''')
    code += '\nconstexpr int ITEM_HEART_CONTAINER = 0x72, ITEM_HEART_PIECE_2 = 0x7A;\nvoid* gItemIcons[256]{};\n'
    code = code.replace('((void)(p), ::color =', '((void)(p), (void)(a), ::color =')
    code = code.replace('((void)(p), format =', '((void)(p), (void)(tex), format =')
    source = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    code = code.replace('/* CUSTOM_ICON_FUNCTIONS */',
        'void LoadCustomItemIcon(bool displayAsEnglish) ' + body(source, 'void LoadCustomItemIcon') + '\n' +
        'void DrawCustomItemIcon(Gfx** p) ' + body(source, 'void DrawCustomItemIcon'))
    engine = (ROOT / 'soh/src/code/z_message_PAL.c').read_text()
    code = code.replace('/* ENGINE_ICON_DRAW */', 'u16 Message_DrawItemIcon(PlayState* play,u16 itemId,Gfx** p,u16 i) ' + body(engine, 'u16 Message_DrawItemIcon'))
    draw = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
    code += '\nstatic int32_t OOT_FillItemIconInfo(RandomizerGet rg,CwItemIconInfo* out,bool resolveProgressive = true) ' + body(draw, 'static int32_t OOT_FillItemIconInfo')
    catalog = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
    assets = (ROOT / 'soh/assets/soh_assets.h').read_text()
    fixtures = (ROOT / 'tests/item_receipts/boss_soul_fixtures.h').read_text()
    rgs = re.findall(r'\bRG_\w+', fixtures)
    rows = []
    for rg in rgs:
        row = next(line for line in catalog.splitlines() if 'itemTable[' + rg + '] =' in line)
        symbol = re.search(r'\.CustomIcon\((\w+)\)', row)[1]
        path = re.search(r'#define d' + symbol[:-3] + r' "([^"]+)"', assets)[1]
        assert path == '__OTR__textures/parameter_static/gBossSoul'
        rows.append('{' + rg + ', "' + path + '"}')
    code += '\nstruct Soul { RandomizerGet id; const char* path; };\nstatic Soul souls[] = {' + ','.join(rows) + '};\n'
    code += r'''
int main() {
    Gfx commands[64]{};
    for (const auto& soul : souls) {
        testPlayer.getItemEntry.getItemId = soul.id;
        iconPath = soul.path;
        iconSize = ICON_SIZE_32; // Each production CustomIcon row uses its default size.
        auto* gfx = commands;
        LoadCustomItemIcon(false);
        assert(!std::strcmp(play.msgCtx.textboxSegment, soul.path));
        Message_DrawItemIcon(&play, ITEM_CUSTOM, &gfx, 0);
        assert(format == G_IM_FMT_RGBA && bits == G_IM_SIZ_32b && width == 32 && height == 32);
        assert(rectangleWidth == 32 && rectangleHeight == 32);
        CwItemIconInfo icon{};
        assert(OOT_FillItemIconInfo(soul.id, &icon) == 1);
        assert(!std::strcmp(icon.path, soul.path) && icon.width == 32 && icon.height == 32 && !icon.isIA8);
        CwItemReceiptPresentation receipt{};
        receipt.singleBox = 1;
        assert(ComboReceipt_CopyIcon(&receipt, &icon, "oot"));
        assert(!std::strcmp(receipt.iconPath, "__OTR__@oot:textures/parameter_static/gBossSoul"));
        CwItemReceiptLayout layout{};
        layout.iconWidth = layout.iconHeight = 24;
        ComboReceipt_DrawIcon(commands, &receipt, &layout, 60, 60, 255);
        assert(format == G_IM_FMT_RGBA && bits == G_IM_SIZ_32b && width == 32 && height == 32);
    }
    std::cout << "PASS 14 OoT-native/imported boss soul native hooks/engine and exported receipt renderer: genuine 32x32 RGBA32 art and OoT owner retained\n";
}
'''
    return code

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    metadata = {}
    header = (ROOT / 'mm/assets/archives/icon_item_static/icon_item_static_yar.h').read_text()
    for region in ('N64_US', 'GC_US'):
        root = ROOT / 'mm/assets/xml' / region
        skull = next(t for t in ET.parse(root / 'interface/icon_item_dungeon_static.xml').iter('Texture') if t.attrib['Name'] == 'gDungeonMapSkullTex')
        assert (skull.attrib['Format'], skull.attrib['Width'], skull.attrib['Height']) == ('rgba16', '16', '16')
        metadata['__OTR__icon_item_dungeon_static/gDungeonMapSkullTex'] = (16, 16, 16)
        names = ('gItemIconCaptainsHatTex', 'gItemIconOdolwasRemainsTex', 'gItemIconGohtsRemainsTex', 'gItemIconGyorgsRemainsTex', 'gItemIconTwinmoldsRemainsTex')
        for name in names:
            texture = next(t for t in ET.parse(root / 'archives/icon_item_static.xml').iter('Texture') if t.attrib['Name'] == name)
            assert (texture.attrib['Format'], texture.attrib['Width'], texture.attrib['Height']) == ('rgba32', '32', '32')
            path = re.search(r'#define d' + name + r' "([^"]+)"', header)[1]
            metadata[path] = (32, 32, 32)
    # OoT's source soul icon has a genuine RGBA32 32x32 layout and no palette;
    # MM's direct RGBA16 skull has no palette either, but a different packed layout.
    png = (ROOT / 'soh/assets/custom/textures/parameter_static/gBossSoul.rgba32.png').read_bytes()
    assert png[:8] == b'\x89PNG\r\n\x1a\n' and png[12:16] == b'IHDR'
    assert struct.unpack('>IIBB', png[16:26]) == (32, 32, 8, 6) # 8-bit RGBA, no palette
    assert 'iconSize_ = ICON_SIZE_32' in (ROOT / 'soh/soh/Enhancements/randomizer/item.h').read_text()
    items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
    receipts = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
    message = (ROOT / 'mm/src/code/z_message.c').read_text()
    code = (ROOT / 'tests/item_receipts/soul_icon_test.cpp').read_text()
    helper = ROOT / 'combo/menu/ComboBossSoulReceiptIcon.h'
    code = code.replace('/* SOUL_RECEIPT_HELPER */', '#include "combo/menu/ComboBossSoulReceiptIcon.h"' if helper.exists() else '')
    code = code.replace('/* NATIVE_ICON_SELECTORS */',
        'const char* GetIconTexturePath(RandoItemId randoItemId) ' + body(items, 'const char* GetIconTexturePath') + '\n' +
        'u8 GetIconForZMessage(RandoItemId randoItemId) ' + body(items, 'u8 GetIconForZMessage'))
    code = code.replace('/* SEED_ICON_SELECTOR */', 'int32_t MM_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out) ' + body(receipts, 'int32_t MM_GetSeedItemIconInfo'))
    code = code.replace('/* NATIVE_MESSAGE_RENDERER */', body(message, '} else if ((msgCtx->itemId == MESSAGE_CUSTOM_ICON_ITEM) && (sMsgCustomIconTex != NULL))'))
    rows = ',\n'.join('{"' + path + '", {' + ','.join(map(str, values)) + '}}' for path, values in metadata.items())
    code = code.replace('/* ASSET_METADATA */', 'struct Metadata { int bits, width, height; };\nstatic const std::map<std::string, Metadata> assetMetadata = {\n' + rows + '\n};')
    includes = ['-I' + str(ROOT / p) for p in ('', 'mm', 'mm/include', 'mm/assets', 'combo/menu')]
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    with tempfile.TemporaryDirectory(prefix='soul-receipt-icons-') as tmp:
        source, binary = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
        source.write_text(code)
        subprocess.run([os.environ.get('CXX', 'c++'), *flags, *includes, str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))
        source.write_text(oot_source())
        subprocess.run([os.environ.get('CXX', 'c++'), *flags, '-Wno-unused-variable', *includes, str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))

if __name__ == '__main__':
    main()
