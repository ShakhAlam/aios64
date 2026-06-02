#include "print.h"
#include <stdint.h>
#include "kernel/sys/x86_64/scheduler.h"
#include "kernel/sys/x86_64/pic.h"
#include "kernel/io/serial.h"
#include "kernel/sys/x86_64/interrupt.h"
#include "kernel/util/shell.h"

#include "kernel/sys/x86_64/sync/spinlock.h"

void init_alloc(void *heap_start, size_t heap_size);
void *alloc(size_t size);
void *memset(void *dest, uint8_t c, size_t n);
extern int getRIP();


struct task* task_list = 0;
struct task *current_task = 0;

struct task*  create_task(void (*func)(), int id);
void switch_to_first_task(struct task *t);

void print_tasklist();

void init_tasking(){
    extern char __heap_start; // defined in ld script
    init_alloc(&__heap_start, 0x200000); // start 2MB heap
    struct task *t1 = create_task(thread1,1);
    struct task *t2 = create_task(thread2,2);
    
    //add_task(t2);
    //for(int i =0; i < 2; i++){

        struct task *t3 = create_task(thread3,3);
    
        add_task(t3);
        add_task(t1);
    
    //}
    current_task = t1;
    log_serial(debug_finer,"<init_tasking> current task id: %d rsp: %x, *rsp=%x, *rsp+8=%x\n", 
        current_task->pid, current_task->rsp, *((uint64_t*)(current_task->rsp)),
        *((uint64_t*)(current_task->rsp+8))  );

    print_tasklist();
    //return;    
    //switch_to_first_task(current_task);
    first_task_start();
}

struct task *
get_current_task(){
    log_serial(debug_finer,"\n<get_current_task> invoked\n");

    return current_task;
}

