#pragma once

#include "engine/components.hpp"
#include "engine/pool.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

struct Slot {
    uint32_t generation = 0;
    bool alive = false;
};

class World {
public:
    Entity create();
    void destroy(Entity e);

    bool alive(Entity e) const;
    uint32_t generation(Entity e) const;
    size_t alive_count() const { return alive_; }

    Pool<Transform> transforms;
    Pool<Velocity> velocities;
    Pool<Appearance> appearances;
    Pool<Collider> colliders;
    Pool<Alien> aliens;
    Pool<Bullet> bullets;
    Pool<Player> players;

private:
    std::vector<Slot> slots_;
    std::vector<uint32_t> free_;
    size_t alive_ = 0;
};
