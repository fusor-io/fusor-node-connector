#include "Utils.h"

unsigned long diff(unsigned long from, unsigned long to)
{
    return to >= from ? to - from : (ULONG_MAX - from) + to + 1;
}