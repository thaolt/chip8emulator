#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h> /* for basename() */

#include "log.h"
#include "termbox.h"
#include "tinycthread.h"
#include "soundio.h"
#include "tbui.h"

#include "chip8emu.h"

#define DISP_FG TB_GREEN
#define DISP_BG TB_BLACK
#define CONTAINER_WIDTH 80
#define CONTAINER_MIN_HEIGHT 25

static mtx_t draw_mtx;
static cnd_t draw_cnd;
static uint8_t keybuffer[0x10] = {0};
static chip8emu_snapshot snapshot;

static tbui_widget_t * container;
static tbui_frame_t *cpu_pane;
static tbui_frame_t *disp_pane;
static tbui_frame_t *keymap_pane;
static tbui_frame_t *opcode_pane;
static tbui_frame_t *logs_pane;
static chip8emu *emu;
static char * basedir;
static int cpu_clk_speed = 500;
static int timer_clk_speed = 60;

static char *default_keymap[0x10] = {
    "1", "2", "3", "4",
    "q", "w", "e", "r",
    "a", "s", "d", "f",
    "z", "x", "c", "v"
};

//static char *game_keymap[0x10] = {
//    0,      0,      0,          0,
//    "up",   "left", "right",    "down",
//    0,      0,      0,          0,
//    0,      0,      0,          0
//};

static char *game_keymap[0x10] = {
    0,      0,      0,          0,
    "w",     "a",   "d",        "s",
    0,      0,      0,          0,
    0,      0,      0,          0
};

static uint32_t game_tb_keymap[0x10] = {
    0,      0,      0,          0,
    TB_KEY_ARROW_UP, TB_KEY_ARROW_LEFT, TB_KEY_ARROW_RIGHT, TB_KEY_ARROW_DOWN,
    0 ,     0,      0,          0,
    0,      0,      0,          0
};

static uint32_t keymap_dispay[0x10] = {
    0,      0,      0,          0,
    'w',    'a',    'd',        's',
    0 ,     0,      0,          0,
    0,      0,      0,          0
};

void cpu_pane_draw_content(tbui_widget_t *widget) {
    for (int i = 0; i < 16; ++i) {
        tbui_printf(widget, 1 + (7 * (i/8)), 9 + i%8, 0, 0, "V%X %02X", i, emu->V[i]);
    }
    tbui_printf(widget, 1, 1, 0, 0, "CLK");
    tbui_printf(widget, 1, 2, 0, 0, " CPU %4dHz", cpu_clk_speed);
    tbui_printf(widget, 1, 3, 0, 0, " TMR %4dHz", timer_clk_speed);

    tbui_printf(widget, 1, 4, 0, 0, "DT #%02X", snapshot.delay_timer);
    tbui_printf(widget, 1, 5, 0, 0, "ST #%02X", snapshot.sound_timer);

    tbui_printf(widget, 1, 6, 0, 0, "PC #%04X", snapshot.pc);
    tbui_printf(widget, 1, 7, 0, 0, " I #%04X", snapshot.I);
    tbui_printf(widget, 1, 8, 0, 0, "SP #%02X", snapshot.sp);
}

