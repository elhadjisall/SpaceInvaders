#pragma once

#include "engine/batch.hpp"
#include "engine/world.hpp"

inline void move_velocities(World& world) {
    for (size_t i = 0; i < world.velocities.size(); ++i) {
        Entity e = world.velocities.entity(i);
        Transform* t = world.transforms.get(e);
        if (!t) continue;
        const Velocity& v = world.velocities.dense(i);
        t->x += v.x;
        t->y += v.y;
    }
}

inline void submit_sprites(const World& world, SpriteBatch& batch) {
    for (size_t i = 0; i < world.appearances.size(); ++i) {
        Entity e = world.appearances.entity(i);
        const Transform* t = world.transforms.get(e);
        if (!t) continue;
        const Appearance& s = world.appearances.dense(i);
        batch.push(s.texture, t->x, t->y, s.w, s.h, s.u0, s.v0, s.u1, s.v1, s.r, s.g, s.b, s.a);
    }
}
