# Getting Started with AIOS64

This guide covers building the AIOS64 kernel ISO, booting it in QEMU, using the shell, reading the serial log and adding your own shell command.

- [1. Prerequisites](#1-prerequisites)
- [2. Build the toolchain image](#2-build-the-toolchain-image)
- [3. Build the kernel ISO](#3-build-the-kernel-iso)
- [4. Run in QEMU](#4-run-in-qemu)
- [5. Using the shell](#5-using-the-shell)
- [6. Serial output and log levels](#6-serial-output-and-log-levels)
- [7. Debugging](#7-debugging)
- [8. Adding a shell command](#8-adding-a-shell-command)
- [9. Troubleshooting](#9-troubleshooting)

---

## 1. Prerequisites

| Tool | Why | Notes |
|---|---|---|
| **Docker** | Provides the `x86_64-elf` cross-compiler, `nasm`, `grub-mkrescue` and `xorriso` | On Windows use Docker Desktop (WSL 2 backend) |
| **QEMU** (`qemu-system-x86_64`) | Runs the ISO | Linux: `apt install qemu-system-x86`. macOS: `brew install qemu`. Windows: the installer from qemu.org, with its folder added to `PATH` |
| **A POSIX shell** | The helper scripts (`docker-build`, `build-x86`, `qemu-run`) are Bash scripts | Linux/macOS terminal, WSL, or Git Bash on Windows |
| *(optional)* **Doxygen + Graphviz** | API reference | `./install_doxygen.sh` (Debian/Ubuntu) |

You do **not** need a cross-compiler on your host. Everything is compiled inside the container.

## 2. Build the toolchain image

From the repository root:

```bash
./docker-build
```

This runs `docker build -t os64-build-env:latest .` using the [Dockerfile](../Dockerfile). The image is based on `randomdude/gcc-cross-x86_64-elf`, which provides `x86_64-elf-gcc` and `x86_64-elf-ld`, and adds `nasm`, `xorriso`, `grub-pc-bin` and `grub-common`. The repository is mounted at `/root/env` inside the container.

You only need to do this once, or again after changing the Dockerfile.

> [Dockerfile.gcc-xcompile-x64](../Dockerfile.gcc-xcompile-x64) is an alternative recipe that builds GCC 13.2 and binutils 2.41 from source. It copies a `files/` directory that is not in this repository, so it cannot be built as-is. Use the default `Dockerfile`.

## 3. Build the kernel ISO

```bash
docker run --rm -v "$(pwd)":/root/env os64-build-env:latest make build-x86_64
```

The `build-x86_64` target:

1. Compiles every `src/impl/kernel/**/*.c` and `src/impl/x86_64/**/*.c` file with `x86_64-elf-gcc -ffreestanding -I src/intf`.
2. Assembles every `src/impl/x86_64/**/*.asm` file with `nasm -f elf64`.
3. Links everything with `x86_64-elf-ld -n -T targets/x86_64/linker.ld` into `dist/x86_64/kernel.bin`.
4. Copies the kernel into `targets/x86_64/iso/boot/` and runs `grub-mkrescue` (BIOS/i386-pc GRUB) to produce **`dist/x86_64/kernel.iso`**.

Object files go to `build/`. Both `build/` and `dist/` are git-ignored.

### Known build issue: `make all`

The `./build-x86` helper runs `make all`, but in the [Makefile](../Makefile) the `all` target is written as:

```make
all: .PHONY
```

`all` therefore depends only on the special `.PHONY` target and has no recipe, so **`make all` does not build the kernel**. Until that is fixed, call `make build-x86_64` directly as shown above, or open a shell in the container and run it there:

```bash
./docker-run.sh
```

```bash
make build-x86_64
```

Also note that `make clean` removes only the assembly object files. To force a full rebuild, delete `build/` and `dist/`.

### Windows notes

- **PowerShell:** Docker accepts Windows paths for the volume mount:

  ```powershell
  docker run --rm -v "${PWD}:/root/env" os64-build-env:latest make build-x86_64
  ```

- **Git Bash:** MSYS may rewrite the `/root/env` part of `-v` arguments. If the build cannot find the `Makefile`, disable path conversion for the command:

  ```bash
  MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd)":/root/env os64-build-env:latest make build-x86_64
  ```

- **WSL:** clone the repository inside the WSL filesystem and use the Linux instructions unchanged.

## 4. Run in QEMU

```bash
./qemu-run
```

which runs:

```bash
qemu-system-x86_64 -serial stdio -cdrom dist/x86_64/kernel.iso
```

- `-cdrom dist/x86_64/kernel.iso` boots the GRUB rescue ISO. GRUB has `timeout=0` and boots the `aios` entry immediately.
- `-serial stdio` connects the guest's COM1 port to your terminal, so kernel log messages appear in the shell you launched QEMU from (see [section 6](#6-serial-output-and-log-levels)).

The same command works in PowerShell once `qemu-system-x86_64.exe` is on your `PATH`.

**Click inside the QEMU window** so it receives keyboard input. Press `Ctrl+Alt+G` to release the mouse and keyboard again.

### What you should see

1. A short GRUB flash. The menu is skipped because `timeout=0`.
2. A black 1920×1080 screen with a yellow `$ ` prompt in the top-left corner.
3. In your terminal (serial), boot diagnostics: the Multiboot2 magic and info address, each Multiboot tag, the framebuffer address/pitch/size/depth, the page-table addresses, `Entering 64bit Mode!`, and then the `**** In 64 bit Kernel! ****` banner from `kernel_main`.

The display is 1920×1080. If the QEMU window is larger than your screen, use QEMU's **View → Zoom to fit**, or add `-display gtk,zoom-to-fit=on` (or `-display sdl`) to the command.

### Useful QEMU variants

The [qemu-run](../qemu-run) script contains these commented-out alternatives:

```bash
qemu-system-x86_64 -d int -serial stdio -cdrom dist/x86_64/kernel.iso
```

```bash
qemu-system-x86_64 -d cpu,int -serial stdio -cdrom dist/x86_64/kernel.iso
```

`-d int` logs every interrupt and exception, and `-d cpu,int` also dumps CPU state. Both are very verbose because the timer fires 100 times per second. Add `-D qemu.log` to write the log to a file, and `-no-reboot -no-shutdown` to stop QEMU from rebooting on a triple fault so you can inspect what happened.

## 5. Using the shell

When the kernel finishes initialising, the scheduler starts the shell task and you get a prompt:

```text
$ _
```

Type a command and press **Enter**. Available commands:

| Command | What it does |
|---|---|
| `show buffers` | Prints the ID of the task running the command and the addresses of the framebuffer and the kernel back buffer |
| `draw cosine` | Draws 50 overlaid cosine curves in blue across the screen |
| `memory map` | Reads `CR3` and walks the page tables, printing every mapped 2 MiB page |

### Prefix matching

The shell compares what you typed against each command name, **up to the length of what you typed**. The first command that matches wins, in the order `show buffers`, `draw cosine`, `memory map`. So:

| You type | Runs |
|---|---|
| `show buffers`, `show`, `s` | `show buffers` |
| `draw cosine`, `draw`, `d` | `draw cosine` |
| `memory map`, `mem`, `m` | `memory map` |
| `draw buffers`, `memory map now`, `x` | *Unknown command* |

When a command matches, the shell echoes a line such as `matched: mem with memory map, i = 2`, where `i` is the command's index.

### Example session

![Session 1](../screenshots/AIOS64-screenshot-sample1.png)

```text
$ draw buffers
Unknown command: draw buffers
$ draw cosine
matched: draw cosine with draw cosine, i = 1
$ show buffers
matched: show buffers with show buffers, i = 0
$ Thread Id = 0
Framebuffer address: 0xe8000000
Backbuffer address: 0x10a1a0
```

Things to notice:

- Each command runs as a **separate task**, so the shell prints its next `$ ` prompt immediately and the command's output can show up after the prompt.
- `draw cosine` draws straight onto the framebuffer underneath the text, which gives the blue triangles.
- `Thread Id` prints as `0` in this capture, even though tasks spawned by the shell are numbered from 10 (see `interpret()` in `shell.c`).
- The framebuffer address (`0xe8000000` here) is whatever the firmware/QEMU reports through Multiboot2, so it may differ on your machine.

![Session 2](../screenshots/AIOS64-screenshot-sample2.png)

`mem` (prefix of `memory map`) dumps the page tables:

- `CR3 = … = PT4` is the physical address of the PML4.
- `PT3 Index: 0 points to PT2` is the page directory that identity maps the first 1 GiB: 512 huge pages `0x0`, `0x200000`, … The low bits `0x83`/`0xe3` are the Present, Writable, Page-Size (and Accessed/Dirty) flags.
- `PT3 Index: 3 points to PT2` is the page directory for the framebuffer at `0xe8000000`. The 7 mapped pages (14 MiB) cover the 1920×1080×4 ≈ 7.9 MiB framebuffer.

### Editing input

- **Backspace** removes the last character from the input buffer. See [Known issues](architecture.md#known-issues): the on-screen handling of Backspace can leave the terminal lock held.
- **Shift** and **Caps Lock** work for letters and symbols. Arrow keys, Ctrl, Alt and the function keys are ignored.
- Lines are limited to 127 characters.

## 6. Serial output and log levels

All kernel diagnostics go to COM1 (port `0x3F8`, 9600 baud 8N1), which QEMU forwards to your terminal with `-serial stdio`.

Kernel code logs through `log_serial(level, fmt, ...)` from [serial.c](../src/impl/kernel/io/serial.c). The levels are:

| Level | Value | Typical use |
|---|---|---|
| `critical` | 0 | Page faults, general protection faults |
| `error` | 1 | |
| `warn` | 2 | |
| `info` | 3 | Timer ticks, task removal, task list dumps |
| `debug` | 4 | Stack dumps in the timer ISR, quantum values |
| `debug_finer` | 5 | Every scheduler decision, task creation, per-thread traces |

Only messages at or below `enabled_loglevel` are printed. It is a compile-time constant in [serial.c:86](../src/impl/kernel/io/serial.c:86):

```c
static const loglevel enabled_loglevel = critical;
```

Change it to `info` or `debug` and rebuild to watch the scheduler at work. Expect a lot of output, because the timer handler logs on every tick at `info`.

`log_serial` supports `%s %c %d %f %x %u %lx %lu`.

## 7. Debugging

**With GDB**, start QEMU paused with a GDB stub:

```bash
qemu-system-x86_64 -s -S -serial stdio -cdrom dist/x86_64/kernel.iso
```

and in another terminal:

```bash
gdb dist/x86_64/kernel.bin -ex "target remote :1234" -ex "break kernel_main" -ex "continue"
```

The kernel is linked at its physical load address (1 MiB, identity mapped), so symbol addresses in `kernel.bin` match the running kernel. Use a GDB with x86-64 support (`gdb-multiarch` on Debian/Ubuntu). The image is not compiled with `-g`, so you get symbols but no source lines unless you add `-g` to the `x86_64-elf-gcc` lines in the Makefile.

**Fault output:**

- **Page fault (#PF, vector 14):** `Page Fault!`, `CR2` (the faulting address) and the error code are printed on screen, and a `critical` serial message names the current task. The faulting task is removed from the run queue.
- **General protection fault (#GP, vector 13):** a `critical` serial message with RIP, the error code (decoded into selector index, GDT/LDT and external flag) and CS. The faulting task is removed.

To test the page-fault path, uncomment the `0xDEEDBEEF` access at the end of `kernel_main` in [main.c](../src/impl/kernel/main.c).

## 8. Adding a shell command

Commands live in [shell.c](../src/impl/kernel/shell/shell.c) and [shell.h](../src/intf/kernel/util/shell.h). As an example, here is how you would add a `hello` command.

**1. Add an enum value** before `unknown` in `src/intf/kernel/util/shell.h`:

```c
enum command{
    showBuffers = 0,
    drawCosine,
    showCR3,
    sayHello,      // new
    unknown
};
```

**2. Write the task function** in `shell.c`. It runs as its own task, so it **must remove itself** when done. Returning from a task function is not supported: there is no return address on a new task's stack.

```c
void hello(int tid){
    int me = get_current_task_id();
    printf_gfx_term(0x00FF00, 0, "Hello from task %d!\n", me);
    remove_task(me);   // does not return
}
```

**3. Register the name** in `commandsMap`. Keep the `UNKNOWN` entry **last**, because the matching loop skips the final entry:

```c
    {
        .cmd = sayHello,
        .cmdStr = "hello"
    },
    {
        .cmd = unknown,
        .cmdStr = "UNKNOWN"
    }
```

**4. Dispatch it** in `interpret()`:

```c
        case sayHello:
            fn = hello;
        break;
```

Rebuild and run. Typing `hello` (or `h`) now spawns your task.

Tips:

- If your command shares state with other tasks, guard it with a `spinlock` (`enter_critical_section` / `leave_critical_section`), as `showRegisterCR3` does.
- Order in `commandsMap` decides which command wins for an ambiguous prefix.
- Each task gets a 4 KiB stack, so avoid large local arrays.

## 9. Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| `make: Nothing to be done for 'all'` / no ISO produced | You ran `./build-x86` or `make all`. Use `make build-x86_64` ([details](#known-build-issue-make-all)) |
| `docker: invalid reference format` or the Makefile isn't found in the container | Path mangling on Windows. Quote `"$(pwd)"`, use `MSYS_NO_PATHCONV=1` in Git Bash, or use the PowerShell form |
| `grub-mkrescue: error: xorriso not found` | Rebuild the image with `./docker-build`. The Dockerfile installs `xorriso` |
| QEMU: `Could not open 'dist/x86_64/kernel.iso'` | The build didn't produce an ISO, or you're not in the repository root |
| Black screen, no prompt, serial output stops before `Entering 64bit Mode!` | The early boot code failed. It writes `ERR: X` to VGA text memory, which is not visible in framebuffer mode. `M` = not loaded by a Multiboot2 loader, `L` = the CPU doesn't support long mode. Use plain `qemu-system-x86_64` (64-bit) and boot from the ISO |
| Keys do nothing | Click inside the QEMU window to give it focus |
| Terminal stops responding after Backspace | Known issue: the Backspace path in `printf_gfx_term` returns without releasing its lock. See [Known issues](architecture.md#known-issues) |
| Window is too big | Use View → Zoom to fit, or `-display gtk,zoom-to-fit=on` |
