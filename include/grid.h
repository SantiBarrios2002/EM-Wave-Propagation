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

} // namespace grid
