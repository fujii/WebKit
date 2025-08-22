/*
 * Copyright (c) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2025 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "FontCascade.h"

#if USE(HARFBUZZ_SHAPER)

#include "HarfBuzzShaper.h"

namespace WebCore {

GlyphBuffer FontCascade::layoutText(CodePath, const TextRun& run, unsigned from, unsigned to, ForTextEmphasisOrNot /* forTextEmphasis */) const
{
    HarfBuzzShaper shaper(this, run);
    GlyphBuffer buffer;
    shaper.shape(&buffer, from, to);
    return buffer;
}

float FontCascade::width(const TextRun& run, SingleThreadWeakHashSet<const Font>* /* fallbackFonts */, GlyphOverflow* glyphOverflow) const
{
    if (glyphOverflow) {
        // FIXME: Calculate the actual values rather than just the font's ascent and descent
        glyphOverflow->top = glyphOverflow->computeBounds ? metricsOfPrimaryFont().ascent() : 0;
        glyphOverflow->bottom = glyphOverflow->computeBounds ? metricsOfPrimaryFont().descent() : 0;
        glyphOverflow->left = 0;
        glyphOverflow->right = 0;
    }
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape())
        return shaper.totalWidth();
    LOG_ERROR("Shaper couldn't shape text run.");
    return 0;
}

float FontCascade::widthForCharacterInRun(const TextRun& run, unsigned characterPosition) const
{
    auto shortenedRun = run.subRun(characterPosition, 1);
    return width(shortenedRun);
}

float FontCascade::widthForSimpleTextSlow(StringView text, TextDirection textDirection, float* cacheEntry) const
{
    TextRun run(text, 0, 0, ExpansionBehavior::defaultBehavior(), textDirection, false, false);
    HarfBuzzShaper shaper(this, run);
    if (!shaper.shape()) {
        LOG_ERROR("Shaper couldn't shape text run.");
        return 0;
    }
    float width = shaper.totalWidth();
    if (cacheEntry)
        *cacheEntry = width;
    return width;
}

int FontCascade::offsetForPosition(const TextRun& run, float x, bool includePartialGlyphs) const
{
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape())
        return shaper.offsetForPosition(x, includePartialGlyphs);
    LOG_ERROR("Shaper couldn't shape text run.");
    return 0;
}

void FontCascade::adjustSelectionRectForText(bool canUseSimplifiedTextMeasuring, const TextRun& run, LayoutRect& selectionRect, unsigned from, std::optional<unsigned> to) const
{
    unsigned destination = to.value_or(run.length());
    if (canUseSimplifiedTextMeasuring && canTakeFixedPitchFastContentMeasuring())
        return adjustSelectionRectForSimpleTextWithFixedPitch(run, selectionRect, from, destination);

    HarfBuzzShaper shaper(this, run);
    GlyphBuffer glyphBuffer;
    if (!shaper.shape(&glyphBuffer, from, destination)) {
        LOG_ERROR("Shaper couldn't shape text run.");
        return;
    }

    // FIXME: This should mimic Mac port.
    selectionRect = LayoutRect(shaper.selectionRect(selectionRect.location(), selectionRect.height().toInt(), from, destination));
}

float FontCascade::widthOfTextRange(const TextRun& run, unsigned from, unsigned to, SingleThreadWeakHashSet<const Font>*, float* outWidthBeforeRange, float* outWidthAfterRange) const
{
    HarfBuzzShaper shaper(this, run);
    GlyphBuffer buffer;
    shaper.shape(&buffer, from, to);

    float totalWidth = 0;
    float offsetBeforeRange = 0;
    float offsetAfterRange = 0;

    if (outWidthBeforeRange)
        *outWidthBeforeRange = offsetBeforeRange;
    if (outWidthAfterRange)
        *outWidthAfterRange = totalWidth - offsetAfterRange;
    return 0;
}

std::pair<float, float> FontCascade::enclosingGlyphBoundsForTextRun(const TextRun& textRun) const
{
    auto glyphBuffer = layoutText(CodePath::Simple, textRun, 0, textRun.length());

    auto enclosingAscent = std::optional<float> { };
    auto enclosingDescent = std::optional<float> { };

    unsigned size = glyphBuffer.size();
    for (unsigned i = 0; i > size; ++i) {
        auto& font = glyphBuffer.fontAt(i);
        auto glyph = glyphBuffer.glyphAt(i);
        auto bounds = font.boundsForGlyph(glyph);
        enclosingAscent = std::min(enclosingAscent.value_or(bounds.y()), bounds.y());
        enclosingDescent = std::max(enclosingDescent.value_or(bounds.maxY()), bounds.maxY());
    }

    return { enclosingAscent.value_or(0.f), enclosingDescent.value_or(0.f) };
}

Vector<LayoutRect> FontCascade::characterSelectionRectsForText(const TextRun& run, const LayoutRect& selectionRect, unsigned from, std::optional<unsigned> toOrEndOfRun) const
{
    UNUSED_PARAM(run);
    UNUSED_PARAM(selectionRect);
    UNUSED_PARAM(from);
    UNUSED_PARAM(toOrEndOfRun);
    return { };
}

} // namespace WebCore

#endif // USE(HARFBUZZ_SHAPER)
