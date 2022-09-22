#include <dparaster/shader.h>

#define UNUSED(X) (void)(X)

enum e_attribute_out {
  AOUT_POSITION,
  AOUT_NORMAL,
  AOUT_COLOR,
  AOUT_COUNT,
};

void shader_default_triangle(const Uniform*restrict uniform, Triangle out[restrict AOUT_COUNT], const Triangle in[restrict AIN_COUNT]){ // Not something found in regular pipelines, but useful for per-triangle stuff
  UNUSED(uniform);
  Vector normal = vnormalize(vcross(
    vsub(in[AIN_POSITION].vertex[1], in[AIN_POSITION].vertex[0]),
    vsub(in[AIN_POSITION].vertex[2], in[AIN_POSITION].vertex[0])
  ));
  out[AOUT_NORMAL].vertex[0] = normal;
  out[AOUT_NORMAL].vertex[1] = normal;
  out[AOUT_NORMAL].vertex[2] = normal;
}

void shader_default_vertex(const Uniform*restrict uniform, Vector out[AOUT_COUNT], const Vector in[AIN_COUNT]){
  out[AOUT_POSITION] = mmulv(uniform->modelview, in[AIN_POSITION]);
  out[AOUT_NORMAL] = mmulv(uniform->modelview, out[AOUT_NORMAL]);
  out[AOUT_COLOR] = in[AIN_COLOR];
}

Vector shader_default_fragment(const Uniform*restrict uniform, double*restrict depth, Vector varying[restrict AOUT_COUNT]){
  UNUSED(depth);
  float ambient_strength = 0.2;
  Vector normal = vnormalize(varying[AOUT_NORMAL]);
  Vector ambient_color = vmulf(varying[AOUT_COLOR], ambient_strength);
  Vector light_direction = vnormalize(vsub(uniform->light, varying[AOUT_POSITION]));
  Vector diffuse_color = vmulf(varying[AOUT_COLOR], fmax(vdot(normal, light_direction), 0.0));
  Vector color = vadd(ambient_color, diffuse_color);
  return color;
}

const ShaderProgram shader_default = {
  .attribute_count = AOUT_COUNT,
  .triangle = shader_default_triangle,
  .vertex   = shader_default_vertex,
  .fragment = shader_default_fragment,
};
