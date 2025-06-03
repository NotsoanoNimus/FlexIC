//
// Created by puhlz on 5/30/25.
//

#include "widget_common.h"


static void
needle_meter__minimalistic__update(widget_t *self)
{
    // TODO: Testing. Remove.
    for (int i = 0; i < self->num_parent_signals; ++i) {
        if (self->parent_signals[i]->real_time_data.has_update) {
            DPRINTLN("[%s] SIGNAL RAW DATA (CHANNEL%u: %s): ", self->label, i, self->parent_signals[i]->name);
            MEMDUMP(&(self->parent_signals[i]->real_time_data.value), 8);
        }
    }
}


static void
needle_meter__minimalistic__draw(widget_t *self, const renderer_t *renderer)
{
    DrawEllipse(300, 300, 20, 80, RAYWHITE);
}


static ic_err_t
needle_meter__minimalistic__init(widget_t *self, const renderer_t *renderer)
{
    return ERR_OK;
}


static ic_err_t
needle_meter__minimalistic__parse_args(widget_t *self)
{
    return ERR_OK;
}
