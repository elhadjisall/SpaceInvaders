#include "game/game.hpp"

#include "engine/systems.hpp"
#include "game/screen.hpp"
#include "game/sprites.hpp"
#include "game/text.hpp"

#include <cstdint>
#include <cstdio>
#include <unordered_set>
#include <vector>

namespace {

constexpr uint8_t kRedR = 128;
constexpr uint8_t kRedG = 0;
constexpr uint8_t kRedB = 0;

uint32_t xorshift32(uint32_t& rng) {
    uint32_t x = rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng = x;
    return x;
}

double random01(uint32_t& rng) {
    return static_cast<double>(xorshift32(rng)) / static_cast<double>(UINT32_MAX);
}

} // namespace

Appearance Game::make_sprite(const TexRect& rect) const {
    Appearance sprite;
    sprite.w = rect.w;
    sprite.h = rect.h;
    sprite.u0 = rect.u0;
    sprite.v0 = rect.v0;
    sprite.u1 = rect.u1;
    sprite.v1 = rect.v1;
    sprite.texture = texture_;
    sprite.r = kRedR;
    sprite.g = kRedG;
    sprite.b = kRedB;
    sprite.a = 255;
    return sprite;
}

bool Game::game_over() const {
    const Player* player = world_.players.get(player_);
    return player && player->lives <= 0;
}

int Game::anim_frame(int type) const {
    int dur = alien_update_frequency_;
    if (dur < 1) dur = 1;
    int frame = anim_time_[type - 1] / dur;
    if (frame < 0) frame = 0;
    if (frame > 1) frame = 1;
    return frame;
}

void Game::spawn_bullet(float x, float y, int dir, const TexRect& rect, uint32_t layer) {
    if (static_cast<int>(world_.bullets.size()) >= kMaxBullets) return;
    Entity e = world_.create();
    world_.transforms.insert(e, Transform{x, y});
    world_.velocities.insert(e, Velocity{0.f, static_cast<float>(dir)});
    world_.appearances.insert(e, make_sprite(rect));
    Collider box;
    box.w = rect.w;
    box.h = rect.h;
    box.layer = layer;
    world_.colliders.insert(e, box);
    world_.bullets.insert(e, Bullet{dir});
}

void Game::reset_swarm(bool fresh_frequency) {
    if (fresh_frequency) alien_update_frequency_ = 120;
    swarm_x_ = 24.f;
    swarm_max_x_ = static_cast<float>(kScreenW - 16 * kAlienCols - 3);
    aliens_killed_ = 0;
    alien_update_timer_ = 0;
    alien_move_dir_ = 4;
    for (int i = 0; i < 3; ++i) anim_time_[i] = 0;

    const int death_w = sprites::death_sprite().width;
    for (int xi = 0; xi < kAlienCols; ++xi) {
        for (int yi = 0; yi < kAlienRows; ++yi) {
            int index = xi * kAlienRows + yi;
            int type = (kAlienRows - yi) / 2 + 1;
            const Bitmap& bmp = sprites::alien(2 * (type - 1));
            float x = 16.f * static_cast<float>(xi) + swarm_x_ +
                      static_cast<float>((death_w - bmp.width) / 2);
            float y = 17.f * static_cast<float>(yi) + 128.f;

            Entity e = aliens_[index];
            world_.transforms.insert(e, Transform{x, y});
            world_.aliens.insert(e, Alien{static_cast<uint8_t>(type), 10});
            const TexRect& rect = atlas_->alien[2 * (type - 1)];
            world_.appearances.insert(e, make_sprite(rect));
            Collider box;
            box.w = rect.w;
            box.h = rect.h;
            box.layer = LAYER_ALIEN;
            world_.colliders.insert(e, box);
        }
    }
}

void Game::init(uint32_t texture, const SpriteAtlas& atlas) {
    atlas_ = &atlas;
    texture_ = texture;
    swarm_max_x_ = static_cast<float>(kScreenW - 16 * kAlienCols - 3);

    for (int i = 0; i < kAlienCount; ++i) aliens_[i] = world_.create();
    player_ = world_.create();

    world_.transforms.insert(player_, Transform{112.f - 5.f, 32.f});
    world_.players.insert(player_, Player{3});
    world_.appearances.insert(player_, make_sprite(atlas.player));
    Collider box;
    box.w = atlas.player.w;
    box.h = atlas.player.h;
    box.layer = LAYER_PLAYER;
    world_.colliders.insert(player_, box);

    reset_swarm(true);
}

