#pragma once

#include <cstdint>
#include <vector>

struct TexRect {
    float u0 = 0.f;
    float v0 = 0.f;
    float u1 = 1.f;
    float v1 = 1.f;
    float w = 0.f;
    float h = 0.f;
};

// Every game sprite and glyph shares this atlas, so a normal frame is one batch
// until the vertex buffer fills.
struct SpriteAtlas {
    static constexpr int kGlyphs = 65;

    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgba;
    TexRect alien[6];
    TexRect death;
    TexRect player;
    TexRect player_bullet;
    TexRect alien_bullet[2];
    TexRect glyphs[kGlyphs];
    TexRect white;
};

SpriteAtlas build_atlas();
