// Execute both engines' per-message stage/apply/draw boundary with the real
// native font widths. GBI recording replaces only the GPU command sink.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include "combo/menu/ComboItemReceiptText.h"
#include "combo/menu/ComboItemReceiptPresentation.h"
using Gfx = int;
constexpr int G_IM_FMT_IA = 1, G_IM_FMT_RGBA = 2, G_IM_SIZ_8b = 1, G_IM_SIZ_32b = 4;
static int format, bits, sourceWidth, sourceHeight, x1, y1, x2, y2, red, green, blue, alpha, draws;
static std::string path;
#define gDPPipeSync(p) ((void)(p))
#define gDPSetCombineMode(p, ...) ((void)(p))
#define gDPSetCombineLERP(p, ...) ((void)(p))
#define gDPSetPrimColor(p, a, b, r, g, bl, al) \
    ((void)(p), red = (r), green = (g), blue = (bl), ::alpha = (al))
#define gDPLoadTextureBlock(p, tex, fmt, siz, w, h, ...) \
    ((void)(p), path = (tex), format = (fmt), bits = (siz), sourceWidth = (w), sourceHeight = (h))
#define gSPTextureRectangle(p, ax, ay, bx, by, ...) \
    ((void)(p), x1 = (ax) / 4, y1 = (ay) / 4, x2 = (bx) / 4, y2 = (by) / 4, ++draws)
#include "combo/menu/ComboItemReceiptRender.h"

namespace Oot {
struct MessageContext {
    struct { char msgBuf[1280]{}; } font;
    int msgLength = 0, msgBufPos = 0, textPosX = 0, textPosY = 0, textColorAlpha = 192;
};
struct PlayState { MessageContext msgCtx; } play;
int R_TEXT_CHAR_SCALE = 75, R_TEXT_LINE_SPACING = 12;
/* OOT_FONT_WIDTHS */
/* OOT_RECEIPT_RENDERER */
}
namespace Mm {
using u16 = uint16_t;
using s16 = int16_t;
struct MessageContext {
    struct { struct { char schar[1280]{}; } msgBuf; } font;
    int msgLength = 0, msgBufPos = 0, unk11FFC = 12, unk11F18 = 0, unk11F1A[3]{};
    int unk11FF8 = 65, unk11FFA = 28, textColorAlpha = 192;
    int textPosX = 0, textPosY = 0, itemId = 0xFE;
    bool textIsCredits = false;
    float textCharScale = 0.75f;
};
struct PlayState { MessageContext msgCtx; } play;
float sCharTexSize = 12, sCharTexScale = 1024 / 0.75f;
/* MM_FONT_WIDTHS */
/* MM_RECEIPT_RENDERER */
constexpr int LANGUAGE_ENG = 1, LANGUAGE_JPN = 0, VB_DRAW_OCARINA_STAFF = 1;
struct { struct { int language = LANGUAGE_ENG; } options; } gSaveContext;
int nativeIconDraws = 0, japaneseDraws = 0, creditsDraws = 0;
bool GameInteractor_Should(int, bool enabled) { return enabled; }
void Message_DrawItemIcon(PlayState*, Gfx**) { ++nativeIconDraws; }
void Message_DrawTextDefault(PlayState*, Gfx**) { ++japaneseDraws; }
void Message_DrawTextCredits(PlayState*, Gfx**) { ++creditsDraws; }
/* MM_TEXT_DISPATCH */
}

