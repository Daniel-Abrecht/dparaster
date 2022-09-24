#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <dparaster/utils.h>
#include <dparaster/bitmap.h>

static int nointr_read_minmax(uint8_t buf[], unsigned min, unsigned max){
  unsigned i = 0;
  while(i < min){
    ssize_t res = read(0, buf+i, max-i);
    if(res == -1 && errno == EINTR)
      continue;
    if(res == -1){
      perror("read");
      return -1;
    }
    if(!res)
      return i;
    i += res;
  }
  return i;
}

int writeall(unsigned i, const uint8_t buf[i]){
  int ret = 0;
  while(i && (ret=write(9, buf, i)) == -1 && errno == EINTR){
    if(ret == -1){
      perror("write");
      return -1;
    }
    buf += ret;
    i   -= ret;
  }
  return 0;
}

int main(int argc, char* argv[]){
  int ret = 0;

  bool dump_input = false;
  for(int i=1; i<argc; i++)
    if(!strcmp(argv[i], "--dump-input"))
      dump_input = true;

  static uint8_t buf[4096];
  int i = 0;
  if((i=nointr_read_minmax(buf, 54, 54)) == -1)
    return 1;

  if(dump_input && writeall(i, buf) == -1)
    ret = 5;

  if(i != 54){
    fprintf(stderr,"invalid bitmap: too short\n");
    return 2;
  }

  struct bmpinfo info = {0};
  if(!bitmap_header_parse(&info, buf))
    ret = 3;

  while((size_t)i < info.data_offset){
    int ret = read(0, buf, info.data_offset-i < sizeof(buf) ? info.data_offset-i : sizeof(buf));
    if(ret == -1 && errno == EINTR)
      continue;
    if(ret == -1){
      perror("read");
      ret = 4;
      break;
    }
    if(!ret)
      break;
    if(dump_input && writeall(ret, buf) == -1){
      ret = 5;
      break;
    }
    i += ret;
  }

#define FIELDS \
  X(hdr,"%"PRIu32,file_size) \
  X(hdr,"%"PRIu32,data_offset) \
  X(hdr,"%"PRIu32,bitmap_info_header_size) \
  X(hdr,"%"PRIu32,width) \
  X(hdr,"%"PRIu32,height) \
  X(hdr,"%"PRIu16,plane_count) \
  X(hdr,"%"PRIu16,bits_per_pixel) \
  X(hdr,"%"PRIu32,compression) \
  X(hdr,"%"PRIu32,image_size) \
  X(hdr,"%"PRIu32,pixels_per_meter_x) \
  X(hdr,"%"PRIu32,pixels_per_meter_y) \
  X(hdr,"%"PRIu32,used_indeces) \
  X(hdr,"%"PRIu32,important_indeces) \
  X(cmp,"%zu",image_data_size)

#define X(P,F,N) printf("%s_%s=" F "\n", #P, #N, info.N);
  FIELDS
  if(info.stride ) printf("cmp_stride=%zu\n", info.stride);
  if(info.format ) printf("cmp_format=%s\n" , info.format);
  if(info.fformat) printf("cmp_fformat=%s\n", info.fformat);
  if(info.gformat) printf("cmp_gformat=%s\n", info.gformat);
  if(info.fourcc ) printf("cmp_fourcc=%s\n" , info.fourcc);
#undef X

  return ret;
}
