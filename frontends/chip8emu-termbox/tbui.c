#include "tbui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "termbox.h"


static tbui_widget_t* _root_widget;

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

static const uint32_t hblock_char[] = {
    ' ', /* 00 */
    0x2580, /* ▀ 01 */
    0x2584, /* ▄ 10 */
    0x2588, /* █ 11 */
};
static const uint32_t qblock_char[] = {
    ' ', /* 0000 */
    0x2598, /* ▘ 0001 */
    0x259D, /* ▝ 0010 */
    0x2580, /* ▀ 0011 */
    0x2596, /* ▖ 0100 */
    0x258C, /* ▌ 0101 */
    0x259E, /* ▞ 0110 */
    0x259B, /* ▛ 0111 */
    0x2597, /* ▗ 1000 */
    0x259A, /* ▚ 1001 */
    0x2590, /* ▐ 1010 */
    0x259C, /* ▜ 1011 */
    0x2584, /* ▄ 1100 */
    0x2599, /* ▙ 1101 */
    0x259F, /* ▟ 1110 */
    0x2588, /* █ 1111 */
};

static void _draw_widget_dummy(tbui_widget_t* widget) {
    (void)widget;
}

static void _widget_dtor_dummy(tbui_widget_t* widget) {
    (void)widget;
}

static void _frame_dtor(tbui_widget_t* widget) {
    tbui_frame_t* frame = widget->impl;
    free(frame);
    widget->impl = NULL;
}

int tbui_init()
{
    _root_widget = malloc(sizeof (tbui_widget_t));
    _root_widget->visible = true;
    _root_widget->children = NULL;
    _root_widget->children_count = 0;
    _root_widget->draw = &_draw_widget_dummy;
    _root_widget->dtor = &_widget_dtor_dummy;
    _root_widget->custom_draw = NULL;
    _root_widget->bound = malloc(sizeof (tbui_bound_t));
    _root_widget->bound->x = 0;
    _root_widget->bound->y = 0;
    _root_widget->bound->w = tb_width();
    _root_widget->bound->h = tb_height();
    _root_widget->parent = NULL;
    return tb_init();
}

void tbui_change_cell(tbui_widget_t* widget, int x, int y, uint32_t ch, uint16_t fg, uint16_t bg)
{
    if (!widget) widget = _root_widget;

    tbui_bound_t * real_bound = tbui_real_bound(widget);

    tb_change_cell(real_bound->x + x, real_bound->y + y, ch, fg, bg);

    free(real_bound);
}


void tbui_print(tbui_widget_t *widget, const char *str, int x, int y, uint16_t fg, uint16_t bg)
{
    if (!widget) widget = (_root_widget);
    while (*str) {
        uint32_t uni;
        str += tb_utf8_char_to_unicode(&uni, str);
        tbui_change_cell(widget, x, y, uni, fg, bg);
        x++;
        if (x == widget->bound->w) break;
    }
}

void tbui_printf(tbui_widget_t* widget, int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...)
{
    if (!widget) widget = (_root_widget);
    char buf[4096];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);
    tbui_print(widget, buf, x, y, fg, bg);
}


static void _draw_frame(tbui_widget_t* widget) {
    tbui_frame_t *frame = (tbui_frame_t *)widget->impl;
    tbui_bound_t *bound = tbui_real_bound(widget);
    int w = bound->w;
    int h = bound->h;

    for (int i = 0; i < h - 1; ++i) {
        tbui_change_cell(widget, 0, 0+i, box_drawing[4], 0, 0);
        tbui_change_cell(widget, w-1, i, box_drawing[4], 0, 0);
    }
    for (int i = 0; i < w - 1; ++i) {
        tbui_change_cell(widget, i, 0, box_drawing[1], 0, 0);
        tbui_change_cell(widget, i, h-1, box_drawing[1], 0, 0);
    }
    tbui_change_cell(widget, 0    , 0     , box_drawing[0], 0, 0);
    tbui_change_cell(widget, w-1  , 0     , box_drawing[3], 0, 0);
    tbui_change_cell(widget, 0    , h-1   , box_drawing[5], 0, 0);
    tbui_change_cell(widget, w-1  , h-1   , box_drawing[7], 0, 0);

    if (frame->title) {
        uint8_t title_len = (uint8_t) strlen(frame->title);
        if (title_len) {
            switch (frame->title_align) {
            case TBUI_ALIGN_LEFT:
                tbui_print(widget, frame->title, 1, 0, 0, 0);
                break;
            case TBUI_ALIGN_CENTER:
                tbui_print(widget, frame->title, w/2 - title_len/2 - title_len%2, 0, 0, 0);
                break;
            case TBUI_ALIGN_RIGHT:
                tbui_print(widget, frame->title, w - 1 - title_len, 0, 0, 0);
                break;
            }
        }
    }

    if (frame->footnote) {
        uint8_t footnote_len = (uint8_t) strlen(frame->footnote);
        if (footnote_len) {
            switch (frame->footnote_align) {
            case TBUI_ALIGN_LEFT:
                tbui_print(widget, frame->footnote, 1, h - 1, 0, 0);
                break;
            case TBUI_ALIGN_CENTER:
                tbui_print(widget, frame->footnote,
                           w/2 - footnote_len/2 - footnote_len%2, h - 1, 0, 0);
                break;
            case TBUI_ALIGN_RIGHT:
                tbui_print(widget, frame->footnote, w - 1 - footnote_len, h - 1, 0, 0);
                break;
            }
        }
    }

    free(bound);
}