void Game::cull_bullets() {
    std::vector<Entity> dead;
    float limit = atlas_->player_bullet.h;
    for (size_t i = 0; i < world_.bullets.size(); ++i) {
        Entity e = world_.bullets.entity(i);
        const Transform* t = world_.transforms.get(e);
        if (!t) continue;
        if (t->y >= static_cast<float>(kScreenH) || t->y < limit) dead.push_back(e);
    }
    for (Entity e : dead) world_.destroy(e);
}

void Game::kill_alien(Entity alien, const Appearance& shot_sprite) {
    Alien* invader = world_.aliens.get(alien);
    Transform* t = world_.transforms.get(alien);
    if (!invader || !t || invader->type == 0) return;

    score_ += 10 * (4 - static_cast<int>(invader->type));
    float old_w = shot_sprite.w;
    t->x -= static_cast<float>((sprites::death_sprite().width - static_cast<int>(old_w)) / 2);
    invader->type = 0;
    world_.colliders.remove(alien);
    world_.appearances.insert(alien, make_sprite(atlas_->death));
    ++aliens_killed_;
    if (aliens_killed_ % 15 == 0) should_change_speed_ = true;
}

void Game::collide() {
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

    std::unordered_set<Entity> spent;
    bool player_hit = false;
    for (const HashPair& pair : pairs) {
        if (spent.count(pair.a) || spent.count(pair.b)) continue;
        const Collider* ca = world_.colliders.get(pair.a);
        const Collider* cb = world_.colliders.get(pair.b);
        if (!ca || !cb) continue;

        Entity bullet = 0;
        Entity other = 0;
        uint32_t bullet_layer = 0;
        uint32_t other_layer = 0;
        bool bullet_pair = false;
        if (ca->layer == LAYER_PLAYER_BULLET || ca->layer == LAYER_ALIEN_BULLET) {
            bullet = pair.a;
            other = pair.b;
            bullet_layer = ca->layer;
            other_layer = cb->layer;
            bullet_pair = true;
        } else if (cb->layer == LAYER_PLAYER_BULLET || cb->layer == LAYER_ALIEN_BULLET) {
            bullet = pair.b;
            other = pair.a;
            bullet_layer = cb->layer;
            other_layer = ca->layer;
            bullet_pair = true;
        }
        if (!bullet_pair) continue;

        if ((bullet_layer == LAYER_PLAYER_BULLET && other_layer == LAYER_ALIEN_BULLET) ||
            (bullet_layer == LAYER_ALIEN_BULLET && other_layer == LAYER_PLAYER_BULLET)) {
            spent.insert(pair.a);
            spent.insert(pair.b);
            continue;
        }

        if (bullet_layer == LAYER_PLAYER_BULLET && other_layer == LAYER_ALIEN) {
            const Appearance* sprite = world_.appearances.get(other);
            Appearance fallback = make_sprite(atlas_->alien[0]);
            kill_alien(other, sprite ? *sprite : fallback);
            spent.insert(bullet);
            spent.insert(other);
            continue;
        }

        if (bullet_layer == LAYER_ALIEN_BULLET && other_layer == LAYER_PLAYER && !player_hit) {
            Player* player = world_.players.get(other);
            if (player && player->lives > 0) --player->lives;
            player_hit = true;
            spent.insert(bullet);
        }
    }

    for (Entity e : spent) {
        if (world_.bullets.has(e)) world_.destroy(e);
    }
}

void Game::sync_sprites() {
    for (int i = 0; i < kAlienCount; ++i) {
        Entity e = aliens_[i];
        Alien* invader = world_.aliens.get(e);
        if (!invader || invader->type == 0) continue;
        int frame = anim_frame(invader->type);
        const TexRect& rect = atlas_->alien[2 * (invader->type - 1) + frame];
        Appearance* sprite = world_.appearances.get(e);
        if (!sprite) continue;
        sprite->u0 = rect.u0;
        sprite->v0 = rect.v0;
        sprite->u1 = rect.u1;
        sprite->v1 = rect.v1;
        sprite->w = rect.w;
        sprite->h = rect.h;
        if (Collider* box = world_.colliders.get(e)) {
            box->w = rect.w;
            box->h = rect.h;
        }
    }

    int bullet_frame = bullet_anim_time_ / 5;
    if (bullet_frame < 0) bullet_frame = 0;
    if (bullet_frame > 1) bullet_frame = 1;
    const TexRect& bullet_rect = atlas_->alien_bullet[bullet_frame];
    for (size_t i = 0; i < world_.bullets.size(); ++i) {
        const Bullet& bullet = world_.bullets.dense(i);
        if (bullet.dir > 0) continue;
        Entity e = world_.bullets.entity(i);
        Appearance* sprite = world_.appearances.get(e);
        if (!sprite) continue;
        sprite->u0 = bullet_rect.u0;
        sprite->v0 = bullet_rect.v0;
        sprite->u1 = bullet_rect.u1;
        sprite->v1 = bullet_rect.v1;
    }
}

