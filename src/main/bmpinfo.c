#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

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

static uint32_t u16le(const uint8_t x[2]){
  return (uint32_t)x[0] | x[1]<<8;
}

static uint32_t u32le(const uint8_t x[4]){
  return (uint32_t)x[0] | x[1]<<8 | x[2]<<16 | x[3]<<24;
}

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

const char* get_format_name(uint32_t c){
  switch(c){
#define X(N,C) case BMP_C_ ## N: return #N;
BMP_COMPRESSION
#undef X
  }
  return 0;
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

  if(buf[0] != 'B' || buf[1] != 'M'){
    fprintf(stderr,"warning: broken bitmap: expected header to start with 'BM'\n");
    ret = 3;
  }

  const uint32_t file_size = u32le(buf+2);
  if(file_size < 52){
    fprintf(stderr,"warning: broken bitmap: specified file_size impossibly small\n");
    ret = 3;
  }

  if(u32le(buf+6))
    fprintf(stderr,"warning: reserved fields set\n");

  const uint32_t data_offset = u32le(buf+10);
  if(data_offset >= file_size){
    fprintf(stderr,"warning: broken bitmap: data_offset >= file_size\n");
    ret = 3;
  }

  const uint32_t bitmap_info_header_size = u32le(buf+14);
  if(bitmap_info_header_size + 14 > data_offset){
    fprintf(stderr,"warning: broken bitmap: bitmap header and image data overlap\n");
    ret = 3;
  }
  if(bitmap_info_header_size < 38){
    fprintf(stderr,"warning: broken bitmap: bitmap header specified to be smaller than possible\n");
    ret = 3;
  }

  const uint32_t width = u32le(buf+18);
  const uint32_t height = u32le(buf+22);

  const uint16_t plane_count = u16le(buf+26);
  const uint16_t bits_per_pixel = u16le(buf+28);
  if(plane_count != 1){
    fprintf(stderr, "warning: broken bitmap: plane count of a bitmap must be 1\n");
    ret = 3;
  }

  const char bgformat[5] = { buf[30], buf[31], buf[32], buf[33], 0 };
  bool gformat_ascii = bgformat[0] >= 0x20 && bgformat[0] < 127
                   && bgformat[1] >= 0x20 && bgformat[1] < 127
                   && bgformat[2] >= 0x20 && bgformat[2] < 127
                   && bgformat[3] >= 0x20 && bgformat[3] < 127;
  const char* gformat = gformat_ascii ? bgformat : 0;
  const uint32_t compression = u32le(buf+30);
  const char* format = get_format_name(compression);
  if(!format && !gformat_ascii)
    fprintf(stderr, "warning: unknown format, if gformat code, should be ascii letters!\n");

  const uint32_t image_size = u32le(buf+34);
  const uint32_t pixels_per_meter_x = u32le(buf+38);
  const uint32_t pixels_per_meter_y = u32le(buf+42);
  const uint32_t used_indeces = u32le(buf+46);
  const uint32_t important_indeces = u32le(buf+50);

  size_t stride = 0;
  size_t image_data_size = image_size;
  if(!compression){
    stride = ((size_t)width * bits_per_pixel + 31)/32*4;
    image_data_size = stride * height;
    switch(bits_per_pixel){
      case 32: gformat="BGRx"; break;
      case 24: gformat="BGR"; break;
      case 15: gformat="BGR15"; break;
      case 16: gformat="BGR16"; break;
      case  8: gformat="BGR8"; break;
      default: fprintf(stderr, "Strange bits per pixel value\n"); break;
    }
  }
  if(!gformat && gformat_ascii)
    gformat = bgformat;

  if(image_size && image_size != image_data_size){
    fprintf(stderr, "warning: broken bitmap: specified image data size seams wrong!\n");
    ret = 3;
  }

  if((uint64_t)image_data_size + data_offset > file_size){
    fprintf(stderr, "warning: broken bitmap: specified file size too small for all the image data!\n");
    ret = 3;
  }

  while((size_t)i < data_offset){
    int ret = read(0, buf, data_offset-i < sizeof(buf) ? data_offset-i : sizeof(buf));
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
  X(hdr,"%"PRIu32,plane_count) \
  X(hdr,"%"PRIu32,bits_per_pixel) \
  X(hdr,"%"PRIu32,compression) \
  X(hdr,"%"PRIu32,image_size) \
  X(hdr,"%"PRIu32,pixels_per_meter_x) \
  X(hdr,"%"PRIu32,pixels_per_meter_y) \
  X(hdr,"%"PRIu32,used_indeces) \
  X(hdr,"%"PRIu32,important_indeces) \
  X(cmp,"%zu",image_data_size)

#define X(P,F,N) printf("%s_%s=" F "\n", #P, #N, N);
  FIELDS
  if(stride) printf("cmp_stride=%zu\n", stride);
  if(gformat) printf("cmp_gformat=%s\n", gformat);
#undef X

  return ret;
}
