#include "Strings.hh"

#include <TString.h>

#include <cstdio>


static constexpr size_t BUFSIZE = 2048;

__attribute__((format(printf, 1, 2)))
const char* LeakStr(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char buf[BUFSIZE];
  vsnprintf(buf, BUFSIZE, fmt, ap);
  va_end(ap);

  return (new TString(buf))->Data();
}

__attribute__((format(printf, 1, 2)))
const char* TmpStr(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char buf[BUFSIZE];
  vsnprintf(buf, BUFSIZE, fmt, ap);
  va_end(ap);

  return Form("%s", buf);
}