void Game::update(const FrameInput& input) {
    if (game_over()) return;

    move_velocities(world_);
    cull_bullets();
    collide();
    if (game_over()) return;

    if (should_change_speed_) {
        should_change_speed_ = false;
        if (alien_update_frequency_ > 1) alien_update_frequency_ /= 2;
    }

    for (int i = 0; i < kAlienCount; ++i) {
        Alien* invader = world_.aliens.get(aliens_[i]);
        if (!invader) continue;
        if (invader->type == 0 && invader->death > 0) --invader->death;
        if (invader->type == 0 && invader->death == 0) world_.appearances.remove(aliens_[i]);
    }

    if (alien_update_timer_ >= alien_update_frequency_) {
        alien_update_timer_ = 0;
        if (static_cast<int>(swarm_x_) + alien_move_dir_ < 0) {
            alien_move_dir_ *= -1;
            for (int i = 0; i < kAlienCount; ++i) {
                if (Transform* t = world_.transforms.get(aliens_[i])) t->y -= 8.f;
            }
        } else if (swarm_x_ > swarm_max_x_ - static_cast<float>(alien_move_dir_)) {
            alien_move_dir_ *= -1;
        }
        swarm_x_ += static_cast<float>(alien_move_dir_);
        for (int i = 0; i < kAlienCount; ++i) {
            if (Transform* t = world_.transforms.get(aliens_[i])) t->x += static_cast<float>(alien_move_dir_);
        }

        if (aliens_killed_ < kAlienCount && static_cast<int>(world_.bullets.size()) < kMaxBullets) {
            int rai = static_cast<int>(static_cast<double>(kAlienCount) * random01(rng_));
            if (rai >= kAlienCount) rai = kAlienCount - 1;
            int guard = 0;
            while (world_.aliens.get(aliens_[rai]) && world_.aliens.get(aliens_[rai])->type == 0 && guard < kAlienCount) {
                rai = static_cast<int>(static_cast<double>(kAlienCount) * random01(rng_));
                if (rai >= kAlienCount) rai = kAlienCount - 1;
                ++guard;
            }
            Alien* shooter = world_.aliens.get(aliens_[rai]);
            Transform* at = world_.transforms.get(aliens_[rai]);
            if (shooter && at && shooter->type != 0) {
                const Bitmap& bmp = sprites::alien(2 * (shooter->type - 1));
                float bx = at->x + static_cast<float>(bmp.width / 2);
                float by = at->y - atlas_->alien_bullet[0].h;
                spawn_bullet(bx, by, -2, atlas_->alien_bullet[0], LAYER_ALIEN_BULLET);
            }
        }
    }

    for (int i = 0; i < 3; ++i) {
        int period = 2 * alien_update_frequency_;
        if (period < 2) period = 2;
        ++anim_time_[i];
        if (anim_time_[i] >= period) anim_time_[i] = 0;
    }
    ++bullet_anim_time_;
    if (bullet_anim_time_ >= 2 * 5) bullet_anim_time_ = 0;
    ++alien_update_timer_;

    Transform* player_t = world_.transforms.get(player_);
    if (player_t && input.move_dir != 0) {
        float step = static_cast<float>(2 * input.move_dir);
        float nx = player_t->x + step;
        float max_x = static_cast<float>(kScreenW) - atlas_->player.w;
        if (nx >= max_x) nx = max_x;
        if (nx <= 0.f) nx = 0.f;
        player_t->x = nx;
    }

    if (aliens_killed_ < kAlienCount) {
        int left = 0;
        while (left < kAlienCount - 1) {
            Alien* invader = world_.aliens.get(aliens_[left]);
            if (invader && invader->type != 0) break;
            ++left;
        }
        Alien* left_alien = world_.aliens.get(aliens_[left]);
        Transform* left_t = world_.transforms.get(aliens_[left]);
        if (left_alien && left_t && left_alien->type != 0) {
            const Bitmap& bmp = sprites::alien(2 * (left_alien->type - 1));
            float pos = left_t->x - static_cast<float>((sprites::death_sprite().width - bmp.width) / 2);
            if (pos > swarm_x_) swarm_x_ = pos;

            int right = kAlienCount - 1;
            while (right > 0) {
                Alien* invader = world_.aliens.get(aliens_[right]);
                if (invader && invader->type != 0) break;
                --right;
            }
            Transform* right_t = world_.transforms.get(aliens_[right]);
            if (right_t) {
                pos = static_cast<float>(kScreenW) - right_t->x - 13.f + pos;
                if (pos > swarm_max_x_) swarm_max_x_ = pos;
            }
        }
    } else {
        // Bullets from the previous wave are cleared with the new formation.
        std::vector<Entity> leftover;
        for (size_t i = 0; i < world_.bullets.size(); ++i) leftover.push_back(world_.bullets.entity(i));
        for (Entity e : leftover) world_.destroy(e);
        reset_swarm(true);
    }

    if (input.fire) {
        if (player_t) {
            float bx = player_t->x + atlas_->player.w / 2.f;
            float by = player_t->y + atlas_->player.h;
            spawn_bullet(bx, by, 2, atlas_->player_bullet, LAYER_PLAYER_BULLET);
        }
    }

    sync_sprites();
}

