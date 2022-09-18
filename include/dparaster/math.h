#ifndef DPARASTER_MATH_H
#define DPARASTER_MATH_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Data structures

typedef struct Vector {
  float data[3];
} Vector;

typedef struct Matrix {
  Vector axis[3];
} Matrix;

typedef struct Triangle {
  Vector vertex[3];
} Triangle;

// Constants

static const Matrix indentity_matrix = {{
  {{1,0,0}},
  {{0,1,0}},
  {{0,0,1}},
}};

// Functions

static inline Vector vmulf (Vector v, double f){ return (Vector){{  v.data[0] * f         , v.data[1] * f         , v.data[2] * f          }}; }
static inline Vector vmul  (Vector a, Vector b){ return (Vector){{  a.data[0] * b.data[0] , a.data[1] * b.data[1] , a.data[2] * b.data[2]  }}; }
static inline Vector vdivf (Vector v, double f){ return (Vector){{  v.data[0] / f         , v.data[1] / f         , v.data[2] / f          }}; }
static inline Vector vdiv  (Vector a, Vector b){ return (Vector){{  a.data[0] / b.data[0] , a.data[1] / b.data[1] , a.data[2] / b.data[2]  }}; }
static inline Vector vaddf (Vector v, double f){ return (Vector){{  v.data[0] + f         , v.data[1] + f         , v.data[2] + f          }}; }
static inline Vector vadd  (Vector a, Vector b){ return (Vector){{  a.data[0] + b.data[0] , a.data[1] + b.data[1] , a.data[2] + b.data[2]  }}; }
static inline Vector vsubf (Vector v, double f){ return (Vector){{  v.data[0] - f         , v.data[1] - f         , v.data[2] - f          }}; }
static inline Vector vsub  (Vector a, Vector b){ return (Vector){{  a.data[0] - b.data[0] , a.data[1] - b.data[1] , a.data[2] - b.data[2]  }}; }
static inline double vdot  (Vector a, Vector b){ return             a.data[0] * b.data[0] + a.data[1] * b.data[1] + a.data[2] * b.data[2]; }
static inline Vector vmaxf (Vector v, double f){ return (Vector){{  fmax(v.data[0],f)     , fmax(v.data[0],f)     , fmax(v.data[0],f)      }}; }
static inline Vector vmaxv (Vector a, Vector b){ return (Vector){{  fmax(a.data[0],b.data[0]), fmax(a.data[1],b.data[1]), fmax(a.data[2],b.data[2]) }}; }
static inline Vector vneg  (Vector v){ return (Vector){{  -v.data[0], -v.data[1], -v.data[2] }}; }
static inline Vector vnormalize(Vector v){   return vdivf(v, sqrt(vdot(v,v)) ); }

static inline Vector mmulv(Matrix m, Vector v){
  return (Vector){{
    m.axis[0].data[0]*v.data[0] + m.axis[1].data[0]*v.data[1] + m.axis[2].data[0]*v.data[2],
    m.axis[0].data[1]*v.data[0] + m.axis[1].data[1]*v.data[1] + m.axis[2].data[1]*v.data[2],
    m.axis[0].data[2]*v.data[0] + m.axis[1].data[2]*v.data[1] + m.axis[2].data[2]*v.data[2],
  }};
}

static inline Vector vcross(Vector a, Vector b){
  return (Vector){{
    a.data[1]*b.data[2] - a.data[2]*b.data[1],
    a.data[2]*b.data[0] - a.data[0]*b.data[2],
    a.data[0]*b.data[1] - a.data[1]*b.data[0],
  }};
}

static inline Vector vinterpolate(Vector a, Vector b, double t){
  return (Vector){{
    a.data[0]*(1.-t) + b.data[0]*t,
    a.data[1]*(1.-t) + b.data[1]*t,
    a.data[2]*(1.-t) + b.data[2]*t,
  }};
}

static inline Vector bcoords_interpolate(Vector v[3], Vector bcoord){
  return (Vector){{
    v[0].data[0]*bcoord.data[0] + v[1].data[0]*bcoord.data[1] + v[2].data[0]*bcoord.data[2],
    v[0].data[1]*bcoord.data[0] + v[1].data[1]*bcoord.data[1] + v[2].data[1]*bcoord.data[2],
    v[0].data[2]*bcoord.data[0] + v[1].data[2]*bcoord.data[1] + v[2].data[2]*bcoord.data[2],
  }};
}

static inline Matrix rotateX(double a){
  a = a / 180 * M_PI;
  return (Matrix){{
    {{ 1,      0,      0 }},
    {{ 0,  cos(a), sin(a) }},
    {{ 0, -sin(a), cos(a) }},
  }};
}

static inline Matrix rotateY(double a){
  a = a / 180 * M_PI;
  return (Matrix){{
    {{  cos(a), 0, -sin(a) }},
    {{       0, 1,      0 }},
    {{  sin(a), 0, cos(a) }},
  }};
}

static inline Matrix rotateZ(double a){
  a = a / 180 * M_PI;
  return (Matrix){{
    {{  cos(a), sin(a), 0}},
    {{ -sin(a), cos(a), 0}},
    {{       0,      0, 1}},
  }};
}

static inline Matrix scale3d(Vector v){
  return (Matrix){{
    {{ v.data[0], 0, 0 }},
    {{ 0, v.data[1], 0 }},
    {{ 0, 0, v.data[2] }},
  }};
}

static inline Matrix scale(double f){
  return (Matrix){{
    {{ f, 0, 0 }},
    {{ 0, f, 0 }},
    {{ 0, 0, f }},
  }};
}

static inline Matrix mmulm(Matrix a, Matrix b){
  return (Matrix){{
    {{
      a.axis[0].data[0]*b.axis[0].data[0] + a.axis[1].data[0]*b.axis[0].data[1] + a.axis[2].data[0]*b.axis[0].data[2],
      a.axis[0].data[1]*b.axis[0].data[0] + a.axis[1].data[1]*b.axis[0].data[1] + a.axis[2].data[1]*b.axis[0].data[2],
      a.axis[0].data[2]*b.axis[0].data[0] + a.axis[1].data[2]*b.axis[0].data[1] + a.axis[2].data[2]*b.axis[0].data[2],
    }},{{
      a.axis[0].data[0]*b.axis[1].data[0] + a.axis[1].data[0]*b.axis[1].data[1] + a.axis[2].data[0]*b.axis[1].data[2],
      a.axis[0].data[1]*b.axis[1].data[0] + a.axis[1].data[1]*b.axis[1].data[1] + a.axis[2].data[1]*b.axis[1].data[2],
      a.axis[0].data[2]*b.axis[1].data[0] + a.axis[1].data[2]*b.axis[1].data[1] + a.axis[2].data[2]*b.axis[1].data[2],
    }},{{
      a.axis[0].data[0]*b.axis[2].data[0] + a.axis[1].data[0]*b.axis[2].data[1] + a.axis[2].data[0]*b.axis[2].data[2],
      a.axis[0].data[1]*b.axis[2].data[0] + a.axis[1].data[1]*b.axis[2].data[1] + a.axis[2].data[1]*b.axis[2].data[2],
      a.axis[0].data[2]*b.axis[2].data[0] + a.axis[1].data[2]*b.axis[2].data[1] + a.axis[2].data[2]*b.axis[2].data[2],
    }},
  }};
}

#endif
