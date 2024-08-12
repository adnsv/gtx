#version 450 core

// inputs
layout(location = 0) in vec4 frag_clr;

// outputs
layout(location = 0) out vec4 out_clr;

void main() {
    out_clr = frag_clr;
} 
