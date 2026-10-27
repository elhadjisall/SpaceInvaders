#include "engine/renderer.hpp"
#include "game/atlas.hpp"
#include "game/game.hpp"
#include "game/screen.hpp"
#include "game/stress.hpp"
#include "game/text.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

struct AppInput {
    int move_dir = 0;
    bool fire = false;
    bool toggle_stress = false;
    bool toggle_cells = false;
    bool quit = false;
};

void error_callback(int, const char* description) {
    std::fprintf(stderr, "glfw: %s\n", description);
}

void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (action == GLFW_REPEAT) return;
    AppInput* input = static_cast<AppInput*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) input->quit = true;
    if (key == GLFW_KEY_RIGHT) {
        if (action == GLFW_PRESS) input->move_dir += 1;
        else if (action == GLFW_RELEASE) input->move_dir -= 1;
    }
    if (key == GLFW_KEY_LEFT) {
        if (action == GLFW_PRESS) input->move_dir -= 1;
        else if (action == GLFW_RELEASE) input->move_dir += 1;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) input->fire = true;
    if (key == GLFW_KEY_F5 && action == GLFW_PRESS) input->toggle_stress = true;
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) input->toggle_cells = true;
}

void draw_cells(SpriteBatch& batch, uint32_t texture, const SpriteAtlas& atlas, const SpatialHash& hash) {
    std::vector<CellDebug> cells;
    hash.debug_cells(cells);
    const TexRect& white = atlas.white;
    for (const CellDebug& cell : cells) {
        batch.push(texture, cell.world_x, cell.world_y, cell.world_w, cell.world_h,
                   white.u0, white.v0, white.u1, white.v1, 255, 255, 255, 48);
    }
}

void draw_hud(SpriteBatch& batch, uint32_t texture, const SpriteAtlas& atlas,
              int entities, int draws, double frame_ms, int cells) {
    char line0[32];
    char line1[32];
    std::snprintf(line0, sizeof(line0), "E%d B%d", entities, draws);
    std::snprintf(line1, sizeof(line1), "%.1fMS C%d", frame_ms, cells);
    draw_text(batch, texture, atlas, line0, 130.f, static_cast<float>(kScreenH) - 14.f, 230, 230, 230);
    draw_text(batch, texture, atlas, line1, 130.f, static_cast<float>(kScreenH) - 24.f, 230, 230, 230);
}

} // namespace

int main(int argc, char** argv) {
    bool stress_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--stress") == 0) stress_mode = true;
        else if (std::strcmp(argv[i], "--help") == 0) {
            std::printf("usage: spaceinvaders [--stress]\n");
            std::printf("  left/right move, space fires, esc quits\n");
            std::printf("  F5 toggles stress mode, F1 draws occupied hash cells\n");
            return 0;
        } else {
            std::fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(kScreenW * 2, kScreenH * 2, "Space Invaders", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }

    AppInput input;
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf("Using OpenGL: %s\n", glGetString(GL_VERSION));
    std::printf("Renderer used: %s\n", glGetString(GL_RENDERER));
    std::printf("Shading Language: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::fflush(stdout);

    Renderer renderer;
    if (!renderer.init()) {
        std::fprintf(stderr, "renderer init failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    SpriteAtlas atlas = build_atlas();
    uint32_t texture = renderer.upload_rgba(atlas.width, atlas.height, atlas.rgba.data());

    Game game;
    game.init(texture, atlas);
    Stress stress;
    stress.init(texture, atlas);

    int shown_entities = 0;
    int shown_draws = 0;
    int shown_cells = 0;
    double shown_ms = 0.0;
    bool show_cells = false;
    double log_ms = 0.0;
    int log_frames = 0;

    double prev = glfwGetTime();
    while (!glfwWindowShouldClose(window) && !input.quit) {
        double now = glfwGetTime();
        double frame_ms = (now - prev) * 1000.0;
        prev = now;

        if (input.toggle_stress) {
            stress_mode = !stress_mode;
            input.toggle_stress = false;
            log_ms = 0.0;
            log_frames = 0;
        }
        if (input.toggle_cells) {
            show_cells = !show_cells;
            input.toggle_cells = false;
        }

        FrameInput frame;
        frame.move_dir = input.move_dir;
        frame.fire = input.fire;
        input.fire = false;

        if (stress_mode) stress.update();
        else game.update(frame);

        int fbw = 0;
        int fbh = 0;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        renderer.begin(kScreenW, kScreenH, fbw, fbh);
        const SpatialHash& hash = stress_mode ? stress.hash() : game.hash();
        if (show_cells) draw_cells(renderer.batch(), texture, atlas, hash);
        if (stress_mode) stress.draw(renderer.batch());
        else game.draw(renderer.batch());
        int entities = stress_mode ? stress.entity_count() : game.entity_count();
        draw_hud(renderer.batch(), texture, atlas, shown_entities, shown_draws, shown_ms, shown_cells);
        renderer.end();

        glfwSwapBuffers(window);
        glfwPollEvents();

        shown_entities = entities;
        shown_draws = renderer.draw_calls();
        shown_cells = hash.cell_count();
        shown_ms = frame_ms;

        if (stress_mode) {
            log_ms += frame_ms;
            ++log_frames;
            if (log_ms >= 1000.0 && log_frames > 0) {
                double avg = log_ms / static_cast<double>(log_frames);
                std::printf("stress entities=%d batches=%d draws=%d frame_ms=%.2f hash_cells=%d\n",
                            entities, renderer.batch_count(), renderer.draw_calls(), avg, hash.cell_count());
                std::fflush(stdout);
                log_ms = 0.0;
                log_frames = 0;
            }
        }
    }

    renderer.destroy_texture(texture);
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
