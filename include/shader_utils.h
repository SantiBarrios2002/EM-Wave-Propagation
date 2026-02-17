#pragma once

#include <GL/glew.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

namespace shader {

inline std::string loadSource(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << path << "\n";
        exit(EXIT_FAILURE);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline GLuint compile(const char* path, GLenum type) {
    std::string src = loadSource(path);
    const char* srcPtr = src.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "Shader compilation error (" << path << "):\n"
                  << log.data() << "\n";
        exit(EXIT_FAILURE);
    }
    return shader;
}

inline GLuint createProgram(const char* vertPath, const char* fragPath) {
    GLuint vs = compile(vertPath, GL_VERTEX_SHADER);
    GLuint fs = compile(fragPath, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        std::cerr << "Shader link error:\n" << log.data() << "\n";
        exit(EXIT_FAILURE);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

inline GLuint createComputeProgram(const char* compPath) {
    GLuint cs = compile(compPath, GL_COMPUTE_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, cs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        std::cerr << "Compute program link error:\n" << log.data() << "\n";
        exit(EXIT_FAILURE);
    }

    glDeleteShader(cs);
    return program;
}

} // namespace shader
