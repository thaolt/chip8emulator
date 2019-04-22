# libchip8emu

## Objectives

* Portable, targets: Windows, POSIX or embedded devices Arduino ATMEGA, STM32, MSP430
* Flexible: opcodes handlers are function pointers so they can be overrided and/or expanded by library users
* Opt to be compliance with ISO C99 

libchip8emu don't get into your way of writing your own emulator.

## How to use the library

If you don't want going to much trouble simply copy `chip8emu.c` and `chip8emu.h` to your project.

Also, be sure to copy `tinycthread.c` and `tinycthread.h` or link to TinyCThread library in your build configuration file (Makefile/CMake/QMake/etc.).

By default, libchip8emu use TinyCThread for handling CPU and Timers clocks. TinyCThread is an awesome threading library for POSIX and Windows OSes. However, if you would like to use your own threading library or you are working with embedded devices that do not have POSIX compliance libraries; then define `CHIP8EMU_NO_THREAD` (`-DCHIP8EMU_NO_THREAD`) in your build configuration. With `CHIP8EMU_NO_THREAD`, you can not use CPU speed and timer speed related functions, neither some api functions that handle emulation threads: `chip8emu_start`, `chip8emu_reset`, `chip8emu_pause`, `chip8emu_resume`. You have to handle these functions by yourself. I will probably write an Arduino example for using hardware interupt timers to implement the emulation cycles.

## How to write your own emulator

**Include `chip8emu.h` header**

```c
#include "chip8emu.h"
```

**Initialize chip8emu instance**

```c
chip8emu *cpu = chip8emu_new();
```

**Write and assign necessary callback functions**

```c
cpu->draw = &draw_callback;
cpu->keystate = &keystate_callback;
cpu->beep = &beep_callback;
```

`draw_callback`, `keystate_callback`, `beep_callback` are your own implementations which you decise how you would like to draw display, where to get keypad input source (either SDL, curses or hardware interupts for embedded devices) and how to output beep sound on your target device.

The callbacks will be called while the emulator is executing cycles. My recommendation for `draw_callback` implementation is only write code for triggering thread conditional signal in the callback for another thread that do actual drawing functionalities, check out any of my frontend for examples.

**Prototypes for above callbacks**

```c
void draw_callback(chip8emu* cpu);
bool keystate_callback(uint8_t key);
void beep_callback(void);
```

Example for `keystate_callback` implementation in SDL2

```c
/* Keypad keymap */
static int keymap[16] = {
    SDLK_KP_0,
    SDLK_KP_1,
    SDLK_KP_2,
    SDLK_KP_3,
    SDLK_KP_4,
    SDLK_KP_5,
    SDLK_KP_6,
    SDLK_KP_7,
    SDLK_KP_8,
    SDLK_KP_9,
    SDLK_KP_ENTER,
    SDLK_KP_PERIOD,
    SDLK_KP_PLUS,
    SDLK_KP_MINUS,
    SDLK_KP_MULTIPLY,
    SDLK_KP_DIVIDE,
};

bool keystate_callback(uint8_t idx) {
    Uint8 *kbstate = SDL_GetKeyState(NULL);
    return (kbstate | keymap[idx]);
}
```

**Load rom file into memory**

```c
chip8emu_load_rom("/path/to/chip8rom.ch8");
```

**Change emulated CPU clock speed**

Default CPU clock speed set to 500Hz, for example: if you would like to change to 1000Hz

```c
chip8emu_set_cpu_speed(cpu, 1000);
```

**Start the emulation**

```c
chip8emu_start(cpu);
```

**Pause, Resume, Reset the emulation**

In your main loop, you could map keys to pause, resume, reset emulation through these funtions:

* `chip8emu_pause(cpu)`
* `chip8emu_resume(cpu)`
* `chip8emu_reset(cpu)`

## With CHIP8EMU_NO_THREAD ( or without TinyCThread )

Poor man's implementation:

```c
#define CHIP8EMU_NO_THREAD
#include "chip8emu.h"

/* ... */

chip8emu *cpu = chip8emu_new();
cpu->draw = &draw_callback;
cpu->keystate = &keystate_callback;
cpu->beep = &beep_callback;

chip8emu_load_rom("/path/to/chip8rom.ch8");
uint32_t cycles = 0;
while (true) {
    cycles++;
    chip8emu_exec_cycle(cpu);
    if (!(cycles%8)) chip8emu_timer_tick(cpu);
    /* get user's key presses, e.g.: getchar() or SDL_GetKeyState .. */
    /* give some delay */
}
/* ... */
```

`if (!(cycles%8)) chip8emu_timer_tick(cpu);` means for 8 cpu cycles give one tick to timers. This won't get very far because the timing is completely wrong. However, it could help quickly test if we can load some ROMs and execute opcodes.

