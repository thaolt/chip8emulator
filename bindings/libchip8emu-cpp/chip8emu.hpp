#ifndef CHIP8EMU_HPP
#define CHIP8EMU_HPP

#include "chip8emu.h"

class Chip8Emu
{
public:
	Chip8Emu();
	~Chip8Emu();

    int execCycle();
    int execTimerTick();
    int loadCode(unsigned char* code_buffer, long code_size);
    int loadRom(const char* filename);

#ifndef CHIP8EMU_NO_THREAD
    int start();
    int reset();
    int pause();
    int resume();
#endif /* CHIP8EMU_NO_THREAD */
private:
    chip8emu* c8e;
};

#endif /* CHIP8EMU_HPP */
