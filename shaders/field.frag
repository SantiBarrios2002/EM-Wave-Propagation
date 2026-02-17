#version 430 core

in  vec2 TexCoord;
out vec4 FragColor;

// Ez field data (same SSBO the compute shader writes to)
layout(std430, binding = 0) readonly buffer EzBuffer {
    float Ez[];
};

uniform int   nx;
uniform int   ny;
uniform float field_scale;
uniform vec2  view_center;   // camera pan offset
uniform float view_zoom;     // camera zoom level
uniform float aspect_ratio;  // window width / height

void main() {
    // Aspect-ratio correction: keep the square grid square on a non-square window
    vec2 uv = TexCoord;
    uv.x = (uv.x - 0.5) * aspect_ratio + 0.5;

    // Camera transform (pan & zoom)
    uv = (uv - 0.5) / view_zoom + view_center + 0.5;

    // Out-of-bounds → black
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Sample field value from SSBO
    int ix = clamp(int(uv.x * float(nx)), 0, nx - 1);
    int iy = clamp(int(uv.y * float(ny)), 0, ny - 1);
    float val = Ez[iy * nx + ix] * field_scale;

    // Diverging colormap: blue ← 0 → red
    val = clamp(val, -1.0, 1.0);
    vec3 color;
    if (val >= 0.0) {
        color = mix(vec3(0.0), vec3(1.0, 0.15, 0.0), val);
    } else {
        color = mix(vec3(0.0), vec3(0.0, 0.3, 1.0), -val);
    }

    // Subtle white glow at high amplitude
    color += vec3(1.0) * pow(abs(val), 3.0) * 0.3;

    FragColor = vec4(color, 1.0);
}
