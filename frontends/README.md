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

**Prototypes for above callbacks**

```c
void draw(chip8emu* cpu);
bool keystate(uint8_t key);
void beep(void);
```


