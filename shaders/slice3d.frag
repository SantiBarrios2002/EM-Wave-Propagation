#version 430 core

in  vec2 TexCoord;
out vec4 FragColor;

// All 6 field SSBOs (same as compute shader)
layout(std430, binding = 0) readonly buffer ExBuffer { float Ex[]; };
layout(std430, binding = 1) readonly buffer EyBuffer { float Ey[]; };
layout(std430, binding = 2) readonly buffer EzBuffer { float Ez[]; };
layout(std430, binding = 3) readonly buffer HxBuffer { float Hx[]; };
layout(std430, binding = 4) readonly buffer HyBuffer { float Hy[]; };
layout(std430, binding = 5) readonly buffer HzBuffer { float Hz[]; };

uniform int   nx, ny, nz;
uniform float field_scale;
uniform int   render_component;  // 0=Ex,1=Ey,2=Ez,3=|E|,4=Hx,5=Hy,6=Hz
uniform int   slice_axis;        // 0=XY, 1=XZ, 2=YZ
uniform int   slice_index;       // position along sliced axis
uniform float aspect_ratio;

int idx(int x, int y, int z) {
    return z * nx * ny + y * nx + x;
}

float sampleField(int x, int y, int z, int comp) {
    int i = idx(x, y, z);
    if (comp == 0) return Ex[i];
    if (comp == 1) return Ey[i];
    if (comp == 2) return Ez[i];
    if (comp == 3) return sqrt(Ex[i]*Ex[i] + Ey[i]*Ey[i] + Ez[i]*Ez[i]);
    if (comp == 4) return Hx[i];
    if (comp == 5) return Hy[i];
    return Hz[i];
}

void main() {
    vec2 uv = TexCoord;

    // Determine grid coordinates based on slice axis
    int gx, gy, gz;
    int dim_u, dim_v;  // dimensions along UV axes

    if (slice_axis == 0) {
        // XY slice at z = slice_index
        dim_u = nx; dim_v = ny;
        gz = slice_index;
        float ratio = float(dim_u) / float(dim_v);
        uv.x = (uv.x - 0.5) * aspect_ratio / ratio + 0.5;
        gx = clamp(int(uv.x * float(dim_u)), 0, dim_u - 1);
        gy = clamp(int(uv.y * float(dim_v)), 0, dim_v - 1);
    } else if (slice_axis == 1) {
        // XZ slice at y = slice_index
        dim_u = nx; dim_v = nz;
        gy = slice_index;
        float ratio = float(dim_u) / float(dim_v);
        uv.x = (uv.x - 0.5) * aspect_ratio / ratio + 0.5;
        gx = clamp(int(uv.x * float(dim_u)), 0, dim_u - 1);
        gz = clamp(int(uv.y * float(dim_v)), 0, dim_v - 1);
    } else {
        // YZ slice at x = slice_index
        dim_u = ny; dim_v = nz;
        gx = slice_index;
        float ratio = float(dim_u) / float(dim_v);
        uv.x = (uv.x - 0.5) * aspect_ratio / ratio + 0.5;
        gy = clamp(int(uv.x * float(dim_u)), 0, dim_u - 1);
        gz = clamp(int(uv.y * float(dim_v)), 0, dim_v - 1);
    }

    // Out-of-bounds check
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float val = sampleField(gx, gy, gz, render_component) * field_scale;

    // For magnitude (component 3), use sequential colormap
    if (render_component == 3) {
        val = clamp(val, 0.0, 1.0);
        // Black → magenta → yellow
        vec3 color;
        if (val < 0.5) {
            color = mix(vec3(0.0), vec3(0.8, 0.0, 0.8), val * 2.0);
        } else {
            color = mix(vec3(0.8, 0.0, 0.8), vec3(1.0, 1.0, 0.2), (val - 0.5) * 2.0);
        }
        color += vec3(1.0) * pow(val, 3.0) * 0.3;
        FragColor = vec4(color, 1.0);
        return;
    }

    // Diverging colormap: blue ← 0 → red (same as 2D)
    val = clamp(val, -1.0, 1.0);
    vec3 color;
    if (val >= 0.0) {
        color = mix(vec3(0.0), vec3(1.0, 0.15, 0.0), val);
    } else {
        color = mix(vec3(0.0), vec3(0.0, 0.3, 1.0), -val);
    }
    color += vec3(1.0) * pow(abs(val), 3.0) * 0.3;

    FragColor = vec4(color, 1.0);
}
