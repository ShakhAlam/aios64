# AIOS64 Architecture

This document explains how AIOS64 is put together: how it boots, how memory is laid out, how interrupts reach the kernel, how tasks are scheduled, and how keyboard input becomes a shell command. Diagrams use [Mermaid](https://mermaid.js.org/), which GitHub renders natively.

Everything described here is the behaviour of the current code, rough edges included. Those are collected under [Known issues](#known-issues).

- [1. Overview](#1-overview)
- [2. Source layout](#2-source-layout)
- [3. Build pipeline](#3-build-pipeline)
- [4. Boot sequence](#4-boot-sequence)
- [5. Memory layout and paging](#5-memory-layout-and-paging)
- [6. Interrupts](#6-interrupts)
- [7. Tasks and scheduling](#7-tasks-and-scheduling)
- [8. Synchronization](#8-synchronization)
- [9. Keyboard input](#9-keyboard-input)
- [10. Graphics and the text terminal](#10-graphics-and-the-text-terminal)
- [11. Serial logging](#11-serial-logging)
- [12. The shell](#12-the-shell)
- [Known issues](#known-issues)

---

## 1. Overview

AIOS64 is a single-address-space, ring-0 kernel. There is no user mode: the shell and its commands are kernel tasks that share one set of page tables and call kernel functions directly.

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart TB
    subgraph FW["Firmware and loader"]
        BIOS["BIOS / SeaBIOS"] --> GRUB["GRUB 2 (Multiboot2)"]
    end

    subgraph BOOT["Early boot (src/impl/x86_64/boot)"]
        HDR["header.asm<br/>Multiboot2 header + framebuffer request"]
        START["main.asm (32-bit)<br/>checks, page tables, long mode"]
        LM["main64.asm (64-bit)<br/>segment setup, call kernel_main"]
    end

    subgraph CORE["Kernel core"]
        KMAIN["kernel_main<br/>main.c"]
        IDT["IDT<br/>idt.c"]
        PIC["PIC + PIT<br/>pic_remap.c"]
        ISR["ISR stubs<br/>isr_stubs.asm"]
        SCHED["Tasks + scheduler<br/>process_test.c"]
        ALLOC["Bump allocator<br/>process_test.c"]
        LOCK["Spinlocks<br/>spinlock.asm"]
    end

    subgraph DRV["Drivers"]
        KBD["PS/2 keyboard<br/>keyboard.c, getchar.c"]
        FB["Framebuffer + terminal<br/>gfx/*.c"]
        SER["COM1 serial<br/>serial.c, serial.asm"]
    end

    subgraph TASKS["Tasks"]
        IDLE["thread1 (idle, hlt loop)"]
        SHELL["thread3 → shell()"]
        CMDS["Command tasks<br/>show buffers / draw cosine / memory map"]
    end

    GRUB --> HDR --> START --> LM --> KMAIN
    KMAIN --> PIC & IDT
    KMAIN --> SCHED
    IDT --> ISR
    ISR --> SCHED
    ISR --> KBD
    SCHED --> ALLOC
    SCHED --> IDLE & SHELL
    SHELL -->|create_task / add_task| CMDS
    SHELL --> KBD
    SHELL & CMDS --> FB
    CORE --> SER
    FB --> LOCK
    SCHED --> LOCK
```

| Hardware | Used for | Code |
|---|---|---|
| Linear framebuffer (from GRUB) | All visible output | `gfx/` |
| 8259 PIC (master + slave) | IRQ routing, remapped to vectors 32–47 | `pic_remap.c` |
| 8253/8254 PIT channel 0 | 100 Hz scheduler tick | `pic_remap.c` (`pic_init_timer`) |
| PS/2 keyboard controller (`0x60`/`0x64`) | Input | `keyboard_handler.c`, `keyboard.c` |
| 16550 UART COM1 (`0x3F8`) | Debug log | `serial.asm`, `serial.c` |
| x87 FPU / SSE | Floating point in C code (e.g. `cosine`) | enabled in `main.asm` |

## 2. Source layout

The tree separates **interfaces** (`src/intf`, headers) from **implementation** (`src/impl`). Inside `impl`, `x86_64/` holds assembly and early-boot code and `kernel/` holds the C kernel.

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart LR
    subgraph intf["src/intf (headers)"]
        I1["multiboot2.h"]
        I2["framebuffer.h · print.h · ascii_font.h"]
        I3["kernel/io: keyboard.h · serial.h"]
        I4["kernel/sys/x86_64: idt.h · interrupt.h · pic.h · ports.h · scheduler.h · sync/spinlock.h"]
        I5["kernel/util: shell.h · math.h"]
        I6["kernel/strings/strings.h · kernel/x86_64/readregs.h"]
    end
    subgraph asm["src/impl/x86_64"]
        A1["boot/header.asm"]
        A2["boot/main.asm"]
        A3["boot/main64.asm"]
        A4["boot/serial.asm · boot/framebuffer.asm · boot/print.c"]
        A5["isr_stubs.asm"]
        A6["scheduler/task_switcher.asm"]
        A7["sync/spinlock.asm"]
        A8["readregs.asm"]
    end
    subgraph c["src/impl/kernel"]
        C1["main.c"]
        C2["sys/x86_64: idt.c · interrupts.c · pic_remap.c · ports.c · isr/*.c"]
        C3["proc/process_test.c"]
        C4["io: keyboard.c · getchar.c · serial.c"]
        C5["gfx: framebuffer.c · pixel.c · draw_char.c · fonts"]
        C6["shell/shell.c"]
        C7["strings/strings.c · util/math.c"]
    end
```

| File | Responsibility |
|---|---|
| [boot/header.asm](../src/impl/x86_64/boot/header.asm) | Multiboot2 header: magic `0xE85250D6` and a framebuffer tag requesting 1920×1080×32 |
| [boot/main.asm](../src/impl/x86_64/boot/main.asm) | 32-bit entry `start`: stack, serial, Multiboot check, framebuffer discovery, page tables, PAE/LME/PG, FPU/SSE, GDT, far jump |
| [boot/main64.asm](../src/impl/x86_64/boot/main64.asm) | 64-bit entry `long_mode_start`: zero the segment registers, call `kernel_main` |
| [boot/framebuffer.asm](../src/impl/x86_64/boot/framebuffer.asm) | Globals `framebuffer_addr/pitch/width/height/depth` filled by the boot code |
| [boot/serial.asm](../src/impl/x86_64/boot/serial.asm) | 32-bit COM1 init (9600 8N1) and print helpers used before long mode |
| [boot/print.c](../src/impl/x86_64/boot/print.c) | `printk` and the legacy VGA text-mode printer at `0xB8000` (not used in framebuffer mode) |
| [isr_stubs.asm](../src/impl/x86_64/isr_stubs.asm) | Interrupt entry points (`isr_timer`, `isr_keyboard`, `isr_page_fault`, `isr_gp_fault`), the **context switch**, `load_idt` |
| [scheduler/task_switcher.asm](../src/impl/x86_64/scheduler/task_switcher.asm) | Older `switch_task` / `switch_to_first_task` routines. Not used by the current timer-driven path |
| [sync/spinlock.asm](../src/impl/x86_64/sync/spinlock.asm) | `enter_critical_section` / `leave_critical_section` |
| [readregs.asm](../src/impl/x86_64/readregs.asm) | `getCR3`, `getRIP` |
| [main.c](../src/impl/kernel/main.c) | `kernel_main`, `page_fault_handler`, back buffer and blit helpers |
| [sys/x86_64/idt.c](../src/impl/kernel/sys/x86_64/idt.c) | IDT table, `set_idt_entry`, `idt_init`, `idt_load` |
| [sys/x86_64/pic_remap.c](../src/impl/kernel/sys/x86_64/pic_remap.c) | PIC remap/mask/unmask/EOI, PIT programming |
| [sys/x86_64/interrupts.c](../src/impl/kernel/sys/x86_64/interrupts.c) | `timer_handler` (tick and quantum accounting) |
| [sys/x86_64/isr/](../src/impl/kernel/sys/x86_64/isr/) | `keyboard_handler` (scancode ring) and `gp_fault_handler` |
| [proc/process_test.c](../src/impl/kernel/proc/process_test.c) | Task creation, run queue, `remove_task`, bump allocator, built-in threads |
| [io/keyboard.c](../src/impl/kernel/io/keyboard.c), [io/getchar.c](../src/impl/kernel/io/getchar.c) | Scancode → ASCII, `getchar`, `k_fgets` |
| [io/serial.c](../src/impl/kernel/io/serial.c) | `write_serial`, `printf_serial`, `log_serial` |
| [gfx/draw_char.c](../src/impl/kernel/gfx/draw_char.c) | Glyph rendering, `vsnprintf`, the scrolling terminal `printf_gfx_term` |
| [gfx/pixel.c](../src/impl/kernel/gfx/pixel.c), [gfx/framebuffer.c](../src/impl/kernel/gfx/framebuffer.c) | `putpixel`, `clear_screen`, `vsync` |
| [shell/shell.c](../src/impl/kernel/shell/shell.c) | The shell loop and its commands |

## 3. Build pipeline

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart LR
    CK["src/impl/kernel/**/*.c"] -->|"x86_64-elf-gcc -ffreestanding -I src/intf"| OK["build/kernel/**/*.o"]
    CX["src/impl/x86_64/**/*.c"] -->|x86_64-elf-gcc| OX["build/x86_64/**/*.o"]
    AX["src/impl/x86_64/**/*.asm"] -->|"nasm -f elf64"| OX
    OK & OX -->|"x86_64-elf-ld -n -T targets/x86_64/linker.ld"| BIN["dist/x86_64/kernel.bin"]
    BIN -->|copy| ISODIR["targets/x86_64/iso/boot/kernel.bin"]
    CFG["targets/x86_64/iso/boot/grub/grub.cfg"] --> ISODIR
    ISODIR -->|"grub-mkrescue (i386-pc)"| ISO["dist/x86_64/kernel.iso"]
```

All tools run inside the `os64-build-env` Docker image. The working target is `make build-x86_64`; `make all` does not build anything (see [Getting Started](getting-started.md#known-build-issue-make-all)).

[grub.cfg](../targets/x86_64/iso/boot/grub/grub.cfg) boots immediately (`timeout=0`) with `multiboot2 /boot/kernel.bin` and `gfxpayload=1920x1080x32`.

## 4. Boot sequence

```mermaid
sequenceDiagram
    autonumber
    participant G as GRUB
    participant S as start (main.asm, 32-bit)
    participant U as COM1
    participant L as long_mode_start (main64.asm)
    participant K as kernel_main (main.c)
    participant T as init_tasking (process_test.c)
    participant I as first_task_start (isr_stubs.asm)

    G->>S: jump to start, EAX = 0x36D76289, EBX = Multiboot2 info
    S->>S: set stack (16 KiB), clear EFLAGS
    S->>U: serial_init (9600 8N1)
    S->>S: check_multiboot, verify magic, walk tags
    S->>S: framebuffer tag (type 8) gives addr, pitch, width, height, depth
    S->>S: check_long_mode (CPUID 0x80000001, LM bit)
    S->>S: setup_page_tables (PML4, PDPT, PD and framebuffer PD)
    S->>S: enable_paging (CR3, CR4.PAE, EFER.LME, CR0.PG)
    S->>S: enable_fpu (CR0.MP/NE, clear EM, CR4.OSFXSR/OSXMMEXCPT)
    S->>U: "Entering 64bit Mode!"
    S->>L: lgdt gdt64, far jmp 0x08:long_mode_start
    L->>L: zero SS/DS/ES/FS/GS
    L->>K: call kernel_main
    K->>U: framebuffer and back buffer info
    K->>K: pic_remap, idt_init, set_idt_entry 13/14/32/33
    K->>K: pic_init_timer(100), unmask IRQ0 and IRQ1, idt_load
    K->>T: init_tasking()
    T->>T: init_alloc(heap, 2 MiB)
    T->>T: create thread1 and thread3, add to run queue
    T->>I: first_task_start()
    I->>I: get_next_task, load its RSP, iretq
    Note over I: The shell task (thread3) runs first and never returns to kernel_main
```

Notes:

- **Multiboot check:** `check_multiboot` compares EAX with `0x36D76289`. If they differ, it jumps to `error` with code `M`. It then walks the Multiboot2 tags (8-byte aligned) from `EBX + 8`, printing each tag type to serial, until it finds the framebuffer tag (type 8). It stores the address, pitch, width, height and depth into the globals in `framebuffer.asm`.
- **Error codes:** `error` writes `ERR: <code>` to VGA text memory at `0xB8000` and halts. The codes are `M` (not Multiboot2) and `L` (no long mode). A `check_cpuid` routine (`C`) exists but is not called.
- **GDT:** `gdt64` contains a null descriptor and one 64-bit code segment (selector `0x08`: executable, code/data, present, L-bit). There is no data descriptor; long mode ignores data segment bases and limits, so the data segment registers are simply loaded with 0.
- **Stack:** the boot stack is 16 KiB in `.bss` (`stack_bottom`…`stack_top`). After `init_tasking` every task runs on its own heap-allocated stack.
- `kernel_main` executes `sti` after `init_tasking`, but `init_tasking` never returns: interrupts are enabled when `iretq` loads the first task's RFLAGS (`0x286`, IF set).

## 5. Memory layout and paging

### Kernel image

[linker.ld](../targets/x86_64/linker.ld) places the kernel at **1 MiB** physical. Because the first GiB is identity mapped, virtual addresses equal physical addresses.

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart TB
    A["0x000000 to 0x0FFFFF<br/>low memory: BIOS, VGA 0xB8000 (unused by the kernel)"]
    B["0x100000: .boot<br/>Multiboot2 header (KEEP)"]
    C[".text<br/>all code"]
    D[".rodata / .data / .bss (placed by ld, not named in linker.ld)<br/>page tables, boot stack, IDT, framebuffer globals,<br/>back_buffer (1920 × 1080 × 4 = ~7.9 MiB)"]
    E[".heap, 4 KiB aligned<br/>__heap_start to __heap_end (1 MiB reserved)"]
    F["... rest of the identity-mapped first GiB ..."]
    G["framebuffer, e.g. 0xE8000000 or 0xFD000000<br/>mapped with 7 × 2 MiB pages"]
    A --> B --> C --> D --> E --> F --> G
```

The `.rodata`, `.data` and `.bss` sections are orphan sections that `ld` places by itself. In the sample run the back buffer is at `0x10A1A0`, just after `.text`. `init_tasking` hands the allocator **2 MiB** starting at `__heap_start`, although the linker script reserves only 1 MiB (see [Known issues](#known-issues)).

### Page tables

The boot code builds 4-level paging using **2 MiB huge pages** (PS bit in the PD entries), with four 4 KiB tables in `.bss`:

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart LR
    CR3["CR3"] --> L4["page_table_l4 (PML4)"]
    L4 -->|"[0]"| L3["page_table_l3 (PDPT)"]
    L3 -->|"[0] → 0 to 1 GiB"| L2["page_table_l2 (PD)<br/>512 × 2 MiB<br/>entry i = i × 2 MiB | 0x83"]
    L3 -->|"[fb >> 30]"| L2FB["page_table_l2_fb (PD)<br/>7 entries starting at (fb >> 21) & 0x1FF<br/>fb + k × 2 MiB | 0x83"]
    L2 --> RAM["physical 0x00000000 to 0x3FFFFFFF<br/>(identity)"]
    L2FB --> VRAM["framebuffer, 14 MiB<br/>(identity)"]
```

- Entry flags `0x83` = Present | Writable | Page Size (2 MiB). The CPU later sets Accessed/Dirty, which is why `memory map` shows some entries as `…e3`.
- The framebuffer gets its own page directory, so it can sit in any GiB below 4 GiB: the boot code reads only the low 32 bits of the address. With QEMU's `0xE8000000` that is PDPT index 3 and PD indices `0x140`–`0x146`, exactly what the `memory map` screenshot shows.
- There is one address space for everything. Nothing changes CR3 after boot.

### Heap

The allocator in `process_test.c` is a **bump allocator**:

```c
void *alloc(size_t size){
    size = (size+15) & ~0xF;           // 16-byte alignment
    if(heap_top + size > heap_end) return 0;
    void *ptr = heap_top; heap_top += size; return ptr;
}
```

It is used for `struct task` objects and 4 KiB task stacks. Nothing is ever freed. `reset_heap()` exists but is not called.

## 6. Interrupts

### Setup

`pic_remap()` re-initialises both 8259 PICs so that IRQ 0–7 map to vectors **32–39** and IRQ 8–15 to **40–47**, then masks every IRQ. `kernel_main` installs four gates (all `0x8E`: present, DPL 0, 64-bit interrupt gate, selector `0x08`), programs the PIT to 100 Hz (divisor 11931, mode 3), unmasks IRQ0 and IRQ1, and loads the IDT with `lidt`.

| Vector | Source | Entry stub | C handler |
|---|---|---|---|
| 13 | #GP general protection fault | `isr_gp_fault` | `gp_fault_handler` |
| 14 | #PF page fault | `isr_page_fault` | `page_fault_handler` |
| 32 | IRQ0, PIT timer | `isr_timer` | `timer_handler`, plus the context switch in assembly |
| 33 | IRQ1, keyboard | `isr_keyboard` | `keyboard_handler` |

All other vectors are zeroed (not present), so any other exception escalates to a double fault and then a triple fault.

### Flow

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart LR
    PIT["PIT ch0<br/>100 Hz"] -->|IRQ0| MPIC
    KB["PS/2 keyboard"] -->|IRQ1| MPIC
    MPIC["Master 8259<br/>offset 0x20"] -->|"vector 32 / 33"| CPU
    SPIC["Slave 8259<br/>offset 0x28<br/>(all masked)"] -.->|cascade IRQ2| MPIC
    CPU["CPU<br/>IDT lookup"] --> T["isr_timer"] & K["isr_keyboard"]
    CPU -->|"#PF (14)"| PF["isr_page_fault"]
    CPU -->|"#GP (13)"| GP["isr_gp_fault"]
    T --> TH["timer_handler()<br/>quantum++"] --> SW{"quantum > 4?"}
    SW -->|no| EOI1["EOI → pop regs → iretq"]
    SW -->|yes| CS["save RSP, switch task<br/>EOI → pop → iretq"]
    K --> KH["keyboard_handler()<br/>inb 0x60 → ring buffer<br/>pic_send_eoi(1)"]
    PF --> PFH["page_fault_handler()<br/>print CR2, remove task"]
    GP --> GPH["gp_fault_handler()<br/>decode error code, remove task"]
```

The stubs save all general-purpose registers and RFLAGS by hand (the code does not use `__attribute__((interrupt))`), call the C handler, restore everything and `iretq`. The timer stub sends EOI itself (`out 0x20, 0x20`), and the keyboard handler calls `pic_send_eoi(1)`.

### Fault handling

Both fault handlers log the fault (critical level) and then call `remove_task()` on the current task. Because the faulting task is the current task, `remove_task` jumps into the scheduler (`first_task_start`) and never returns. The rest of the system keeps running. The page-fault handler also prints `CR2` and the error code on screen.

## 7. Tasks and scheduling

### The task structure

From [scheduler.h](../src/intf/kernel/sys/x86_64/scheduler.h). The offsets matter because assembly code reads `rsp` at `+0x08` and `quantum` at `+0x48`.

| Offset | Field | Meaning |
|---|---|---|
| `0x00` | `next` | Next task in the circular run queue |
| `0x08` | `rsp` | Saved stack pointer. Points at the saved register frame |
| `0x10` | `rip` | Entry point (bookkeeping) |
| `0x18` | `rflags` | Unused by the current switch path |
| `0x20`–`0x40` | `rbx rbp r12 r13 r14` | Used only by the older `switch_task` routine |
| `0x48` | `quantum` | Ticks consumed in the current time slice |
| `0x50` | `prev` | Previous task in the run queue |
| | `pid` | Task ID |
| | `state` | `allocated`, `ready`, `running`, `waiting`, `done` |
| | `entry` | Entry function |

### Task lifecycle

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
stateDiagram-v2
    [*] --> allocated: create_task() allocates struct + 4 KiB stack and builds the initial frame
    allocated --> ready: add_task() links it into the run queue
    ready --> running: timer ISR picks it (get_next_task)
    running --> ready: quantum expired (5 ticks)
    running --> [*]: remove_task(own id), or a #PF/#GP fault
    note right of running
        Only allocated and ready are written to
        task.state. running/waiting/done are
        defined but not yet tracked.
    end note
```

### Initial stack frame

`create_task(func, id)` allocates a 4 KiB stack and pre-builds the frame that the timer ISR's epilogue expects, so a new task starts the first time `iretq` "returns" into it:

```text
high addresses  ┌──────────────────────────┐ ← stack + 4096 - 8   (used as the SS slot)
                │ RSP for iretq            │
                │ RFLAGS = 0x286 (IF=1)    │
                │ CS     = 0x08            │
                │ RIP    = func            │ ← iretq frame
                │ 16 register slots        │   popped by the timer stub:
                │  (all 0 except the       │   popf, r15 … r8, rbp, rdi,
                │   slot holding the id)   │   rsi, rdx, rcx, rbx, rax
low addresses   └──────────────────────────┘ ← task->rsp
```

### Run queue

Tasks form a **circular doubly-linked list** and `current_task` points into it. `add_task` inserts the new task just before `current_task`, at the tail of the ring.

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart LR
    T1["thread1 (pid 1)<br/>idle: hlt loop"] -->|next| T3["thread3 (pid 3)<br/>shell()"]
    T3 -->|next| C10["command task<br/>(pid 10, 11, …)"]
    C10 -->|next| T1
    CUR(["current_task"]) -.-> T3
```

At boot, `init_tasking` creates `thread1` (pid 1) and `thread3` (pid 3, which calls `shell()`). It also creates `thread2` (pid 2), which is never added to the queue. It sets `current_task = thread1` and enters `first_task_start`, which advances to `thread1->next`, so **the shell is the first task to run**. Each shell command adds one more task to the ring until it removes itself.

### Context switch (timer tick)

```mermaid
sequenceDiagram
    autonumber
    participant A as Task A (running)
    participant ISR as isr_timer
    participant C as timer_handler (C)
    participant N as get_next_task (C)
    participant B as Task B

    A->>ISR: IRQ0 (CPU pushes SS, RSP, RFLAGS, CS, RIP)
    ISR->>ISR: cli, push rax…r15, pushf
    ISR->>C: timer_handler(rsp)
    C-->>ISR: current_task->quantum++ (wraps 5 → 1)
    alt quantum ≤ 4
        ISR->>ISR: EOI, popf, pop r15…rax, sti
        ISR-->>A: iretq (same task continues)
    else quantum > 4 (5th tick)
        ISR->>ISR: current_task->rsp = rsp (saves A's frame)
        Note over ISR: label first_task_start
        ISR->>N: get_next_task()
        N-->>ISR: current_task = current_task->next (B)
        ISR->>ISR: rsp = B->rsp (B's saved frame)
        ISR->>ISR: EOI, popf, pop r15…rax, sti
        ISR-->>B: iretq (B resumes where it was preempted)
    end
```

All of a task's state lives **on its own stack**: the CPU's interrupt frame plus the registers pushed by the stub. Switching tasks only means saving one RSP and loading another, and the shared epilogue restores the rest. At 100 Hz with a 5-tick slice, each task runs for about **50 ms** before being preempted.

`first_task_start` is a label in the middle of `isr_timer`. `init_tasking` jumps to it to start the first task, and `remove_task` jumps to it to leave a task that has removed itself.

[task_switcher.asm](../src/impl/x86_64/scheduler/task_switcher.asm) and `schedule()` in `process_test.c` are an earlier, cooperative `switch_task`-based design. They are still linked but are not called on the current path.

### Removing a task

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart TD
    R["remove_task(id)"] --> CLI["cli + lock remove_task_spinlock"]
    CLI --> F["find_task(id)"]
    F -->|"not found"| RET["unlock, sti, return NULL"]
    F -->|found| UL["unlink: prev->next = next, next->prev = prev"]
    UL --> ISCUR{"is it current_task?"}
    ISCUR -->|no| RET2["unlock, sti, return task"]
    ISCUR -->|yes| ADV["current_task = t->next<br/>unlock, sti"]
    ADV --> FTS["jump to first_task_start<br/>(never returns)"]
```

Shell commands end by calling `remove_task(get_current_task_id())`. Task functions must never return: there is no return address on a fresh task's stack.

## 8. Synchronization

AIOS64 runs on one CPU, but tasks are preempted at arbitrary points, so shared state is protected with spinlocks:

```nasm
enter_critical_section:        ; rdi = &lock
.try_lock:
    mov rax, [rdi]
    test rax, rax
    jnz .wait                  ; already held, spin
    mov rax, 0
    mov rcx, 1
    lock cmpxchg [rdi], rcx    ; atomically 0 → 1
    jz .got_lock
.wait:
    pause
    jmp .try_lock
```

`leave_critical_section` stores 0. A task that finds a lock held spins until its time slice ends and the holder gets to run and release it.

| Lock | Protects | Where |
|---|---|---|
| `printf_gfx_lock` | Terminal cursor and glyph drawing | `printf_gfx_term` in `draw_char.c` |
| `scroll_backbuffer_lock1` | Framebuffer scroll | `scroll_backbuffer` in `draw_char.c` |
| `remove_task_spinlock` | Run-queue unlinking (also wrapped in `cli`/`sti`) | `remove_task` in `process_test.c` |
| `showRegisterCR3_lock` | The `memory map` command's page-table walk | `shell.c` |

Interrupt handlers do not take locks. They run with interrupts disabled (`cli` in each stub), and the keyboard ring buffer is a single-producer / single-consumer queue with `volatile` indices.

## 9. Keyboard input

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart LR
    KEY["Key press"] --> CTRL["PS/2 controller<br/>port 0x60"]
    CTRL -->|IRQ1| H["keyboard_handler()<br/>inb 0x60"]
    H --> RING[("scancode_buffer[128]<br/>head / tail ring")]
    RING --> RSB["read_scancode_blocking()<br/>hlt while empty"]
    RSB --> S2A["scancode_to_ascii()<br/>set 1, Shift, Caps Lock"]
    S2A --> GC["getchar()"]
    GC --> FG["k_fgets(buf, 128)<br/>echo, Backspace, Enter"]
    FG --> SH["shell()"]
```

- **Producer:** `keyboard_handler` appends the raw set-1 scancode if the ring isn't full (otherwise the byte is dropped), then sends EOI.
- **Consumer:** `read_scancode_blocking` executes `hlt` until the ring is non-empty, so a waiting shell sleeps until the next interrupt (keyboard or timer).
- **Translation:** break codes (bit 7) update modifier state and return 0. Shift (`0x2A`/`0x36`) is tracked while held, and Caps Lock (`0x3A`) toggles and affects letter rows only. Unsupported keys map to 0.
- **Line editing:** `k_fgets` skips up to two 0 characters, echoes each character through `printf_gfx_term`, handles Backspace (`0x08`/`0x7F`) by moving the index back, and ends at `\n`/`\r`. The line is NUL-terminated and the newline is not stored.

`keyboard.c` also contains `getch()`, a polling driver that reads port `0x64`/`0x60` directly. It predates the interrupt-driven path and is unused.

## 10. Graphics and the text terminal

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart TB
    P["printf_gfx_term(color, bg, fmt, ...)"] --> V["vsnprintf → 1 KiB buffer"]
    V --> LOOP{"for each char"}
    LOOP -->|"'\n' or line full"| NL["new line: y += 14"]
    NL -->|"y + 14 ≥ 1080"| SC["scroll_backbuffer(framebuffer)<br/>move pixels up 20 rows"]
    LOOP -->|"0x08"| BS["move cursor back, draw space"]
    LOOP -->|printable| DC["draw_char(x, y, c)<br/>8×13 glyph in a 10×14 cell"]
    DC --> PP["putpixel(x, y, color)<br/>fb[y × 1920 + x] = color"]
    PP --> FB[("Linear framebuffer<br/>32 bpp")]
```

- **Pixels:** `putpixel` writes 32-bit values directly to the framebuffer. The stride is hard-coded to 1920 pixels and the `pitch` argument is ignored. Colours are `0x00RRGGBB`.
- **Fonts:** three bitmap fonts are compiled in (`ascii_7x11_font`, `ascii_8x13_font`, `ascii_16x26_font`). The terminal uses the 8×13 font in a 10×14 character cell, which gives 192 columns by 77 rows.
- **Cursor:** `printf_gfx_term` keeps a static cursor shared by all tasks, guarded by `printf_gfx_lock`, so output from concurrent tasks interleaves line by line rather than pixel by pixel.
- **Scrolling** copies the framebuffer up in place and clears the bottom rows.
- **Back buffer:** `back_buffer` (in `main.c`) and `blit_to_framebuffer` / `clear_back_buffer` support double buffering, but the terminal and `draw cosine` currently draw straight to the framebuffer. `vsync()` polls VGA port `0x3DA`, which is not meaningful for a linear framebuffer, and is not called.

## 11. Serial logging

```mermaid
%%{init: {"htmlLabels": false, "flowchart": {"htmlLabels": false}}}%%
flowchart LR
    subgraph BOOT32["32-bit boot"]
        B1["print_string_serial / print_hex_serial<br/>(serial.asm)"]
    end
    subgraph KERN64["64-bit kernel"]
        L["log_serial(level, fmt, ...)"] -->|"level ≤ enabled_loglevel"| W
        PF["printf_serial(fmt, ...)"] --> W
        SP["strprint_serial / hexprint_serial"] --> W
        W["write_serial(c)<br/>outb 0x3F8"]
    end
    B1 --> UART
    W --> UART["COM1 16550<br/>9600 8N1"]
    UART -->|"qemu -serial stdio"| HOST["Host terminal"]
```

`serial_init` (32-bit) programs COM1 once during boot. `write_serial` writes to the data register without polling the line-status register, which is fine under QEMU. The level filter `enabled_loglevel` defaults to `critical`. See [Getting Started → Serial output](getting-started.md#6-serial-output-and-log-levels).

## 12. The shell

The shell is the `thread3` task. Its loop prints `$ `, reads a line with `k_fgets`, matches it against `commandsMap`, and hands it to `interpret()`. `interpret()` turns the command into a function pointer and **spawns a new task** for it, with task IDs counting up from 10.

```mermaid
sequenceDiagram
    autonumber
    actor U as User
    participant SH as shell task (pid 3)
    participant KB as k_fgets / keyboard
    participant IN as interpret()
    participant SC as scheduler
    participant CMD as command task (pid 10+)
    participant FB as printf_gfx_term

    SH->>FB: "$ "
    SH->>KB: k_fgets(buf, 128)
    U->>KB: types "mem" + Enter
    KB-->>SH: "mem"
    SH->>SH: strncmp(buf, cmdStr, strlen(buf)) for each command
    SH->>FB: "matched: mem with memory map, i = 2"
    SH->>IN: interpret({showCR3}, "mem")
    IN->>SC: create_task(showRegisterCR3, id++), add_task(t)
    IN-->>SH: return 0
    SH->>FB: "$ " (prompt comes back immediately)
    SC->>CMD: on a later tick, iretq into showRegisterCR3
    CMD->>FB: dump of CR3, PT3, PT2 entries
    CMD->>SC: remove_task(own pid) and never returns
```

### Command table

```c
struct Command commandsMap[] = {
    { .cmd = showBuffers, .cmdStr = "show buffers" },
    { .cmd = drawCosine,  .cmdStr = "draw cosine"  },
    { .cmd = showCR3,     .cmdStr = "memory map"   },
    { .cmd = unknown,     .cmdStr = "UNKNOWN"      }   // sentinel, must be last
};
```

Matching compares the input against each name for `strlen(input)` characters, so any non-empty **prefix** of a name selects it, and the first match in table order wins. Input longer than the name, or input that differs anywhere, falls through to `Unknown command`.

| Command | Task function | Behaviour |
|---|---|---|
| `show buffers` | `showBuf` | Prints the current task ID, `framebuffer_addr` and `&back_buffer` |
| `draw cosine` | `drawCos` | For `f = 1..49`, plots `y = (x mod 400) · cos(x / f) + 500` for `x ∈ [0, 600π)` in steps of 0.1, colour `0xFF × f`. `cosine()` is a 10-term Taylor series |
| `memory map` | `showRegisterCR3` | Under `showRegisterCR3_lock`: reads CR3, follows PML4[0] to the PDPT, and for every non-empty PDPT entry prints each non-zero PD entry and a count |

See [Getting Started → Adding a shell command](getting-started.md#8-adding-a-shell-command) for how to add a command.

---

## Known issues

These are behaviours of the current code worth knowing about when reading or extending it.

| Area | Issue | Location |
|---|---|---|
| Build | `all: .PHONY` means `make all` (and `./build-x86`) builds nothing. Use `make build-x86_64`. `make clean` removes only assembly objects | [Makefile](../Makefile) |
| Scheduler | `get_next_task()` has no `return` on its success path. The timer stub uses RAX as the next task, so this relies on RAX happening to hold `current_task` | [process_test.c:67](../src/impl/kernel/proc/process_test.c:67) |
| Scheduler | `create_task`'s 16 register slots are one slot off from the order the timer stub pops them: the task ID ends up in RBP and RDI receives 0. Task functions therefore call `get_current_task_id()` instead of using their argument | [process_test.c:139](../src/impl/kernel/proc/process_test.c:139) |
| Scheduler | `add_task` sets `tail->prev = current_task`, which corrupts `prev` links once the ring has three or more tasks | [process_test.c:224](../src/impl/kernel/proc/process_test.c:224) |
| Scheduler | `find_task` returns early unless the ID is the current task, so `remove_task` can only remove the calling task | [process_test.c:261](../src/impl/kernel/proc/process_test.c:261) |
| Scheduler | When the current task removes itself, `current_task` is advanced and `first_task_start` advances it again, so one task is skipped for a slice | [process_test.c:296](../src/impl/kernel/proc/process_test.c:296) |
| Scheduler | FPU/SSE state is not saved on a context switch, so tasks using floating point (`draw cosine`) can corrupt each other's FP registers | [isr_stubs.asm](../src/impl/x86_64/isr_stubs.asm) |
| Memory | `init_alloc` is given 2 MiB, but `linker.ld` reserves only 1 MiB for `.heap` | [process_test.c:25](../src/impl/kernel/proc/process_test.c:25), [linker.ld](../targets/x86_64/linker.ld) |
| Memory | `memory map` masks entries with `0xFFFFFF00` and treats them as 32-bit pointers. Fine while tables are below 4 GiB | [shell.c](../src/impl/kernel/shell/shell.c) |
| Memory | Framebuffers above 4 GiB are not supported: the boot code reads only the low 32 bits of the address | [main.asm](../src/impl/x86_64/boot/main.asm) |
| Terminal | The Backspace branch of `printf_gfx_term` returns without `leave_critical_section`, leaving `printf_gfx_lock` held so later output can spin forever | [draw_char.c:348](../src/impl/kernel/gfx/draw_char.c:348) |
| Terminal | Lines advance 14 px, but `scroll_backbuffer` scrolls 20 px | [draw_char.c:164](../src/impl/kernel/gfx/draw_char.c:164) |
| Graphics | `putpixel` hard-codes a 1920-pixel stride and ignores `pitch` | [pixel.c](../src/impl/kernel/gfx/pixel.c) |
| Interrupts | `isr_page_fault` does not pop the CPU-pushed error code. This doesn't matter today because the handler never returns | [isr_stubs.asm](../src/impl/x86_64/isr_stubs.asm) |
