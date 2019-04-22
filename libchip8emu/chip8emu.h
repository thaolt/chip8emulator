#ifndef CHIP8EMU_H_
#define CHIP8EMU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define C8ERR_OK 0

typedef struct chip8emu_snapshot chip8emu_snapshot;
typedef struct chip8emu chip8emu;

struct chip8emu_snapshot {
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
};

struct chip8emu
{
    uint8_t   memory[4096];
    uint8_t   gfx[64 * 32];
    uint8_t   V[16];        /* registers from V0 .. VF */

    uint16_t  I;            /* index register */
    uint16_t  pc;           /* program counter */
    uint16_t  opcode;

    uint8_t   delay_timer;
    uint8_t   sound_timer;

    uint16_t  stack[16];
    uint16_t  sp;           /* stack pointer */

    /* opcode handling functions, can be overrided */
    int  (*opcode_handlers[0x10])(chip8emu *);
    
    /* API callbacks */
    void (*draw)(chip8emu *);
    bool (*keystate)(chip8emu *, uint8_t);
    void (*beep)(chip8emu *);
    void (*log)(chip8emu *, int log_level, const char *file, int line, const char* message);

#ifndef CHIP8EMU_NO_THREAD
    bool paused;

    void* _cpu_clk_delay;     /* struct timespec */
    void* _timer_clk_delay;   /* struct timespec */

    /* clock threads */
    void* thrd_clk_timers;
    void* thrd_clk_cpu;
    /* emulation threads */
    void* thrd_cpu_cycle;
    void* thrd_timer_tick;

    /* mutexes */
    void* mtx_cpu;
    void* mtx_timers;
    void* mtx_pause;
    /* conditional signals */
    void* cnd_clk_timers;
    void* cnd_clk_cpu;
    void* cnd_resume_cpu;
    void* cnd_resume_timers;
#endif /* CHIP8EMU_NO_THREAD */
};

chip8emu* chip8emu_new(void);
void chip8emu_free(chip8emu*);
int chip8emu_load_code(chip8emu *emu, uint8_t* code, long code_size);
int chip8emu_load_rom(chip8emu* emu, const char* filename);
void chip8emu_exec_cycle(chip8emu *emu);
void chip8emu_timer_tick(chip8emu *emu);

#ifndef CHIP8EMU_NO_THREAD
/* */
void chip8emu_start(chip8emu *emu);
void chip8emu_pause(chip8emu *emu);
void chip8emu_resume(chip8emu *emu);
void chip8emu_reset(chip8emu *emu);

void chip8emu_set_cpu_speed(chip8emu *emu, long speed_in_hz);
long chip8emu_get_cpu_speed(chip8emu *emu);
void chip8emu_set_timer_speed(chip8emu *emu, long speed_in_hz);
long chip8emu_get_timer_speed(chip8emu *emu);

#endif /* CHIP8EMU_NO_THREAD */

/**
  * can be use with thread or without thread 
  * with threads: do not use take_snapshot inside of any API callback function
  * it's very likely to create thread deadlocks
  **/
void chip8emu_take_snapshot(chip8emu *emu, chip8emu_snapshot* snapshot);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8EMU_H_ */
