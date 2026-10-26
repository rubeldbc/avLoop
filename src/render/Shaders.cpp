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
    // The canvas's video with the clip effects (D-017) as EffectsPass makes them: blur and sharpen as Gaussians of
    // 7 × 7 taps (sharpen's soft picture the blur's σ and its own together), the colour by a 3D table of
    // ClipEffects::colour, then posterize, the vignette and the opacity. A close look; the Preview shows the exact one.
    return R"(#version 330 core
in vec2 vUv;
uniform sampler2D uTex;
uniform sampler3D uLut;
uniform float uOpacity;
uniform bool uEffects;
uniform bool uColour;
uniform float uLutSize;
uniform int uLevels;
uniform float uVignette;
uniform float uVignetteInner;
uniform vec2 uTexel;
uniform float uBlur;
uniform float uSoft;
uniform float uSharpen;
out vec4 fragColor;
vec4 gauss(vec2 uv, float sigma) {
    if (sigma < 0.35) return texture(uTex, uv);
    float spacing = sigma * 0.75;
    float bias = max(0.0, log2(spacing));
    vec4 sum = vec4(0.0);
    float total = 0.0;
    for (int j = -3; j <= 3; ++j)
        for (int i = -3; i <= 3; ++i) {
            vec2 d = vec2(i, j) * spacing;
            float w = exp(-dot(d, d) / (2.0 * sigma * sigma));
            sum += w * texture(uTex, uv + d * uTexel, bias);
            total += w;
        }
    return sum / total;
}
void main() {
    if (!uEffects) {
        fragColor = texture(uTex, vUv) * uOpacity;
        return;
    }
    vec4 c = gauss(vUv, uBlur);
    if (uSharpen > 0.0) {
        vec4 soft = gauss(vUv, uSoft);
        c.rgb = clamp(c.rgb + uSharpen * (c.rgb - soft.rgb), vec3(0.0), vec3(c.a));
    }
    if (c.a <= 0.0) {
        fragColor = vec4(0.0);
        return;
    }
    vec3 rgb = min(c.rgb / c.a, vec3(1.0));
    if (uColour) rgb = texture(uLut, rgb * ((uLutSize - 1.0) / uLutSize) + 0.5 / uLutSize).rgb;
    if (uLevels >= 2) {
        float n = float(uLevels - 1);
        rgb = floor(clamp(rgb, 0.0, 1.0) * n + 0.5) / n;
    }
    if (uVignette != 0.0) {
        vec2 d = (vUv - 0.5) * 2.0;
        float w = smoothstep(uVignetteInner, uVignetteInner + 0.65, sqrt(dot(d, d) / 2.0));
        rgb = uVignette > 0.0 ? rgb * (1.0 - uVignette * w) : rgb + (-uVignette) * w * (1.0 - rgb);
    }
    float a = c.a * uOpacity;
    fragColor = vec4(rgb * a, a);
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
