#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "termbox.h"
#include "tinycthread.h"
#include "soundio.h"
#include "tbui.h"

#include "chip8emu.h"

#define DISPLAY_WIDTH   64
#define DISPLAY_HEIGHT  16 /* x 2 */

#define DISP_FG TB_CYAN
#define DISP_BG TB_BLACK

static const uint32_t box_drawing[] = {
/*0*/    0x250C, /* ┌ */
/*1*/    0x2500, /* ─ */
/*2*/    0x252C, /* ┬ */
/*3*/    0x2510, /* ┐ */
/*4*/    0x2502, /* │ */
/*5*/    0x2514, /* └ */
/*6*/    0x2534, /* ┴ */
/*7*/    0x2518, /* ┘ */
};

static const uint32_t block_char[] = {
    ' ',
    0x2580, /* ▀ */
    0x2584, /* ▄ */
    0x2588, /* █ */
};

static mtx_t draw_mtx;
static cnd_t draw_cnd;
static uint8_t keybuffer[0x10] = {0};

static tbui_frame_t *reg_pane;
static tbui_frame_t *cpu_pane;
static tbui_frame_t *disp_pane;
static chip8emu *emu;


void disp_pane_draw_content(tbui_widget_t *widget) {
    uint8_t *buffer = emu->gfx;
    int offset_x = widget->bound.x + 1;
    int offset_y = widget->bound.y + 1;

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

void reg_pane_draw_content(tbui_widget_t *widget) {
    for (int i = 0; i < 16; ++i) {
        tbui_printf(&widget->bound, 2, 1 + i, 0, 0, " V%X %02X", i, emu->V[i]);
    }
}

void cpu_pane_draw_content(tbui_widget_t *widget) {
    tbui_bound_t* bound = &widget->bound;
    tbui_printf(bound, 2, 1, 0, 0, "CLK");
    tbui_printf(bound, 2, 2, 0, 0, "  CPU %4dHz", chip8emu_get_cpu_speed(emu));
    tbui_printf(bound, 2, 3, 0, 0, "  TMR %4dHz", chip8emu_get_timer_speed(emu));

    tbui_printf(bound, 2, 4, 0, 0, "DT #%02X", emu->delay_timer);
    tbui_printf(bound, 2, 5, 0, 0, "ST #%02X", emu->sound_timer);

    tbui_printf(bound, 2, 6, 0, 0, "PC #%04X", emu->pc);
    tbui_printf(bound, 2, 7, 0, 0, " I #%04X", emu->I);
    tbui_printf(bound, 2, 8, 0, 0, "SP #%02X", emu->sp);
}

void draw_keyboard() {

}


void beep_callback() {

}

void draw_callback(chip8emu *emu) {
    (void)emu;
    mtx_lock(&draw_mtx);
    cnd_signal(&draw_cnd);
    mtx_unlock(&draw_mtx);
}

bool keystate_callback(uint8_t key) {
    bool ret;
    if (keybuffer[key] > 3) keybuffer[key] = 3;
    ret = keybuffer[key] > 0;
    if (ret) keybuffer[key]--;
    return ret;
}


int display_draw_thread(void *arg) {
    (void)arg;
    tb_clear();

    reg_pane = tbui_new_frame();
    reg_pane->title = "Registers";
    reg_pane->title_align = TBUI_ALIGN_LEFT;
    reg_pane->widget->bound = (tbui_bound_t) {66, 0, 11, 18};
    reg_pane->widget->visible = true;
    reg_pane->widget->content_draw = &reg_pane_draw_content;
    tbui_append_child(NULL, reg_pane->widget);

    cpu_pane = tbui_new_frame();
    cpu_pane->title = "CPU";
    cpu_pane->title_align = TBUI_ALIGN_LEFT;
    cpu_pane->widget->bound = (tbui_bound_t) {77, 0, 16, 18};
    cpu_pane->widget->visible = true;
    cpu_pane->widget->content_draw = &cpu_pane_draw_content;
    tbui_append_child(NULL, cpu_pane->widget);

    disp_pane = tbui_new_frame();
    disp_pane->title = "Display";
    disp_pane->title_align = TBUI_ALIGN_LEFT;
    disp_pane->widget->bound = (tbui_bound_t) {30, 18, 66, 18};
    disp_pane->widget->visible = true;
    disp_pane->widget->content_draw = &disp_pane_draw_content;
    tbui_append_child(NULL, disp_pane->widget);

    tbui_redraw(NULL);

    while (true) {
        mtx_lock(&draw_mtx);
        cnd_wait(&draw_cnd, &draw_mtx);
        tbui_redraw(NULL);
        tb_present();
        mtx_unlock(&draw_mtx);
    }
}

int keypad_thread(void *arg) {
    chip8emu * emu = (chip8emu*) arg;
    bool quit = false;
    struct tb_event ev;
    while (!quit) {
	tb_peek_event(&ev, 10);
	if (ev.type == TB_EVENT_KEY)
        switch (ev.ch) {
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
        case 'r':
            chip8emu_reset(emu);
            break;
        case ']':
            chip8emu_set_cpu_speed(emu, chip8emu_get_cpu_speed(emu) + 100);
            break;
        case '[':
            chip8emu_set_cpu_speed(emu, chip8emu_get_cpu_speed(emu) - 100);
            break;
        case 'p':
            if (emu->paused)
                chip8emu_resume(emu);
            else {
                chip8emu_pause(emu);
                tbui_frame_t *pause_frame = tbui_new_frame();
                pause_frame->widget->bound = (tbui_bound_t) {28, 7, 10, 3};
                pause_frame->widget->visible = true;
                tbui_append_child(NULL, pause_frame->widget);

                tbui_print(NULL, " PAUSED ", 29, 8, 0, 0);

                tbui_frame_t *open_frame = tbui_new_frame();
                open_frame->widget->bound = (tbui_bound_t) {28, 20, 30, 10};
                open_frame->title = "Open ROMS";
                open_frame->footnote = "(*) New slot";
                open_frame->widget->visible = true;
                open_frame->title_align = TBUI_ALIGN_RIGHT;
                tbui_append_child(NULL, open_frame->widget);

                tbui_redraw(NULL);
            }
            break;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv; /* unused variables */

    int ret = tbui_init();
    if (ret) {
        log_error("tb_init() failed with error code %d\n", ret);
        return 1;
    }

    emu = chip8emu_new();
    emu->draw = &draw_callback;
    emu->keystate= &keystate_callback;
    emu->beep = &beep_callback;

    thrd_t thrd_draw;
    thrd_t thrd_keypad;

    chip8emu_load_rom(emu, "/home/thaolt/Workspaces/roms/TETRIS");
    chip8emu_set_cpu_speed(emu, 1000);
    chip8emu_start(emu);

    if (thrd_create(&thrd_draw, display_draw_thread, (void*)emu) != thrd_success) {
        log_error("Cannot create draw thread!");
        goto quit;
    }

    if (thrd_create(&thrd_keypad, keypad_thread, (void*)emu) != thrd_success) {
        log_error("Cannot create keypad thread!");
        goto quit;
    }

    thrd_join(thrd_keypad, NULL);

quit:
    tbui_shutdown();
    return 0;
}
