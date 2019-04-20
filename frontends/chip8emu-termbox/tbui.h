#ifndef TBUI_H_
#define TBUI_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TBUI_ALIGN_LEFT,
    TBUI_ALIGN_CENTER,
    TBUI_ALIGN_RIGHT
} tbui_alignment_t;

typedef enum  {
    TBUI_BORDER_BOX,
    TBUI_CONTENT_BOX
} box_style_t;

typedef enum  {
    TBUI_BORDER_SINGLE,
    TBUI_BORDER_DOUBLE
} border_style_t;

typedef struct tbui_widget_t tbui_widget_t;
typedef struct tbui_frame_opt_t tbui_frame_opt_t;
typedef struct tbui_frame_t tbui_frame_t;
typedef struct tbui_bound_t tbui_bound_t;

struct tbui_bound_t {
    int x, y, w, h;
};

struct tbui_widget_t {
    tbui_bound_t bound;
    bool visible;
    void* impl;
    void (*draw)(tbui_widget_t *);
    void (*content_draw)(tbui_widget_t *);
    tbui_widget_t** children;
    int children_count;
};

struct tbui_frame_t {
    const char* title;
    const char* footnote;
    border_style_t border_style;
    tbui_alignment_t title_align;
    tbui_widget_t* widget;
};

void tbui_change_cell(tbui_bound_t* bound, int x, int y, uint32_t ch, uint16_t fg, uint16_t bg);
void tbui_print(tbui_bound_t* bound, const char *str, int x, int y, uint16_t fg, uint16_t bg);
void tbui_printf(tbui_bound_t* bound, int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...);

void tbui_draw_frame(tbui_frame_t* frame);

void tbui_draw_multipanes(uint8_t x, uint8_t y, ...);

tbui_frame_t* tbui_new_frame(void);

void tbui_append_child(tbui_widget_t *parent, tbui_widget_t *child);

int tbui_init(void);
void tbui_shutdown(void);
void tbui_redraw(tbui_widget_t* widget);

#endif /* TBUI_H_ */
