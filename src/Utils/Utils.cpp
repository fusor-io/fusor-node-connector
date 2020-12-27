#include "Utils.h"

unsigned long diff(unsigned long from, unsigned long to)
{
    return to >= from ? to - from : (ULONG_MAX - from) + to + 1;
}

/**
 * Check how much milliseconds passed from the provided time
 * It also handles time counter overflow condition
 */
unsigned long getTimeout(unsigned long start)
{
  return diff(start, millis());
}