#pragma once
void hexprint_serial(unsigned long n);
void strprint_serial(char *str);
void write_serial(char c);
void printf_serial(const char* fmt, ...) ;


typedef enum loglevel{
    critical = 0,
    error = 1,
    warn = 2,
    info = 3,
    debug = 4
} loglevel;


void log_serial(loglevel l, char* fmt, ...);
