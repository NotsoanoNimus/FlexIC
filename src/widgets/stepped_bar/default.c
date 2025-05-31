//
// Created by puhlz on 5/30/25.
//

#include "widget_common.h"

#include <raylib.h>


static void
stepped_bar__default__update(widget_t *self)
{
    // TODO: Fix this once signal parsing works.
    MY_X = x_pos->value;
    MY_Y = y_pos->value;
    MY_ANGLE = rotation->value;

    for (int i = 0; i < self->num_parent_signals; ++i) {
        if (self->parent_signals[i]->real_time_data.has_update) {
            DPRINTLN("[%s] SIGNAL RAW DATA (CHANNEL%u: %s): ", self->label, i, self->parent_signals[i]->name);
            MEMDUMP(&(self->parent_signals[i]->real_time_data.value), 8);
            DPRINTLN(">>>>> (%u, %u, %f deg)", MY_X, MY_Y, MY_ANGLE);
        }
    }
}


static void
stepped_bar__default__draw(widget_t *self)
{
    // TODO: Just an example.
    DrawRectanglePro(
        (Rectangle){ MY_X, MY_Y, MY_WIDTH, MY_HEIGHT },
        (Vector2){ (MY_WIDTH/2), (MY_HEIGHT/2) },
        MY_ANGLE,
        ORANGE
    );
}


static ic_err_t
stepped_bar__default__parse_args(widget_t *self)
{
    return ERR_OK;
}
