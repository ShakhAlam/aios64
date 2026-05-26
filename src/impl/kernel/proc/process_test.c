#include "print.h"
#include <stdint.h>
#include "kernel/sys/x86_64/scheduler.h"
#include "kernel/sys/x86_64/pic.h"
#include "kernel/io/serial.h"
#include "kernel/sys/x86_64/interrupt.h"
#include "kernel/util/shell.h"

void init_alloc(void *heap_start, size_t heap_size);
void *alloc(size_t size);
void *memset(void *dest, uint8_t c, size_t n);
extern int getRIP();


static struct task* task_list = 0;
 struct task *current_task = 0;

struct task*  create_task(void (*func)(), int id);
void switch_to_first_task(struct task *t);

void print_tasklist();

void init_tasking(){
    extern char __heap_start; // defined in ld script
    init_alloc(&__heap_start, 0x200000); // start 2MB heap
    struct task *t1 = create_task(thread1,1);
    struct task *t2 = create_task(thread2,2);
    struct task *t3 = create_task(thread3,3);
    
    add_task(t1);
    add_task(t2);
    add_task(t3);
    
    //current_task = t1;
    printf_serial("<init_tasking> current task id: %d rsp: %x, *rsp=%x, *rsp+8=%x\n", 
        current_task->pid, current_task->rsp, *((uint64_t*)(current_task->rsp)),
        *((uint64_t*)(current_task->rsp+8))  );

    print_tasklist();
    //return;    
    switch_to_first_task(current_task);
}


void schedule(){

    if(!current_task || !current_task->next){
        printf_gfx_term(0xFFFF00,0,"current task is null at: %x next: %x\n", current_task,current_task->next);
        return;
    } 

    struct task* next_task = current_task->next;

    if(!next_task){        
        strprint_serial("\nin schedule() - next_task is null\n");
        //pic_send_eoi(0);
        return;
    }

    if(current_task == next_task){
        strprint_serial("\nin schedule() - next_task is same\n");
        //pic_send_eoi(0);
        return;    
    }
    
    strprint_serial("\nin schedule()\n");

    printf_serial("<schd> current task id: %d rsp: %x, *rsp=%x, *rsp+8=%x\n", 
        current_task->pid, current_task->rsp, *((uint64_t*)(current_task->rsp)),
        *((uint64_t*)(current_task->rsp+8))  );

    printf_serial("<schd> next task id: %d rsp: %x, *rsp=%x\n", 
            next_task->pid, next_task->rsp, *((uint64_t*)(next_task->rsp)) );
    
    pic_send_eoi(0);

    strprint_serial("\nin schedule() - after eoi and now calling switch_task\n");
    struct task *prev = current_task;
    current_task = next_task;
    uint64_t rip = getRIP();
    printf_serial("\nrip = %x\n", rip);

    switch_task(prev,next_task);
    strprint_serial("\nin schedule() after switch_task\n");
    current_task = next_task;
}


struct task* 
create_task(void (*func)(), int id) {
    struct task* t = (struct task*) alloc(sizeof(struct task));
    memset(t, 0, sizeof(struct task));

    uint64_t* stack = (uint64_t*) alloc(STACK_SIZE);

    uint64_t* sp = stack + (STACK_SIZE / sizeof(uint64_t)) - 1;

    t->rsp = (uint64_t) sp;
    
    // Setup stack for iretq: RFLAGS, CS, RIP
    *(--sp) = sp;
    *(--sp) = 0x202;                  // RFLAGS (interrupts enabled)
    *(--sp) = 0x08;                   // CS
    *(--sp) = (uint64_t) func;        // RIP

    t->rsp = (uint64_t) sp;
    t->next = 0;

    t->entry = func;

    t->pid = id;
    t->state = allocated;
    t->rip = func;


    printf_serial("created task %d rsp=%x *rsp=%x rip=%x\n",
        id, t->rsp, *(uint64_t*)t->rsp, t->rip);


    return t;
}