void tbui_redraw(tbui_widget_t* widget)
{
    if (!widget) {
        widget = _root_widget;
    }

    widget->draw(widget);
    if (widget->custom_draw)
        widget->custom_draw(widget);

    for (int i = 0; i < widget->children_count; ++i) {
        tbui_widget_t *child = widget->children[i];
        if (child->visible)
            tbui_redraw(child);
    }
    tb_present();
}

tbui_frame_t *tbui_new_frame(tbui_widget_t* parent)
{
    tbui_frame_t* frame = malloc(sizeof (tbui_frame_t));

    frame->widget = tbui_new_widget(parent);
    frame->widget->impl = frame;
    frame->widget->draw = &_draw_frame;
    frame->widget->dtor = &_frame_dtor;

    frame->title = NULL;
    frame->footnote = NULL;
    frame->title_align = TBUI_ALIGN_CENTER;
    frame->border_style = TBUI_BORDER_SINGLE;

    return frame;
}

void tbui_shutdown()
{
    tbui_delete(_root_widget);
    tb_shutdown();
}

void tbui_child_append(tbui_widget_t *parent, tbui_widget_t *child)
{
    if (!parent) parent = _root_widget;

    if (!parent->children) {
        parent->children_count++;
        parent->children = malloc(sizeof (tbui_widget_t*));
    } else {
        size_t current_size = sizeof(tbui_widget_t*) * (unsigned long)parent->children_count;
        parent->children_count++;
        tbui_widget_t** new_children = malloc(current_size + sizeof(tbui_widget_t*));
        memcpy(new_children, parent->children, current_size);
        free(parent->children);
        parent->children = new_children;
    }
    child->parent = parent;
    parent->children[parent->children_count - 1] = child;
}

void tbui_child_remove(tbui_widget_t *parent, tbui_widget_t *child)
{
    for (int i = 0; i < parent->children_count; ++i) {
        if (parent->children[i] == child) {
            tbui_widget_t** old_children = parent->children;
            size_t child_size = sizeof(tbui_widget_t*);
            size_t new_size = child_size * ((unsigned long)parent->children_count - 1);
            if (new_size) {
                tbui_widget_t** new_children = malloc(new_size);
                if (i > 0)
                    memcpy(new_children, parent->children, child_size * (unsigned long)i);
                if (i < parent->children_count - 1) {
                    memcpy(
                        new_children + ((unsigned long)i*child_size),
                        parent->children + ((unsigned long)(i+1)* child_size),
                        (unsigned long)(parent->children_count - (i + 1
                                                                  )) * child_size
                    );
                }
                parent->children = new_children;
            }
            free(old_children);
            --parent->children_count;
            break;
        }
    }
}

void tbui_child_delete(tbui_widget_t *parent, tbui_widget_t *child)
{
    tbui_child_remove(parent, child);
    tbui_delete(child);
}

void tbui_draw_hbitmap_mono(tbui_bound_t* bound, uint8_t *buffer, uint16_t fg_color, uint16_t bg_color, int real_width, int real_height)
{
    int offset_x = bound->x;
    int offset_y = bound->y;
    int scaled_width = real_width;
    int scaled_height = real_height / 2 + (real_height % 2);

    for (int line = 0; line < scaled_height; ++line) {
        for (int x = 0; x < scaled_width; ++x) {
            int y = line * 2;
            int y2 = y + 1;
            uint8_t block = 0;
            block |= buffer[y * real_width + x] << 0;
            block |= buffer[y2 * real_width + x] << 1;
            tb_change_cell(x + offset_x, line + offset_y, hblock_char[block], fg_color, bg_color);
        }
    }
}

