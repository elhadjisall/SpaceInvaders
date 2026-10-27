#include "game/atlas.hpp"

#include "game/sprites.hpp"

#include <cstdio>

namespace {

struct Packer {
    int width;
    int height;
    int x = 1;
    int y = 1;
    int row_h = 0;
    std::vector<uint8_t>* rgba = nullptr;

    TexRect place(const Bitmap& bmp) {
        if (x + bmp.width + 1 > width) {
            x = 1;
            y += row_h + 1;
            row_h = 0;
        }
        if (y + bmp.height >= height) {
            std::fprintf(stderr, "atlas overflow at %d,%d for %dx%d\n", x, y, bmp.width, bmp.height);
        }
        int px = x;
        int py = y;
        for (int row = 0; row < bmp.height; ++row) {
            int dst_row = py + (bmp.height - 1 - row);
            for (int col = 0; col < bmp.width; ++col) {
                uint8_t on = bmp.pixels[row * bmp.width + col];
                size_t di = static_cast<size_t>((dst_row * width + (px + col)) * 4);
                if (on) {
                    (*rgba)[di + 0] = 255;
                    (*rgba)[di + 1] = 255;
                    (*rgba)[di + 2] = 255;
                    (*rgba)[di + 3] = 255;
                }
            }
        }
        TexRect rect;
        rect.w = static_cast<float>(bmp.width);
        rect.h = static_cast<float>(bmp.height);
        rect.u0 = static_cast<float>(px) / static_cast<float>(width);
        rect.v0 = static_cast<float>(py) / static_cast<float>(height);
        rect.u1 = static_cast<float>(px + bmp.width) / static_cast<float>(width);
        rect.v1 = static_cast<float>(py + bmp.height) / static_cast<float>(height);
        x += bmp.width + 1;
        if (bmp.height > row_h) row_h = bmp.height;
        return rect;
    }
};

} // namespace

SpriteAtlas build_atlas() {
    SpriteAtlas atlas;
    atlas.width = 256;
    atlas.height = 128;
    atlas.rgba.assign(static_cast<size_t>(atlas.width * atlas.height * 4), 0);

    Packer pack;
    pack.width = atlas.width;
    pack.height = atlas.height;
    pack.rgba = &atlas.rgba;

    for (int i = 0; i < 6; ++i) atlas.alien[i] = pack.place(sprites::alien(i));
    atlas.death = pack.place(sprites::death_sprite());
    atlas.player = pack.place(sprites::player_sprite());
    atlas.player_bullet = pack.place(sprites::player_bullet_sprite());
    atlas.alien_bullet[0] = pack.place(sprites::alien_bullet_sprite(0));
    atlas.alien_bullet[1] = pack.place(sprites::alien_bullet_sprite(1));
    for (int i = 0; i < SpriteAtlas::kGlyphs; ++i) atlas.glyphs[i] = pack.place(sprites::glyph(i));

    Bitmap white;
    static const uint8_t white_px[] = {1};
    white.width = 1;
    white.height = 1;
    white.count = 1;
    white.pixels = white_px;
    atlas.white = pack.place(white);
    return atlas;
}
