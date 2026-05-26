#pragma once
#include <stdint.h>

extern void switch_task(struct task *current, struct task *next);

void init_tasking();
void schedule();
struct task *create_task(void (*entry)(), int id);
int add_task(struct task* new_task);
void thread1();
void thread2();
void thread3();

void idle_task();

enum task_state {
    allocated  = 0,
    ready,
    running,
    waiting,
    done
};

struct task {
    struct task* next;      // 0x00 - Next task in the run queue
    uint64_t rsp;           // 0x08 - Stack pointer
    uint64_t rip;           // 0x10 - Instruction pointer (used by iretq)
    uint64_t rflags;        // 0x18 - Flags (used by iretq)

    uint64_t rbx;           // 0x20 - Callee-saved registers
    uint64_t rbp;           // 0x28
    uint64_t r12;           // 0x30
    uint64_t r13;           // 0x38
    uint64_t r14;           // 0x40

    // Bookkeeping fields (not touched by switch_task)
    struct task* prev;
    
    int pid;
    int state;
    void (*entry)(void);
    char name[20];
    
}; //__attribute((packed));

/*
struct task {
    uint64_t rip;
    uint64_t *stack_ptr;
    void (*entry)(void);

    uint64_t pid;
    int state;

    struct task *next;
    struct task *prev;
        
};*/


//
#define STACK_SIZE 4096
