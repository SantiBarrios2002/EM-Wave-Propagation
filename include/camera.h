#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>

struct Camera2D {
    glm::vec2 center = {0.0f, 0.0f};  // pan offset in UV space
    float zoom = 1.0f;

    bool dragging = false;
    double lastX = 0.0, lastY = 0.0;

    float panSpeed  = 0.002f;
    float zoomSpeed = 0.1f;
    float minZoom   = 0.5f;
    float maxZoom   = 10.0f;

    void processMouseButton(int button, int action, int /*mods*/, GLFWwindow* window) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            dragging = (action == GLFW_PRESS);
            if (dragging)
                glfwGetCursorPos(window, &lastX, &lastY);
        }
    }

    void processMouseMove(double x, double y) {
        if (dragging) {
            float dx = static_cast<float>(x - lastX);
            float dy = static_cast<float>(y - lastY);
            center.x -= dx * panSpeed / zoom;
            center.y += dy * panSpeed / zoom;  // flip y
        }
        lastX = x;
        lastY = y;
    }

    void processScroll(double /*xoffset*/, double yoffset) {
        zoom *= (1.0f + static_cast<float>(yoffset) * zoomSpeed);
        zoom = std::clamp(zoom, minZoom, maxZoom);
    }

    void processKey(int key, int /*scancode*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS && key == GLFW_KEY_R) {
            center = {0.0f, 0.0f};
            zoom = 1.0f;
        }
    }
};

inline void setupCameraCallbacks(GLFWwindow* window, Camera2D* camera) {
    glfwSetWindowUserPointer(window, camera);

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int btn, int action, int mods) {
        auto* cam = static_cast<Camera2D*>(glfwGetWindowUserPointer(win));
        cam->processMouseButton(btn, action, mods, win);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        auto* cam = static_cast<Camera2D*>(glfwGetWindowUserPointer(win));
        cam->processMouseMove(x, y);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* win, double xoff, double yoff) {
        auto* cam = static_cast<Camera2D*>(glfwGetWindowUserPointer(win));
        cam->processScroll(xoff, yoff);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int scan, int action, int mods) {
        auto* cam = static_cast<Camera2D*>(glfwGetWindowUserPointer(win));
        cam->processKey(key, scan, action, mods);
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(win, GLFW_TRUE);
    });
}
