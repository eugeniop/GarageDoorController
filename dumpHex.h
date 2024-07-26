#ifndef DUMPHEX_H
#define DUMPHEX_H

#include "streamPrintf.h"

void dumpHex(Stream * out, const void * data, size_t size) {
  char ascii[17];
  size_t i, j;
  ascii[16] = '\0';
  for (i = 0; i < size; ++i) {
    s_printf(out, "%02X ", ((unsigned char*)data)[i]);
    if(((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
      ascii[i % 16] = ((unsigned char*)data)[i];
    }else{
      ascii[i % 16] = '.';
    }
    if((i+1) % 8 == 0 || i+1 == size){
      out->print(" ");
      if((i+1) % 16 == 0) {
        out->print("|  ");
        out->print(ascii);
        out->print(" \r\n");
      }else if(i+1 == size){
        ascii[(i+1) % 16] = '\0';
        if((i+1) % 16 <= 8){
          out->print(" ");
        }
        for (j = (i+1) % 16; j < 16; ++j) {
          out->print("   ");
        }
        out->print("|  ");
        out->print(ascii);
        out->print(" \r\n");
      }
    }
  }
}

#endif // DUMPHEX_H
