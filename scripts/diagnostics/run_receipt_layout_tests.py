#!/usr/bin/env python3
"""Run both production receipt renderers against their native buffer capacities."""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess
import tempfile


def block(source, start):
    begin = source.index('{', source.index(start))
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]


def build_layout_fixture(ROOT):
    engine = (ROOT / 'soh/src/code/z_message_PAL.c').read_text()
    mm_message = (ROOT / 'mm/src/code/z_message.c').read_text()
    layout = (ROOT / 'tests/item_receipts/layout_test.cpp').read_text()
    font_header = (ROOT / 'soh/include/z64.h').read_text()
    buffer_size = re.search(r'^#define MESSAGE_BUF_SIZE \d+.*$', font_header, re.M)
    assert buffer_size, 'Missing native OoT message capacity'
    layout = layout.replace('/* OOT_MESSAGE_BUFFER_SIZE */', buffer_size.group(0))
    mm_font_header = (ROOT / 'mm/include/z64message.h').read_text()
    mm_buffer_size = re.search(r'unsigned char schar\[(\d+)\];\s*// msgBuf', mm_font_header)
    assert mm_buffer_size, 'Missing native MM message capacity'
    layout = layout.replace('/* MM_MESSAGE_BUFFER_SIZE */',
                            '#define MM_MESSAGE_BUF_SIZE ' + mm_buffer_size.group(1))
    for tag, game_engine, font_name, font_type in (
        ('OOT', engine, 'sFontWidths', '144'),
        ('MM', mm_message, 'sNESFontWidths', '160')):
        font_engine = game_engine if tag == 'OOT' else (ROOT / 'mm/src/code/z_message_nes.c').read_text()
        layout = layout.replace('/* ' + tag + '_FONT_WIDTHS */',
            'float ' + font_name + '[' + font_type + '] = ' + block(font_engine, 'f32 ' + font_name) + ';')
        start = game_engine.index('static CwItemReceiptPresentation sItemReceiptPresentation;')
        end = game_engine.index('u16 Message_DrawItemIcon' if tag == 'OOT' else 'void Message_DrawItemIcon', start)
        layout = layout.replace('/* ' + tag + '_RECEIPT_RENDERER */', game_engine[start:end])
    nes = (ROOT / 'mm/src/code/z_message_nes.c').read_text()
    nes_draw = block(nes, 'void Message_DrawTextNES')
    begin = nes_draw.index('    msgCtx->textPosX =')
    end = nes_draw.index('    msgCtx->textColorR =', begin)
    dispatch = ('void Message_DrawTextNES(PlayState* play, Gfx** gfxP, u16 textDrawPos) {\n'
                'MessageContext* msgCtx = &play->msgCtx; Gfx* gfx = *gfxP; s16 sp130;\n' +
                nes_draw[begin:end] + '(void)sp130; (void)textDrawPos; *gfxP = gfx;\n}\n')
    dispatch += 'void Message_DrawText(PlayState* play, Gfx** gfxP) ' + block(mm_message, 'void Message_DrawText(')
    line_position = block(nes, '} else if ((curChar == MESSAGE_CARRIAGE_RETURN)')
    line_position = line_position[1:line_position.index('            spC6++;')]
    dispatch += ('\nvoid DecodeReceiptLine(int spC6, float spA4) {\n'
                 'MessageContext* msgCtx = &play.msgCtx;\n' + line_position + '}\n')
    space = nes_draw[nes_draw.index("            case ' ':") + len("            case ' ':"):]
    space = space[:space.index('                break;')]
    glyph = re.search(r'msgCtx->textPosX \+= \(s32\)\(sNESFontWidths\[\(u8\)character - \' \'\] \* msgCtx->textCharScale\);', nes_draw)[0]
    dispatch += ('\nint NativeLineWidth(const std::string& line) {\n'
                 'MessageContext* msgCtx = &play.msgCtx; msgCtx->textPosX = 0;\n'
                 # MM's color commands do not index the native glyph table.
                 'for (uint8_t character : line) { if (character <= 8) continue; if (character == \' \') {\n' + space +
                 '} else {\n' + glyph.replace('(s32)', '(int)').replace('(u8)', '(uint8_t)') +
                 '} } return msgCtx->textPosX; }\n')
    layout = layout.replace('/* MM_TEXT_DISPATCH */', dispatch)
    oot_decode = block(engine, 'void Message_Decode(PlayState* play)')
    oot_position = 'if (msgCtx->textBoxType != TEXTBOX_TYPE_NONE_BOTTOM) ' + block(
        oot_decode, 'if (msgCtx->textBoxType != TEXTBOX_TYPE_NONE_BOTTOM)')
    oot_position += '\nif (sItemReceiptPresentation.singleBox) ' + block(
        oot_decode, 'if (sItemReceiptPresentation.singleBox)')
    layout = layout.replace('/* OOT_DECODE_POSITION */',
        'void FinishReceiptDecodePosition(int numLines) { MessageContext* msgCtx = &play.msgCtx;\n'
        'R_TEXT_INIT_YPOS = R_TEXTBOX_Y + 8;\n' + oot_position + '\n}\n')
    mm_decode = block(mm_message, 'void Message_Decode(PlayState* play)')
    mm_position = 'if (curChar == MESSAGE_BOX_BREAK2) ' + block(nes, 'if (curChar == MESSAGE_BOX_BREAK2)')
    start = nes.index('if (curChar == MESSAGE_BOX_BREAK2)')
    remaining = nes[start + len(mm_position):]
    # Include native END positioning too, before the receipt-only override.
    mm_position += ' else ' + block(remaining, 'else {')
    mm_position += '\nif (sItemReceiptPresentation.singleBox) ' + block(
        mm_decode, 'if (sItemReceiptPresentation.singleBox)')
    layout = layout.replace('/* MM_DECODE_POSITION */',
        '#define XREG(i) ((i) == 13 ? 4 : (i) == 10 ? 22 : (i) == 11 ? 16 : 12)\n'
        'constexpr int MESSAGE_BOX_BREAK2 = 0x12, TEXTBOX_TYPE_3 = 3, TEXTBOX_TYPE_4 = 4;\n'
        'void FinishReceiptDecodePosition(int numLines) { MessageContext* msgCtx = &play.msgCtx;\n'
        'int curChar = 0xBF;\n' + mm_position + '\n}\n#undef XREG\n')
    return layout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='receipt-layout-') as folder:
        path = Path(folder)
        cpp = path / 'layout.cpp'
        cpp.write_text(build_layout_fixture(args.root))
        flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror']
        if args.sanitizers:
            flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                      '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                        '-I', str(args.root), str(cpp), '-o', str(path / 'layout')], check=True)
        subprocess.run([str(path / 'layout')], check=True)


if __name__ == '__main__':
    main()
