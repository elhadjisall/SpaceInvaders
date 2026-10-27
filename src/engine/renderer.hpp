#pragma once

#include "engine/batch.hpp"

#include <cstdint>

// One program, one dynamic VBO, one dynamic IBO.
// end() issues one glDrawElements per batch range.
class Renderer {
public:
    bool init();
    void shutdown();

    uint32_t upload_rgba(int width, int height, const uint8_t* rgba);
    void destroy_texture(uint32_t texture);

    void begin(int logical_w, int logical_h, int framebuffer_w, int framebuffer_h);
    SpriteBatch& batch() { return batch_; }
    void end();

    int draw_calls() const { return draw_calls_; }
    int batch_count() const { return batch_.batch_count(); }

private:
    bool compile_program();

    SpriteBatch batch_;
    uint32_t program_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    uint32_t ibo_ = 0;
    int proj_loc_ = -1;
    int tex_loc_ = -1;
    int draw_calls_ = 0;
    intptr_t vbo_bytes_ = 0;
    intptr_t ibo_bytes_ = 0;
};