/*** 
               |          |
       current |<----next-| new
               |----prev->|

             _________________________                      
             |                        |
             v                        |
    prev <- current_task -> next--\   |
    |        ^                    |   |
    |        |                    |   |
    |        |____________________|   |
    |_________________________________|

             _________________________                      
             |                        |
             v                        |
    prev <- current_task -> next--\   |
    |               ^              |  |
    |________       |____          |  |
             v          |          |  |
    prev <- new_task-> next       |   |
    |        ^                    |   |
    |        |                    |   |
    |        |____________________|   |
    |_________________________________|


*/
int
add_task(struct task* new_task){

    printf_serial("adding task %d rsp=%x *rsp=%x rip=%x\n",
        new_task->pid, new_task->rsp, *(uint64_t*)new_task->rsp, new_task->rip);

    if(!current_task){
        printf_serial("current task is null; adding task %d rsp=%x *rsp=%x rip=%x\n",
            new_task->pid, new_task->rsp, *(uint64_t*)new_task->rsp, new_task->rip);
    
        current_task = new_task;
        current_task->next = current_task;
        current_task->prev = current_task;
    }else{
        struct task *tail = current_task->prev; // same as current_task in the beginning

        tail->next = new_task;                  // current_task->next = new_task
        tail->prev = current_task;

        new_task->prev = tail;                  // new_task->prev = current_task
        new_task->next = current_task;

        current_task->prev = new_task;
        //current_task->next = tail;
    }

    new_task->state = ready;

    if (!task_list) {
        task_list = current_task;
        
    } 

    return 0;
}

void
print_tasklist(){
    struct task *t = current_task;
    do{
        printf_serial("task id: %d rsp: %x, *rsp=%x\n", 
            t->pid, t->rsp, *((uint64_t*)(t->rsp)) );
            
        t = t->next;
    }while (t && t != current_task);
    
}

// ++++ alloc.c

static uint8_t *heap_base = 0;
static uint8_t *heap_top = 0;
static uint8_t *heap_end = 0;

void init_alloc(void *heap_start, size_t heap_size){
    heap_base = (uint8_t *)heap_start;      // set heap_base from heap_start param
    heap_top = heap_base;                   // heap_top is same as heap_base when initialized
    heap_end = heap_base + heap_size;       
}

void *alloc(size_t size){
    //align to 16 bytes
    size = (size+15) & ~0xF;

    // check if out of memory
    if(heap_top + size > heap_end){
        return 0; // OOM
    }

    void *ptr = heap_top;
    heap_top += size;                       // increase alloc size
    return ptr;
}

void reset_heap(){
    heap_top = heap_base;               // heap_top is same as heap_base when reset
}


void *
memset(void *dest, uint8_t c, size_t n){
    uint8_t *ptr = (uint8_t *)dest;
    while(n--)
        *ptr++ = c;
    return dest;
}

// ---- alloc.c



void thread1(){
    
    static int counter = 0;

    //sti();
    while(1){
        printf_serial("<thread1> current task rsp: %x, *rsp: %x, -8: %x, *rsp-8=%x\n", 
            current_task->rsp,*((uint64_t*)(current_task->rsp)), 
            current_task->rsp-8, *((uint64_t*)(current_task->rsp -8)) );

        printf_serial("\n--------> 1 : %d\n", counter++);
        asm volatile("hlt");
        //for(volatile uint64_t x =0; x < 0xFFFFFF;x++);
    }
}


void thread2(){
    static int counter = 0;
    //sti();
    while(1){
        printf_serial("<thread2> current task rsp: %x, *rsp: %x, -8: %x, *rsp-8=%x\n", 
            current_task->rsp,*((uint64_t*)(current_task->rsp)), 
            current_task->rsp-8, *((uint64_t*)(current_task->rsp -8)) );

        printf_serial("\n--------> 2 : %d\n", counter++);
        asm volatile("hlt");
        //for(volatile uint64_t x =0; x < 0xFFFFFF;x++);
    }
}

void thread3(){
    static int counter = 0;
    //sti();
    while(1){
        printf_serial("<thread3> current task rsp: %x, *rsp: %x, -8: %x, *rsp-8=%x\n", 
            current_task->rsp,*((uint64_t*)(current_task->rsp)), 
            current_task->rsp-8, *((uint64_t*)(current_task->rsp -8)) );

        printf_serial("\n--------> 3 : %d\n", counter++);
        asm volatile("hlt");
        //for(volatile uint64_t x =0; x < 0xFFFFFF;x++);
    }

    //shell();
    
}

void idle_task(){
    sti();
    while(1){
        strprint_serial("\n--------> 0\n");
        asm volatile("hlt");
    }
}