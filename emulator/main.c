#include "chip8emu.h"

int main(int argc, char **argv) {
    chip8emu *emu = chip8emu_new();
    uint8_t * code = 0;


    chip8emu_load_rom(emu, code, 4);

    chip8emu_exec_cycle(emu);


    chip8emu_free(emu);
    return 0;
}
