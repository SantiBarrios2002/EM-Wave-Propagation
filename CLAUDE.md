# CLAUDE.md — EM Wave Propagation Simulator

## Project Overview

A real-time electromagnetic wave propagation simulator built in C++ with OpenGL, following the same architecture as [kavan010/black_hole](https://github.com/kavan010/black_hole). The project visualizes Maxwell's equations in action — showing E and B field vectors, polarization states, wave interference, diffraction, reflection, and refraction — all rendered as beautiful, interactive GPU-accelerated visuals suitable for YouTube-quality content.

**Inspiration mapping:**
| Kavan's Black Hole | This Project (EM Waves) |
|---|---|
| Schwarzschild geodesic equations | Maxwell's curl equations (∇×E, ∇×B) |
| Gravitational lensing ray tracing | EM ray tracing (Snell's law, Fresnel coefficients) |
| Spacetime curvature grid | E/B field vector grid visualization |
| Accretion disk halo | Antenna radiation patterns, wavefront surfaces |
| `geodesic.comp` compute shader | `maxwell.comp` FDTD compute shader |
| `black_hole.cpp` main renderer | `em_wave.cpp` main renderer |

---

## Tech Stack (Matching Kavan's Approach)

- **Language:** C++17
- **Graphics:** OpenGL 4.3+ (for compute shaders)
- **Windowing:** GLFW3
- **GL Loading:** GLEW
- **Math:** GLM
- **Build:** CMake + vcpkg
- **Shaders:** GLSL 430 (vertex, fragment, compute)
- **Optional later:** Dear ImGui (for interactive parameter controls)

---

## Repository Structure

```
em_wave/
├── CMakeLists.txt
├── vcpkg.json
├── README.md
├── CLAUDE.md                    ← this file
│
├── shaders/
│   ├── field.vert               ← vertex shader for field visualization
│   ├── field.frag               ← fragment shader (color mapping fields)
│   ├── maxwell.comp             ← compute shader: FDTD Maxwell solver
│   ├── wave_render.vert         ← vertex shader for 3D wavefront mesh
│   ├── wave_render.frag         ← fragment shader for wavefront surface
│   ├── arrow.vert               ← instanced arrow rendering (field vectors)
│   ├── arrow.frag               ← arrow coloring by magnitude/phase
│   └── postprocess.frag         ← bloom/glow post-processing
│
├── 2D_wave.cpp                  ← standalone: 2D wave propagation (entry point)
├── 3D_wave.cpp                  ← standalone: 3D volumetric wave (entry point)
├── polarization.cpp             ← standalone: polarization visualizer (entry point)
├── diffraction.cpp              ← standalone: single/double slit diffraction (entry point)
├── antenna.cpp                  ← standalone: dipole antenna radiation pattern (entry point)
│
├── include/
│   ├── em_common.h              ← shared structs, constants (ε₀, μ₀, c)
│   ├── shader_utils.h           ← shader compilation/linking helpers
│   ├── camera.h                 ← orbital camera controller
│   └── grid.h                   ← field grid data structures
│
└── assets/
    └── colormaps/               ← 1D textures for field-to-color mapping
```

Each `.cpp` file is a **self-contained simulation mode** with its own `main()`, just like Kavan's `2D_lensing.cpp`, `black_hole.cpp`, `CPU-geodesic.cpp`, and `ray_tracing.cpp`. CMake builds each as a separate executable target.

---

## Physics Engine Design

### Core Algorithm: FDTD (Finite-Difference Time-Domain)

This is the standard numerical method for solving Maxwell's equations on a grid. It is the EM equivalent of what Kavan does with Schwarzschild geodesic integration — a step-by-step numerical solver, but run massively parallel on the GPU.

**Maxwell's curl equations (source-free, linear media):**

```
∂B/∂t = -∇ × E
∂E/∂t = (1/εμ) ∇ × B
```

**Yee grid discretization:**
- E and B field components are staggered in space by half a cell
- E and B are staggered in time by half a timestep (leapfrog)
- This naturally enforces ∇·E = 0 and ∇·B = 0

**Courant stability condition:**
```
Δt ≤ Δx / (c · √dim)
```
where `dim` = 2 or 3 depending on simulation mode.

### Data Flow (GPU Pipeline)

```
CPU (host)                              GPU (device)
───────────                             ────────────
Set simulation params ──► UBO ────────► maxwell.comp (FDTD update)
                                           │
                                           ▼
                                        SSBO: E[nx][ny][nz], B[nx][ny][nz]
                                           │
                                           ▼
                                        field.vert + field.frag (render)
                                           │
                                           ▼
                                        postprocess.frag (bloom/glow)
                                           │
                                           ▼
                                        Framebuffer → screen
```

This mirrors Kavan's pattern: `black_hole.cpp` sends UBO data → `geodesic.comp` runs the heavy physics → result is rendered via vertex/fragment shaders.

---

## Milestones & Build Order

### Phase 1 — Scaffolding & 2D Wave (`2D_wave.cpp`)
**Goal:** A propagating 2D sinusoidal EM wave on screen. Proof of life.

**Tasks:**
1. Set up CMake + vcpkg with GLEW, GLFW, GLM dependencies
2. Create window, OpenGL context, basic render loop
3. Implement 2D FDTD in `maxwell.comp`:
   - Grid: 512×512 cells
   - TM mode (Ez, Hx, Hy) — simplest 2D Maxwell
   - Soft sinusoidal point source at center
   - Absorbing boundary conditions (simple Mur ABC or CPML)
4. Render Ez field as a color-mapped fullscreen quad
   - Blue (negative) → black (zero) → red (positive)
   - Use 1D texture lookup for the colormap
5. Add basic orbital camera (pan/zoom for 2D)

**Deliverable:** Rippling 2D wave expanding from a point source. First video-worthy shot.

**Key shader: `maxwell.comp` (2D TM mode)**
```glsl
#version 430
layout(local_size_x = 16, local_size_y = 16) in;

layout(std430, binding = 0) buffer EzBuffer { float Ez[]; };
layout(std430, binding = 1) buffer HxBuffer { float Hx[]; };
layout(std430, binding = 2) buffer HyBuffer { float Hy[]; };

layout(std140, binding = 0) uniform SimParams {
    int   nx, ny;
    float dx, dt;
    float eps0, mu0;
    float source_freq;
    float time;
};

// Yee leapfrog update...
```

---

### Phase 2 — 3D Volumetric Wave (`3D_wave.cpp`)
**Goal:** Extend to full 3D Maxwell. Render as volumetric slices or isosurfaces.

**Tasks:**
1. Extend `maxwell.comp` to 3D: all 6 field components (Ex, Ey, Ez, Hx, Hy, Hz)
2. Grid: 128³ (fits comfortably in GPU memory, ~50MB for 6 float fields)
3. Implement 3D source: oscillating electric dipole at grid center
4. Render options (implement at least two):
   - **Cross-section slice:** Show a 2D cut through the 3D volume (user-selectable plane)
   - **Vector field arrows:** Instanced rendering of small arrows showing E direction/magnitude on a subsampled grid
   - **Isosurface:** Marching cubes on |E| to show wavefront shells
5. Orbital 3D camera with mouse control

**Deliverable:** Rotating 3D view of an EM wave radiating from a dipole. The "wow" shot.

---

### Phase 3 — Polarization Visualizer (`polarization.cpp`)
**Goal:** Show linear, circular, and elliptical polarization as animated 3D vector traces.

**Tasks:**
1. CPU-side parametric wave: E(z,t) = E₀ₓ cos(kz - ωt) x̂ + E₀ᵧ cos(kz - ωt + δ) ŷ
2. Render a propagation axis (z-axis) with E-field vectors drawn perpendicular
3. At each sample point along z, draw the instantaneous E vector as a colored arrow
4. Draw the "trace" (Lissajous figure) at the end as a ribbon/tube
5. Interactive controls:
   - Amplitude ratio (E₀ₓ / E₀ᵧ)
   - Phase difference (δ): 0° = linear, 90° = circular, else elliptical
   - Frequency
6. Animate: advance time, watch the field vectors rotate

**Deliverable:** The classic telecom textbook figure, but alive and interactive. Great for short-form content.

**Telecom relevance:** This is how polarization diversity works in MIMO antennas, satellite comms, and fiber optics.

---

### Phase 4 — Diffraction & Interference (`diffraction.cpp`)
**Goal:** Single slit, double slit, and aperture diffraction of EM waves.

**Tasks:**
1. Use 2D FDTD engine from Phase 1
2. Add material boundaries: PEC (perfect electric conductor) walls with gaps
3. Implement scenarios:
   - **Single slit:** plane wave hitting a wall with one gap → diffraction pattern
   - **Double slit:** Young's experiment → interference fringes
   - **Circular aperture:** Airy pattern (relevant to dish antennas, fiber coupling)
4. Add an "intensity detector" line/plane that accumulates |E|² over time → shows diffraction pattern building up
5. Toggle between real-time wave view and accumulated intensity view

**Deliverable:** Visually satisfying wave-through-slit animations. Classic physics but rendered beautifully.

**Telecom relevance:** Aperture diffraction governs antenna beamwidth, Fresnel zones, and optical fiber mode coupling.

---

### Phase 5 — Antenna Radiation Pattern (`antenna.cpp`)
**Goal:** Simulate and render the radiation pattern of basic antenna types.

**Tasks:**
1. Use 3D FDTD engine from Phase 2
2. Implement antenna source types:
   - **Hertzian dipole:** point current source → omnidirectional donut pattern
   - **Half-wave dipole:** spatially extended current → slightly focused pattern
   - **Dipole array (2-4 elements):** introduce phase delay between elements → beam steering
3. Near-field to far-field transform (or just run FDTD long enough and sample far grid boundary)
4. Render radiation pattern as:
   - 3D colored surface (r = gain in that direction)
   - Overlay with the antenna geometry at center
   - Optional: animated wavefronts expanding outward
5. Interactive: adjust element spacing, phase offset, frequency

**Deliverable:** The crown jewel for a telecom portfolio. Rotating 3D antenna patterns with live beam steering.

**Telecom relevance:** This is the foundation of 5G beamforming, radar, satellite antennas — your bread and butter.

---

## Rendering Techniques

### Color Mapping
Map field magnitude to color using a 1D texture (colormap). Good choices:
- **Diverging (blue-white-red):** for signed field values (Ez, Hx, etc.)
- **Sequential (black-magenta-yellow):** for magnitude |E|
- **Phase-mapped (HSV hue wheel):** for showing wavefront phase

### Glow / Bloom Post-Processing
Kavan's videos have a distinctive visual quality. Add a bloom pass:
1. Render scene to FBO
2. Extract bright pixels (threshold)
3. Gaussian blur the bright pixels
4. Additively blend back onto the scene

### Instanced Arrow Rendering
For vector field visualization:
- Create a single arrow mesh (cone + cylinder, ~50 triangles)
- Use instanced rendering with a per-instance buffer containing: position, direction, magnitude, color
- Scale arrow length by field magnitude
- Color by phase or magnitude

### Camera
- Orbital camera: arcball rotation, scroll-to-zoom
- Smooth interpolation for cinematic sweeps (useful for video recording)
- Keyboard presets for canonical views (front, top, side)

---

## Uniform Buffer Object (UBO) Layout

Following Kavan's pattern of passing simulation parameters via UBO:

```cpp
struct SimParams {
    // Grid
    int   nx, ny, nz;
    float dx;               // spatial step (meters)
    float dt;               // time step (seconds)

    // Physics
    float epsilon_0;        // 8.854e-12 F/m
    float mu_0;             // 1.257e-6 H/m
    float c;                // 2.998e8 m/s

    // Source
    float source_freq;      // Hz (e.g., 1e9 for 1 GHz)
    float source_amplitude;
    int   source_x, source_y, source_z;
    int   source_type;      // 0=point, 1=dipole, 2=plane wave

    // Time
    float current_time;
    int   timestep;

    // Material (for boundaries)
    float sigma;            // conductivity for lossy media

    // Rendering
    float field_scale;      // visual scaling factor
    int   render_component; // 0=Ex, 1=Ey, 2=Ez, 3=|E|, 4=Hx...
};
```

---

## CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.16)
project(em_wave LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

# Shared sources
set(COMMON_SOURCES
    include/shader_utils.h
    include/camera.h
    include/em_common.h
    include/grid.h
)

set(COMMON_LIBS
    GLEW::GLEW
    glfw
    glm::glm
    OpenGL::GL
)

# Each simulation mode is its own executable (Kavan's pattern)
add_executable(2D_wave   2D_wave.cpp   ${COMMON_SOURCES})
add_executable(3D_wave   3D_wave.cpp   ${COMMON_SOURCES})
add_executable(polarization polarization.cpp ${COMMON_SOURCES})
add_executable(diffraction  diffraction.cpp  ${COMMON_SOURCES})
add_executable(antenna      antenna.cpp      ${COMMON_SOURCES})

target_link_libraries(2D_wave      PRIVATE ${COMMON_LIBS})
target_link_libraries(3D_wave      PRIVATE ${COMMON_LIBS})
target_link_libraries(polarization PRIVATE ${COMMON_LIBS})
target_link_libraries(diffraction  PRIVATE ${COMMON_LIBS})
target_link_libraries(antenna      PRIVATE ${COMMON_LIBS})

# Copy shaders to build directory
file(COPY shaders/ DESTINATION ${CMAKE_BINARY_DIR}/shaders)
```

---

## vcpkg.json

```json
{
  "name": "em-wave",
  "version": "0.1.0",
  "dependencies": [
    "glew",
    "glfw3",
    "glm"
  ]
}
```

---

## Build Instructions

```bash
git clone https://github.com/<you>/em_wave.git
cd em_wave
vcpkg install
vcpkg integrate install
mkdir build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
./build/2D_wave      # run the 2D simulation
```

**Debian/Ubuntu alternative (no vcpkg):**
```bash
sudo apt update
sudo apt install build-essential cmake libglew-dev libglfw3-dev libglm-dev libgl1-mesa-dev
mkdir build && cmake -B build -S . && cmake --build build
```

---

## Physical Constants & Telecom Reference Values

```
ε₀ = 8.854187817e-12  F/m    (vacuum permittivity)
μ₀ = 1.2566370614e-6  H/m    (vacuum permeability)
c  = 2.99792458e8     m/s    (speed of light)

Useful telecom frequencies to simulate:
  HF radio:     3-30 MHz       λ = 10-100 m
  WiFi:         2.4 GHz        λ = 12.5 cm
  5G mmWave:    28 GHz         λ = 10.7 mm
  Fiber optic:  193 THz        λ = 1550 nm

Note: For visual simulations, use NORMALIZED units.
Set c=1, dx=1, dt=0.5 (Courant factor 0.5 in 2D).
Map "frequency" to visual wavelengths (e.g., 20-100 cells per λ).
Then annotate the video with real-world equivalents.
```

---

## Video Content Plan (YouTube Shorts / Full Videos)

Following Kavan's content strategy of building the simulation incrementally and documenting the journey:

| Video # | Title | Simulation Mode | Duration |
|---------|-------|-----------------|----------|
| 1 | "I simulated electromagnetic waves in C++" | 2D_wave | 60s short |
| 2 | "How Maxwell's equations actually look" | 3D_wave (dipole radiating) | 60s short |
| 3 | "Polarization explained visually" | polarization | 60s short |
| 4 | "Double slit experiment but it's radio waves" | diffraction | 60s short |
| 5 | "How 5G beamforming actually works" | antenna (array) | 60s short |
| 6 | "I built a full EM wave simulator — here's how" | All modes, code walkthrough | 10-15min |

---

## Implementation Notes & Gotchas

### GPU Compute Shader Limits
- `GL_MAX_COMPUTE_WORK_GROUP_COUNT`: check at runtime
- For 128³ grid: dispatch 8×8×8 work groups of 16×16×1 local size
- Total invocations: 128³ = 2M per timestep — runs in <1ms on modern GPUs

### Absorbing Boundary Conditions
Without ABC, waves reflect off the grid edges creating artifacts.
- **Phase 1 (simple):** First-order Mur ABC — 3 lines of code per boundary
- **Phase 3+ (better):** CPML (Convolutional Perfectly Matched Layer) — requires auxiliary fields but gives <-60dB reflection

### Numerical Dispersion
FDTD introduces grid dispersion (wave speed depends on propagation angle relative to grid).
- Keep **≥20 cells per wavelength** to minimize dispersion error (<1%)
- For videos, this is good enough. For engineering accuracy, you'd need higher-order schemes.

### Recording Videos
- Use OBS or similar screen recorder at 60fps
- Add slow-motion camera sweeps for cinematic effect
- Consider adding a time counter / frequency readout overlay
- Export to FFmpeg for final processing

---

## Stretch Goals (Future Phases)

- [ ] **Refraction through materials:** Add spatially-varying ε(x,y,z) → simulate prisms, lenses, dielectric waveguides
- [ ] **Waveguide modes:** Rectangular and circular waveguide mode visualization (TE/TM modes)
- [ ] **Fiber optic simulation:** Total internal reflection in a cylindrical dielectric → show LP modes
- [ ] **Phased array beamformer:** N-element array with interactive phase control → real-time beam steering
- [ ] **MIMO channel visualization:** Multiple TX/RX antennas with multipath → show spatial multiplexing
- [ ] **Urban RF propagation:** Import a simple 2D building layout, shoot rays → coverage map
- [ ] **Dear ImGui overlay:** Add real-time parameter controls (frequency slider, source type dropdown, polarization knobs)
- [ ] **Spectrum analyzer view:** Show FFT of field at a probe point alongside the spatial view
- [ ] **VR mode:** Render stereoscopic for VR headsets — walk inside the EM field

---

## References

### Code References
- [kavan010/black_hole](https://github.com/kavan010/black_hole) — Architecture template
- [LearnOpenGL.com](https://learnopengl.com/) — OpenGL fundamentals
- [LearnOpenGL: Compute Shaders](https://learnopengl.com/Guest-Articles/2022/Compute-Shaders/Introduction) — Compute shader basics

### Physics References
- Allen Taflove, *Computational Electrodynamics: The Finite-Difference Time-Domain Method*, 3rd ed. — The FDTD bible
- John David Jackson, *Classical Electrodynamics* — Maxwell's equations reference
- Dennis Sullivan, *Electromagnetic Simulation Using the FDTD Method* — Practical FDTD cookbook (shorter than Taflove)
- [MIT OpenCourseWare 6.013](https://ocw.mit.edu/courses/6-013-electromagnetics-and-applications-fall-2005/) — Electromagnetics and Applications

### Telecom-Specific
- Constantine Balanis, *Antenna Theory: Analysis and Design* — Radiation patterns, arrays, beamforming
- Simon Haykin, *Communication Systems* — Modulation, propagation, system-level context
- 3GPP TR 38.901 — 5G channel model (for realistic RF propagation parameters)
