#include "game/stress.hpp"

#include "engine/systems.hpp"
#include "game/screen.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <unordered_set>
#include <vector>

namespace {

uint32_t xorshift32(uint32_t& rng) {
    uint32_t x = rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng = x;
    return x;
}

int rand_range(uint32_t& rng, int lo, int hi) {
    uint32_t span = static_cast<uint32_t>(hi - lo + 1);
    return lo + static_cast<int>(xorshift32(rng) % span);
}

} // namespace

void Stress::init(uint32_t texture, const SpriteAtlas& atlas) {
    texture_ = texture;
    atlas_ = &atlas;
    uint32_t rng = 0x51u;

    const TexRect& sprite = atlas.alien[0];
    for (int i = 0; i < kCount; ++i) {
        Entity e = world_.create();
        float w = sprite.w;
        float h = sprite.h;
        float x = static_cast<float>(rand_range(rng, 0, kScreenW - static_cast<int>(w) - 1));
        float y = static_cast<float>(rand_range(rng, 0, kScreenH - static_cast<int>(h) - 1));
        int vx = rand_range(rng, -3, 3);
        int vy = rand_range(rng, -3, 3);
        if (vx == 0) vx = 1;
        if (vy == 0) vy = -1;

        world_.transforms.insert(e, Transform{x, y});
        world_.velocities.insert(e, Velocity{static_cast<float>(vx), static_cast<float>(vy)});
        Appearance look;
        look.w = w;
        look.h = h;
        look.u0 = sprite.u0;
        look.v0 = sprite.v0;
        look.u1 = sprite.u1;
        look.v1 = sprite.v1;
        look.texture = texture_;
        look.r = static_cast<uint8_t>(160 + (i * 17) % 80);
        look.g = static_cast<uint8_t>((i * 29) % 180);
        look.b = static_cast<uint8_t>(40 + (i * 13) % 160);
        look.a = 255;
        world_.appearances.insert(e, look);
        Collider box;
        box.w = w;
        box.h = h;
        box.layer = LAYER_STRESS;
        world_.colliders.insert(e, box);
    }
}

void Stress::update() {
    move_velocities(world_);

    for (size_t i = 0; i < world_.velocities.size(); ++i) {
        Entity e = world_.velocities.entity(i);
        Transform* t = world_.transforms.get(e);
        Velocity* v = world_.velocities.get(e);
        const Appearance* sprite = world_.appearances.get(e);
        if (!t || !v || !sprite) continue;
        if (t->x < 0.f) {
            t->x = 0.f;
            v->x = std::fabs(v->x);
        } else if (t->x + sprite->w > static_cast<float>(kScreenW)) {
            t->x = static_cast<float>(kScreenW) - sprite->w;
            v->x = -std::fabs(v->x);
        }
        if (t->y < 0.f) {
            t->y = 0.f;
            v->y = std::fabs(v->y);
        } else if (t->y + sprite->h > static_cast<float>(kScreenH)) {
            t->y = static_cast<float>(kScreenH) - sprite->h;
            v->y = -std::fabs(v->y);
        }
    }

    hash_.clear();
    for (size_t i = 0; i < world_.colliders.size(); ++i) {
        Entity e = world_.colliders.entity(i);
        const Transform* t = world_.transforms.get(e);
        if (!t) continue;
        const Collider& box = world_.colliders.dense(i);
        hash_.insert(e, t->x, t->y, box.w, box.h);
    }

    std::vector<HashPair> pairs;
    hash_.query(pairs);
    std::unordered_set<Entity> bumped;
    for (const HashPair& pair : pairs) {
        if (!bumped.insert(pair.a).second || !bumped.insert(pair.b).second) continue;
        Velocity* va = world_.velocities.get(pair.a);
        Velocity* vb = world_.velocities.get(pair.b);
        if (!va || !vb) continue;
        float tx = va->x;
        float ty = va->y;
        va->x = vb->x;
        va->y = vb->y;
        vb->x = tx;
        vb->y = ty;
    }
}

void Stress::draw(SpriteBatch& batch) {
    submit_sprites(world_, batch);
}