void tbui_draw_qbitmap_mono(tbui_bound_t* bound, uint8_t *buffer, uint16_t fg_color, uint16_t bg_color, int real_width, int real_height)
{
    int offset_col = bound->x;
    int offset_row = bound->y;

    int real_cols = 64;
//    int real_rows = 32;

    int scaled_cols = 32;
    int scaled_rows = 16;

    for (int row = 0; row < scaled_rows; ++row) {
        for (int col = 0; col < scaled_cols; ++col) {
            int x = col * 2;
            int y = row * 2 * real_cols;
            int x2 = x + 1;
            int y2 = (row * 2 + 1) * real_cols;

            int block = 0;
            block |= buffer[y+x] << 0;
            block |= buffer[y+x2] << 1;
            block |= buffer[y2+x] << 2;
            block |= buffer[y2+x2] << 3;

            tb_change_cell(col + offset_col, row + offset_row, qblock_char[block], fg_color, bg_color);
        }
    }
}

void tbui_draw_brbitmap_mono(tbui_bound_t* bound, uint8_t *buffer, uint16_t fg_color, uint16_t bg_color, int real_width, int real_height)
{
    int offset_col = bound->x + 1;
    int offset_row = bound->y + 1;

    int real_cols = 64;

    int scaled_cols = 32;
    int scaled_rows = 8;

    for (int row = 0; row < scaled_rows; ++row) {
        for (int col = 0; col < scaled_cols; ++col) {
            int x1 = col * 2;
            int y1 = row * 4 * real_cols;
            int x2 = x1 + 1;
            int y2 = (row * 4 + 1) * real_cols;
            int y3 = (row * 4 + 2) * real_cols;
            int y4 = (row * 4 + 3) * real_cols;

            int block = 0;
            block |= buffer[y1+x1] << 0;
            block |= buffer[y2+x1] << 1;
            block |= buffer[y3+x1] << 2;
            block |= buffer[y1+x2] << 3;
            block |= buffer[y2+x2] << 4;
            block |= buffer[y3+x2] << 5;
            block |= buffer[y4+x1] << 6;
            block |= buffer[y4+x2] << 7;

            tb_change_cell(col + offset_col, row + offset_row, 0x2800+(uint32_t)block, fg_color, bg_color);
        }
    }
}

tbui_bound_t* tbui_real_bound(tbui_widget_t *widget)
{
    tbui_bound_t *yield_bound = malloc(sizeof (tbui_bound_t));

    if (widget->parent == 0) {
        memcpy(yield_bound, widget->bound, sizeof (tbui_bound_t));
        return yield_bound;
    }

    tbui_bound_t *parent_bound = tbui_real_bound(widget->parent);
    yield_bound->x = widget->bound->x + parent_bound->x;
    yield_bound->y = widget->bound->y + parent_bound->y;
    yield_bound->w = widget->bound->w;
    yield_bound->h = widget->bound->h;
    free(parent_bound);
    return yield_bound;
}

tbui_widget_t *tbui_new_widget(tbui_widget_t* parent)
{
    if (!parent) parent = _root_widget;
    tbui_widget_t* widget = malloc(sizeof (tbui_widget_t));
    widget->bound = malloc(sizeof (tbui_bound_t));
    widget->bound->x = 0;
    widget->bound->y = 0;
    widget->bound->w = 2;
    widget->bound->h = 2;
    widget->visible = false;
    widget->impl = 0;
    widget->dtor = &_widget_dtor_dummy;
    widget->draw = &_draw_widget_dummy;
    widget->custom_draw = 0;
    widget->parent = parent;
    widget->children = 0;
    widget->children_count = 0;
    return widget;
}

static void _draw_label(tbui_widget_t* widget) {
    tbui_label_t *label = (tbui_label_t *)widget->impl;
    tbui_print(widget, label->text, 0, 0, label->fg_color, label->bg_color);
}

tbui_label_t *tbui_new_label(tbui_widget_t *parent)
{
    tbui_label_t* label = malloc(sizeof (tbui_label_t));

    label->widget = tbui_new_widget(parent);
    label->widget->impl = label;
    label->widget->draw = &_draw_label;

    label->text = 0;
    label->text_align = TBUI_ALIGN_LEFT;
    label->fg_color = TB_DEFAULT;
    label->bg_color = TB_DEFAULT;

    return label;
}

void tbui_set_bound(tbui_widget_t *widget, int x, int y, int w, int h)
{
    widget->bound->x = x;
    widget->bound->y = y;
    widget->bound->w = w;
    widget->bound->h = h;
}

void tbui_set_visible(tbui_widget_t *widget, bool visible)
{
    widget->visible = visible;
}

