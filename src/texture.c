#define _DEFAULT_SOURCE
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dparaster/texture.h>

static const struct texture_loader* loader_list;

struct texture* texture_load(const char* file){
  int fd = open(file, O_RDONLY);
  if(!fd)
    goto error;
  struct stat sb;
  fstat(fd, &sb);
  void* memory = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if(memory == MAP_FAILED)
    goto error_after_open;
  struct texture* texture = malloc(sizeof(struct texture));
  if(!texture)
    goto error_after_mmap;
  const struct texture_loader* loader;
  for(loader=loader_list; loader; loader=loader->next){
    memset(texture, 0, sizeof(*texture));
    texture->impl = loader;
    texture->file_length = sb.st_size;
    texture->file_content = memory;
    if(loader->can_handle && !loader->can_handle(texture))
      continue;
    if(loader->load(texture))
      break;
  }
  if(!loader)
    goto error_after_alloc;
  if(memory > texture->img || (uint8_t*)memory+sb.st_size <= (uint8_t*)texture->img){
    munmap((void*)texture->file_content, texture->file_length);
    texture->file_content = 0;
    texture->file_length = 0;
  }
  return texture;
error_after_alloc:
  free(texture);
error_after_mmap:
  munmap(memory, sb.st_size);
error_after_open:
  close(fd);
error:
  return 0;
}

void texture_free(struct texture* texture){
  if(texture->impl->free){
    texture->impl->free(texture);
  }else if(texture->file_content > texture->img || (uint8_t*)(texture->file_content)+texture->file_length <= (uint8_t*)texture->img){
    free((void*)texture->img);
  }
  if(texture->file_content){
    munmap((void*)texture->file_content, texture->file_length);
    texture->file_content = 0;
    texture->file_length = 0;
  }
  free(texture);
}

void texture_texel_get_raw(const struct texture* texture, uint16_t result[], long long coord[], enum texture_lookup_mode tlm[]){
  size_t offset = 0;
  {
    size_t last_stride = strnlen(texture->format, 4);
    for(unsigned i=0; i<texture->dimension_count; i++){
      long long c = coord[i];
      switch(tlm[i]){
        case TL_REPEAT: {
          c = c % texture->size[i];
          if(c < 0)
            c += texture->size[i];
        } break;
        case TL_CLAMP: {
          if(c < 0) c = 0;
          if((size_t)c >= texture->size[i])
            c = texture->size[i]-1;
        } break;
      }
      if(texture->flip[i])
        c = texture->size[i]-1 - c;
      offset += last_stride * c;
      size_t stride = texture->stride[i];
      if(!stride)
        stride = last_stride * texture->size[i];
      last_stride = stride;
    }
  }
  for(unsigned i=0,j=0,n=4; i<n && texture->format[i]; i++,j++){
    if(texture->format[i] == 'X')
      continue;
    result[j] = ((uint8_t*)texture->img)[offset+i] * 0x101;
  }
}

Vector texture_texel_get(const struct texture* texture, long long coord[], enum texture_lookup_mode tlm[]){
  uint16_t channel[4] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF};
  texture_texel_get_raw(texture, channel, coord, tlm);
  Vector ret = {{1,1,1,1}};
  for(unsigned i=0,j=0,n=4; i<n; i++,j++)
  switch(texture->format[i]){
    case 'X': j--; break;
    case 'R': ret.data[0] = (float)channel[j] / 0xFFFFu; break;
    case 'G': ret.data[1] = (float)channel[j] / 0xFFFFu; break;
    case 'B': ret.data[2] = (float)channel[j] / 0xFFFFu; break;
    case 'A': ret.data[3] = (float)channel[j] / 0xFFFFu; break;
    case 0: i=-2; break;
  }
  return ret;
}

Vector texture_lookup(const struct texture* texture, float coord[], enum texture_lookup_mode tlm[]){
  long long texcoord[texture->dimension_count];
  for(size_t i=0,n=texture->dimension_count; i<n; i++)
    texcoord[i] = texture->size[i] * coord[i]; // Note: Discarding fraction, nearest approach
  return texture_texel_get(texture, texcoord, tlm);
}

void texture_loader_register(struct texture_loader* impl){
  impl->next = loader_list;
  loader_list = impl;
}
