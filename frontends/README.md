# libchip8emu frontends

## Available frontends

### Console (text-based)

* Termbox
* CDK (Curses Development Kit)

### Graphical

* SDL2
* GDI (planned)
* X11 XLIB (planned)

## How to write your own frontend

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

The callbacks will be called while the emulator is executing cycles. My recommendation for `draw_callback` implementation is only write code for triggering thread conditional signal in the callback for another thread that do actual drawing functionalities, check out any of my frontend for example implementation.

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

_I don't include example for termbox keystate callback here because it's quite long, you could checkout the source code to see the implementation._

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

