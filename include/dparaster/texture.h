#ifndef DPARASTER_TEXTURE_H
#define DPARASTER_TEXTURE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <dparaster/math.h>

struct texture {
  const struct texture_loader* impl;
  char format[5]; // RGBAX
  size_t file_length;
  const void* file_content;
  uint8_t dimension_count;
  size_t size[3];
  size_t stride[3];
  const void* img;
};

enum texture_lookup_mode {
  TL_REPEAT,
  TL_CLAMP,
};

struct texture* texture_load(const char* texture);
void texture_free(struct texture* texture);

void texture_texel_get_raw(const struct texture* texture, uint16_t result[], size_t coord[], enum texture_lookup_mode tlm[]);
Vector texture_texel_get(const struct texture* texture, size_t coord[], enum texture_lookup_mode tlm[]);
Vector texture_lookup(const struct texture* texture, float coord[], enum texture_lookup_mode tlm[]);

typedef bool texture_loader__can_handle(const struct texture* texture);
typedef bool texture_loader__load(struct texture* texture);
typedef void texture_loader__free(struct texture* texture);

struct texture_loader {
  const struct texture_loader* next;
  texture_loader__can_handle* can_handle;
  texture_loader__load* load;
  texture_loader__free* free;
};
void texture_loader_register(struct texture_loader* impl);

#define IMPLEMENT_TEXTURE_LOADER \
  __attribute__((weak)) texture_loader__can_handle tl_can_handle; \
  texture_loader__load tl_load; \
  __attribute__((weak)) texture_loader__free tl_free; \
  static struct texture_loader tl_texture_loader = { \
    .can_handle = tl_can_handle, \
    .load = tl_load, \
    .free = tl_free, \
  }; \
  __attribute__((constructor,used)) \
  static void tl_texture_loader_register(void){ \
    texture_loader_register(&tl_texture_loader); \
  }

#endif
