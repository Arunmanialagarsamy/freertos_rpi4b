#ifndef UARTSTRING_H
#define UARTSTRING_H

int my_strlen(const char *str);
void my_strcpy(char *dest, const char *src);
void my_memset(void *ptr, int value, int num);
void my_strncpy(char *dest, const char *src, int n);
int my_strncmp(const char *s1, const char *s2, int n);
char *my_strncat(char *dest, const char *src, int n);
void my_sprintf(char *str, const char *format, ...);

#endif
