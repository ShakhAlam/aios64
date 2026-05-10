#include "print.h"
/* Printf and related */

// Declare your serial write function (should be implemented elsewhere)
extern void write_serial(char c);

static void print_string(const char* str) {
    while (*str) {
        write_serial(*str++);
    }
}

static void print_number(int value) {
    char buffer[12];  // Enough for 32-bit int
    int i = 0;
    if (value < 0) {
        write_serial('-');
        value = -value;
    }
    do {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    } while (value);
    while (i--) write_serial(buffer[i]);
}

static void print_hex(unsigned int value) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    int i = 0;
    do {
        buffer[i++] = hex_chars[value % 16];
        value /= 16;
    } while (value);
    write_serial('0');
    write_serial('x');
    while (i--) write_serial(buffer[i]);
}

// Simple floating-point to string conversion function (basic precision)
static void print_float(float value) {
    if (value < 0) {
        write_serial('-');
        value = -value;
    }

    int int_part = (int)value;
    float frac_part = value - int_part;

    // Print the integer part
    print_number(int_part);

    // Print the decimal point
    write_serial('.');

    // Print the fractional part (up to 6 decimal places)
    for (int i = 0; i < 6; ++i) {
        frac_part *= 10;
        int digit = (int)frac_part;
        write_serial('0' + digit);
        frac_part -= digit;
    }
}

static void print_scientific(float value) {
    // This function will print a number in scientific notation, i.e., 1.23e4
    int exponent = 0;
    if (value == 0) {
        write_serial('0');
        write_serial('.');
        write_serial('0');
        return;
    }

    // Normalize the number to be in the range [1.0, 10.0)
    while (value >= 10.0f) {
        value /= 10.0f;
        exponent++;
    }
    while (value < 1.0f) {
        value *= 10.0f;
        exponent--;
    }

    // Print the normalized value
    print_float(value);

    // Print the exponent part (e.g., "e+03")
    write_serial('e');
    if (exponent >= 0) {
        write_serial('+');
    } else {
        write_serial('-');
        exponent = -exponent;
    }
    print_number(exponent);
}

void printk(const char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char* str = __builtin_va_arg(args, const char*);
                    print_string(str);
                    break;
                }
                case 'd': {
                    int val = __builtin_va_arg(args, int);
                    print_number(val);
                    break;
                }
                case 'x': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    print_hex(val);
                    break;
                }
                case 'f': {
                    float val = __builtin_va_arg(args, double); // GCC promotes float to double in varargs
                    print_float(val);
                    break;
                }
                case 'g': {
                    float val = __builtin_va_arg(args, double); // GCC promotes float to double in varargs
                    if (val > 1.0e6 || val < 1.0e-4) {
                        print_scientific(val); // Use scientific notation if large/small
                    } else {
                        print_float(val); // Otherwise, print in fixed-point notation
                    }
                    break;
                }
                default:
                    write_serial('%');
                    write_serial(*fmt);
                    break;
            }
        } else {
            write_serial(*fmt);
        }
        fmt++;
    }

    __builtin_va_end(args);
}

/******************************************/

const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*) 0xb8000;
size_t col = 0;
size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;

void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}

void print_newline() {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }

    clear_row(NUM_COLS - 1);
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (col > NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
}

void print_str(char* str) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) str[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}
