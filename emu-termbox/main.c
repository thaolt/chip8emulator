#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "termbox.h"
#include "log.h"
#include "chip8emu.h"
#include "tinycthread.h"

#define DISPLAY_WIDTH   64
#define DISPLAY_HEIGHT  16 /* x 2 */

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

    const char *disp_title = "Display";
    uint8_t disp_title_len = (uint8_t) strlen(disp_title);

    const char *reg_pane_title = "Registers";
    uint8_t reg_pane_title_len = (uint8_t) strlen(reg_pane_title);

    tb_change_cell(0, 0, box_drawing[0], 0, 0);
    tb_print(disp_title, 1, 0, 0, 0);
    for (int x = disp_title_len+1; x < DISPLAY_WIDTH + 1; ++x) {
        tb_change_cell(x, 0, box_drawing[1], 0, 0);
    }
    tb_change_cell(DISPLAY_WIDTH+1, 0, box_drawing[2], 0, 0);
    tb_print(reg_pane_title, DISPLAY_WIDTH+2, 0, 0, 0);
    for (int x = DISPLAY_WIDTH + 2 + reg_pane_title_len; x < tb_width()-1; ++x) {
        tb_change_cell(x, 0, box_drawing[1], 0, 0);
    }
    tb_change_cell(tb_width()-1, 0, box_drawing[3], 0, 0);
    for (int y = 1; y < DISPLAY_HEIGHT + 1; ++y) {
        tb_change_cell(0, y, box_drawing[4], 0, 0);
        tb_change_cell(DISPLAY_WIDTH + 1, y, box_drawing[4], 0, 0);
        tb_change_cell(tb_width()-1, y, box_drawing[4], 0, 0);
    }
    tb_change_cell(0, DISPLAY_HEIGHT+1, box_drawing[5], 0, 0);
    for (int x = 1; x < DISPLAY_WIDTH + 1; ++x) {
        tb_change_cell(x, DISPLAY_HEIGHT+1, box_drawing[1], 0, 0);
    }
    tb_change_cell(DISPLAY_WIDTH+1, DISPLAY_HEIGHT+1, box_drawing[6], 0, 0);
    for (int x = DISPLAY_WIDTH + 2; x < tb_width()-1; ++x) {
        tb_change_cell(x, DISPLAY_HEIGHT+1, box_drawing[1], 0, 0);
    }
    tb_change_cell(tb_width()-1, DISPLAY_HEIGHT+1, box_drawing[7], 0, 0);

}

void beep() {

}

void draw_registers(chip8emu* emu) {
    for (int i = 0; i < 16; ++i) {
        tb_printf(64 + 3, 1 + i, 0, 0, "V%X %02X", i, emu->V[i]);
    }
    tb_present();
}

void draw_all(chip8emu* emu) {
    tb_clear();
    draw_layout();
    draw_display(emu->gfx);
    draw_registers(emu);
    tb_present();
}

static cnd_t draw_cnd;
static mtx_t draw_mtx;
static mtx_t key_mtx;

int emulator_thread(void* arg) {
    chip8emu * emu = (chip8emu*) arg;
    clock_t start_clk = clock();
    clock_t elapsed_clk;
    for (;;) {
        elapsed_clk = clock() - start_clk;
        if (elapsed_clk >= (CLOCKS_PER_SEC/100)) { /* 100Hz */
            start_clk = clock();
            mtx_lock(&key_mtx);
            chip8emu_exec_cycle(emu);
            mtx_unlock(&key_mtx);
            draw_registers(emu);
            tb_present();            
        }
    }
}

void draw_callback(uint8_t *buf) {
    (void)buf;
    mtx_lock(&draw_mtx);
    cnd_signal(&draw_cnd);
    mtx_unlock(&draw_mtx);
}

int display_draw_thread(void *arg) {
    chip8emu * emu = (chip8emu*) arg;
    for (;;) {
        mtx_lock(&draw_mtx);
        cnd_wait(&draw_cnd, &draw_mtx);
        draw_display(emu->gfx);
        mtx_unlock(&draw_mtx);
    }
}

int timer_tick_thread(void *arg) {
    chip8emu * emu = (chip8emu*) arg;
    clock_t start_clk = clock();
    clock_t elapsed_clk;
    for (;;) {
        elapsed_clk = clock() - start_clk;
        if (elapsed_clk >= (CLOCKS_PER_SEC/60)) { /* 60Hz */
            chip8emu_timer_tick(emu);
            start_clk = clock();
        }
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv; /* unused variables */

    chip8emu *emu = chip8emu_new();
    emu->draw = &draw_callback;
    emu->beep = &beep;

    int ret = tb_init();
    if (ret) {
        log_error("tb_init() failed with error code %d\n", ret);
        return 1;
    }

    chip8emu_load_rom(emu, "/home/thaolt/Workspaces/roms/UFO");

    draw_all(emu);

    thrd_t thrd_emu;
    thrd_t thrd_timer;
    thrd_t thrd_draw;

    if (thrd_create(&thrd_draw, display_draw_thread, (void*)emu) != thrd_success) {
        log_error("Cannot create draw thread!");
        goto quit;
    }

    if (thrd_create(&thrd_emu, emulator_thread, (void*)emu) != thrd_success) {
        log_error("Cannot create emulator thread!");
        goto quit;
    }

    if (thrd_create(&thrd_timer, timer_tick_thread, (void*)emu) != thrd_success) {
        log_error("Cannot create timer thread!");
        goto quit;
    }

    struct tb_event ev;

    while (true) {
        tb_peek_event(&ev, 50);
        switch (ev.type) {
        case TB_EVENT_KEY:
            switch (ev.key) {
            case TB_KEY_ESC:
                goto quit;
                break;
            case TB_KEY_ENTER:
                mtx_lock(&key_mtx);
                emu->key[0xA] = 1;
                mtx_unlock(&key_mtx);
                break;
            default:
                mtx_lock(&key_mtx);
                switch (ev.ch) {
                case '0':
                    emu->key[0x0] = 1;
                    break;
                case '1':
                    emu->key[0x1] = 1;
                    break;
                case '2':
                    emu->key[0x2] = 1;
                    break;
                case '3':
                    emu->key[0x3] = 1;
                    break;
                case '4':
                    emu->key[0x4] = 1;
                    break;
                case '5':
                    emu->key[0x5] = 1;
                    break;
                case '6':
                    emu->key[0x6] = 1;
                    break;
                case '7':
                    emu->key[0x7] = 1;
                    break;
                case '8':
                    emu->key[0x8] = 1;
                    break;
                case '9':
                    emu->key[0x9] = 1;
                    break;
                case '+':
                    emu->key[0xB] = 1;
                    break;
                case '-':
                    emu->key[0xC] = 1;
                    break;
                case '*':
                    emu->key[0xD] = 1;
                    break;
                case '/':
                    emu->key[0xE] = 1;
                    break;
                case '.':
                    emu->key[0xF] = 1;
                    break;
                default:
                    memset(emu->key, 0, 16);
                }
                mtx_unlock(&key_mtx);
            }
            break;
        case TB_EVENT_RESIZE:
            mtx_lock(&draw_mtx);
            draw_all(emu);
            mtx_unlock(&draw_mtx);
            break;
        }
    }

quit:
    tb_shutdown();
    return 0;
}
