#include "print.h"

int 
strncmp(const char* s1, const char* s2, int n) {
    //printf_gfx_term(0xFFFFFFFF,0,"in strncmp: s1: %s, s2: %s, n = %d\n", s1, s2, n);
    
    for (int i = 0; i < n; i++) {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];
        
        if (c1 != c2) {
            //while(1);
            return c1 - c2;
        }

        if (c1 == '\0') {
            break;
        }
    }

    //printf_gfx_term(0xFFFFFFFF,0,"exit strncmp: s1: %s, s2: %s n = %d\n", s1, s2,n);
    
    return 0;
}

int strlen(const char *s1){
    //printf_gfx_term(0xFFFFFFFF,0,"in strlen");
   
    int len = 0;
    while(*s1++)
        len++;
        
    return len;
}