
layout(set = 0, binding = 0) uniform mvp_block {
    mat4 mvp;
};

// input from the vertex shader
layout(lines_adjacency) in; 
layout(location = 0) in float geom_th[4];
layout(location = 1) in vec4 geom_clr[4];

// output to the fragment shader
layout(triangle_strip, max_vertices = 64) out;
layout(location = 0) out vec4 frag_clr;

void emit(vec2 pos, vec4 clr) 
{
    frag_clr = clr;
    gl_Position = mvp * vec4(pos, 0, 1);
    EmitVertex();
}

float cross2(vec2 a, vec2 b) 
{
    return a.x * b.y - b.x * a.y;
}

void fan(vec2 p, vec2 c, vec2 a, vec2 m, vec2 b, float r, vec4 clr, vec4 edg) {
    vec2 am = normalize(a + m);
    vec2 mb = normalize(m + b);

    float re = r + 1.0;
    emit(p, clr);
    emit(c + r * a, clr);
    emit(c + r * am, clr);
    emit(c + re * a, edg);
    emit(c + re * am, edg);
    EndPrimitive();
    emit(p, clr);
    emit(c + r * am, clr);
    emit(c + r * m, clr);
    emit(c + re * am, edg);
    emit(c + re * m, edg);
    EndPrimitive();
    emit(p, clr);
    emit(c + r * m, clr);
    emit(c + r * mb, clr);
    emit(c + re * m, edg);
    emit(c + re * mb, edg);
    EndPrimitive();
    emit(p, clr);
    emit(c + r * mb, clr);
    emit(c + r * b, clr);
    emit(c + re * mb, edg);
    emit(c + re * b, edg);
    EndPrimitive();
}

void main() 
{	
    vec2 p0 = gl_in[0].gl_Position.xy;
    vec2 p1 = gl_in[1].gl_Position.xy;
    vec2 p2 = gl_in[2].gl_Position.xy;
    vec2 p3 = gl_in[3].gl_Position.xy;

    vec4 c1 = geom_clr[1];
    vec4 c2 = geom_clr[2];
    vec4 c1e = vec4(c1.xyz, 0);
    vec4 c2e = vec4(c2.xyz, 0);

    float th1 = geom_th[1];
    float th2 = geom_th[2];

    vec2 d0 = normalize(p1 - p0);
    vec2 d1 = normalize(p2 - p1);
    vec2 d2 = normalize(p3 - p2);

    vec2 n0 = vec2(-d0.y, d0.x);
    vec2 n1 = vec2(-d1.y, d1.x);
    vec2 n2 = vec2(-d2.y, d2.x);

    float dp1 = dot(d0, d1);
    float dp2 = dot(d1, d2);

    float cw1 = 2.0 * step(cross2(d0, d1), 0.0) - 1.0;
    float cw2 = 2.0 * step(cross2(d1, d2), 0.0) - 1.0;

    vec2 t1a = n1;
    vec2 t2a = n1;
    vec2 t1b = n1;
    vec2 t2b = n1;

    vec2 m1 = (n0 + n1) / (1.0 + dp1);
    vec2 m2 = (n1 + n2) / (1.0 + dp2);

    if (p0 == p1) {
        // start
        fan(p1, p1, n1, -d1, -n1, th1, c1, c1e);
    }
    else if (dp1 < -0.25) {
        // overlap joint
        fan(p1, p1, n0 * cw1, normalize(d0-d1), n1 * cw1, th1, c1, c1e);  
    } 
    else if (dp1 > 0.85) {
        // miter
        t1a = m1;
        t1b = t1a;
    } else {
        // rounded joint
        fan(p1 - m1 * th1 * cw1, p1, n0 * cw1, normalize(n0 + n1) * cw1,  n1 * cw1, th1, c1, c1e);
        t1a = mix(t1a, m1, float(cw1 < 0.0));
        t1b = mix(t1b, m1, float(cw1 >= 0.0));
    }

    if (p2==p3) {
        // end
        fan(p2, p2, -n1, d1, n1, th2, c2, c2e);
    }
    else if (dp2 >= -0.25) {
        // before rounded joint   
        t2a = mix(t2a, m2, float(cw2 < 0.0 || dp2 > 0.85));
        t2b = mix(t2b, m2, float(cw2 >= 0.0 || dp2 > 0.85));
    }
    emit(p1 + t1a * th1, c1);
    emit(p1 - t1b * th1, c1);
    emit(p2 + t2a * th2, c2);
    emit(p2 - t2b * th2, c2);
    EndPrimitive(); 

    // aa edge
    emit(p1 + t1a * (th1 + 1.0), c1e);
    emit(p1 + t1a * th1, c1);
    emit(p2 + t2a * (th2 + 1.0), c2e);
    emit(p2 + t2a * th2, c2);
    EndPrimitive(); 

    emit(p1 - t1b * th1, c1);
    emit(p1 - t1b * (th1 + 1.0), c1e);
    emit(p2 - t2b * th2, c2);
    emit(p2 - t2b * (th2 + 1.0), c2e);
    EndPrimitive(); 
} 