int main() {
    (void)&Mm::Message_DrawItemReceiptIcon;
    CwItemReceiptPresentation p{};
    p.singleBox = 1;
    p.rewardLine = 2;
    const CwItemIconInfo fire{ "__OTR__textures/icon_item_24_static/gQuestIconMedallionFireTex", 24, 24, 0, 0, {} };
    assert(ComboReceipt_CopyIcon(&p, &fire, "oot"));
    const std::string body = "You found the Deku Tree Compass!\x01It points to Volvagia!\x01"
                             "Defeating the boss grants the Fire Medallion!\x13\x9C\x02";
    std::memcpy(Oot::play.msgCtx.font.msgBuf, body.data(), body.size());
    Oot::play.msgCtx.msgLength = body.size();
    Oot::Message_SetItemReceiptPresentation(&p);
    // Staging owns its own copy, including the resource path.
    p.iconPath[0] = '\0';
    Oot::Message_ApplyItemReceiptLayout(&Oot::play);
    assert(Oot::R_TEXT_CHAR_SCALE > 30 && Oot::R_TEXT_CHAR_SCALE < 75);
    assert(Oot::R_TEXT_LINE_SPACING == 12 && Oot::Message_HasItemReceiptIcon());
    assert(Oot::sItemReceiptLayout.iconX + Oot::sItemReceiptLayout.iconWidth <= 220);
    Oot::play.msgCtx.textPosX = 65 + Oot::sItemReceiptLayout.iconX - 4;
    Oot::play.msgCtx.textPosY = 52;
    Gfx commands[64]{};
    Gfx* gfx = commands;
    Oot::Message_DrawItemReceiptIcon(&Oot::play, &gfx);
    assert(draws == 1 && path == "__OTR__@oot:textures/icon_item_24_static/gQuestIconMedallionFireTex");
    assert(format == G_IM_FMT_RGBA && bits == G_IM_SIZ_32b && sourceWidth == 24 && sourceHeight == 24);
    assert(x1 > 65 && x2 <= 285 && y1 > 28 && y2 <= 76 && x2 - x1 == 24 && y2 - y1 == 24);
    assert(red == 255 && green == 255 && blue == 255 && alpha == 192);
    Oot::Message_SetItemReceiptPresentation(nullptr);
    Oot::Message_DrawItemReceiptIcon(&Oot::play, &gfx);
    assert(draws == 1 && !Oot::Message_HasItemReceiptIcon());

    p = {};
    p.singleBox = 1;
    p.rewardLine = 2;
    const CwItemIconInfo clef{ "__OTR__icon_item_static_yar/gItemIconSongNoteTex", 16, 24, 1, 1,
                              { 128, 216, 240, 255 } };
    assert(ComboReceipt_CopyIcon(&p, &clef, "mm"));
    const std::string text = ComboItemReceiptText::FromNeiMarkup(
        "You found the Snowhead Compass!&It points to Goht!&Defeating the boss grants the Progressive Goron Lullaby!");
    std::memcpy(Mm::play.msgCtx.font.msgBuf.schar + 11, text.data(), text.size());
    Mm::play.msgCtx.font.msgBuf.schar[text.size() + 11] = '\xBF';
    Mm::play.msgCtx.msgLength = text.size() + 12;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    assert(Mm::play.msgCtx.textCharScale > 0.25f && Mm::play.msgCtx.textCharScale < 0.75f);
    for (int line = 0; line < 3; ++line) {
        Mm::DecodeReceiptLine(line, 120.0f + line * 10.0f);
        assert(Mm::play.msgCtx.unk11F1A[line] == 0 && "native English decode shifted the fitted receipt away from its sprite");
    }
    assert(Mm::sItemReceiptLayout.iconX + Mm::sItemReceiptLayout.iconWidth <= 220);
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && path == "__OTR__@mm:icon_item_static_yar/gItemIconSongNoteTex");
    assert(format == G_IM_FMT_IA && bits == G_IM_SIZ_8b && sourceWidth == 16 && sourceHeight == 24);
    assert(x2 - x1 == 16 && y2 - y1 == 24 && x2 <= 285 && y1 > 28);
    assert(red == 128 && green == 216 && blue == 240 && alpha == 192);
    Mm::play.msgCtx.msgBufPos = Mm::sItemReceiptLayout.firstPageEnd + 12;
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && Mm::play.msgCtx.textCharScale == 0.75f); // attribution's next page is ordinary
    Mm::Message_SetItemReceiptPresentation(nullptr);
    Mm::play.msgCtx.itemId = 1;
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && Mm::nativeIconDraws == 1);
    p.iconWidth = 65;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && Mm::nativeIconDraws == 1 && !ComboReceipt_HasIcon(&Mm::sItemReceiptPresentation));
    Mm::gSaveContext.options.language = Mm::LANGUAGE_JPN;
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(Mm::japaneseDraws == 1 && draws == 2);
    Mm::play.msgCtx.textIsCredits = true;
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(Mm::creditsDraws == 1 && draws == 2);
    // NES spaces have a fixed six-pixel advance even when glyphs shrink.
    p = {};
    p.singleBox = 1;
    p.rewardLine = 2;
    const CwItemIconInfo key{ "__OTR__icon_item_static_yar/gItemIconBossKeyTex", 32, 32, 0, 0, {} };
    assert(ComboReceipt_CopyIcon(&p, &key, "mm"));
    const std::string reward = "Defeating the boss grants the Stone Tower Boss Key!";
    const std::string compact = ComboItemReceiptText::FromNeiMarkup(
        "You found the Stone Tower Compass!&It points to Twinmold!&" + reward);
    std::memcpy(Mm::play.msgCtx.font.msgBuf.schar + 11, compact.data(), compact.size());
    Mm::play.msgCtx.font.msgBuf.schar[compact.size() + 11] = '\xBF';
    Mm::play.msgCtx.msgLength = compact.size() + 12;
    Mm::play.msgCtx.msgBufPos = 0;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    const int nativeEnd = Mm::NativeLineWidth(reward);
    assert(Mm::sItemReceiptLayout.iconX >= nativeEnd + 4 && "reward text overlaps its final-line sprite");
    assert(Mm::sItemReceiptLayout.iconX + Mm::sItemReceiptLayout.iconWidth <= 220);
    std::cout << "Real OoT/MM font fitting, English dispatch, reward-line rectangles, routed sprites, IA8/tint, native icon fallback and reset passed\n";
}
