//
// Created by puhlz on 5/28/25.
//

#ifndef WIDGET_H
#define WIDGET_H

#include "flex_ic.h"
#include "renderer.h"



/* Forward declaration. */
typedef struct widget widget_t;

/* Widget one-time hook to parse widget_opts into usable per-skin features/options.
    NOTE: we try to enforce this as a one-time method because it's called implicitly during
    skin setup, a la 'set_hooks_for_skin' */
typedef
ic_err_t (*_func__widget_parse_args)(
    widget_t *self
);

/* Widget renderer method to update a display item (NOT draw components to the renderer). */
typedef
void (*_func__widget_update)(
    widget_t *self
);

/* Widget renderer method to draw a display item inside the renderer loop. */
typedef
void (*_func__widget_draw)(
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
    _func__widget_update        update;
    _func__widget_draw          draw;

    const char *label;
    const char *type;
    int argc;
    char **argv;
    char *skin_name;
    dbc_signal_t **parent_signals;
    uint32_t num_parent_signals;
    widget_state_t state;
};


/* Widget factory prototype for registered widget_types. */
typedef
ic_err_t (*func__widget_factory_create)(
    widget_t *self
);


extern widget_t **global_widgets;
extern uint32_t num_global_widgets;


ic_err_t load_widgets(char *mutable_configuration);


/* Various widget macros and functions to use for shorthanding or common operations. */
#define MY_X self->state.position.x
#define MY_Y self->state.position.y
#define MY_WIDTH self->state.resolution.x
#define MY_HEIGHT self->state.resolution.y
#define MY_Z_INDEX self->state.z_index

extern const char *widget_default_skin_name;
#define DEFAULT_SKIN widget_default_skin_name

#define REGISTER_SKIN(widget, name) \
    set_hooks_for_skin( \
        self, \
        AS_LITERAL(name), \
        widget##__##name##__update, \
        widget##__##name##__draw, \
        widget##__##name##__parse_args \
    );

#define CHANNEL(x) self->parent_signals[(x)]->real_time_data

void init_channel(widget_t *self, int channel_number, real_time_data_t **out);

void set_hooks_for_skin(
    widget_t *self,
    const char *skin_name,
    _func__widget_update update_hook,
    _func__widget_draw draw_hook,
    _func__widget_parse_args parse_args_hook
);



#endif   /* WIDGET_H */
