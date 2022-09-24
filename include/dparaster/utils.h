#ifndef DPARASTER_UTILS_H
#define DPARASTER_UTILS_H

static inline uint32_t u16le(const uint8_t x[2]){
  return (uint32_t)x[0] | x[1]<<8;
}

static inline uint32_t u32le(const uint8_t x[4]){
  return (uint32_t)x[0] | x[1]<<8 | x[2]<<16 | x[3]<<24;
}

#endif
