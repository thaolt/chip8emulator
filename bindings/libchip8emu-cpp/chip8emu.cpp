#include "chip8emu.hpp"

Chip8Emu::Chip8Emu()
{
    this->c8e = chip8emu_new();
}

int Chip8Emu::execCycle()
{
    chip8emu_exec_cycle(this->c8e);
    return C8ERR_OK;
}

int Chip8Emu::execTimerTick()
{
    chip8emu_timer_tick(this->c8e);
    return C8ERR_OK;
}

int Chip8Emu::loadCode(unsigned char *code_buffer, long code_size)
{
    chip8emu_load_code(this->c8e, code_buffer, code_size);
    return C8ERR_OK;
}

int Chip8Emu::loadRom(const char *filename)
{
    chip8emu_load_rom(this->c8e, filename);
    return C8ERR_OK;
}

#ifndef CHIP8EMU_NO_THREAD
int Chip8Emu::start()
{
    chip8emu_start(this->c8e);
    return C8ERR_OK;
}

int Chip8Emu::reset()
{
    chip8emu_reset(this->c8e);
    return C8ERR_OK;
}

int Chip8Emu::pause()
{
    chip8emu_pause(this->c8e);
    return C8ERR_OK;
}

int Chip8Emu::resume()
{
    chip8emu_resume(this->c8e);
    return C8ERR_OK;
}
#endif /* CHIP8EMU_NO_THREAD */
