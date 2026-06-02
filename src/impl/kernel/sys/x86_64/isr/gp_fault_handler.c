#include "kernel/io/serial.h"
#include <stdint.h>
#include "kernel/sys/x86_64/scheduler.h"

/*

GPF Error Code Meaning
Bits:
15              3 2       1       0
+----------------+-------+-------+-------+
|   Segment Index | TI=1? | EXT=1? | 0    |
+----------------+-------+-------+-------+

    Bits 3–15: Segment Selector Index — the index into GDT or LDT that caused the fault.

    Bit 2 (TI): Table Indicator

        0 = GDT

        1 = LDT

    Bit 1 (EXT): External Event

        0 = caused by CPU (software fault)

        1 = external (e.g., interrupt, task switch, etc.)

    Bit 0: Always 0 for GPFs
*/
void gp_fault_handler(uint64_t* stack) {
    __asm__ volatile("cli");

    log_serial(critical,"Exception: General Protection Fault!\n");

    uint64_t rip = stack[1]; // [rsp + 8] = RIP
    uint64_t error_code = stack[0]; // [rsp] = Error Code
    uint64_t cs = stack[2];
    struct task *t = get_current_task();

    log_serial(critical,
        "Current Task ID = %d, RIP = 0x%lx, code = 0x%lx," 
        " segment selector = 0x%x," 
        " table indicator = %d," 
        " table type = %s,"
        " event = %d,"
        " event type = %s,"
        " cs = %lx\n", 
        t->pid,
        rip, 
        error_code,error_code >> 3,
        (error_code >> 2) & 1,
        ((error_code >> 2) & 1) == 0 ? "GDT" : "LDT",
        error_code & 1, 
        error_code & 1 == 0 ? "Internal CPU" : "External (int or task switch)", 
        cs);


    //int tid = get_current_task_id();

    
    //TODO: need proper task switch to another
    remove_task(t->pid);
    //first_task_start();
    //while (1) { __asm__("hlt"); }
    __asm__ volatile("sti");

}
