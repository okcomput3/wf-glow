#pragma once

namespace wf {
namespace glow_decoration {

// Vertex shader - simple passthrough (vertices in NDC)
static const char* glow_vertex_shader = R"glsl(
#version 300 es
precision highp float;

layout(location = 0) in vec2 a_position;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)glsl";

// Fragment shader - same as original working version
static const char* glow_fragment_shader = R"glsl(
#version 300 es
precision highp float;

uniform vec2 u_resolution;
uniform vec4 u_border_box;     // x, y, width, height of the window
uniform vec4 u_glow_color;
uniform vec4 u_glow_color_2;
uniform float u_glow_radius;
uniform float u_glow_intensity;
uniform float u_border_width;
uniform float u_time;
uniform int u_enable_gradient;
uniform float u_gradient_angle;
uniform float u_corner_radius;

out vec4 fragColor;

float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

vec2 rotate2D(vec2 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2(p.x * c - p.y * s, p.x * s + p.y * c);
}

void main() {
    // Use gl_FragCoord.y directly (no inversion)
    vec2 uv = gl_FragCoord.xy;
    vec2 center = u_border_box.xy + u_border_box.zw * 0.5;
    vec2 halfSize = u_border_box.zw * 0.5;
    vec2 p = uv - center;
    
    float cornerR = min(u_corner_radius, min(halfSize.x, halfSize.y));
    float dist = sdRoundedBox(p, halfSize, cornerR);
    
    float innerEdge = -u_border_width;
    float outerEdge = 0.0;
    float glowEnd = u_glow_radius;
    
    vec4 glowColor = u_glow_color;
    
    if (u_enable_gradient == 1) {
        float angle = radians(u_gradient_angle);
        vec2 rotatedP = rotate2D(p / halfSize, angle);
        float gradientPos = rotatedP.x * 0.5 + 0.5;
        gradientPos += sin(u_time * 0.5 + rotatedP.y * 2.0) * 0.1;
        gradientPos = clamp(gradientPos, 0.0, 1.0);
        glowColor = mix(u_glow_color, u_glow_color_2, gradientPos);
    }
    
    float pulse = 1.0 + sin(u_time * 2.0) * 0.05;
    float glowFactor = 0.0;
    
    if (dist > innerEdge && dist < glowEnd) {
        if (dist <= outerEdge) {
            glowFactor = 1.0;
        } else {
            float glowDist = dist / u_glow_radius;
            glowFactor = exp(-glowDist * 3.0) * u_glow_intensity * pulse;
            float edgeNoise = sin(atan(p.y, p.x) * 8.0 + u_time) * 0.02;
            glowFactor += edgeNoise * glowFactor;
        }
    }
    
    if (glowFactor > 0.001) {
        float bloom = (dist <= outerEdge && dist > innerEdge) ? 1.2 : 1.0;
        vec3 finalColor = glowColor.rgb * bloom;
        float alpha = glowColor.a * glowFactor;
        fragColor = vec4(finalColor * alpha, alpha);
    } else {
        discard;
    }
}
)glsl";

} // namespace glow_decoration
} // namespace wf