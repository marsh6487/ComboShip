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
    int32_t singleBox;
    int32_t rewardLine;
    char iconPath[512];
    int32_t iconWidth, iconHeight, iconIA8, iconHasColor;
    uint8_t iconColor[4];
} CwItemReceiptPresentation;

typedef struct {
    int32_t textScale; /* percent; native Latin receipts normally use 75 */
    int32_t iconX, iconY, iconWidth, iconHeight;
    uint32_t firstPageEnd;
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

/* MM advances spaces by a fixed six pixels and truncates each Latin glyph.
 * Match the native pen rather than scaling a summed line width. */
static inline int32_t ComboReceipt_MmLineWidth(const char* body, size_t size, unsigned requestedLine, int32_t textScale,
                                               const float* widths, size_t count) {
    int32_t width = 0;
    unsigned line = 0;
    const float scale = textScale / 100.0f;
    for (size_t i = 0; i < size; ++i) {
        const uint8_t c = (uint8_t)body[i];
        if (c == 0xBF || c == 0x10)
            break;
        if (c == 0x11) {
            if (++line >= 3)
                break;
        } else if (line == requestedLine && c >= 0x20 && (size_t)(c - 0x20) < count) {
            width += c == ' ' ? 6 : (int32_t)(widths[c - 0x20] * scale);
        }
    }
    return width;
}

/* Measure the actual encoded first page with the renderer's native font
 * widths. Keep the three authored lines together, fitting long seed names
 * instead of generating extra pages. Sprite dimensions remain logical. */
static inline CwItemReceiptLayout ComboReceipt_Layout(const CwItemReceiptPresentation* p, const char* body, size_t size,
                                                      int mm, const float* widths, size_t count) {
    CwItemReceiptLayout result = { 75, 0, 0, 0, 0, 0 };
    float lines[3] = { 0, 0, 0 };
    unsigned line = 0;
    size_t i;
    for (i = 0; i < size; ++i) {
        uint8_t c = (uint8_t)body[i];
        if (c == (mm ? 0xBF : 0x02) || c == (mm ? 0x10 : 0x04))
            break;
        if (c == (mm ? 0x11 : 0x01)) {
            if (++line >= 3)
                break;
        } else if (!mm && (c == 0x05 || c == 0x13 || c == 0x06 || c == 0x14)) {
            if (i + 1 < size)
                ++i;
        } else if (c >= 0x20 && (size_t)(c - 0x20) < count) {
            lines[line] += widths[c - 0x20];
        }
    }
    result.firstPageEnd = (uint32_t)i;
    int hasIcon = ComboReceipt_HasIcon(p);
    if (hasIcon) {
        result.iconWidth = p->iconWidth > 24 ? 24 : p->iconWidth;
        result.iconHeight = result.iconWidth * p->iconHeight / p->iconWidth;
        if (result.iconHeight > 24) {
            result.iconHeight = 24;
            result.iconWidth = result.iconHeight * p->iconWidth / p->iconHeight;
        }
    }
    if (mm) {
        while (result.textScale > 1) {
            int fits = 1;
            for (unsigned n = 0; n <= line && n < 3; ++n) {
                const int32_t available = 220 - (hasIcon && (int32_t)n == p->rewardLine ? result.iconWidth + 4 : 0);
                if (ComboReceipt_MmLineWidth(body, size, n, result.textScale, widths, count) > available)
                    fits = 0;
            }
            if (fits)
                break;
            --result.textScale;
        }
    } else {
        for (unsigned n = 0; n <= line && n < 3; ++n) {
            float available = 220.0f - (hasIcon && (int32_t)n == p->rewardLine ? result.iconWidth + 4 : 0);
            if (lines[n] > 0 && lines[n] * result.textScale / 100.0f > available)
                result.textScale = (int32_t)(available * 100.0f / lines[n]);
        }
    }
    if (result.textScale < 1)
        result.textScale = 1;
    if (hasIcon) {
        result.iconX = (mm ? ComboReceipt_MmLineWidth(body, size, p->rewardLine, result.textScale, widths, count)
                           : (int32_t)(lines[p->rewardLine] * result.textScale / 100.0f)) +
                       4;
        result.iconY = p->rewardLine * 12 - (result.iconHeight - result.textScale * 16 / 100) / 2;
    }
    return result;
}

#endif
