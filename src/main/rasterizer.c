#include <dparaster/model.h>
#include <dparaster/rasterizer.h>
#include <dparaster/bitmap.h>
#include <stdlib.h>
#include <stdio.h>

// program logic
struct params {
  const char* file;
  uint32_t w;
  uint32_t h;
  double ry, rx;
};

struct params parse_args(int argc, char* argv[]){
  struct params p = {
    .w = 800,
    .h = 600,
    .ry = -20,
    .rx =  25
  };
  for(int i=1; i<argc; i++){
    if(argv[i][0] == '-' && argv[i][1] != '\0'){
      if(argv[i][2] != '\0' || i+1 >= argc)
        goto usage;
      switch(argv[i][1]){
        case 'w': p.w = atoi(argv[++i]); break;
        case 'h': p.h = atoi(argv[++i]); break;
        case 'y': p.ry = atof(argv[++i]); break;
        case 'x': p.rx = atof(argv[++i]); break;
        default: goto usage;
      }
    }else{
      if(i+1 != argc)
        goto usage;
      p.file = argv[i];
    }
  }
  if(!p.file)
    goto usage;
  return p;
usage:
  fprintf(stderr, "usage: %s [-w w|-h h|-y ry|-x rx] file.bmp\n", *argv);
  exit(1);
}

int main(int argc, char* argv[]){
  int ret = 0;
  const struct params p = parse_args(argc, argv);

  // Where do we place the light?
  Vector light = {{1,-1,-1}};

  // We rotate the world
  Matrix m_view = indentity_matrix;
  m_view = mmulm(rotateY(p.ry), m_view);
  m_view = mmulm(rotateX(p.rx), m_view);

  // Initialise screen buffer
  uint8_t (*image)[p.w][4] = calloc(1,sizeof(uint8_t[p.h][p.w][4]));
  if(!image)
    return 1;
  double (*depth)[p.w] = malloc(sizeof(double[p.h][p.w]));
  if(!depth){
    free(image);
    return 1;
  }
  for(uint32_t y=0; y<p.h; y++)
    for(uint32_t x=0; x<p.w; x++)
      depth[y][x] = INFINITY;

  // Draw image
  {
    Matrix m_model = scale(0.5); // We scale down our cube
    Geometry yellow_box = geometry_with_flat_color(&box, (Vector){{1,1,0}});
    draw(p.w,p.h,image,depth, &shader_default, &(Uniform){
      .modelview = mmulm(m_view, m_model),
      .light = light, // This places the light relative to the camera
//      .light = mmulv(m_view, light), // This places it in the world (so it's rotated with it and so on
    }, &yellow_box);
  }

  if(!bitmap_save(p.file, p.w,p.h,image))
    ret = 1;

  free(depth);
  free(image);
  return ret;
}
