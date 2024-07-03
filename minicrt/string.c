#include "minicrt.h"

/* reverse: reverse string s in place */
void reverse(char s[]) {
  int c, i, j;
  for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

void itoa(int n, char *s) {
  int i, sign;
  if ((sign = n) < 0) { /* record sign */
    n = -n;             /* make n positive */
  }
    
  i = 0;
  do {                       /* generate digits in reverse order */
    s[i++] = n % 10 + '0'; /* get next digit */
  } while ((n /= 10) > 0);   /* delete it */
  
  if (sign < 0) {
    s[i++] = '-';
  }
  
  s[i] = '\0';
  reverse(s);
}

int strcmp(const char *src, const char *dest) {
  for ( ; *src == *dest; src++, dest++) {
    if (*src == '\0')
      return 0;
  }

  return *src - *dest;
}


void strcpy(char *src, char *dest) {
  while (*src++ = *dest++)
    ;
}


unsigned int strlen(const char *str) {  
  unsigned int i = 0;
  while (*str++ != '\0') {
    ++i;
  }

  return i;
}