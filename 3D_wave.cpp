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
constexpr int NX = 128;
constexpr int NY = 128;
constexpr int NZ = 128;
constexpr int GRID_SIZE = NX * NY * NZ;

// ── Simulation ──
constexpr int   STEPS_PER_FRAME = 1;
constexpr float SOURCE_FREQ     = 0.06f;  // normalized (wavelength ~ 17 cells)
constexpr float SOURCE_AMP      = 1.0f;

// ── State ──
Camera3D camera;
int renderComponent = 2;  // default: Ez
int sliceAxis       = 0;  // default: XY
int sliceIndex      = NZ / 2;

const char* componentNames[] = {"Ex", "Ey", "Ez", "|E|", "Hx", "Hy", "Hz"};
const char* axisNames[]      = {"XY", "XZ", "YZ"};

int getSliceMax() {
    if (sliceAxis == 0) return NZ - 1;
    if (sliceAxis == 1) return NY - 1;
    return NX - 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Engine — owns all OpenGL state, following kavan010/black_hole architecture
// ─────────────────────────────────────────────────────────────────────────────
struct Engine {
    GLFWwindow* window = nullptr;

    // Programs
    GLuint computeProgram = 0;
    GLuint renderProgram  = 0;

    // SSBOs (6 field components on the GPU)
    GLuint ssbo[6] = {};  // Ex, Ey, Ez, Hx, Hy, Hz

    // UBO
    GLuint simParamsUBO = 0;

    // Fullscreen quad
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    // Cached uniform locations — render program
    GLint loc_nx               = -1;
    GLint loc_ny               = -1;
    GLint loc_nz               = -1;
    GLint loc_field_scale      = -1;
    GLint loc_render_component = -1;
    GLint loc_slice_axis       = -1;
    GLint loc_slice_index      = -1;
    GLint loc_aspect_ratio     = -1;

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

        window = glfwCreateWindow(WIDTH, HEIGHT, "EM Wave - 3D FDTD", nullptr, nullptr);
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
        computeProgram = shader::createComputeProgram("shaders/maxwell3d.comp");
        renderProgram  = shader::createProgram("shaders/field.vert",
                                               "shaders/slice3d.frag");
    }

    void initBuffers() {
        std::vector<float> zeros(GRID_SIZE, 0.0f);

        for (int i = 0; i < 6; ++i) {
            glGenBuffers(1, &ssbo[i]);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         GRID_SIZE * sizeof(float),
                         zeros.data(), GL_DYNAMIC_COPY);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, ssbo[i]);
        }

        // SimParams3D UBO at binding 0
        glGenBuffers(1, &simParamsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, simParamsUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(SimParams3D), nullptr,
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
        loc_nx               = glGetUniformLocation(renderProgram, "nx");
        loc_ny               = glGetUniformLocation(renderProgram, "ny");
        loc_nz               = glGetUniformLocation(renderProgram, "nz");
        loc_field_scale      = glGetUniformLocation(renderProgram, "field_scale");
        loc_render_component = glGetUniformLocation(renderProgram, "render_component");
        loc_slice_axis       = glGetUniformLocation(renderProgram, "slice_axis");
        loc_slice_index      = glGetUniformLocation(renderProgram, "slice_index");
        loc_aspect_ratio     = glGetUniformLocation(renderProgram, "aspect_ratio");

        loc_updateStep = glGetUniformLocation(computeProgram, "updateStep");
    }

    // ── Per-frame work ──────────────────────────────────────────────────────

    void uploadSimParams(int timestep) {
        SimParams3D p{};
        p.nx               = NX;
        p.ny               = NY;
        p.nz               = NZ;
        p.source_x         = NX / 2;
        p.source_y         = NY / 2;
        p.source_z         = NZ / 2;
        p.dx               = em::DX;
        p.dt               = em::DT_3D;
        p.time             = timestep * em::DT_3D;
        p.source_freq      = SOURCE_FREQ;
        p.source_amp       = SOURCE_AMP;
        p.field_scale      = 1.0f;
        p.timestep         = timestep;
        p.render_component = renderComponent;
        p.slice_axis       = sliceAxis;
        p.slice_index      = sliceIndex;

        glBindBuffer(GL_UNIFORM_BUFFER, simParamsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SimParams3D), &p);
    }

    void updateFields(int timestep) {
        glUseProgram(computeProgram);
        uploadSimParams(timestep);

        GLuint gx = (NX + 7) / 8;
        GLuint gy = (NY + 7) / 8;
        GLuint gz = (NZ + 7) / 8;

        // Pass 1 — H field update
        glUniform1i(loc_updateStep, 0);
        glDispatchCompute(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Pass 2 — E field update + source + absorbing boundary
        glUniform1i(loc_updateStep, 1);
        glDispatchCompute(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void render() {
        glUseProgram(renderProgram);

        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);
        float aspect = (winH > 0) ? static_cast<float>(winW) / winH : 1.0f;

        glUniform1i(loc_nx, NX);
        glUniform1i(loc_ny, NY);
        glUniform1i(loc_nz, NZ);
        glUniform1f(loc_field_scale, 15.0f);
        glUniform1i(loc_render_component, renderComponent);
        glUniform1i(loc_slice_axis, sliceAxis);
        glUniform1i(loc_slice_index, sliceIndex);
        glUniform1f(loc_aspect_ratio, aspect);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // ── Cleanup ─────────────────────────────────────────────────────────────

    void cleanup() {
        for (int i = 0; i < 6; ++i)
            glDeleteBuffers(1, &ssbo[i]);
        glDeleteBuffers(1, &simParamsUBO);
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        glDeleteProgram(computeProgram);
        glDeleteProgram(renderProgram);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Keyboard handler (separate from camera, handles simulation controls)
// ─────────────────────────────────────────────────────────────────────────────
void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    // Forward to camera
    camera.processKey(key, 0, action, 0);

    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        // Slice axis: 1=XY, 2=XZ, 3=YZ
        case GLFW_KEY_1:
            sliceAxis = 0;
            sliceIndex = std::min(sliceIndex, getSliceMax());
            break;
        case GLFW_KEY_2:
            sliceAxis = 1;
            sliceIndex = std::min(sliceIndex, getSliceMax());
            break;
        case GLFW_KEY_3:
            sliceAxis = 2;
            sliceIndex = std::min(sliceIndex, getSliceMax());
            break;

        // Move slice: +/- or ]/[
        case GLFW_KEY_EQUAL:      // + key
        case GLFW_KEY_RIGHT_BRACKET:
            sliceIndex = std::min(sliceIndex + 1, getSliceMax());
            break;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_LEFT_BRACKET:
            sliceIndex = std::max(sliceIndex - 1, 0);
            break;

        // Cycle field component: C
        case GLFW_KEY_C:
            renderComponent = (renderComponent + 1) % 7;
            break;

        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    Engine engine;
    engine.init();

    // Set up camera callbacks (mouse orbit, scroll zoom)
    setupCamera3DCallbacks(engine.window, &camera);

    // Override key callback to add simulation controls
    glfwSetKeyCallback(engine.window, keyCallback);

    std::cout << "\n=== Controls ===\n"
              << "  Mouse drag: orbit camera (unused in slice mode)\n"
              << "  Scroll: zoom (unused in slice mode)\n"
              << "  1/2/3: slice axis (XY/XZ/YZ)\n"
              << "  +/-  : move slice plane\n"
              << "  C    : cycle field component\n"
              << "  R    : reset camera\n"
              << "  ESC  : quit\n\n";

    int    timestep    = 0;
    double lastFPSTime = glfwGetTime();
    int    frameCount  = 0;

    while (!glfwWindowShouldClose(engine.window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (int i = 0; i < STEPS_PER_FRAME; ++i) {
            engine.updateFields(timestep);
            ++timestep;
        }

        engine.render();

        // FPS + status in title bar
        ++frameCount;
        double now = glfwGetTime();
        if (now - lastFPSTime >= 1.0) {
            std::string title = "EM Wave - 3D FDTD | "
                + std::to_string(frameCount) + " fps | Step "
                + std::to_string(timestep) + " | "
                + componentNames[renderComponent] + " "
                + axisNames[sliceAxis] + " slice="
                + std::to_string(sliceIndex);
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
