#ifndef COMBO_ITEM_RECEIPT_PRESENTATION_H
#define COMBO_ITEM_RECEIPT_PRESENTATION_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ComboItemDrawABI.h"
#include "ComboItemIconOwnership.h"

/* Value-owned receipt metadata. No save, inventory, C++ object, or temporary
 * texture pointer crosses the DLL boundary. Zero means ordinary formatting. */
typedef struct {
    int32_t singleBox; /* authored information layout; long hints may page */
    int32_t rewardLine;
    char iconPath[512];
    int32_t iconWidth, iconHeight, iconIA8, iconHasColor;
    uint8_t iconColor[4];
} CwItemReceiptPresentation;

typedef struct {
    int32_t textScale; /* percent; native Latin receipts normally use 75 */
    int32_t iconX, iconY, iconWidth, iconHeight;
    uint32_t firstPageEnd; /* end of the reward's page after wrapping */
    uint32_t iconPageStart, bodySize;
} CwItemReceiptLayout;

typedef int32_t (*Fn_GetDungeonItemReceiptPresentation)(const char*, CwItemReceiptPresentation*);

static inline int ComboReceipt_HasIcon(const CwItemReceiptPresentation* p) {
    return p && p->singleBox == 1 && p->rewardLine >= 0 && p->rewardLine < 3 &&
           memchr(p->iconPath, '\0', sizeof(p->iconPath)) && !strncmp(p->iconPath, "__OTR__", 7) && p->iconWidth > 0 &&
           p->iconWidth <= 64 && p->iconHeight > 0 && p->iconHeight <= 64 && (p->iconIA8 == 0 || p->iconIA8 == 1) &&
           (p->iconHasColor == 0 || p->iconHasColor == 1);
}

static inline int ComboReceipt_CopyIcon(CwItemReceiptPresentation* p, const CwItemIconInfo* icon, const char* owner) {
    if (!p || !icon || !icon->path || strncmp(icon->path, "__OTR__", 7) || icon->width < 1 || icon->width > 64 ||
        icon->height < 1 || icon->height > 64 || (icon->isIA8 != 0 && icon->isIA8 != 1) ||
        (icon->hasColor != 0 && icon->hasColor != 1))
        return 0;
    if (ComboIconUsesMmOwnership(icon->path))
        owner = "mm";
    int n = !strncmp(icon->path, "__OTR__@", 8)
                ? snprintf(p->iconPath, sizeof(p->iconPath), "%s", icon->path)
                : snprintf(p->iconPath, sizeof(p->iconPath), "__OTR__@%s:%s", owner, icon->path + 7);
    if (n < 0 || (size_t)n >= sizeof(p->iconPath)) {
        p->iconPath[0] = '\0';
        return 0;
    }
    p->iconWidth = icon->width;
    p->iconHeight = icon->height;
    p->iconIA8 = icon->isIA8;
    p->iconHasColor = icon->hasColor;
    memcpy(p->iconColor, icon->color, sizeof(p->iconColor));
    return ComboReceipt_HasIcon(p);
}

/* Both Latin renderers truncate each glyph and advance spaces by six pixels.
 * A fixed native scale preserves the proportions between letters and words. */
static inline int32_t ComboReceipt_GlyphWidth(uint8_t c, const float* widths, size_t count) {
    return c == ' ' ? 6 : c >= 0x20 && (size_t)(c - 0x20) < count ? (int32_t)(widths[c - 0x20] * 0.75f) : 0;
}

static inline size_t ComboReceipt_CommandSize(uint8_t c, int mm) {
    if (mm)
        return c == 0x14 ? 2 : c >= 0x1B && c <= 0x1F ? 3 : 1;
    return c == 0x05 || c == 0x06 || c == 0x0C || c == 0x0E || c == 0x13 || c == 0x14 || c == 0x1E ? 2
           : c == 0x07 || c == 0x11 || c == 0x12                                                   ? 3
                                                                                                   : 1;
}

static inline int32_t ComboReceipt_LineWidth(const char* body, size_t size, int mm, const float* widths, size_t count) {
    int32_t width = 0;
    for (size_t i = 0; i < size;) {
        uint8_t c = (uint8_t)body[i];
        size_t command = ComboReceipt_CommandSize(c, mm);
        if (command > size - i)
            break;
        // MM's END byte is also in the font table; commands never advance
        // the native pen, even when their numeric value has a glyph width.
        if (command == 1 && c != (mm ? 0xBF : 0x02))
            width += ComboReceipt_GlyphWidth(c, widths, count);
        i += command;
    }
    return width;
}

/* Reflow once before native decoding. Authored hint paragraphs keep their line
 * breaks; long names wrap at words and every fourth line becomes a normal page.
 * The final reward sprite follows its text onto that page. No font shrinking,
 * story commands, save changes or inventory-dependent item resolution occurs. */
