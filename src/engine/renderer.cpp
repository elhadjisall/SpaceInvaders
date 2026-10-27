#include "engine/renderer.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace {

const char* kVertexSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec2 a_uv;\n"
    "layout(location = 2) in vec4 a_color;\n"
    "uniform mat4 u_proj;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    v_uv = a_uv;\n"
    "    v_color = a_color;\n"
    "    gl_Position = u_proj * vec4(a_pos, 0.0, 1.0);\n"
    "}\n";

const char* kFragmentSrc =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "in vec4 v_color;\n"
    "uniform sampler2D u_tex;\n"
    "out vec4 frag;\n"
    "void main() {\n"
    "    vec4 t = texture(u_tex, v_uv);\n"
    "    if (t.a < 0.5) discard;\n"
    "    frag = vec4(v_color.rgb, v_color.a);\n"
    "}\n";

unsigned compile_shader(unsigned type, const char* src) {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "shader compile failed:\n%s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

bool Renderer::compile_program() {
    unsigned vs = compile_shader(GL_VERTEX_SHADER, kVertexSrc);
    unsigned fs = compile_shader(GL_FRAGMENT_SHADER, kFragmentSrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "program link failed:\n%s\n", log);
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    proj_loc_ = glGetUniformLocation(program_, "u_proj");
    tex_loc_ = glGetUniformLocation(program_, "u_tex");
    return true;
}

bool Renderer::init() {
    if (!compile_program()) return false;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ibo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, 1, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 1, nullptr, GL_DYNAMIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(sizeof(SpriteVertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SpriteVertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SpriteVertex, u)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SpriteVertex, r)));

    glBindVertexArray(0);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);
    glUniform1i(tex_loc_, 0);
    glActiveTexture(GL_TEXTURE0);
    return true;
}

void Renderer::shutdown() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ibo_) glDeleteBuffers(1, &ibo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
    vbo_ = ibo_ = vao_ = program_ = 0;
}

uint32_t Renderer::upload_rgba(int width, int height, const uint8_t* rgba) {
    unsigned tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

void Renderer::destroy_texture(uint32_t texture) {
    unsigned tex = texture;
    glDeleteTextures(1, &tex);
}

void Renderer::begin(int logical_w, int logical_h, int framebuffer_w, int framebuffer_h) {
    glViewport(0, 0, framebuffer_w, framebuffer_h);
    glClearColor(0.f, 128.f / 255.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program_);
    glm::mat4 proj = glm::ortho(0.f, static_cast<float>(logical_w), 0.f, static_cast<float>(logical_h), -1.f, 1.f);
    glUniformMatrix4fv(proj_loc_, 1, GL_FALSE, glm::value_ptr(proj));
    batch_.begin();
    draw_calls_ = 0;
}

void Renderer::end() {
    batch_.end();
    const std::vector<SpriteVertex>& vertices = batch_.vertices();
    const std::vector<uint32_t>& indices = batch_.indices();
    if (indices.empty()) {
        draw_calls_ = 0;
        return;
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    intptr_t vbytes = static_cast<intptr_t>(vertices.size() * sizeof(SpriteVertex));
    if (vbytes > vbo_bytes_) {
        glBufferData(GL_ARRAY_BUFFER, vbytes, vertices.data(), GL_DYNAMIC_DRAW);
        vbo_bytes_ = vbytes;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, vbytes, vertices.data());
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    intptr_t ibytes = static_cast<intptr_t>(indices.size() * sizeof(uint32_t));
    if (ibytes > ibo_bytes_) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibytes, indices.data(), GL_DYNAMIC_DRAW);
        ibo_bytes_ = ibytes;
    } else {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ibytes, indices.data());
    }

    draw_calls_ = 0;
    uint32_t bound = 0xffffffffu;
    for (const BatchRange& range : batch_.ranges()) {
        if (range.index_count == 0) continue;
        if (range.texture != bound) {
            glBindTexture(GL_TEXTURE_2D, range.texture);
            bound = range.texture;
        }
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(range.index_count),
                       GL_UNSIGNED_INT,
                       reinterpret_cast<void*>(static_cast<uintptr_t>(range.first_index) * sizeof(uint32_t)));
        ++draw_calls_;
    }
}
