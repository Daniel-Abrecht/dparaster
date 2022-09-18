#include <dparaster/rasterizer.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef struct PolySlice {
  double y, x[2];
  Vector baryzentric[2];
} PolySlice;

static int PolySlice_cut(               // Returns the number of PolySlice sections that need to be drawn.
  PolySlice out[restrict 4],            // Saves the poly slices here
  PolySlice*const restrict a,           // Top slice to be further split & fit to the boundaries
  const PolySlice*restrict const b,     // Bottom slice to be further split & fit to the boundaries
  const double boundary[restrict 2][2], // Boundary to fit the poly slices to
  double last,                          // This was the previous slice y coordinate. We can omit slices equal (or smaller) than it, as it should have the same properties
  const double epsilon[2]               // There are calculation errors with double values. This is for compensating for that. If it's too small, sections will be missing. Too big, and ther'll be artefacts at the left / right edges. Just set it to 1/w, which is half a pixel in world coordinates.
){
  const double dy = b->y - a->y;
  const double dx[2] = { b->x[0] - a->x[0], b->x[1] - a->x[1] };
  const double sx[4] = {
    boundary[0][0]-a->x[0], boundary[0][0]-a->x[1],
    boundary[1][0]-a->x[0], boundary[1][0]-a->x[1],
  };
  // It will work even without the check for sx[] == 0. on most/all architectures,
  // but it'd be ub to rely on a division by zero, so let's be explicit about it.
  double ty[4] = {
    dx[0] ? sx[0]/dx[0] : 0., dx[1] ? sx[1]/dx[1] : 0.,
    dx[0] ? sx[2]/dx[0] : 1., dx[1] ? sx[3]/dx[1] : 1.,
  };
  // sorting network
#define SWAP(a,b) {double tmp=b; b=a; a=tmp;}
  if(ty[0]>ty[2]) SWAP(ty[0],ty[2]);
  if(ty[1]>ty[3]) SWAP(ty[1],ty[3]);
  if(ty[0]>ty[1]) SWAP(ty[0],ty[1]);
  if(ty[2]>ty[3]) SWAP(ty[2],ty[3]);
  if(ty[1]>ty[2]) SWAP(ty[1],ty[2]);
#undef SWAP

  int si = 0;
  for(int i=0; i<4; i++){
    double tY = ty[i];
    if(tY <= 0.) tY = 0.;
    if(tY >= 1.) tY = 1.;
    double y = a->y*(1.-tY) + b->y*tY;
    if(y < boundary[0][1])
      y = boundary[0][1];
    if(y > boundary[1][1])
      y = boundary[1][1];
    if(y < a->y || y > b->y || y <= last)
      continue;
    PolySlice result = { .y=y };
    if(dy){
      double t = (y-a->y)/dy;
      if(t <= 0.) t=0.;
      if(t >= 1.) t=1.;
      result.x[0] = a->x[0]*(1.-t) + b->x[0]*t;
      result.x[1] = a->x[1]*(1.-t) + b->x[1]*t;
      result.baryzentric[0] = vinterpolate(a->baryzentric[0], b->baryzentric[0], t);
      result.baryzentric[1] = vinterpolate(a->baryzentric[1], b->baryzentric[1], t);
    }else{
      if(a->x[0] < b->x[0]){
        result.x[0] = a->x[0];
        result.baryzentric[0] = a->baryzentric[0];
      }else{
        result.x[0] = b->x[0];
        result.baryzentric[0] = b->baryzentric[0];
      }
      if(a->x[1] < b->x[1]){
        result.x[1] = a->x[1];
        result.baryzentric[1] = a->baryzentric[1];
      }else{
        result.x[1] = b->x[1];
        result.baryzentric[1] = b->baryzentric[1];
      }
    }

    if(result.x[0] > boundary[1][0] && result.x[0]-epsilon[0] < boundary[1][0])
      result.x[0] = boundary[1][0];
    if(result.x[1] < boundary[0][0] && result.x[1]+epsilon[0] > boundary[0][0])
      result.x[1] = boundary[0][0];
    if(result.x[0] > result.x[1]) result.x[0] = result.x[1];
    if(result.x[1] < result.x[0]) result.x[1] = result.x[0];

    const double sx=result.x[0], ex=result.x[1];
    const double dx = ex - sx;
    const Vector sb = result.baryzentric[0];
    const Vector eb = result.baryzentric[1];
    if(sx < boundary[0][0]){
      result.x[0] = boundary[0][0];
      const double t = dx ? (boundary[0][0]-sx)/dx : 0.;
      result.baryzentric[0] = vinterpolate(sb, eb, t);
    }
    if(ex > boundary[1][0]){
      result.x[1] = boundary[1][0];
      const double t = dx ? (boundary[1][0]-sx)/dx : 1.;
      result.baryzentric[1] = vinterpolate(sb, eb, t);
    }
    if(result.x[0] > boundary[1][0] || result.x[1] < boundary[0][0])
      continue;
    last = y;
    out[si++] = result;
  }
  return si;
}

