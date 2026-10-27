#include "engine/world.hpp"

Entity World::create() {
    Entity e;
    if (!free_.empty()) {
        e = free_.back();
        free_.pop_back();
        slots_[e].alive = true;
    } else {
        e = static_cast<Entity>(slots_.size());
        Slot slot;
        slot.generation = 1;
        slot.alive = true;
        slots_.push_back(slot);
    }
    ++alive_;
    return e;
}

void World::destroy(Entity e) {
    if (!alive(e)) return;
    transforms.remove(e);
    velocities.remove(e);
    appearances.remove(e);
    colliders.remove(e);
    aliens.remove(e);
    bullets.remove(e);
    players.remove(e);
    slots_[e].alive = false;
    slots_[e].generation += 1;
    free_.push_back(e);
    --alive_;
}

bool World::alive(Entity e) const {
    return e < slots_.size() && slots_[e].alive;
}

uint32_t World::generation(Entity e) const {
    if (e >= slots_.size()) return 0;
    return slots_[e].generation;
}
