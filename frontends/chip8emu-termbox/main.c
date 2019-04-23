#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#include "log.h"
#include "termbox.h"
#include "tinycthread.h"
#include "soundio.h"
#include "tbui.h"

#include "chip8emu.h"

#define DISP_FG TB_CYAN
#define DISP_BG TB_BLACK
#define CONTAINER_WIDTH 92
#define CONTAINER_MIN_HEIGHT 25

static mtx_t draw_mtx;
static cnd_t draw_cnd;
static uint8_t keybuffer[0x10] = {0};
static chip8emu_snapshot snapshot;

static tbui_widget_t * container;
static tbui_frame_t *cpu_pane;
static tbui_frame_t *disp_pane;
static chip8emu *emu;

void cpu_pane_draw_content(tbui_widget_t *widget) {
    for (int i = 0; i < 16; ++i) {
        tbui_printf(widget, 17, 1 + i, 0, 0, " V%X %02X", i, emu->V[i]);
    }
    tbui_printf(widget, 2, 1, 0, 0, "CLK");
    tbui_printf(widget, 2, 2, 0, 0, "  CPU %4dHz", chip8emu_get_cpu_speed(emu));
    tbui_printf(widget, 2, 3, 0, 0, "  TMR %4dHz", chip8emu_get_timer_speed(emu));

    tbui_printf(widget, 2, 4, 0, 0, "DT #%02X", snapshot.delay_timer);
    tbui_printf(widget, 2, 5, 0, 0, "ST #%02X", snapshot.sound_timer);

    tbui_printf(widget, 2, 6, 0, 0, "PC #%04X", snapshot.pc);
    tbui_printf(widget, 2, 7, 0, 0, " I #%04X", snapshot.I);
    tbui_printf(widget, 2, 8, 0, 0, "SP #%02X", snapshot.sp);
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
        0, CONTAINER_WIDTH, tb_height());
    tbui_set_visible(container, true);
    tbui_child_append(NULL, container);

    cpu_pane = tbui_new_frame(container);
    cpu_pane->title = "[ CPU ]";
    cpu_pane->title_align = TBUI_ALIGN_LEFT;
    tbui_set_bound(cpu_pane->widget, 66, 0, 26, 18);
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
        mtx_lock(&draw_mtx);
        cnd_wait(&draw_cnd, &draw_mtx);
        chip8emu_take_snapshot(emu, &snapshot);
        tbui_redraw(NULL);
        tb_present();
        frame_count++;
        elapsed_time = (uint32_t)time(NULL) - start_time;
        if (elapsed_time >= 3) {
            fps = (uint8_t) (((float)frame_count / elapsed_time)+fps)/2;
            sprintf(fps_str, "( %d FPS )", fps);
            start_time = (uint32_t)time(NULL);
            frame_count = 0;
        }
        mtx_unlock(&draw_mtx);
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
            case TB_KEY_CTRL_O:
                break;
            case TB_KEY_CTRL_X:
                quit = true;
                break;
            case TB_KEY_CTRL_R:
                chip8emu_reset(emu);
                break;
            case TB_KEY_CTRL_RSQ_BRACKET:
                chip8emu_set_cpu_speed(emu, chip8emu_get_cpu_speed(emu) + 100);
                break;
            case TB_KEY_CTRL_LSQ_BRACKET:
                chip8emu_set_cpu_speed(emu, chip8emu_get_cpu_speed(emu) - 100);
                break;
            case TB_KEY_SPACE:
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
            case TB_KEY_ARROW_UP:
                keybuffer[0x4]++;
                break;
            case TB_KEY_ARROW_DOWN:
                keybuffer[0x7]++;
                break;
            case TB_KEY_ARROW_LEFT:
                keybuffer[0x5]++;
                break;
            case TB_KEY_ARROW_RIGHT:
                keybuffer[0x6]++;
                break;
            default:
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
                }
                break;
            }

        if (ev.type == TB_EVENT_RESIZE) {
            mtx_lock(&draw_mtx);
            tbui_set_bound(
                container,
                tb_width()/2 - CONTAINER_WIDTH/2 - CONTAINER_WIDTH%2,
                0,
                CONTAINER_WIDTH,
                tb_height()
            );
            tb_clear(); tbui_redraw(NULL);
            mtx_unlock(&draw_mtx);
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

    /*
    if (tb_width() < CONTAINER_WIDTH || tb_height() < CONTAINER_MIN_HEIGHT) {
        log_error("terminal geometry (width x height) has to be at least 92x25 column.");
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
    chip8emu_set_cpu_speed(emu, 1200);
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
