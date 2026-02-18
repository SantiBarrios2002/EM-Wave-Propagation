#pragma once

// Grid utilities for FDTD simulation
namespace grid {

// Convert 2D grid coordinates to linear index
inline int idx2d(int x, int y, int nx) {
    return y * nx + x;
}

// Check if coordinates are within grid bounds
inline bool inBounds(int x, int y, int nx, int ny) {
    return x >= 0 && x < nx && y >= 0 && y < ny;
}

// Convert 3D grid coordinates to linear index
inline int idx3d(int x, int y, int z, int nx, int ny) {
    return z * nx * ny + y * nx + x;
}

// Check if 3D coordinates are within grid bounds
inline bool inBounds3D(int x, int y, int z, int nx, int ny, int nz) {
    return x >= 0 && x < nx && y >= 0 && y < ny && z >= 0 && z < nz;
}

} // namespace grid