void Game::draw(SpriteBatch& batch) {
    const Player* player = world_.players.get(player_);
    int lives = player ? player->lives : 0;

    if (lives <= 0) {
        draw_text(batch, texture_, *atlas_, "GAME OVER",
                  static_cast<float>(kScreenW) / 2.f - 30.f,
                  static_cast<float>(kScreenH) / 2.f,
                  kRedR, kRedG, kRedB);
        draw_text(batch, texture_, *atlas_, "SCORE", 4.f,
                  static_cast<float>(kScreenH) - 7.f - 7.f,
                  kRedR, kRedG, kRedB);
        char score_text[16];
        std::snprintf(score_text, sizeof(score_text), "%d", score_);
        draw_text(batch, texture_, *atlas_, score_text,
                  4.f + 2.f * atlas_->glyphs[0].w,
                  static_cast<float>(kScreenH) - 2.f * atlas_->glyphs[0].h - 12.f,
                  kRedR, kRedG, kRedB);
        return;
    }

    submit_sprites(world_, batch);

    draw_text(batch, texture_, *atlas_, "SCORE", 4.f,
              static_cast<float>(kScreenH) - 7.f - 7.f,
              kRedR, kRedG, kRedB);
    char score_text[16];
    std::snprintf(score_text, sizeof(score_text), "%d", score_);
    draw_text(batch, texture_, *atlas_, score_text,
              4.f + 2.f * atlas_->glyphs[0].w,
              static_cast<float>(kScreenH) - 2.f * atlas_->glyphs[0].h - 12.f,
              kRedR, kRedG, kRedB);

    char credit_text[16];
    std::snprintf(credit_text, sizeof(credit_text), "CREDIT %02d", credits_);
    draw_text(batch, texture_, *atlas_, credit_text, 164.f, 7.f, kRedR, kRedG, kRedB);

    char lives_text[16];
    std::snprintf(lives_text, sizeof(lives_text), "%d", lives);
    draw_text(batch, texture_, *atlas_, lives_text, 4.f, 7.f, kRedR, kRedG, kRedB);

    float xp = 11.f + atlas_->glyphs[0].w;
    for (int i = 0; i < lives - 1; ++i) {
        const TexRect& ship = atlas_->player;
        batch.push(texture_, xp, 7.f, ship.w, ship.h, ship.u0, ship.v0, ship.u1, ship.v1, kRedR, kRedG, kRedB, 255);
        xp += ship.w + 2.f;
    }

    const TexRect& white = atlas_->white;
    batch.push(texture_, 0.f, 16.f, static_cast<float>(kScreenW), 1.f,
               white.u0, white.v0, white.u1, white.v1, kRedR, kRedG, kRedB, 255);
}
