#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "termbox.h"
#include "log.h"
#include "chip8emu.h"

static const uint32_t box_drawing[] = {
    0x250C, /* ┌ */
    0x2500, /* ─ */
    0x252C, /* ┬ */
    0x2510, /* ┐ */
    0x2502, /* │ */
    0x2514, /* └ */
    0x2534, /* ┴ */
    0x2518, /* ┘ */
};

static const uint32_t block_char[] = {
    ' ',
    0x2580, /* ▀ */
    0x2584, /* ▄ */
    0x2588, /* █ */
};

void tb_print(const char *str, int x, int y, uint16_t fg, uint16_t bg)
{
    while (*str) {
        uint32_t uni;
        str += tb_utf8_char_to_unicode(&uni, str);
        tb_change_cell(x, y, uni, fg, bg);
        x++;
    }
}

void tb_printf(int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...)
{
    char buf[4096];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);
    tb_print(buf, x, y, fg, bg);
}

void draw_display(uint8_t *buffer) {
    uint8_t offset_x = 1;
    uint8_t offset_y = 1;

    for (int line = 0; line < 16; ++line) {
        for (int x = 0; x < 64; ++x) {
            int y = line * 2;
            int y2 = y + 1;
            if (buffer[y*64+x] != 0 && buffer[y2*64+x] != 0) {
                tb_change_cell(x + offset_x, line + offset_y, block_char[3], TB_BLUE, TB_BLACK);
            } else if (buffer[y2*64+x] != 0) {
                tb_change_cell(x + offset_x, line + offset_y, block_char[2], TB_BLUE, TB_BLACK);
            } else if (buffer[y*64+x] != 0) {
                tb_change_cell(x + offset_x, line + offset_y, block_char[1], TB_BLUE, TB_BLACK);
            } else {
                tb_change_cell(x + offset_x, line + offset_y, block_char[0], TB_BLUE, TB_BLACK);
            }
        }
    }
}

void draw_keyboard() {

}


void draw_layout() {
    uint8_t display_width = 64;
    uint8_t display_height = 16;
    const char *disp_title = "Display";
    uint8_t disp_title_len = (uint8_t) strlen(disp_title);

    const char *reg_pane_title = "Registers";
    uint8_t reg_pane_title_len = (uint8_t) strlen(reg_pane_title);

    tb_change_cell(0, 0, box_drawing[0], 0, 0);
    tb_print(disp_title, 1, 0, 0, 0);
    for (int x = disp_title_len+1; x < display_width + 1; ++x) {
        tb_change_cell(x, 0, box_drawing[1], 0, 0);
    }
    tb_change_cell(display_width+1, 0, box_drawing[2], 0, 0);
    tb_print(reg_pane_title, display_width+2, 0, 0, 0);
    for (int x = display_width + 2 + reg_pane_title_len; x < tb_width()-1; ++x) {
        tb_change_cell(x, 0, box_drawing[1], 0, 0);
    }
    tb_change_cell(tb_width()-1, 0, box_drawing[3], 0, 0);
    for (int y = 1; y < display_height + 1; ++y) {
        tb_change_cell(0, y, box_drawing[4], 0, 0);
        tb_change_cell(display_width + 1, y, box_drawing[4], 0, 0);
        tb_change_cell(tb_width()-1, y, box_drawing[4], 0, 0);
    }
    tb_change_cell(0, display_height+1, box_drawing[5], 0, 0);
    for (int x = 1; x < display_width + 1; ++x) {
        tb_change_cell(x, display_height+1, box_drawing[1], 0, 0);
    }
    tb_change_cell(display_width+1, display_height+1, box_drawing[6], 0, 0);
    for (int x = display_width + 2; x < tb_width()-1; ++x) {
        tb_change_cell(x, display_height+1, box_drawing[1], 0, 0);
    }
    tb_change_cell(tb_width()-1, display_height+1, box_drawing[7], 0, 0);

}

uint8_t *disp_buffer;

void beep() {

}

void draw_registers(chip8emu* emu) {
    for (int i = 0; i < 16; ++i) {
        tb_printf(64 + 3, 1 + i, 0, 0, "V%X %02X", i, emu->V[i]);
    }
    tb_present();
}

void draw_all() {
    tb_clear();
    draw_layout();
    draw_display(disp_buffer);
    tb_present();
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    chip8emu *emu = chip8emu_new();
    emu->beep = &beep;
    disp_buffer = emu->gfx;
    emu->gfx[0] = 1;
    emu->gfx[64] = 1;
    emu->gfx[65] = 1;

    FILE *fileptr;
    uint8_t *code_buffer;
    unsigned long filelen;

    fileptr = fopen("/home/thaolt/Workspaces/build-chip8emulator-Desktop-Minimum-Size-Release/emu-termbox/UFO", "rb");
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filelen = (unsigned long) ftell(fileptr);             // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file
    int buffer_len = (int) (filelen+1)*sizeof(char);

    code_buffer = (char *)malloc(buffer_len); // Enough memory for file + \0
    fread(code_buffer, filelen, 1, fileptr); // Read in the entire file
    fclose(fileptr); // Close the file

    int ret = tb_init();
    if (ret) {
        log_error("tb_init() failed with error code %d\n", ret);
        return 1;
    }

    chip8emu_load_rom(emu, code_buffer, buffer_len);

    draw_all();

    struct tb_event ev;
    while (true) {
        tb_peek_event(&ev, 10);
        chip8emu_exec_cycle(emu);
        draw_registers(emu);
        if (emu->draw_flag) {
            draw_display(emu->gfx);
            emu->draw_flag = false;
        }
        switch (ev.type) {
        case TB_EVENT_KEY:
            switch (ev.key) {
            case TB_KEY_ESC:
                goto done;
                break;
            }
            break;
        case TB_EVENT_RESIZE:
            draw_all();
            break;
        }
    }
done:
    tb_shutdown();
    return 0;
}
