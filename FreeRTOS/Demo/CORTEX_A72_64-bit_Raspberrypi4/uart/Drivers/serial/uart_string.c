#include "uart_string.h"
#include <stdarg.h>

int my_strlen(const char *str) {
    int length = 0;

    // Loop until the null terminator is encountered
    while (*str != '\0') {
        length++;
        str++;
    }

    return length;
}

void my_strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++))
        ;
}

void my_memset(void *ptr, int value, int num) {
    unsigned char *p = ptr;
    for (int i = 0; i < num; ++i) {
        *p++ = (unsigned char)value;
    }
}

void my_strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }

    for (; i < n; ++i) {
        dest[i] = '\0';
    }
}

int my_strncmp(const char *s1, const char *s2, int n) {
    while (n-- > 0 && (*s1 || *s2)) {
        if (*s1 != *s2)
            return (*s1 - *s2);
        if (*s1 == '\0' || *s2 == '\0')
            break;
        s1++;
        s2++;
    }
    return 0;
}


char *my_strncat(char *dest, const char *src, int n) {
    int dest_len = my_strlen(dest);
    int i;

    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[dest_len + i] = src[i];

    dest[dest_len + i] = '\0';

    return dest;
}

void my_sprintf(char *str, const char *format, ...) {
    char *p = str;
    va_list args;
    va_start(args, format);

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'c':
                    *p++ = (char)va_arg(args, int); // Cast to char
                    break;
                case 'd':
                    {
                        int num = va_arg(args, int);
                        int divisor = 1;
                        if (num < 0) {
                            *p++ = '-';
                            num = -num;
                        }
                        while (num / divisor >= 10) {
                            divisor *= 10;
                        }
                        while (divisor > 0) {
                            *p++ = '0' + num / divisor;
                            num %= divisor;
                            divisor /= 10;
                        }
                    }
                    break;
                case 's':
                    {
                        const char *s = va_arg(args, const char *);
                        while (*s != '\0') {
                            *p++ = *s++;
                        }
                    }
                    break;
                default:
                    *p++ = *format;
                    break;
            }
	 } else {
            *p++ = *format;
        }
        format++;
    }
    *p = '\0';
    va_end(args);
}

