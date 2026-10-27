#include "engine/batch.hpp"

#include <cstdio>

namespace {

void push_n(SpriteBatch& batch, int n, uint32_t texture) {
    for (int i = 0; i < n; ++i) {
        float x = static_cast<float>(i);
        batch.push(texture, x, 0.f, 4.f, 4.f, 0.f, 0.f, 1.f, 1.f, 255, 255, 255, 255);
    }
}

} // namespace

int test_batch() {
    {
        SpriteBatch batch(128);
        batch.begin();
        push_n(batch, 100, 1);
        batch.end();
        if (batch.batch_count() != 1 || batch.sprite_count() != 100) {
            std::fprintf(stderr, "batch: shared texture produced %d batches\n", batch.batch_count());
            return 1;
        }
        if (batch.vertices().size() != 400 || batch.indices().size() != 600) {
            std::fprintf(stderr, "batch: vertex or index count is wrong\n");
            return 1;
        }
    }

    {
        SpriteBatch batch(32);
        batch.begin();
        push_n(batch, 100, 1);
        batch.end();
        // 32 + 32 + 32 + 4
        if (batch.batch_count() != 4) {
            std::fprintf(stderr, "batch: full buffer produced %d batches, expected 4\n", batch.batch_count());
            return 1;
        }
        int sprites = 0;
        for (const BatchRange& range : batch.ranges()) {
            if (range.texture != 1) {
                std::fprintf(stderr, "batch: range changed texture on its own\n");
                return 1;
            }
            sprites += static_cast<int>(range.sprites);
        }
        if (sprites != 100) {
            std::fprintf(stderr, "batch: ranges lost sprites\n");
            return 1;
        }
    }

    {
        SpriteBatch batch(64);
        batch.begin();
        for (int i = 0; i < 20; ++i) {
            uint32_t texture = (i % 2 == 0) ? 1u : 2u;
            batch.push(texture, static_cast<float>(i), 0.f, 2.f, 2.f, 0.f, 0.f, 1.f, 1.f, 255, 0, 0, 255);
        }
        batch.end();
        if (batch.batch_count() != 20) {
            std::fprintf(stderr, "batch: texture changes produced %d batches, expected 20\n", batch.batch_count());
            return 1;
        }
    }

    {
        SpriteBatch batch(64);
        batch.begin();
        push_n(batch, 10, 1);
        push_n(batch, 10, 2);
        batch.end();
        if (batch.batch_count() != 2) {
            std::fprintf(stderr, "batch: one texture change produced %d batches\n", batch.batch_count());
            return 1;
        }
    }

    std::printf("ok batch shared=1 filled=4 switches=20\n");
    return 0;
}
