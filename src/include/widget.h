//
// Created by puhlz on 5/28/25.
//

#ifndef WIDGET_H
#define WIDGET_H

#include "flex_ic.h"
#include "renderer.h"



/* Forward declaration. */
typedef struct widget widget_t;

/* Widget renderer method to update/draw a display item (can be partial). */
typedef
ic_err_t (*_func__widget_draw)(
    widget_t *self
);

/* Widget state. */
typedef
struct {
    vec2_t position;
    vec2_t resolution;
    uint32_t z_index;   /* control widget rendering orders */
    volatile bool visible;   /* whether to draw the widget */
    volatile void *internal;   /* state object custom to the widget instance and type */
} widget_state_t;


/* Widget renderer main 'object'. */
struct widget
{
    _func__widget_draw      draw;

    const char *label;
    const char *type;
    char *param_string;
    dbc_signal_t *parent_signal;
    widget_state_t state;
};


/* Widget factory prototype for registered widget_types. */
typedef
ic_err_t (*func__widget_factory_create)(
    widget_t *self,
    real_time_data_t *real_time_data_reference,
    uint32_t num_real_time_data_references
);


extern widget_t **global_widgets;
extern uint32_t num_global_widgets;


ic_err_t load_widgets(char *mutable_configuration);


/* Various widget macros to use for shorthand. */
#define MY_X self->state.position.x
#define MY_Y self->state.position.y
#define MY_WIDTH self->state.resolution.x
#define MY_HEIGHT self->state.resolution.y
#define MY_Z_INDEX self->state.z_index



#endif   /* WIDGET_H */
