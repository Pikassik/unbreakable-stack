#include <headers/UnbreakableStackFunctions.h>

char SymbolFromXDigit(unsigned char digit) {
  if (digit < 10) {
    return digit + '0';
  } else {
    return digit - 10 + 'A';
  }
}