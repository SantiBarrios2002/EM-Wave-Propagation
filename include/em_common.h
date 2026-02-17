#pragma once

#include <glm/glm.hpp>

// Normalized simulation constants
// In normalized units: c = 1, dx = 1, eps0 = 1, mu0 = 1
namespace em {

constexpr float C    = 1.0f;   // speed of light (normalized)
constexpr float DX   = 1.0f;   // spatial step (normalized)
constexpr float DT   = 0.5f;   // time step (Courant factor 0.5 for 2D stability)
constexpr float EPS0 = 1.0f;   // vacuum permittivity (normalized)
constexpr float MU0  = 1.0f;   // vacuum permeability (normalized)

// Physical constants (SI) - for annotation/reference
constexpr double C_SI    = 2.99792458e8;    // m/s
constexpr double EPS0_SI = 8.854187817e-12; // F/m
constexpr double MU0_SI  = 1.2566370614e-6; // H/m

} // namespace em

// Simulation parameters — matches GLSL std140 UBO layout.
// All members are 4-byte scalars packed sequentially.
// Total: 48 bytes (3 * 16, already std140-aligned).
struct SimParams {
    int   nx;           // grid width
    int   ny;           // grid height
    int   source_x;     // source position x
    int   source_y;     // source position y
    float dx;           // spatial step
    float dt;           // time step
    float time;         // current simulation time
    float source_freq;  // source frequency (normalized)
    float source_amp;   // source amplitude
    float field_scale;  // visual scaling factor
    int   timestep;     // current timestep number
    int   _pad0;        // padding to 48 bytes
};
