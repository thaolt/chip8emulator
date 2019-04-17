#ifndef CHIP8EMU_H_
#define CHIP8EMU_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct chip8emu chip8emu;

struct chip8emu
{
    uint16_t  opcode;
    uint8_t   memory[4096];
    uint8_t   V[16];        /* registers from V0 .. VF */
    uint16_t  I;            /* index register */
    uint16_t  pc;           /* program counter */

    uint8_t   gfx[64 * 32];

    uint8_t   delay_timer;
    uint8_t   sound_timer;

    uint16_t  stack[16];
    uint16_t  sp;           /* stack pointer */

    uint8_t   key[16];
    bool draw_flag;
    void (*beep)();

};

chip8emu* chip8emu_new(void);
void chip8emu_free(chip8emu*);
void chip8emu_load_rom(chip8emu *emu, uint8_t* code, int code_size);
void chip8emu_exec_cycle(chip8emu *emu);


#endif /* CHIP8EMU_H_ */
