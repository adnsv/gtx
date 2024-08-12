#version 450 core

// inputs
layout(location = 0) in vec2 in_pos;
layout(location = 1) in float in_thk;
layout(location = 2) in vec4 in_clr;

// output to the geometry shader
layout(location = 0) out float geom_th;
layout(location = 1) out vec4 geom_clr;

void main() {
    gl_Position = vec4(in_pos.x, in_pos.y, 0.0, 1.0);
    geom_th = max(0.25, (in_thk - 1.0) * 0.5); // half-thickness
    geom_clr = in_clr;
}
