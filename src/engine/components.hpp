#pragma once

#include <cstdint>

struct Transform {
    float x = 0.f;
    float y = 0.f;
};

struct Velocity {
    float x = 0.f;
    float y = 0.f;
};

// One textured quad. Texture id is a GL name when a context exists, and just
// an integer key for the headless batcher.
struct Appearance {
    float w = 0.f;
    float h = 0.f;
    float u0 = 0.f;
    float v0 = 0.f;
    float u1 = 1.f;
    float v1 = 1.f;
    uint32_t texture = 0;
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

struct Collider {
    float w = 0.f;
    float h = 0.f;
    uint32_t layer = 0;
};

enum CollisionLayer : uint32_t {
    LAYER_PLAYER = 1u << 0,
    LAYER_ALIEN = 1u << 1,
    LAYER_PLAYER_BULLET = 1u << 2,
    LAYER_ALIEN_BULLET = 1u << 3,
    LAYER_STRESS = 1u << 4
};

// type 0 is a dead invader playing its explosion.
struct Alien {
    uint8_t type = 1;
    uint8_t death = 10;
};

// dir is pixels per frame on y. Positive travels up.
struct Bullet {
    int dir = 0;
};

struct Player {
    int lives = 3;
};