static inline CwItemReceiptLayout ComboReceipt_Layout(const CwItemReceiptPresentation* p, char* body, size_t size,
                                                      size_t capacity, int mm, const float* widths, size_t count) {
    CwItemReceiptLayout result = { 75, 0, 0, 0, 0, 0, 0, (uint32_t)size };
    const CwItemReceiptLayout unchanged = result;
    char wrapped[1280];
    if (!body || size > capacity || capacity > sizeof(wrapped))
        return result;
    const uint8_t newline = mm ? 0x11 : 0x01;
    const uint8_t page = mm ? 0x10 : 0x04;
    const uint8_t end = mm ? 0xBF : 0x02;
    const int hasIcon = ComboReceipt_HasIcon(p);
    // Two-line compass receipts align the sprite's top with the boss row.
    // Its height then leaves the title row free to use the whole text width.
    const int bossLineIcon = hasIcon && p->rewardLine == 1;
    if (hasIcon) {
        result.iconWidth = p->iconWidth > 24 ? 24 : p->iconWidth;
        result.iconHeight = result.iconWidth * p->iconHeight / p->iconWidth;
        if (result.iconHeight > 24) {
            result.iconHeight = 24;
            result.iconWidth = result.iconHeight * p->iconWidth / p->iconHeight;
        }
    }
    size_t length = 0, lineStart = 0, space = (size_t)-1, pageStart = 0;
    unsigned line = 0;
    int32_t width = 0, pageWidth = 0;
    uint8_t color = mm ? 0 : 0x40, spaceColor = color;
    int mainBody = 1, iconPositioned = 0, bossLine = 0;
    for (size_t i = 0; i < size;) {
        const uint8_t c = (uint8_t)body[i];
        const size_t command = ComboReceipt_CommandSize(c, mm);
        if (command > size - i || command > capacity - length)
            return unchanged;
        if ((mm && c <= 8) || (!mm && c == 0x05))
            color = mm ? c : (uint8_t)body[i + 1];
        if (c == page || c == end) {
            if (mainBody && hasIcon) {
                result.iconX = (bossLineIcon ? width : (width > pageWidth ? width : pageWidth)) + 4;
                result.iconY = (int32_t)line * 12 - (bossLineIcon ? 0 : (result.iconHeight - 12) / 2);
                result.iconPageStart = (uint32_t)pageStart;
                result.firstPageEnd = (uint32_t)length;
                iconPositioned = 1;
            }
            mainBody = 0;
        }
        if (c == newline || c == page) {
            if (c == newline)
                bossLine = 1;
            if (width > pageWidth)
                pageWidth = width;
            uint8_t br = c;
            if (c == newline && ++line == 3) {
                br = page;
                line = 0;
            } else if (c == page) {
                line = 0;
            }
            wrapped[length++] = (char)br;
            // A generated page starts with the color active at its boundary.
            if (br == page) {
                pageStart = length;
                pageWidth = 0;
                if (c != page) {
                    if (length + (mm ? 1 : 2) > capacity)
                        return unchanged;
                    if (!mm)
                        wrapped[length++] = 0x05;
                    wrapped[length++] = (char)color;
                }
            }
            width = 0;
            lineStart = length;
            space = (size_t)-1;
            ++i;
            continue;
        }
        // Centered sprites span adjacent rows. A top-aligned boss sprite only
        // needs its column reserved after the authored title break.
        const int32_t available = 220 - (mainBody && hasIcon && (!bossLineIcon || bossLine) ? result.iconWidth + 4 : 0);
        const int32_t glyph = c != end && command == 1 ? ComboReceipt_GlyphWidth(c, widths, count) : 0;
        if (width + glyph > available && width > 0) {
            const uint8_t br = ++line == 3 ? (line = 0, page) : newline;
            size_t split = space != (size_t)-1 ? space : length;
            uint8_t splitColor = space != (size_t)-1 ? spaceColor : color;
            const int32_t finishedWidth =
                ComboReceipt_LineWidth(wrapped + lineStart, split - lineStart, mm, widths, count);
            if (finishedWidth > pageWidth)
                pageWidth = finishedWidth;
            if (space == (size_t)-1) {
                if (length == capacity)
                    return unchanged;
                wrapped[length++] = (char)br;
            } else {
                wrapped[split] = (char)br;
            }
            lineStart = split + 1;
            if (br == page) {
                pageStart = lineStart;
                pageWidth = 0;
                size_t restore = mm ? 1 : 2;
                if (restore > capacity - length)
                    return unchanged;
                memmove(wrapped + lineStart + restore, wrapped + lineStart, length - lineStart);
                if (!mm)
                    wrapped[lineStart++] = 0x05;
                wrapped[lineStart++] = (char)splitColor;
                length += restore;
            }
            width = ComboReceipt_LineWidth(wrapped + lineStart, length - lineStart, mm, widths, count);
            space = (size_t)-1;
        }
        if (command > capacity - length)
            return unchanged;
        if (c == ' ' && command == 1) {
            space = length;
            spaceColor = color;
        }
        memcpy(wrapped + length, body + i, command);
        length += command;
        width += glyph;
        i += command;
    }
    if (hasIcon && !iconPositioned) {
        result.iconX = (bossLineIcon ? width : (width > pageWidth ? width : pageWidth)) + 4;
        result.iconY = (int32_t)line * 12 - (bossLineIcon ? 0 : (result.iconHeight - 12) / 2);
        result.iconPageStart = (uint32_t)pageStart;
        result.firstPageEnd = (uint32_t)length;
    }
    memcpy(body, wrapped, length);
    result.bodySize = (uint32_t)length;
    return result;
}

#endif