void draw_keyboard(tbui_widget_t *widget) {
    /* tbui_change_cell(widget, 0, 0, 0x251C, 0 ,0);
    tbui_change_cell(widget, widget->bound->w - 1, 0, 0x252C, 0 ,0);
    */
    uint16_t fg, bg;
    for (int i = 1; i <= 9; ++i) {
        int x = ((i-1) % 3) * 3 + 1;
        int y = 1 + ((i-1) / 3);
        fg = i % 2 ? TB_WHITE : TB_BLACK;
        bg = i % 2 ? TB_BLACK : TB_WHITE;
        tbui_print(widget, "   ", x, y, fg, bg);
        tbui_change_cell(widget, x + 1, y, keymap_dispay[i], fg, bg);
    }

    fg = TB_BLACK; bg = TB_WHITE;

    tbui_print(widget, "   ", 10, 1, fg, bg);
    tbui_change_cell(widget, 11, 1, keymap_dispay[0xC], fg, bg);

    tbui_print(widget, "   ", 10, 3, fg, bg);
    tbui_change_cell(widget, 11, 3, keymap_dispay[0xE], fg, bg);

    tbui_print(widget, "   ", 1, 4, fg, bg);
    tbui_change_cell(widget, 2, 4, keymap_dispay[0xA], fg, bg);

    tbui_print(widget, "   ", 7, 4, fg, bg);
    tbui_change_cell(widget, 8, 4, keymap_dispay[0xB], fg, bg);


    fg = TB_WHITE; bg = TB_BLACK;

    tbui_print(widget, "   ", 10, 2, fg, bg);
    tbui_change_cell(widget, 11, 2, keymap_dispay[0xD], fg, bg);

    tbui_print(widget, "   ", 10, 4, fg, bg);
    tbui_change_cell(widget, 11, 4, keymap_dispay[0xF], fg, bg);

    tbui_print(widget, "   ", 4, 4, fg, bg);
    tbui_change_cell(widget, 5, 4, keymap_dispay[0x0], fg, bg);
}

void draw_toolbar(tbui_widget_t *widget) {
    tbui_bound_t* bound = tbui_real_bound(widget);
    /* int line1y = bound->h - 2;*/
    int line2y = bound->h - 1;
    int col_w = 14;
    int x = (tb_width() - CONTAINER_WIDTH)/2 - (tb_width() - CONTAINER_WIDTH)%2;
    int col_idx = 0;
    int col = col_idx * col_w;

    tbui_print(widget, " ^X", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "Exit", x + col + 4, line2y, 0, 0);
    col += 9;

    tbui_print(widget, " ^O", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "LdROM", x + col + 4, line2y, 0, 0);
    col += 10;

    tbui_print(widget, " ^P", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "Pause", x + col + 4, line2y, 0, 0);
    col += 10;

    tbui_print(widget, " ^R", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "RST", x + col + 4, line2y, 0, 0);
    col += 8;

    tbui_print(widget, " ^[", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "ClkDn", x + col + 4, line2y, 0, 0);
    col += 10;

    tbui_print(widget, " ^]", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "ClkUp", x + col + 4, line2y, 0, 0);
    col += 10;

    tbui_print(widget, " ^M", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "Sv", x + col + 4, line2y, 0, 0);
    col += 7;

    tbui_print(widget, " ^L", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "Ld", x + col + 4, line2y, 0, 0);
    col += 7;

    tbui_print(widget, " ^H", x + col, line2y, TB_BLACK, TB_WHITE);
    tbui_print(widget, "Help", x + col + 4, line2y, 0, 0);
    col += 9;

    free(bound);
}

void relayout() {

}


void beep_callback() {

}

void draw_callback(chip8emu *emu) {
    (void)emu;
    mtx_lock(&draw_mtx);
    cnd_broadcast(&draw_cnd);
    mtx_unlock(&draw_mtx);
}

bool keystate_callback(chip8emu *emu, uint8_t key) {
    (void)emu;
    bool ret;
    if (keybuffer[key] > 1) keybuffer[key] = 2;
    ret = keybuffer[key] > 0;
    if (ret) keybuffer[key]--;
    return ret;
}

