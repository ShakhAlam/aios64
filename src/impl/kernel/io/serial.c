#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include "kernel/io/serial.h"

void write_serial(char c) {
    __asm__ volatile (
        "movb %0, %%al \n\t"  // Load character into AL
        "movw $0x3F8, %%dx \n\t"  // Load COM1 port address into DX
        "outb %%al, %%dx \n\t"  // Output character to COM1
        :
        : "r"(c)  // Input operand: character to print
        : "al", "dx"  // Clobbered registers
    );
}

void strprint_serial(char *str){
    while(*str++){
        write_serial(*str);
    }
}

void hexprint_serial(unsigned long n){
    for(int x=sizeof(n)-1; x >= 0; x--){
        char ch = (char)(((n >> (x*4)) & 0xf));
        ch = (ch > 10) ? ch + 55 : ch + 48;
        write_serial((char)ch);
    }
    write_serial((char)'\n');
}

static void print_char(char c) {
    write_serial(c);
}

static void print_string(const char* s) {
    while (*s) write_serial(*s++);
}

static void print_unsigned(uint64_t value, int base, bool uppercase) {
    char buf[32];
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;

    if (value == 0) {
        write_serial('0');
        return;
    }

    while (value && i < 31) {
        buf[i++] = digits[value % base];
        value /= base;
    }

    while (i--) write_serial(buf[i]);
}

static void print_signed(int value) {
    if (value < 0) {
        write_serial('-');
        print_unsigned(-value, 10, false);
    } else {
        print_unsigned(value, 10, false);
    }
}

static void print_float(double value) {
    if (value < 0) {
        write_serial('-');
        value = -value;
    }

    int int_part = (int)value;
    double frac_part = value - int_part;

    print_unsigned(int_part, 10, false);
    write_serial('.');
    for (int i = 0; i < 6; i++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        write_serial('0' + digit);
        frac_part -= digit;
    }
}

void printf_serial(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            switch (*fmt) {
                case 's': print_string(va_arg(args, const char*)); break;
                case 'c': write_serial((char)va_arg(args, int)); break;
                case 'd': print_signed(va_arg(args, int)); break;
                case 'f': print_float(va_arg(args, double)); break;
                case 'x': print_unsigned(va_arg(args, unsigned int), 16, false); break;
                case 'u': print_unsigned(va_arg(args, unsigned long), 10, false); break;
                case 'l':
                    if (*(fmt + 1) == 'u') {
                        fmt++;
                        print_unsigned(va_arg(args, unsigned long), 10, false);
                    }
                    break;
                default:
                    write_serial('%');
                    write_serial(*fmt);
            }
        } else {
            write_serial(*fmt);
        }
        fmt++;
    }

    va_end(args);
}
