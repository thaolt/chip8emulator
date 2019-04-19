#ifndef CHIP8EMU_H_
#define CHIP8EMU_H_

#include <stdint.h>
#include <stdbool.h>

#define C8ERR_OK 0

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

#ifdef CHIP8EMU_USED_THREAD
    void* _cpu_clk_delay;     /* in struct timespec */
    void* _timer_clk_delay;   /* in struct timespec */
#endif /* CHIP8EMU_USED_THREAD */

    void (*draw)(chip8emu *);
    bool (*keystate)(uint8_t);
    void (*beep)(void);
};

chip8emu* chip8emu_new(void);
void chip8emu_free(chip8emu*);
int chip8emu_load_code(chip8emu *emu, uint8_t* code, long code_size);
int chip8emu_load_rom(chip8emu* emu, const char* filename);
void chip8emu_exec_cycle(chip8emu *emu);
void chip8emu_timer_tick(chip8emu *emu);

#ifdef CHIP8EMU_USED_THREAD
/* */
void chip8emu_start(chip8emu *emu);
void chip8emu_pause(chip8emu *emu);
void chip8emu_resume(chip8emu *emu);
void chip8emu_reset(chip8emu *emu);

void chip8emu_set_cpu_speed(chip8emu *emu, long speed_in_hz);
long chip8emu_get_cpu_speed(chip8emu *emu);
void chip8emu_set_timer_speed(chip8emu *emu, long speed_in_hz);
long chip8emu_get_timer_speed(chip8emu *emu);
#endif /* CHIP8EMU_USED_THREAD */

#endif /* CHIP8EMU_H_ */
