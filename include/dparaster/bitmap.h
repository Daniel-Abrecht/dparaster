#ifndef DPARASTER_BITMAP_H
#define DPARASTER_BITMAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool bitmap_save(
  const char*restrict file,
  const uint32_t w,
  const uint32_t h,
  uint8_t image[h][w][4]
);

struct bmpinfo {
  uint32_t file_size;
  uint32_t data_offset;
  uint32_t bitmap_info_header_size;
  uint32_t width;
  uint32_t height;
  uint16_t plane_count;
  uint16_t bits_per_pixel;
  uint32_t compression;
  uint32_t image_size;
  uint32_t pixels_per_meter_x;
  uint32_t pixels_per_meter_y;
  uint32_t used_indeces;
  uint32_t important_indeces;

  size_t image_data_size;
  size_t stride;

  const char* format;
  const char* fformat;
  const char* gformat;
  const char* fourcc;
};

bool bitmap_header_parse(struct bmpinfo*restrict info, const uint8_t buf[static restrict 54]);

#define BMP_COMPRESSION \
   X(RAW      ,  0) \
   X(RLE8     ,  1) \
   X(RLE4     ,  2) \
   X(BITFIELDS,  3) \
   X(JPEG     ,  4) \
   X(PNG      ,  5) \
   X(CMYK     , 11) \
   X(CMYKRLE8 , 12) \
   X(CMYKRLE4 , 13)

enum BMP_compression {
#define X(N,C) BMP_C_ ## N = C,
BMP_COMPRESSION
#undef X
  BMP_COMPRESSION_COUNT
};

static inline const char* get_format_name(uint32_t c){
  switch(c){
#define X(N,C) case BMP_C_ ## N: return #N;
BMP_COMPRESSION
#undef X
  }
  return 0;
}
#undef BMP_COMPRESSION

#endif
