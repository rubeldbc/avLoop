#include "render/Shaders.h"

namespace avloop::render::shaders {

const char* transformVertex() {
    return R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
uniform mat4 uMatrix;
out vec2 vUv;
void main() {
    vUv = aUv;
    gl_Position = uMatrix * vec4(aPos, 0.0, 1.0);
}
)";
}

const char* textureFragment() {
    return R"(#version 330 core
in vec2 vUv;
uniform sampler2D uTex;
uniform float uOpacity;
out vec4 fragColor;
void main() {
    fragColor = texture(uTex, vUv) * uOpacity;
}
)";
}

const char* checkerFragment() {
    return R"(#version 330 core
uniform float uCell;
uniform vec4 uDark;
uniform vec4 uLight;
out vec4 fragColor;
void main() {
    vec2 c = floor(gl_FragCoord.xy / uCell);
    fragColor = mod(c.x + c.y, 2.0) < 1.0 ? uDark : uLight;
}
)";
}

const char* outputVertex() {
    return R"(#version 330 core
void main() {
    // One triangle over the whole viewport.
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";
}

const char* outputFragment() {
    // Output row 0 is the FBO's first row: gl_FragCoord is the pixel's centre in output pixels, no flip.
    return R"(#version 330 core
uniform sampler2D uTex0;
uniform sampler2D uTex1;
uniform sampler2D uTex2;
uniform sampler2D uTex3;
uniform vec4 uPlace[4];
uniform float uWeight[4];
uniform int uCount;
uniform sampler2D uMask;
uniform bool uHasMask;
uniform bool uOpaque;
uniform vec3 uBackground;
out vec4 fragColor;
vec4 layer(sampler2D t, vec4 m, vec2 p) {
    return texture(t, vec2(m.x * p.x + m.y, m.z * p.y + m.w));
}
void main() {
    vec2 p = gl_FragCoord.xy;
    vec4 c = vec4(0.0);
    if (uCount > 0) c += uWeight[0] * layer(uTex0, uPlace[0], p);
    if (uCount > 1) c += uWeight[1] * layer(uTex1, uPlace[1], p);
    if (uCount > 2) c += uWeight[2] * layer(uTex2, uPlace[2], p);
    if (uCount > 3) c += uWeight[3] * layer(uTex3, uPlace[3], p);
    if (uHasMask) c *= texelFetch(uMask, ivec2(p), 0).r;
    if (uOpaque) c = vec4(c.rgb + uBackground * (1.0 - c.a), 1.0);
    fragColor = c;
}
)";
}

} // namespace avloop::render::shaders
