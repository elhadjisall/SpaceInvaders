#include "engine/batch.hpp"

SpriteBatch::SpriteBatch(int capacity) : capacity_(capacity < 1 ? 1 : capacity) {}

void SpriteBatch::set_capacity(int capacity) {
    capacity_ = capacity < 1 ? 1 : capacity;
}

void SpriteBatch::begin() {
    vertices_.clear();
    indices_.clear();
    ranges_.clear();
    sprite_count_ = 0;
    sprites_in_range_ = 0;
    open_ = false;
    current_texture_ = 0;
    range_start_ = 0;
}

void SpriteBatch::close_range() {
    if (!open_ || sprites_in_range_ == 0) {
        open_ = false;
        sprites_in_range_ = 0;
        return;
    }
    BatchRange range;
    range.texture = current_texture_;
    range.first_index = range_start_;
    range.index_count = static_cast<uint32_t>(sprites_in_range_ * 6);
    range.sprites = static_cast<uint32_t>(sprites_in_range_);
    ranges_.push_back(range);
    open_ = false;
    sprites_in_range_ = 0;
}

void SpriteBatch::push(uint32_t texture,
                       float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (open_ && (texture != current_texture_ || sprites_in_range_ >= capacity_)) {
        close_range();
    }
    if (!open_) {
        open_ = true;
        current_texture_ = texture;
        sprites_in_range_ = 0;
        range_start_ = static_cast<uint32_t>(indices_.size());
    }

    float cr = static_cast<float>(r) / 255.f;
    float cg = static_cast<float>(g) / 255.f;
    float cb = static_cast<float>(b) / 255.f;
    float ca = static_cast<float>(a) / 255.f;

    uint32_t base = static_cast<uint32_t>(vertices_.size());
    SpriteVertex q0{x,     y,     u0, v0, cr, cg, cb, ca};
    SpriteVertex q1{x + w, y,     u1, v0, cr, cg, cb, ca};
    SpriteVertex q2{x + w, y + h, u1, v1, cr, cg, cb, ca};
    SpriteVertex q3{x,     y + h, u0, v1, cr, cg, cb, ca};
    vertices_.push_back(q0);
    vertices_.push_back(q1);
    vertices_.push_back(q2);
    vertices_.push_back(q3);

    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);

    ++sprites_in_range_;
    ++sprite_count_;
}

void SpriteBatch::end() {
    close_range();
}
