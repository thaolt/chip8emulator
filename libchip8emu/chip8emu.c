#include <stdlib.h>
#include "chip8emu.h"


chip8emu *chip8emu_new()
{
    chip8emu* emu = malloc(sizeof (chip8emu));
    return emu;
}

void chip8emu_load_rom(chip8emu *emu, uint16_t *code)
{

}

chip8emu *chip8emu_free(chip8emu *emu)
{
    free(emu);
}
