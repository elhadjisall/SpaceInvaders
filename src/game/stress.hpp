#pragma once

#include "engine/batch.hpp"
#include "engine/spatial_hash.hpp"
#include "engine/world.hpp"
#include "game/atlas.hpp"

class Stress {
public:
    static constexpr int kCount = 640;

    void init(uint32_t texture, const SpriteAtlas& atlas);
    void update();
    void draw(SpriteBatch& batch);

    const SpatialHash& hash() const { return hash_; }
    int entity_count() const { return static_cast<int>(world_.alive_count()); }

private:
    World world_;
    SpatialHash hash_{32.f};
    uint32_t texture_ = 0;
    const SpriteAtlas* atlas_ = nullptr;
};
