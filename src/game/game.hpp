#pragma once

#include "engine/batch.hpp"
#include "engine/spatial_hash.hpp"
#include "engine/world.hpp"
#include "game/atlas.hpp"

struct FrameInput {
    int move_dir = 0;
    bool fire = false;
};

class Game {
public:
    static constexpr int kAlienCols = 11;
    static constexpr int kAlienRows = 5;
    static constexpr int kAlienCount = kAlienCols * kAlienRows;
    static constexpr int kMaxBullets = 128;

    void init(uint32_t texture, const SpriteAtlas& atlas);
    void update(const FrameInput& input);
    void draw(SpriteBatch& batch);

    const SpatialHash& hash() const { return hash_; }
    int entity_count() const { return static_cast<int>(world_.alive_count()); }
    bool game_over() const;

private:
    Appearance make_sprite(const TexRect& rect) const;
    void reset_swarm(bool fresh_frequency);
    void spawn_bullet(float x, float y, int dir, const TexRect& rect, uint32_t layer);
    void cull_bullets();
    void collide();
    void kill_alien(Entity alien, const Appearance& shot_sprite);
    void sync_sprites();
    int anim_frame(int type) const;

    World world_;
    SpatialHash hash_{32.f};
    const SpriteAtlas* atlas_ = nullptr;
    uint32_t texture_ = 0;

    Entity aliens_[kAlienCount] = {};
    Entity player_ = 0;

    int score_ = 0;
    int credits_ = 0;
    int aliens_killed_ = 0;
    int alien_update_frequency_ = 120;
    int alien_update_timer_ = 0;
    bool should_change_speed_ = false;
    int alien_move_dir_ = 4;
    float swarm_x_ = 24.f;
    float swarm_max_x_ = 0.f;
    int anim_time_[3] = {};
    int bullet_anim_time_ = 0;
    uint32_t rng_ = 13;
};