static void setup_ui() {
    /* setup UI widgets */
    tb_clear();

    container = tbui_new_widget(NULL);
    tbui_set_bound(container,
        tb_width()/2 - CONTAINER_WIDTH/2 - CONTAINER_WIDTH%2,
        0, CONTAINER_WIDTH, tb_height()-1);
    tbui_set_visible(container, true);
    tbui_child_append(NULL, container);

    tbui_set_user_draw_func(NULL, &draw_toolbar);

    cpu_pane = tbui_new_frame(container);
    cpu_pane->title = "[ CPU ]";
    cpu_pane->title_align = TBUI_ALIGN_LEFT;
    tbui_set_bound(cpu_pane->widget, 66, 0, 14, 18);
    tbui_set_visible(cpu_pane->widget, true);
    tbui_child_append(container, cpu_pane->widget);
    cpu_pane->widget->custom_draw = &cpu_pane_draw_content;

    disp_pane = tbui_new_frame(NULL);
    disp_pane->title = "[ Display ]";
    disp_pane->title_align = TBUI_ALIGN_LEFT;
    disp_pane->footnote_align = TBUI_ALIGN_RIGHT;
    tbui_set_bound(disp_pane->widget, 0, 0, 66, 18);
    tbui_set_visible(disp_pane->widget, true);
    tbui_child_append(container, disp_pane->widget);
    {
        tbui_monobitmap_t* disp_bitmap = tbui_new_monobitmap(disp_pane->widget);
        tbui_set_bound(disp_bitmap->widget, 1, 1, 64, 16);
        tbui_set_visible(disp_bitmap->widget, true);
        tbui_child_append(disp_pane->widget, disp_bitmap->widget);
        disp_bitmap->real_width = 64;
        disp_bitmap->real_height = 32;
        disp_bitmap->bitmap_style = TBUI_BITMAP_HALF_BLOCK;
        disp_bitmap->fg_color = DISP_FG;
        disp_bitmap->bg_color = DISP_BG;
        disp_bitmap->data = snapshot.gfx;

        tbui_frame_t *pause_frame = tbui_new_frame(disp_pane->widget);
        tbui_set_bound(pause_frame->widget, 28, 7, 10, 3);
        tbui_set_visible(pause_frame->widget, false);
        tbui_child_append(disp_pane->widget, pause_frame->widget);
        {
            tbui_label_t *lbl_pause = tbui_new_label(pause_frame->widget);
            lbl_pause->text = " PAUSED ";
            lbl_pause->widget->visible = true;
            tbui_set_bound(lbl_pause->widget, 1, 1, 0xFF, 0xFF);
            tbui_child_append(pause_frame->widget, lbl_pause->widget);
        }
    }

    keymap_pane = tbui_new_frame(container);
    keymap_pane->title = "[ Keymap ]";
    keymap_pane->title_align = TBUI_ALIGN_LEFT;
    tbui_set_bound(keymap_pane->widget, 0, 18, 14, 6);
    tbui_set_visible(keymap_pane->widget, true);
    tbui_child_append(container, keymap_pane->widget);
    keymap_pane->widget->custom_draw = &draw_keyboard;

    opcode_pane = tbui_new_frame(container);
    opcode_pane->title = "[ OpCodes ]";
    opcode_pane->title_align = TBUI_ALIGN_LEFT;
    tbui_set_bound(opcode_pane->widget, 14, 18, 20, 255);
    tbui_set_visible(opcode_pane->widget, true);
    tbui_child_append(container, opcode_pane->widget);

    logs_pane = tbui_new_frame(container);
    logs_pane->title = "[ Logs ]";
    logs_pane->title_align = TBUI_ALIGN_LEFT;
    tbui_set_bound(logs_pane->widget, 34, 18, 255, 255);
    tbui_set_visible(logs_pane->widget, true);
    tbui_child_append(container, logs_pane->widget);

    tbui_redraw(NULL);
}


int display_draw_thread(void *arg) {
    (void)arg;

    setup_ui();

    uint32_t start_time = (uint32_t)time(NULL);
    uint32_t elapsed_time = (uint32_t)time(NULL) - start_time;
    uint32_t frame_count = 0;
    uint8_t fps = 0;
    char *fps_str = calloc(sizeof (char), 40);
    disp_pane->footnote = fps_str;
    while (true) {
//        mtx_lock(&draw_mtx);
        cnd_wait(&draw_cnd, &draw_mtx);
        chip8emu_take_snapshot(emu, &snapshot);
        tbui_redraw(disp_pane->widget);
        tbui_redraw(cpu_pane->widget);
        tb_present();
        frame_count++;
        elapsed_time = (uint32_t)time(NULL) - start_time;
        if (elapsed_time >= 3) {
            fps = (uint8_t) (((float)frame_count / elapsed_time)+fps)/2;
            sprintf(fps_str, "( %d FPS )", fps);
            start_time = (uint32_t)time(NULL);
            frame_count = 0;
        }
//        mtx_unlock(&draw_mtx);
    }
}

