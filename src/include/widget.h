//
// Created by puhlz on 5/28/25.
//

#ifndef WIDGET_H
#define WIDGET_H

#include "flex_ic.h"
#include "renderer.h"



/* Forward declaration. */
typedef struct widget widget_t;

/* Widget renderer method to initialize a display item. */
typedef
ic_err_t (*_func__widget_init)(
    widget_t *self,
    vehicle_data_t *current_data,
    char *param_string   /* colon-separated string delimiting widget-specific params */
);

/* Widget renderer method to fully redraw a display item. */
typedef
ic_err_t (*_func__widget_redraw)(
    widget_t *self,
    vehicle_data_t *current_data
);

/* Widget renderer method to update/draw a display item (can be partial). */
typedef
ic_err_t (*_func__widget_draw)(
    widget_t *self,
    vehicle_data_t *current_data
);

/* Widget state. */
typedef
struct {
    vec2_t position;
    vec2_t resolution;
    uint32_t z_index;   /* control widget rendering orders */
    volatile bool visible;   /* whether to draw the widget */
    // volatile bool refresh_me;   /* marked as 'true' by the CAN bus thread to indicate a pending redraw (if visible) */
    volatile void *internal;   /* state object custom to the widget instance and type */
} widget_state_t;


/* Widget renderer main 'object'. */
struct widget
{
    _func__widget_init      init;
    _func__widget_redraw    redraw;
    _func__widget_draw      draw;

    const char *label;
    const char *type;
    dbc_signal_t *parent_signal;
    widget_state_t state;
};

/* Macro to free a widget. */
#define FREE_WIDGET(w) \
    if (NULL != w) free((w)->internal); free(w); w = NULL;


extern widget_t **global_widgets;


ic_err_t load_widgets(char *mutable_configuration);



#endif   /* WIDGET_H */
