/* gcc -Og -S cond.c */
void cond(short a, short *p) {
  if (a && *p < a) 
    *p = a;
}