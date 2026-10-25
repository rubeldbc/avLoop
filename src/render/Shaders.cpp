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

} // namespace avloop::render::shaders
