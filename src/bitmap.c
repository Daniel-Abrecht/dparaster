#include <dparaster/bitmap.h>
#include <stdio.h>
#include <string.h>

bool bitmap_save(
  const char*restrict file,
  const uint32_t w,
  const uint32_t h,
  uint8_t image[h][w][4]
){
  FILE* nf = 0;
  FILE* of = 0;
  if(!strcmp(file, "-")){
    of = stdout;
  }else{
    of = nf = fopen(file, "wb");
    if(!nf) return false;
  }

  const size_t ims = sizeof(uint8_t[h][w][4]);
  const uint8_t header[54] = {
    'B','M', (sizeof(header)+ims),(sizeof(header)+ims)>>8,(sizeof(header)+ims)>>16,(sizeof(header)+ims)>>24, 0,0,0,0, sizeof(header),sizeof(header)>>8,sizeof(header)>>16,sizeof(header)>>24,
    40,0,0,0, w,w>>8,w>>16,w>>24, h,h>>8,h>>16,h>>24, 1,0, 32,0, 0,0,0,0, ims,ims>>8,ims>>16,ims>>24, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
  };
  fwrite(header, 1, sizeof(header), of);
  fwrite(image , 1, ims, of);

  if(nf) fclose(nf);
  return true;
}