// Note: We don't draw things with z<-1, but whings with z>1 are drawn.
void draw_triangle(
  const uint32_t w,
  const uint32_t h,
  uint8_t image[restrict h][w][4],
  double depth_plane[restrict h][w],
  const ShaderProgram*const restrict shader,
  const Uniform*const restrict uniform,
  Triangle triangle[]
){
  int si = 0;
  PolySlice slice[8] = {0}; // TODO: I don't think it really ever needs all 8
  const unsigned attribute_count = shader->attribute_count;

  if( triangle->vertex[0].data[2] < -1
   && triangle->vertex[1].data[2] < -1
   && triangle->vertex[2].data[2] < -1
  ) return;

  int a=0, b=1, c=2;
  { // Split triangle into vertical trapezoid slices, so we can just render what's inside the view port, and don't have to deal with coords outside -1..1 later.
    if(triangle->vertex[a].data[1] > triangle->vertex[b].data[1]){ a=1, b=0; }
    if(triangle->vertex[a].data[1] > triangle->vertex[c].data[1]){ c=a, a=2; }
    if(triangle->vertex[b].data[1] > triangle->vertex[c].data[1]){ int t=c; c=b, b=t; }
    if(triangle->vertex[a].data[1] >  1) return;
    if(triangle->vertex[c].data[1] < -1) return;
    const double dcy = triangle->vertex[c].data[1] - triangle->vertex[a].data[1];

    PolySlice aslice = {
      .y    = triangle->vertex[a].data[1],
      .x[0] = triangle->vertex[a].data[0],
      .x[1] = triangle->vertex[a].data[0],
      .baryzentric = {
        {{1,0,0}},
        {{1,0,0}},
      },
    };

    double bct = dcy ? (triangle->vertex[b].data[1] - triangle->vertex[a].data[1]) / dcy : 0.5;
    double bx2 = bct * (triangle->vertex[c].data[0] - triangle->vertex[a].data[0]) + triangle->vertex[a].data[0];
    PolySlice bslice;
    bslice.y      = triangle->vertex[b].data[1];
    if(bx2 < triangle->vertex[b].data[0]){
      bslice.x[1] = triangle->vertex[b].data[0];
      bslice.x[0] = bx2;
      bslice.baryzentric[1] = (Vector){{0,1,0}};
      bslice.baryzentric[0] = (Vector){{1.-bct,0,bct}};
    }else{
      bslice.x[0] = triangle->vertex[b].data[0];
      bslice.x[1] = bx2;
      bslice.baryzentric[0] = (Vector){{0,1,0}};
      bslice.baryzentric[1] = (Vector){{1.-bct,0,bct}};
    }

    PolySlice cslice = {
      .y    = triangle->vertex[c].data[1],
      .x[0] = triangle->vertex[c].data[0],
      .x[1] = triangle->vertex[c].data[0],
      .baryzentric = {
        {{0,0,1}},
        {{0,0,1}},
      },
    };

    const double epsilon[2] = {1./w, 1./h};

    // Split the poly sections to the boundary
    si += PolySlice_cut(&slice[si], &aslice, &bslice, (const double[2][2]){{-1,-1},{1,1}}, si ? slice[si-1].y : -2, epsilon);
    si += PolySlice_cut(&slice[si], &bslice, &cslice, (const double[2][2]){{-1,-1},{1,1}}, si ? slice[si-1].y : -2, epsilon);
  }

  // Breseham would probably be faster, but this was simpler to figure out & I'm lazy
  for(int i=0; i<si-1; i++){
    const PolySlice*const restrict s = &slice[i];
    const PolySlice*const restrict e = &slice[i+1];
    const uint32_t sy = (s->y+1.)/2. * (h-1);
    const uint32_t ey = (e->y+1.)/2. * (h-1);
    const uint32_t sxa[2] = { (s->x[0]+1.)/2.*(w-1), (s->x[1]+1.)/2.*(w-1) };
    const uint32_t exa[2] = { (e->x[0]+1.)/2.*(w-1), (e->x[1]+1.)/2.*(w-1) };
    const uint32_t ly = (ey - sy) ?  (ey - sy) : 1;
    for(uint32_t y=sy; y<=ey; y++){
      const uint32_t iy = h-y-1;
      // FIXME: In theory, I'd need 65bit in the absolute worst case
      // And don't use double here, integer arithmetic is used to avoid blank pixels due to non-linear precision errors
      const uint32_t sx = ( (uint64_t)sxa[0]*(ly-(y-sy)) + (uint64_t)exa[0]*(y-sy) )/ly;
      const uint32_t ex = ( (uint64_t)sxa[1]*(ly-(y-sy)) + (uint64_t)exa[1]*(y-sy) )/ly;
      const uint32_t lx = (ex - sx) ? (ex - sx) : 1;
      const double ty = ((double)y-sy)/ly;
      const Vector sb = vinterpolate(s->baryzentric[0], e->baryzentric[0], ty);
      const Vector eb = vinterpolate(s->baryzentric[1], e->baryzentric[1], ty);
      for(uint32_t x=sx; x<=ex; x++){
        const double tx = ((double)x-sx)/lx;
        const Vector bcoord = vinterpolate(sb, eb, tx);
        Vector varying[attribute_count];
        for(unsigned i=0; i<attribute_count; i++)
          varying[i] = bcoords_interpolate((Vector[]){
            triangle[i].vertex[a],
            triangle[i].vertex[b],
            triangle[i].vertex[c],
          }, bcoord);
        Vector color = {0};
        double depth = varying->data[2];
        color = shader->fragment(uniform, &depth, varying);
        if(depth != depth || depth > depth_plane[y][x] || depth < -1)
          continue;
        depth_plane[y][x] = depth;
        color = vmulf(color, 0x100);
        if(color.data[0] <= 0x00) color.data[0] = 0x00;
        if(color.data[1] <= 0x00) color.data[1] = 0x00;
        if(color.data[2] <= 0x00) color.data[2] = 0x00;
        if(color.data[0] >= 0xFF) color.data[0] = 0xFF;
        if(color.data[1] >= 0xFF) color.data[1] = 0xFF;
        if(color.data[2] >= 0xFF) color.data[2] = 0xFF;
        // iy is a flipped versions of y.
        image[iy][x][2] = color.data[0];
        image[iy][x][1] = color.data[1];
        image[iy][x][0] = color.data[2];
        image[iy][x][3] = 0xFF;
      }
    }
  }
}


