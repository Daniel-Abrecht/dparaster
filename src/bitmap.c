#include <dparaster/bitmap.h>
#include <dparaster/texture.h>
#include <dparaster/utils.h>
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

bool bitmap_header_parse(struct bmpinfo*restrict info, const uint8_t buf[static restrict 54]){
  bool ok = true;

  if(buf[0] != 'B' || buf[1] != 'M'){
    fprintf(stderr,"warning: broken bitmap: expected header to start with 'BM'\n");
    ok = false;
  }

  const uint32_t file_size = u32le(buf+2);
  if(file_size < 52){
    fprintf(stderr,"warning: broken bitmap: specified file_size impossibly small\n");
    ok = false;
  }

  if(u32le(buf+6))
    fprintf(stderr,"warning: reserved fields set\n");

  const uint32_t data_offset = u32le(buf+10);
  if(data_offset >= file_size){
    fprintf(stderr,"warning: broken bitmap: data_offset >= file_size\n");
    ok = false;
  }

  const uint32_t bitmap_info_header_size = u32le(buf+14);
  if(bitmap_info_header_size + 14 > data_offset){
    fprintf(stderr,"warning: broken bitmap: bitmap header and image data overlap\n");
    ok = false;
  }
  if(bitmap_info_header_size < 38){
    fprintf(stderr,"warning: broken bitmap: bitmap header specified to be smaller than possible\n");
    ok = false;
  }

  const uint32_t width = u32le(buf+18);
  const uint32_t height = u32le(buf+22);

  const uint16_t plane_count = u16le(buf+26);
  const uint16_t bits_per_pixel = u16le(buf+28);
  if(plane_count != 1){
    fprintf(stderr, "warning: broken bitmap: plane count of a bitmap must be 1\n");
    ok = false;
  }

  const char bgformat[5] = { buf[30], buf[31], buf[32], buf[33], 0 };
  bool fourcc_ascii = bgformat[0] >= 0x20 && bgformat[0] < 127
                   && bgformat[1] >= 0x20 && bgformat[1] < 127
                   && bgformat[2] >= 0x20 && bgformat[2] < 127
                   && bgformat[3] >= 0x20 && bgformat[3] < 127;
  const char* fourcc = fourcc_ascii ? bgformat : 0;
  const uint32_t compression = u32le(buf+30);
  const char* format = get_format_name(compression);
  if(!format && !fourcc_ascii)
    fprintf(stderr, "warning: unknown format, if gformat code, should be ascii letters!\n");

  const uint32_t image_size = u32le(buf+34);
  const uint32_t pixels_per_meter_x = u32le(buf+38);
  const uint32_t pixels_per_meter_y = u32le(buf+42);
  const uint32_t used_indeces = u32le(buf+46);
  const uint32_t important_indeces = u32le(buf+50);

  const char* gformat = 0;
  const char* fformat = 0;
  size_t stride = 0;
  size_t image_data_size = image_size;
  if(!compression){
    stride = ((size_t)width * bits_per_pixel + 31)/32*4;
    image_data_size = stride * height;
    switch(bits_per_pixel){
      case 32: fourcc="BGRX"; fformat="bgra"    ; gformat="BGRx" ; break;
      case 24: fourcc="BR24"; fformat="bgr24"   ; gformat="BGR"  ; break;
      case 16: fourcc="BR16"; fformat="rgb565le"; gformat="BGR16"; break;
      case 15: fourcc="BGR5"; fformat="rgb555le"; gformat="BGR15"; break;
      case  8: fourcc="BGR8"; fformat="bgr8"    ; gformat="BGR8" ; break;
      default: fprintf(stderr, "Strange bits per pixel value\n"); break;
    }
  }

  if(image_size && image_size != image_data_size){
    fprintf(stderr, "warning: broken bitmap: specified image data size seams wrong!\n");
    ok = false;
  }

  if((uint64_t)image_data_size + data_offset > file_size){
    fprintf(stderr, "warning: broken bitmap: specified file size too small for all the image data!\n");
    ok = false;
  }

  info->file_size = file_size;
  info->data_offset = data_offset;
  info->bitmap_info_header_size = bitmap_info_header_size;
  info->width = width;
  info->height = height;
  info->plane_count = plane_count;
  info->bits_per_pixel = bits_per_pixel;
  info->compression = compression;
  info->image_size = image_size;
  info->pixels_per_meter_x = pixels_per_meter_x;
  info->pixels_per_meter_y = pixels_per_meter_y;
  info->used_indeces = used_indeces;
  info->important_indeces = important_indeces;

  info->image_data_size = image_data_size;
  info->stride = stride;
  info->format = format;
  info->fformat = fformat;
  info->gformat = gformat;
  info->fourcc = fourcc;

  return ok;
}

bool tl_can_handle(const struct texture* texture){
  return texture->file_length >= 54
      && ((char*)texture->file_content)[0] == 'B'
      && ((char*)texture->file_content)[1] == 'M';
}

bool tl_load(struct texture* texture){
  if(texture->file_length < 54)
    return false;
  struct bmpinfo bmp = {0};
  if(!bitmap_header_parse(&bmp, texture->file_content))
    return false;
  if((size_t)bmp.data_offset+bmp.image_size > texture->file_length)
    return false;
  if(bmp.compression){
    fprintf(stderr, "Only uncompressed 32 or 24 bpp bitmaps supported!\n");
    return false;
  }
  switch(bmp.bits_per_pixel){
    case 32: strcpy(texture->format, "BGRX"); break;
    case 24: strcpy(texture->format, "BGR" ); break;
    default: fprintf(stderr, "Only uncompressed 32 or 24 bpp bitmaps supported!\n"); break;
  }
  texture->dimension_count = 2;
  texture->img = (uint8_t*)texture->file_content + bmp.data_offset;
  texture->size[0] = bmp.width;
  texture->size[1] = bmp.height;
  texture->stride[1] = bmp.stride;
  return true;
}

IMPLEMENT_TEXTURE_LOADER

