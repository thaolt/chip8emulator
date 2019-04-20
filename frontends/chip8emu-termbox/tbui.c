#include "tbui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "termbox.h"


static tbui_widget_t _root_widget;

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

int tbui_init()
{
    _root_widget.visible = true;
    _root_widget.children = 0;
    _root_widget.children_count = 0;
    _root_widget.draw = 0;
    _root_widget.content_draw = 0;
    _root_widget.bound.x = 0;
    _root_widget.bound.y = 0;
    _root_widget.bound.w = tb_width();
    _root_widget.bound.h = tb_width();

    return tb_init();
}

void tbui_change_cell(tbui_bound_t *bound, int x, int y, uint32_t ch, uint16_t fg, uint16_t bg)
{
    if (!bound) bound = &(_root_widget.bound);
    tb_change_cell(bound->x + x, bound->y + y, ch, fg, bg);
}


void tbui_print(tbui_bound_t* bound, const char *str, int x, int y, uint16_t fg, uint16_t bg)
{
    if (!bound) bound = &(_root_widget.bound);
    while (*str) {
        uint32_t uni;
        str += tb_utf8_char_to_unicode(&uni, str);
        tbui_change_cell(bound, x, y, uni, fg, bg);
        x++;
        if (x == bound->w) break;
    }
}

void tbui_printf(tbui_bound_t* bound, int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...)
{
    if (!bound) bound = &(_root_widget.bound);
    char buf[4096];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);
    tbui_print(bound, buf, x, y, fg, bg);
}


static void _draw_frame(tbui_widget_t* widget) {
    tbui_frame_t *frame = (tbui_frame_t *)widget->impl;
    tbui_bound_t *bound = &widget->bound;
    int w = bound->w;
    int h = bound->h;

    for (int i = 0; i < h - 1; ++i) {
        tbui_change_cell(bound, 0, 0+i, box_drawing[4], 0, 0);
        tbui_change_cell(bound, w-1, i, box_drawing[4], 0, 0);
    }
    for (int i = 0; i < w - 1; ++i) {
        tbui_change_cell(bound, i, 0, box_drawing[1], 0, 0);
        tbui_change_cell(bound, i, h-1, box_drawing[1], 0, 0);
    }
    tbui_change_cell(bound, 0    , 0     , box_drawing[0], 0, 0);
    tbui_change_cell(bound, w-1  , 0     , box_drawing[3], 0, 0);
    tbui_change_cell(bound, 0    , h-1   , box_drawing[5], 0, 0);
    tbui_change_cell(bound, w-1  , h-1   , box_drawing[7], 0, 0);

    if (frame->title) {
        uint8_t title_len = (uint8_t) strlen(frame->title);
        if (title_len) {
            switch (frame->title_align) {
            case TBUI_ALIGN_LEFT:
                tbui_print(bound, frame->title, 1, 0, 0, 0);
                break;
            case TBUI_ALIGN_CENTER:
                tbui_print(bound, frame->title, w/2 - title_len/2 - title_len%2, 0, 0, 0);
                break;
            case TBUI_ALIGN_RIGHT:
                tbui_print(bound, frame->title, w - 1 - title_len, 0, 0, 0);
                break;
            }
        }
    }

    if (frame->footnote && strlen(frame->footnote) > 0) {
        tbui_print(&widget->bound, frame->footnote, 1, h - 1, 0, 0);
    }

    if (widget->content_draw) widget->content_draw(widget);
}

void tbui_draw_frame(tbui_frame_t* frame)
{
    _draw_frame(frame->widget);
}

void tbui_redraw(tbui_widget_t* widget)
{
    if (!widget)
        widget = &_root_widget;
    else {
        widget->draw(widget);
    }
    for (int i = 0; i < widget->children_count; ++i) {
        tbui_widget_t *child = _root_widget.children[i];
        if (child->visible) tbui_redraw(child);
    }
    tb_present();
}

tbui_frame_t *tbui_new_frame()
{
    tbui_frame_t* frame = malloc(sizeof (tbui_frame_t));
    frame->title = 0;
    frame->footnote = 0;
    frame->title_align = TBUI_ALIGN_CENTER;
    frame->border_style = TBUI_BORDER_SINGLE;
    frame->widget = malloc(sizeof (tbui_widget_t));
    frame->widget->bound.x = 0;
    frame->widget->bound.y = 0;
    frame->widget->bound.w = 2;
    frame->widget->bound.h = 2;
    frame->widget->visible = false;
    frame->widget->impl = frame;
    frame->widget->draw = &_draw_frame;
    frame->widget->content_draw = 0;
    frame->widget->children = 0;
    frame->widget->children_count = 0;
    return frame;
}

void tbui_shutdown()
{
    tb_shutdown();
}

void tbui_append_child(tbui_widget_t *parent, tbui_widget_t *child)
{
    if (!child) return;

    if (!parent) parent = &_root_widget;

    if (!parent->children) {
        parent->children_count++;
        parent->children = malloc(sizeof (tbui_widget_t*));
    } else {
        size_t current_size = sizeof(tbui_widget_t*) * (unsigned long)parent->children_count;
        parent->children_count++;
        tbui_widget_t** new_children = malloc(current_size + sizeof(tbui_widget_t*));
        memcpy(new_children, parent->children, current_size);
    }
    parent->children[parent->children_count - 1] = child;
}