// Draw the geometry
void draw(
  const uint32_t w,
  const uint32_t h,
  uint8_t image[h][w][4],
  double depth[h][w],
  const ShaderProgram*const restrict shader,
  const Uniform*const restrict uniform,
  const Geometry*const restrict geometry
){
  const unsigned attribute_count = shader->attribute_count;
  for(unsigned i=0; i<geometry->triangle_count; i++){
    Triangle triangle_in[AIN_COUNT] = {0};
    for(enum e_attribute_in j=0; j<AIN_COUNT; j++){
      const Attribute attribute = geometry->attribute[j];
      if(attribute.index){
        const unsigned*restrict indeces = attribute.index[i];
        for(unsigned k=0; k<3; k++)
          triangle_in[j].vertex[k] = attribute.vertex ? attribute.vertex[indeces[k]] : (Vector){{0}};
      }else{
        for(unsigned k=0; k<3; k++)
          triangle_in[j].vertex[k] = attribute.vertex_default;
      }
    }
    Triangle triangle_out[attribute_count];
    memset(triangle_out, 0, sizeof(triangle_out));
    shader->triangle(uniform, triangle_out, triangle_in);
    for(unsigned k=0; k<3; k++){
      Vector input[AIN_COUNT];
      for(enum e_attribute_in j=0; j<AIN_COUNT; j++)
        input[j] = triangle_in[j].vertex[k];
      Vector output[attribute_count];
      for(unsigned j=0; j<attribute_count; j++)
        output[j] = triangle_out[j].vertex[k];
      shader->vertex(uniform, output, input);
      for(unsigned j=0; j<attribute_count; j++)
        triangle_out[j].vertex[k] = output[j];
    }
    draw_triangle(w,h,image,depth, shader, uniform, triangle_out);
  }
}
