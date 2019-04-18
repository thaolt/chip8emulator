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

#define DISP_FG TB_CYAN
#define DISP_BG TB_BLACK

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

static mtx_t draw_mtx;
static mtx_t timer_mtx;
static mtx_t emu_cycle_mtx;

static cnd_t draw_cnd;
static cnd_t clk_timer_cnd;
static cnd_t clk_emu_cnd;

static uint8_t keybuffer[0x10] = {0};

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
                tb_change_cell(x + offset_x, line + offset_y, block_char[3], DISP_FG, DISP_BG);
            } else if (buffer[y2*64+x] != 0) {
                tb_change_cell(x + offset_x, line + offset_y, block_char[2], DISP_FG, DISP_BG);
            } else if (buffer[y*64+x] != 0) {
                tb_change_cell(x + offset_x, line + offset_y, block_char[1], DISP_FG, DISP_BG);
            } else {
                tb_change_cell(x + offset_x, line + offset_y, block_char[0], DISP_FG, DISP_BG);
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
}

void draw_all(chip8emu* emu) {
    tb_clear();
    draw_layout();
    draw_display(emu->gfx);
    draw_registers(emu);
    tb_present();
}

int emulator_thread(void* arg) {
    chip8emu * emu = (chip8emu*) arg;
    while (true) {
        mtx_lock(&emu_cycle_mtx);
        cnd_wait(&clk_emu_cnd, &emu_cycle_mtx);
        chip8emu_exec_cycle(emu);
        mtx_unlock(&emu_cycle_mtx);
    }
}

void draw_callback(chip8emu *emu) {
    (void)emu;
}

bool keystate_callback(uint8_t key) {
    bool ret;
    if (keybuffer[key] > 3) keybuffer[key] = 3;
    ret = keybuffer[key] > 0;
    if (ret) keybuffer[key]--;
    return ret;
}

int display_draw_thread(void *arg) {
    chip8emu * emu = (chip8emu*) arg;

    draw_all(emu);

    uint8_t counter = 0;

    while (true) {
        mtx_lock(&draw_mtx);
        cnd_wait(&draw_cnd, &draw_mtx);

        counter++;
        if (counter == 2) {
            draw_display(emu->gfx);
            draw_registers(emu);
            tb_present();
            counter = 0;
        }

        mtx_unlock(&draw_mtx);
    }

}

int timer_tick_thread(void *arg) {
    chip8emu * emu = (chip8emu*) arg;
    while (true) {
        mtx_lock(&timer_mtx);
        cnd_wait(&clk_timer_cnd, &timer_mtx);
        chip8emu_timer_tick(emu);
        mtx_unlock(&timer_mtx);
    }
}

int keypad_thread(void *arg) {
    (void)arg;
    bool quit = false;
    int c = getchar();
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 10000000
    };
    while (!quit) {
        thrd_sleep(&delay, 0);
        switch (c) {
        case '0':
            keybuffer[0x0]++;
            break;
        case '1':
            keybuffer[0x1]++;
            break;
        case '2':
            keybuffer[0x2]++;
            break;
        case '3':
            keybuffer[0x3]++;
            break;
        case '4':
            keybuffer[0x4]++;
            break;
        case '5':
            keybuffer[0x5]++;
            break;
        case '6':
            keybuffer[0x6]++;
            break;
        case '7':
            keybuffer[0x7]++;
            break;
        case '8':
            keybuffer[0x8]++;
            break;
        case '9':
            keybuffer[0x9]++;
            break;
        case '+':
            keybuffer[0xB]++;
            break;
        case '-':
            keybuffer[0xC]++;
            break;
        case '*':
            keybuffer[0xD]++;
            break;
        case '/':
            keybuffer[0xE]++;
            break;
        case '.':
            keybuffer[0xF]++;
            break;
        case 'q':
            quit = true;
            break;
        }
        c = getchar();
    }
    return 0;
}

int clk_timer_thread(void *arg) {
    (void)arg;
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 16666666
    };
    while (true) {
        thrd_sleep(&delay, 0);
        cnd_signal(&clk_timer_cnd);
    }
}

int clk_emu_cycle_thread(void *arg) {
    (void)arg;
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000
    };
    while (true) {
        thrd_sleep(&delay, 0);
        cnd_signal(&clk_emu_cnd);
        mtx_lock(&draw_mtx);
        cnd_signal(&draw_cnd);
        mtx_unlock(&draw_mtx);
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv; /* unused variables */

    int ret = tb_init();
    if (ret) {
        log_error("tb_init() failed with error code %d\n", ret);
        return 1;
    }

    chip8emu *emu = chip8emu_new();
    emu->draw = &draw_callback;
    emu->keystate= &keystate_callback;
    emu->beep = &beep;

    chip8emu_load_rom(emu, "/home/thaolt/Workspaces/roms/TETRIS");

    thrd_t thrd_emu;
    thrd_t thrd_timer;
    thrd_t thrd_draw;
    thrd_t thrd_keypad;

    /* clock threads */
    thrd_t thrd_timer_clk;
    thrd_t thrd_emu_cycle_clk;

    if (thrd_create(&thrd_timer_clk, clk_timer_thread, (void*)0) != thrd_success) {
        log_error("Cannot create timer clock thread!");
        goto quit;
    }

    if (thrd_create(&thrd_emu_cycle_clk, clk_emu_cycle_thread, (void*)0) != thrd_success) {
        log_error("Cannot create emu cycle timer clock thread!");
        goto quit;
    }

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

    if (thrd_create(&thrd_keypad, keypad_thread, (void*)0) != thrd_success) {
        log_error("Cannot create keypad thread!");
        goto quit;
    }

    thrd_join(thrd_keypad, NULL);

quit:
    tb_shutdown();
    return 0;
}
