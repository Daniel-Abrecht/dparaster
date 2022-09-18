#ifndef DPARASTER_RASTERIZER_H
#define DPARASTER_RASTERIZER_H

#include <dparaster/shader.h>
#include <stdint.h>

void draw_triangle(
  const uint32_t w,
  const uint32_t h,
  uint8_t image[restrict h][w][4],
  double depth_plane[restrict h][w],
  const ShaderProgram*const restrict shader,
  const Uniform*const restrict uniform,
  Triangle triangle[]
);

// Draw the geometry
void draw(
  const uint32_t w,
  const uint32_t h,
  uint8_t image[h][w][4],
  double depth[h][w],
  const ShaderProgram*const restrict shader,
  const Uniform*const restrict uniform,
  const Geometry*const restrict geometry
);

#endif
