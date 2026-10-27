#pragma once

#include "engine/batch.hpp"
#include "game/atlas.hpp"

inline void draw_text(SpriteBatch& batch, uint32_t texture, const SpriteAtlas& atlas,
                      const char* text, float x, float y,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    float xp = x;
    for (const char* p = text; *p != '\0'; ++p) {
        int index = static_cast<int>(*p) - 32;
        if (index < 0 || index >= SpriteAtlas::kGlyphs) continue;
        const TexRect& glyph = atlas.glyphs[index];
        batch.push(texture, xp, y, glyph.w, glyph.h, glyph.u0, glyph.v0, glyph.u1, glyph.v1, r, g, b, a);
        xp += glyph.w + 1.f;
    }
}
