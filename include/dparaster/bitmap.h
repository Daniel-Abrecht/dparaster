#ifndef DPARASTER_BITMAP_H
#define DPARASTER_BITMAP_H

#include <stdint.h>
#include <stdbool.h>

bool bitmap_save(
  const char*restrict file,
  const uint32_t w,
  const uint32_t h,
  uint8_t image[h][w][4]
);

#endif