int
get_current_task_id(){
    struct task *t = get_current_task();
    if(t)
        return t->pid;
    else
        return -1;
}
struct task *
get_next_task(){

    log_serial(debug_finer,"\n<get_next_task> invoked\n");

    if(!current_task || !current_task->next){
        log_serial(debug_finer,"<get_next_task> current task or next is null. current: %x next: %x\n", 
                   current_task,current_task->next);
        return 0;
    } 

    struct task* next_task = current_task->next;

    if(current_task == next_task){
        log_serial(debug_finer,"\n<get_next_task> - next_task is same\n");
        return 0;    
    }
    
    log_serial(debug_finer,"<get_next_task> current task id: %d rsp: %x, *rsp=%x, *rsp+8=%x\n", 
        current_task->pid, current_task->rsp, *((uint64_t*)(current_task->rsp)),
        *((uint64_t*)(current_task->rsp+8))  );

    log_serial(debug_finer,"<get_next_task> next task id: %d rsp: %x, *rsp=%x\n", 
            next_task->pid, next_task->rsp, *((uint64_t*)(next_task->rsp)) );
    
    current_task = next_task;
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

    log_serial(debug_finer,"<schd> current task id: %d rsp: %x, *rsp=%x, *rsp+8=%x\n", 
        current_task->pid, current_task->rsp, *((uint64_t*)(current_task->rsp)),
        *((uint64_t*)(current_task->rsp+8))  );

    log_serial(debug_finer,"<schd> next task id: %d rsp: %x, *rsp=%x\n", 
            next_task->pid, next_task->rsp, *((uint64_t*)(next_task->rsp)) );
    
    pic_send_eoi(0);

    strprint_serial("\nin schedule() - after eoi and now calling switch_task\n");
    struct task *prev = current_task;
    current_task = next_task;
    uint64_t rip = getRIP();
    log_serial(debug_finer,"\nrip = %x\n", rip);

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
    *(--sp) = 0x286;                  // RFLAGS (interrupts enabled)
    *(--sp) = 0x08;                   // CS
    *(--sp) = (uint64_t) func;        // RIP
    //if(current_task == 0){
    //set the registers on the stack
    *(--sp) = 0;    // rflags
    *(--sp) = 0;    // rax
    *(--sp) = 0;    // rbx
    *(--sp) = 0;    // rcx - param 4
    *(--sp) = 0;    // rdx - param 3
    *(--sp) = 0;    // rsi - param 2
    *(--sp) = id;    // rdi - fn param 1
    *(--sp) = 0;    // rbp
    *(--sp) = 0;    // r8  - param 5
    *(--sp) = 0;    // r9  - param 6
    *(--sp) = 0;    // r10
    *(--sp) = 0;    // r11
    *(--sp) = 0;    // r12
    *(--sp) = 0;    // r13
    *(--sp) = 0;    // r14
    *(--sp) = 0;    // r15

    //}

    t->rsp = (uint64_t) sp;
    t->next = 0;

    t->entry = func;

    t->pid = id;
    t->state = allocated;
    t->rip = func;

    t->quantum = 0;

    log_serial(debug_finer,
        "created task %d rsp=%x *rsp=%x rip=%x quantum=%d\n",
        id, t->rsp, *(uint64_t*)t->rsp, t->rip,t->quantum);


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
struct task *
add_task(struct task* new_task){

    log_serial(debug_finer,"adding task %d rsp=%x *rsp=%x rip=%x\n",
        new_task->pid, new_task->rsp, *(uint64_t*)new_task->rsp, new_task->rip);

    if(!current_task){
        log_serial(debug_finer,"current task is null; adding task %d rsp=%x *rsp=%x rip=%x\n",
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

    return new_task;
}


struct task *
find_task(int id){
    struct task *t = 0;

    log_serial(info, 
        "\n<find_task>: %d\n", id);
  
    //return 0;
    
    if(current_task && current_task->pid == id){

        log_serial(info, 
            "<find_task>: current task matches: %d\n", id);
      
        return current_task;
    }
    else
        return 0;

    
    for(t = current_task->next; t && t != current_task; 
        t = t->next){
            if(t->pid == id){
                log_serial(info, 
                    "found task: %d\n", id);

                return t;
            }
    }

    return 0;
}

static spinlock remove_task_spinlock =0;

struct task * 
remove_task(int id){
    __asm__ volatile ("cli");
            
    enter_critical_section(&remove_task_spinlock);

    log_serial(info,
        "<remove_task>: %d\n", id);
        //return 0;

    struct task *t = 0; 
    t = find_task(id);
    
    if(t){
         log_serial(info,
            "removing task: %d\n", id);

        print_tasklist();
        
        if(t->prev) {
            log_serial(info,
                "removing task: %d setting prev->next link. t->prev: %x t->prev->id : %d t->prev->next %x\n", id,
                t->prev,t->prev->pid,
                t->prev?t->prev->next:0);                
            t->prev->next = t->next;
        }else{
            log_serial(info,
                "removing task: %d could not set prev->next link. t->prev: %lx t->prev->next\n", id,
                t->prev,t->prev?t->prev->next:0);                
        }
        if(t->next){
            log_serial(info,
                "removing task: %d setting next->prev link. t->next: %x t->next->id : %d t->next->prev %x\n", id,
                t->next,t->next->pid,
                t->prev?t->next->prev:0);                
            t->next->prev = t->prev;
        } else{
            log_serial(info,
                "removing task: %d could not set next->prev link. t->next: %x t->next->prev %x\n", id,
                t->next,t->prev?t->next->prev:0);                
        }
        
        if(t && t == current_task){
            current_task = t->next;
            //TODO: need synchronization to fix
            //issue with overwriting current task in isr_timer
            
            print_tasklist();
            leave_critical_section(&remove_task_spinlock);
            __asm__ volatile ("sti");

            first_task_start();
        }
    }

    log_serial(info,
               "<remove_task>: returning %d\n", id);

    print_tasklist();
    leave_critical_section(&remove_task_spinlock);
    __asm__ volatile ("sti");
            
    return t;
}

struct task * 
exit_task(int id){
    asm volatile ("cli");

    struct task * t = remove_task(id);
    if(t){
        printf_serial(debug, "<exit_task> task id: %d\n", t->pid);
        
    }
    asm volatile ("sti");
    
    return t;
}

void
print_tasklist(){
    struct task *t = current_task;
    do{
        log_serial(info,"task list: %s task id: %d rsp: %x, *rsp=%x\n", 
            t == current_task? "current":"next",t->pid, t->rsp, *((uint64_t*)(t->rsp)) );
            
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
    log_serial(debug_finer,"<thread1> entered\n");
    
    //sti();
    while(1){
        log_serial(debug_finer,"<thread1> current task rsp: %x, *rsp: %x, -8: %x, *rsp-8=%x\n", 
            current_task->rsp,*((uint64_t*)(current_task->rsp)), 
            current_task->rsp-8, *((uint64_t*)(current_task->rsp -8)) );

        log_serial(debug_finer,"\n--------> 1 : %d\n", counter++);
        asm volatile("hlt");
        //for(volatile uint64_t x =0; x < 0xFFFFFF;x++);
    }
}


void thread2(){
    static int counter = 0;
    log_serial(debug_finer,"<thread2> entered\n");

    //sti();
    while(1){
        log_serial(debug_finer,"<thread2> current task rsp: %x, *rsp: %x, -8: %x, *rsp-8=%x\n", 
            current_task->rsp,*((uint64_t*)(current_task->rsp)), 
            current_task->rsp-8, *((uint64_t*)(current_task->rsp -8)) );

        log_serial(debug_finer,"\n--------> 2 : %d\n", counter++);
        asm volatile("hlt");
        //for(volatile uint64_t x =0; x < 0xFFFFFF;x++);
    }
}

void thread3(int tid){
    static int counter = 0;
    //sti();
    /*while(1){
        log_serial(debug_finer,"<thread3> current task rsp: %x, *rsp: %x, -8: %x, *rsp-8=%x\n", 
            current_task->rsp,*((uint64_t*)(current_task->rsp)), 
            current_task->rsp-8, *((uint64_t*)(current_task->rsp -8)) );

        log_serial(debug_finer,"\n--------> 3 : %d\n", counter++);
        asm volatile("hlt");
        //for(volatile uint64_t x =0; x < 0xFFFFFF;x++);
    }*/

    shell(tid);
    
}

void idle_task(){
    sti();
    while(1){
        strprint_serial("\n--------> 0\n");
        asm volatile("hlt");
    }
}