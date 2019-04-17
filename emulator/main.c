#include "chip8emu.h"

int main(int argc, char **argv) {
    chip8emu *emu = chip8emu_new();
    uint16_t * code = {1U, 2U, 3U};


    chip8emu_load_rom(emu, code);


    chip8emu_free(emu);
    return 0;
}
