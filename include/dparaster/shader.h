#ifndef DPARASTER_SHADER_H
#define DPARASTER_SHADER_H

#include <dparaster/geometry.h>

typedef struct Uniform {
  Matrix modelview;
  Vector light;
} Uniform;

typedef void shader_triangle(const Uniform*restrict uniform, Triangle out[restrict], const Triangle in[restrict AIN_COUNT]); // Not something found in regular pipelines, but useful for per-triangle stuff
typedef void shader_vertex(const Uniform*restrict uniform, Vector out[], const Vector in[AIN_COUNT]);
typedef Vector shader_fragment(const Uniform*restrict uniform, double*restrict depth, Vector varying[restrict]);

typedef struct ShaderProgram {
  unsigned attribute_count;
  shader_triangle* triangle;
  shader_vertex*   vertex;
  shader_fragment* fragment;
} ShaderProgram;

shader_triangle shader_default_triangle;
shader_vertex   shader_default_vertex;
shader_fragment shader_default_fragment;

extern const ShaderProgram shader_default;

#endif
