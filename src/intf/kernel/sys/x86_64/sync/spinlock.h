#pragma once
#include <stdint.h>

typedef uint64_t spinlock;

extern void enter_critical_section(uint64_t *lock);
extern void leave_critical_section(uint64_t *lock);
