#pragma once

// The GLSL 3.30 core shaders AvLoop draws with (AVADR-0001): the canvas (AV-003) and the FrameRenderer's GPU back end
// (AV-007) use the same transform shader, so the video stands in the preview and the export as on the canvas.
namespace avloop::render::shaders {

// Scene-space quad → clip space: `uMatrix`; passes the texture coordinate on.
const char* transformVertex();
// A premultiplied RGBA texture, times `uOpacity`.
const char* textureFragment();
// A checkerboard of `uCell` device pixels in `uDark` and `uLight`: where nothing is drawn (transparency).
const char* checkerFragment();
// The FrameRenderer's one pass (AVADR-0001): a triangle covering the output; for each output pixel up to four layers
// sampled at u = ax·x + bx, v = ay·y + by (`uPlace[i]` = ax, bx, ay, by; GL_LINEAR, transparent border), summed by
// `uWeight[i]`, times the mask (texel at the pixel), over `uBackground` when `uOpaque`. The CPU back end's maths.
const char* outputVertex();
const char* outputFragment();

} // namespace avloop::render::shaders
