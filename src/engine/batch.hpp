#pragma once

#include <cstdint>
#include <vector>

struct SpriteVertex {
    float x, y, u, v;
    float r, g, b, a;
};

struct BatchRange {
    uint32_t texture = 0;
    uint32_t first_index = 0;
    uint32_t index_count = 0;
    uint32_t sprites = 0;
};

// CPU-side sprite batch. One range becomes one draw call.
// A new range starts when the texture changes or the range hits capacity.
class SpriteBatch {
public:
    static constexpr int kDefaultCapacity = 512;

    explicit SpriteBatch(int capacity = kDefaultCapacity);

    void set_capacity(int capacity);
    void begin();
    void push(uint32_t texture,
              float x, float y, float w, float h,
              float u0, float v0, float u1, float v1,
              uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void end();

    int batch_count() const { return static_cast<int>(ranges_.size()); }
    int sprite_count() const { return sprite_count_; }
    const std::vector<SpriteVertex>& vertices() const { return vertices_; }
    const std::vector<uint32_t>& indices() const { return indices_; }
    const std::vector<BatchRange>& ranges() const { return ranges_; }

private:
    void close_range();

    int capacity_;
    int sprite_count_ = 0;
    int sprites_in_range_ = 0;
    bool open_ = false;
    uint32_t current_texture_ = 0;
    uint32_t range_start_ = 0;
    std::vector<SpriteVertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<BatchRange> ranges_;
};
