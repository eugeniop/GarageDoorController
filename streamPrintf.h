#ifndef STREAM_PRINTF_H
#define STREAM_PRINTF_H

#include <stdarg.h>

#define SPRINTF_BUF 500

/*
 * Helper function because not all boards implement printf in Print
 */
int s_printf(Stream * s, const char * fmt, ...){
  char buf[SPRINTF_BUF];
  int n = 0;
  va_list ap;
  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  s->write(buf);
  va_end(ap);
  return n;
}

#endif
