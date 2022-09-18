#ifndef DPARASTER_GEOMETRY_H
#define DPARASTER_GEOMETRY_H

#include <dparaster/math.h>

enum e_attribute_in {
  AIN_POSITION,
  AIN_COLOR,
  AIN_COUNT,
};

typedef struct Attribute {
  union {
    const Vector *vertex;
    // If there is no index, this will be the default for all vertices
    Vector vertex_default;
  };
  const unsigned (*index)[3];
} Attribute;

typedef struct Geometry {
  Attribute attribute[AIN_COUNT];
  unsigned triangle_count;
} Geometry;

static inline Geometry geometry_with_flat_color(const Geometry* pg, Vector color){
  Geometry g = *pg;
  g.attribute[AIN_COLOR] = (Attribute){ .vertex_default = color };
  return g;
}

#endif
