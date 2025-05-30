//
// Created by puhlz on 5/30/25.
//

#include "widget.h"


static void
needle_meter__minimalistic__update(widget_t *self)
{
    // TODO: Testing. Remove.
    for (int i = 0; i < self->num_parent_signals; ++i) {
        if (self->parent_signals[i]->real_time_data.has_update) {
            DPRINTLN("[%s] SIGNAL RAW DATA (CHANNEL%u: %s): ", self->label, i, self->parent_signals[i]->name);
            MEMDUMP(self->parent_signals[i]->real_time_data.data, SIGNAL_REAL_TIME_DATA_BUFFER_SIZE);
            MY_X = *(uint16_t *)(CHANNEL(0).data);
        }
    }
}


static void
needle_meter__minimalistic__draw(widget_t *self)
{
    DrawEllipse(300, 300, 20, 80, RAYWHITE);
}