void tbui_show(tbui_widget_t *widget)
{
    tbui_set_visible(widget, true);
}

void tbui_hide(tbui_widget_t *widget)
{
    tbui_set_visible(widget, false);
}

void tbui_set_user_draw_func(tbui_widget_t *widget, void (*func)(tbui_widget_t *))
{
    widget->custom_draw = func;
}

static void _draw_monobitmap(tbui_widget_t* widget) {
    tbui_monobitmap_t *bitmap = (tbui_monobitmap_t *)widget->impl;
    tbui_bound_t *real_bound = tbui_real_bound(widget);

    switch (bitmap->bitmap_style) {
    case TBUI_BITMAP_QUARTER_BLOCK:
        tbui_draw_qbitmap_mono(real_bound, bitmap->data, bitmap->fg_color, bitmap->bg_color, bitmap->real_width, bitmap->real_height);
        break;
    case TBUI_BITMAP_BRAILLE_BLOCK:
        tbui_draw_brbitmap_mono(real_bound, bitmap->data, bitmap->fg_color, bitmap->bg_color, bitmap->real_width, bitmap->real_height);
        break;
    default: /* TBUI_BITMAP_HALF_BLOCK */
        tbui_draw_hbitmap_mono(real_bound, bitmap->data, bitmap->fg_color, bitmap->bg_color, bitmap->real_width, bitmap->real_height);
        break;
    }
    free(real_bound);
}


tbui_monobitmap_t *tbui_new_monobitmap(tbui_widget_t *parent)
{
    tbui_monobitmap_t* bitmap = malloc(sizeof (tbui_monobitmap_t));

    bitmap->widget = tbui_new_widget(parent);
    bitmap->widget->impl = bitmap;
    bitmap->widget->draw = &_draw_monobitmap;

    bitmap->fg_color = TB_DEFAULT;
    bitmap->bg_color = TB_DEFAULT;
    bitmap->bitmap_style = TBUI_BITMAP_HALF_BLOCK;

    return bitmap;
}

int tbui_delete(tbui_widget_t *widget)
{
    for (int i = 0; i < widget->children_count; ++i) {
        tbui_delete(widget->children[i]);
    }

    widget->dtor(widget);
    free(widget->bound);
    free(widget);
    return 0;
}

static bool _filedialog_dummy_filter(const char* filename) {
    (void)filename;
    return true;
}

void tbui_fill_rect(tbui_widget_t* widget, int x, int y, int w, int h, uint32_t ch, uint16_t fg, uint16_t bg) {
    tbui_bound_t* real_bound = tbui_real_bound(widget);
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            tb_change_cell(real_bound->x + i + x, real_bound->y + j + y, ch, fg, bg);
        }
    }
    free(real_bound);
}

int tbui_exdiaglog_openfile(char *out_filename,
                             const char *frame_title,
                             const char *frame_footnote,
                             char *start_dir,
                             bool (*filter_func)(const char *))
{
    int diag_w = 56;
    int diag_h = 22;
    int retcode = false;
    bool finish = false;
    char * curdir = strdup(start_dir);
    if (!filter_func) filter_func = &_filedialog_dummy_filter;

    /* setup */
    tbui_frame_t *frame = tbui_new_frame(NULL);
    tbui_set_bound(frame->widget,
                   tb_width()/2 - diag_w/2,
                   tb_height()/2 - diag_h/2,
                   diag_w, diag_h);
    frame->title = frame_title;
    frame->footnote = frame_footnote;
    frame->title_align = TBUI_ALIGN_CENTER;
    frame->border_style = TBUI_BORDER_THIN_SHADOW;
    tbui_set_visible(frame->widget, true);
    tbui_child_append(_root_widget, frame->widget);

    struct dirent **namelist;
    int entries_count = scandir(curdir, &namelist, NULL, alphasort);
    tbui_redraw(NULL);
    tbui_fill_rect(frame->widget, 1, 1, diag_w - 2, diag_h - 2, ' ', 0, 0);
    tb_present();

    /* event loop */
    struct tb_event ev;
    while (!finish) {
        tb_poll_event(&ev);
        if (ev.type != TB_EVENT_KEY)
            continue;
        switch (ev.key) {
        case TB_KEY_TAB:
            break;
        case TB_KEY_ENTER:
            break;
        case TB_KEY_ARROW_UP:
            break;
        case TB_KEY_ARROW_DOWN:
            break;
        case TB_KEY_ESC:
            finish = true;
            break;
        }
    }

    tbui_child_delete(_root_widget, frame->widget);
    tb_clear();
    return retcode;
}