int keypad_thread(void *arg) {
    chip8emu * emu = (chip8emu*) arg;
    bool quit = false;
    struct tb_event ev;
    while (!quit) {
        tb_poll_event(&ev);
        if (ev.type == TB_EVENT_KEY)
            switch (ev.key) {
            case TB_KEY_CTRL_O: {
                bool was_paused = emu->paused;
                if (!was_paused)
                    chip8emu_pause(emu);
                char filepath[1024] = {0};
                int ok = tbui_exdiaglog_openfile(
                    filepath,
                    "[ Open ROM ]",
                    "<ESC>Cancel-<ENTER>Select-<TAB>Switch",
                    basedir, 0
                );
                if (ok) {

                } else {
                    if (!was_paused)
                        chip8emu_resume(emu);
                }
                tbui_redraw(NULL);
                break;
            }
            case TB_KEY_CTRL_X:
                quit = true;
                break;
            case TB_KEY_CTRL_R:
                if (emu->paused) {
                    disp_pane->widget->children[1]->visible = false;
                }
                chip8emu_reset(emu);
                break;
            case TB_KEY_CTRL_RSQ_BRACKET:
                mtx_lock(&draw_mtx);
                cpu_clk_speed += 100;
                chip8emu_set_cpu_speed(emu, cpu_clk_speed);
                mtx_unlock(&draw_mtx);
                break;
            case TB_KEY_CTRL_LSQ_BRACKET:
                mtx_lock(&draw_mtx);
                cpu_clk_speed -= 100;
                chip8emu_set_cpu_speed(emu, cpu_clk_speed);
                mtx_unlock(&draw_mtx);
                break;
            case TB_KEY_CTRL_P:
                if (emu->paused) {
                    disp_pane->widget->children[1]->visible = false;
                    tbui_redraw(NULL);
                    chip8emu_resume(emu);
                } else {
                    chip8emu_pause(emu);
                    disp_pane->widget->children[1]->visible = true;
                    tbui_redraw(NULL);
                }
                break;
            case TB_KEY_CTRL_M:
                break;
            case TB_KEY_CTRL_L:
                break;
            default: {
                for (int i = 0; i < 0x10; ++i) {
                    if (game_keymap[i]) {
                        if (strlen(game_keymap[i])>1) {
                            if (ev.key == game_tb_keymap[i]) {
                                keybuffer[i]++;
                                break;
                            }
                        } else {
                            if ((uint8_t)ev.ch == game_keymap[i][0]) {
                                keybuffer[i]++;
                                break;
                            }
                        }
                    }
                }
            }
            }

        if (ev.type == TB_EVENT_RESIZE) {
            thrd_sleep(&(struct timespec){
                           .tv_sec = 0,
                           .tv_nsec = 500000000
                       }, NULL);
            mtx_lock(&draw_mtx);
            tbui_set_bound(NULL, 0, 0, tb_width(), tb_height());
            tbui_set_bound(
                container,
                tb_width()/2 - CONTAINER_WIDTH/2 - CONTAINER_WIDTH%2,
                0,
                CONTAINER_WIDTH,
                tb_height()-1
            );
            tb_clear(); tbui_redraw(NULL);
            mtx_unlock(&draw_mtx);
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    basedir = dirname(argv[0]);

    thrd_sleep(&(struct timespec){
                   .tv_sec = 0,
                   .tv_nsec = 500000000
               }, NULL);

    int ret = tbui_init();
    if (ret) {
        log_error("tb_init() failed with error code %d\n", ret);
        return 1;
    }

    /*
    if (tb_width() < CONTAINER_WIDTH || tb_height() < CONTAINER_MIN_HEIGHT) {
        log_error("terminal geometry (width x height) has to be at least 92x25.");
        goto quit;
    }
    */

    emu = chip8emu_new();
    emu->draw = &draw_callback;
    emu->keystate= &keystate_callback;
    emu->beep = &beep_callback;

    thrd_t thrd_draw;
    thrd_t thrd_keypad;

    chip8emu_load_rom(emu, "/home/thaolt/Workspaces/roms/TETRIS");
    cpu_clk_speed = 1200;
    chip8emu_set_cpu_speed(emu, cpu_clk_speed);
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
