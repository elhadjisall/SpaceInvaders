#include "engine/spatial_hash.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <unordered_set>
#include <vector>

namespace {

struct Body {
    float x, y, w, h, vx, vy;
};

uint32_t xorshift32(uint32_t& rng) {
    uint32_t x = rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng = x;
    return x;
}

bool overlap_body(const Body& a, const Body& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

uint64_t pair_key(Entity a, Entity b) {
    uint32_t lo = a < b ? a : b;
    uint32_t hi = a < b ? b : a;
    return (static_cast<uint64_t>(lo) << 32) | hi;
}

} // namespace

int test_spatial() {
    // Two boxes in neighboring cells, and one far away. The far box must not
    // be tested. The neighbors must be tested once.
    {
        SpatialHash hash(32.f);
        hash.insert(1, 0.f, 0.f, 10.f, 10.f);
        hash.insert(2, 40.f, 0.f, 10.f, 10.f);
        hash.insert(3, 200.f, 200.f, 10.f, 10.f);
        std::vector<HashPair> pairs;
        hash.query(pairs);
        if (hash.stats().pair_tests != 1 || !pairs.empty() || hash.cell_count() != 3) {
            std::fprintf(stderr, "spatial: neighbor probe pair_tests=%d pairs=%zu cells=%d\n",
                         hash.stats().pair_tests, pairs.size(), hash.cell_count());
            return 1;
        }
    }

    constexpr int kCount = 640;
    constexpr float kW = 480.f;
    constexpr float kH = 360.f;
    uint32_t rng = 13;
    std::vector<Body> bodies(static_cast<size_t>(kCount));
    for (int i = 0; i < kCount; ++i) {
        Body& b = bodies[static_cast<size_t>(i)];
        b.w = 12.f;
        b.h = 12.f;
        if (i < 16) {
            b.x = 40.f + static_cast<float>(i % 4) * 4.f;
            b.y = 40.f + static_cast<float>(i / 4) * 4.f;
            b.vx = 1.f;
            b.vy = 1.f;
        } else {
            b.x = static_cast<float>(xorshift32(rng) % 460);
            b.y = static_cast<float>(xorshift32(rng) % 340);
            int vx = static_cast<int>(xorshift32(rng) % 5) - 2;
            int vy = static_cast<int>(xorshift32(rng) % 5) - 2;
            if (vx == 0) vx = 1;
            if (vy == 0) vy = -1;
            b.vx = static_cast<float>(vx);
            b.vy = static_cast<float>(vy);
        }
    }

    for (int step = 0; step < 40; ++step) {
        for (Body& b : bodies) {
            b.x += b.vx;
            b.y += b.vy;
            if (b.x < 0.f) {
                b.x = 0.f;
                b.vx = std::fabs(b.vx);
            } else if (b.x + b.w > kW) {
                b.x = kW - b.w;
                b.vx = -std::fabs(b.vx);
            }
            if (b.y < 0.f) {
                b.y = 0.f;
                b.vy = std::fabs(b.vy);
            } else if (b.y + b.h > kH) {
                b.y = kH - b.h;
                b.vy = -std::fabs(b.vy);
            }
        }
    }

    SpatialHash hash(40.f);
    for (int i = 0; i < kCount; ++i) {
        const Body& b = bodies[static_cast<size_t>(i)];
        hash.insert(static_cast<Entity>(i), b.x, b.y, b.w, b.h);
    }
    std::vector<HashPair> pairs;
    hash.query(pairs);

    for (const HashPair& p : pairs) {
        if (!overlap_body(bodies[p.a], bodies[p.b])) {
            std::fprintf(stderr, "spatial: hash reported a pair that does not overlap\n");
            return 1;
        }
    }

    std::unordered_set<uint64_t> found;
    for (const HashPair& p : pairs) found.insert(pair_key(p.a, p.b));

    constexpr int kSample = 48;
    int sample_hits = 0;
    for (int i = 0; i < kSample; ++i) {
        for (int j = i + 1; j < kSample; ++j) {
            if (!overlap_body(bodies[static_cast<size_t>(i)], bodies[static_cast<size_t>(j)])) continue;
            ++sample_hits;
            if (!found.count(pair_key(static_cast<Entity>(i), static_cast<Entity>(j)))) {
                std::fprintf(stderr, "spatial: missed sample pair %d,%d\n", i, j);
                return 1;
            }
        }
    }
    if (sample_hits == 0) {
        std::fprintf(stderr, "spatial: sample had no overlaps, the check was vacuous\n");
        return 1;
    }

    const int brute = kCount * (kCount - 1) / 2;
    if (!(hash.stats().pair_tests < brute)) {
        std::fprintf(stderr, "spatial: pair_tests=%d brute=%d\n", hash.stats().pair_tests, brute);
        return 1;
    }
    if (hash.cell_count() < 2) {
        std::fprintf(stderr, "spatial: grid collapsed to %d cells\n", hash.cell_count());
        return 1;
    }

    std::printf("ok spatial n=%d pair_tests=%d brute=%d sample_hits=%d cells=%d\n",
                kCount, hash.stats().pair_tests, brute, sample_hits, hash.cell_count());
    return 0;
}
