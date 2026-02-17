#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "em_common.h"
#include "shader_utils.h"
#include "camera.h"

// ── Window ──
constexpr int WIDTH  = 1280;
constexpr int HEIGHT = 720;

// ── Grid ──
constexpr int NX = 512;
constexpr int NY = 512;
constexpr int GRID_SIZE = NX * NY;

// ── Simulation ──
constexpr int   STEPS_PER_FRAME = 4;
constexpr float SOURCE_FREQ     = 0.04f;  // normalized (wavelength ~ 25 cells)
constexpr float SOURCE_AMP      = 1.0f;

// ── Globals ──
Camera2D camera;

// ─────────────────────────────────────────────────────────────────────────────
// Engine — owns all OpenGL state, following kavan010/black_hole architecture
// ─────────────────────────────────────────────────────────────────────────────
struct Engine {
    GLFWwindow* window = nullptr;

    // Programs
    GLuint computeProgram = 0;
    GLuint renderProgram  = 0;

    // SSBOs (field data lives on the GPU)
    GLuint ezSSBO = 0;
    GLuint hxSSBO = 0;
    GLuint hySSBO = 0;

    // UBO
    GLuint simParamsUBO = 0;

    // Fullscreen quad
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    // Cached uniform locations — render program
    GLint loc_nx           = -1;
    GLint loc_ny           = -1;
    GLint loc_field_scale  = -1;
    GLint loc_view_center  = -1;
    GLint loc_view_zoom    = -1;
    GLint loc_aspect_ratio = -1;

    // Cached uniform locations — compute program
    GLint loc_updateStep = -1;

    // ── Initialisation ──────────────────────────────────────────────────────

    void init() {
        initWindow();
        initShaders();
        initBuffers();
        initQuad();
        cacheUniformLocations();
    }

    void initWindow() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialise GLFW\n";
            exit(EXIT_FAILURE);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "EM Wave - 2D FDTD", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);  // vsync

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialise GLEW\n";
            exit(EXIT_FAILURE);
        }

        std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    }

    void initShaders() {
        computeProgram = shader::createComputeProgram("shaders/maxwell.comp");
        renderProgram  = shader::createProgram("shaders/field.vert",
                                               "shaders/field.frag");
    }

    void initBuffers() {
        std::vector<float> zeros(GRID_SIZE, 0.0f);

        auto makeSSBO = [&](GLuint& ssbo, GLuint binding) {
            glGenBuffers(1, &ssbo);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         GRID_SIZE * sizeof(float),
                         zeros.data(), GL_DYNAMIC_COPY);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
        };

        makeSSBO(ezSSBO, 0);  // Ez at binding 0
        makeSSBO(hxSSBO, 1);  // Hx at binding 1
        makeSSBO(hySSBO, 2);  // Hy at binding 2

        // SimParams UBO at binding 0 (UBO and SSBO namespaces are separate)
        glGenBuffers(1, &simParamsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, simParamsUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(SimParams), nullptr,
                     GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, simParamsUBO);
    }

    void initQuad() {
        // clang-format off
        float verts[] = {
        //  pos (xy)       texcoord (uv)
            -1.f,  1.f,   0.f, 1.f,
            -1.f, -1.f,   0.f, 0.f,
             1.f, -1.f,   1.f, 0.f,

            -1.f,  1.f,   0.f, 1.f,
             1.f, -1.f,   1.f, 0.f,
             1.f,  1.f,   1.f, 1.f,
        };
        // clang-format on

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        // location 0 — position (vec2)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // location 1 — texcoord (vec2)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float),
                              (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void cacheUniformLocations() {
        loc_nx           = glGetUniformLocation(renderProgram, "nx");
        loc_ny           = glGetUniformLocation(renderProgram, "ny");
        loc_field_scale  = glGetUniformLocation(renderProgram, "field_scale");
        loc_view_center  = glGetUniformLocation(renderProgram, "view_center");
        loc_view_zoom    = glGetUniformLocation(renderProgram, "view_zoom");
        loc_aspect_ratio = glGetUniformLocation(renderProgram, "aspect_ratio");

        loc_updateStep = glGetUniformLocation(computeProgram, "updateStep");
    }

    // ── Per-frame work ──────────────────────────────────────────────────────

    void uploadSimParams(int timestep) {
        SimParams p{};
        p.nx          = NX;
        p.ny          = NY;
        p.source_x    = NX / 2;
        p.source_y    = NY / 2;
        p.dx          = em::DX;
        p.dt          = em::DT;
        p.time        = timestep * em::DT;
        p.source_freq = SOURCE_FREQ;
        p.source_amp  = SOURCE_AMP;
        p.field_scale = 1.0f;
        p.timestep    = timestep;
        p._pad0       = 0;

        glBindBuffer(GL_UNIFORM_BUFFER, simParamsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SimParams), &p);
    }

    void updateFields(int timestep) {
        glUseProgram(computeProgram);
        uploadSimParams(timestep);

        GLuint gx = (NX + 15) / 16;
        GLuint gy = (NY + 15) / 16;

        // Pass 1 — H field update
        glUniform1i(loc_updateStep, 0);
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Pass 2 — E field update + source + absorbing boundary
        glUniform1i(loc_updateStep, 1);
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void render() {
        glUseProgram(renderProgram);

        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);
        float aspect = (winH > 0) ? static_cast<float>(winW) / winH : 1.0f;

        glUniform1i(loc_nx, NX);
        glUniform1i(loc_ny, NY);
        glUniform1f(loc_field_scale, 15.0f);    // amplify for visibility
        glUniform2f(loc_view_center, camera.center.x, camera.center.y);
        glUniform1f(loc_view_zoom, camera.zoom);
        glUniform1f(loc_aspect_ratio, aspect);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // ── Cleanup ─────────────────────────────────────────────────────────────

    void cleanup() {
        glDeleteBuffers(1, &ezSSBO);
        glDeleteBuffers(1, &hxSSBO);
        glDeleteBuffers(1, &hySSBO);
        glDeleteBuffers(1, &simParamsUBO);
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        glDeleteProgram(computeProgram);
        glDeleteProgram(renderProgram);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    Engine engine;
    engine.init();

    setupCameraCallbacks(engine.window, &camera);

    int    timestep    = 0;
    double lastFPSTime = glfwGetTime();
    int    frameCount  = 0;

    while (!glfwWindowShouldClose(engine.window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Run several FDTD steps per rendered frame
        for (int i = 0; i < STEPS_PER_FRAME; ++i) {
            engine.updateFields(timestep);
            ++timestep;
        }

        engine.render();

        // FPS + timestep counter in title bar
        ++frameCount;
        double now = glfwGetTime();
        if (now - lastFPSTime >= 1.0) {
            std::string title = "EM Wave - 2D FDTD | "
                + std::to_string(frameCount) + " fps | Step "
                + std::to_string(timestep);
            glfwSetWindowTitle(engine.window, title.c_str());
            frameCount  = 0;
            lastFPSTime = now;
        }

        glfwSwapBuffers(engine.window);
        glfwPollEvents();
    }

    engine.cleanup();
    glfwDestroyWindow(engine.window);
    glfwTerminate();
    return 0;
}
