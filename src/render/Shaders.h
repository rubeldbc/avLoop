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

} // namespace avloop::render::shaders
