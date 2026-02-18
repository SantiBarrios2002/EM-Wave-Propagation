#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

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

// 3D orbital camera with arcball rotation
struct Camera3D {
    float yaw   = -90.0f;   // horizontal angle (degrees)
    float pitch  = 20.0f;   // vertical angle (degrees)
    float distance = 2.0f;  // distance from target

    glm::vec3 target = {0.0f, 0.0f, 0.0f};  // look-at point

    bool dragging = false;
    double lastX = 0.0, lastY = 0.0;

    float rotateSpeed = 0.3f;
    float zoomSpeed   = 0.1f;
    float minDist     = 0.5f;
    float maxDist     = 10.0f;
    float minPitch    = -89.0f;
    float maxPitch    = 89.0f;

    glm::vec3 getPosition() const {
        float rad_yaw   = glm::radians(yaw);
        float rad_pitch = glm::radians(pitch);
        float cp = std::cos(rad_pitch);
        return target + distance * glm::vec3(
            cp * std::cos(rad_yaw),
            std::sin(rad_pitch),
            cp * std::sin(rad_yaw)
        );
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 getProjectionMatrix(float aspect) const {
        return glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);
    }

    void processMouseButton(int button, int action, int /*mods*/, GLFWwindow* window) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_LEFT) {
            dragging = (action == GLFW_PRESS);
            if (dragging)
                glfwGetCursorPos(window, &lastX, &lastY);
        }
    }

    void processMouseMove(double x, double y) {
        if (dragging) {
            float dx = static_cast<float>(x - lastX);
            float dy = static_cast<float>(y - lastY);
            yaw   += dx * rotateSpeed;
            pitch += dy * rotateSpeed;
            pitch = std::clamp(pitch, minPitch, maxPitch);
        }
        lastX = x;
        lastY = y;
    }

    void processScroll(double /*xoffset*/, double yoffset) {
        distance *= (1.0f - static_cast<float>(yoffset) * zoomSpeed);
        distance = std::clamp(distance, minDist, maxDist);
    }

    void processKey(int key, int /*scancode*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS && key == GLFW_KEY_R) {
            yaw = -90.0f;
            pitch = 20.0f;
            distance = 2.0f;
            target = {0.0f, 0.0f, 0.0f};
        }
    }
};

inline void setupCamera3DCallbacks(GLFWwindow* window, Camera3D* camera) {
    glfwSetWindowUserPointer(window, camera);

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int btn, int action, int mods) {
        auto* cam = static_cast<Camera3D*>(glfwGetWindowUserPointer(win));
        cam->processMouseButton(btn, action, mods, win);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        auto* cam = static_cast<Camera3D*>(glfwGetWindowUserPointer(win));
        cam->processMouseMove(x, y);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* win, double xoff, double yoff) {
        auto* cam = static_cast<Camera3D*>(glfwGetWindowUserPointer(win));
        cam->processScroll(xoff, yoff);
    });
}

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
