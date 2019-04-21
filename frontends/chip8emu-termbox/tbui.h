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

typedef enum  {
    TBUI_BITMAP_HALF_BLOCK,
    TBUI_BITMAP_QUARTER_BLOCK,
    TBUI_BITMAP_BRAILLE_BLOCK
} bitmap_style_t;

typedef struct tbui_bound_t tbui_bound_t;
typedef struct tbui_widget_t tbui_widget_t;
typedef struct tbui_frame_t tbui_frame_t;
typedef struct tbui_label_t tbui_label_t;
typedef struct tbui_monobitmap_t tbui_monobitmap_t;

struct tbui_bound_t {
    int x, y, w, h;
};

struct tbui_widget_t {
    tbui_bound_t* bound;
    bool visible;
    void* impl;
    void (*draw)(tbui_widget_t *);
    void (*custom_draw)(tbui_widget_t *);
    tbui_widget_t** children;
    tbui_widget_t* parent;
    int children_count;
};

struct tbui_frame_t {
    const char* title;
    const char* footnote;
    border_style_t border_style;
    tbui_alignment_t title_align;
    tbui_widget_t* widget;
};

struct tbui_label_t {
    const char* text;
    tbui_alignment_t text_align;
    uint16_t fg_color;
    uint16_t bg_color;
    tbui_widget_t* widget;
};

struct tbui_monobitmap_t {
    int real_width, real_height;
    uint16_t fg_color;
    uint16_t bg_color;
    uint8_t *data;
    bitmap_style_t bitmap_style;
    tbui_widget_t* widget;
};

void tbui_change_cell(tbui_widget_t* widget, int x, int y, uint32_t ch, uint16_t fg, uint16_t bg);
void tbui_print(tbui_widget_t* widget, const char *str, int x, int y, uint16_t fg, uint16_t bg);
void tbui_printf(tbui_widget_t* widget, int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...);

/* draw using half blocks characters */
void tbui_draw_hbitmap_mono(tbui_bound_t* bound, uint8_t *buffer, uint16_t fg_color, uint16_t bg_color, int real_width, int real_height);
/* draw using quarter blocks characters */
void tbui_draw_qbitmap_mono(tbui_bound_t* bound, uint8_t *buffer, uint16_t fg_color, uint16_t bg_color, int real_width, int real_height);
/* draw using braille characters */
void tbui_draw_brbitmap_mono(tbui_bound_t* bound, uint8_t *buffer, uint16_t fg_color, uint16_t bg_color, int real_width, int real_height);

tbui_widget_t* tbui_new_widget(tbui_widget_t* parent);
tbui_bound_t* tbui_real_bound(tbui_widget_t *widget);
void tbui_set_bound(tbui_widget_t *widget, int x, int y, int w, int h);
void tbui_show(tbui_widget_t* widget);
void tbui_hide(tbui_widget_t* widget);
void tbui_set_visible(tbui_widget_t* widget, bool visible);
void tbui_set_draw_func(tbui_widget_t* widget, void (*func)(tbui_widget_t *));

tbui_frame_t* tbui_new_frame(tbui_widget_t* parent);
tbui_label_t* tbui_new_label(tbui_widget_t* parent);
tbui_monobitmap_t* tbui_new_monobitmap(tbui_widget_t* parent);

void tbui_append_child(tbui_widget_t *parent, tbui_widget_t *child);
void tbui_remove_child(tbui_widget_t *parent, tbui_widget_t *child);

int tbui_init(void);
void tbui_shutdown(void);
void tbui_redraw(tbui_widget_t* widget);

#endif /* TBUI_H_ */
