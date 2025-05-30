//
// Created by puhlz on 5/29/25.
//

#include <raylib.h>
#include <stdio.h>

#include "widget.h"


static ic_err_t
internal__needle_meter_draw(widget_t *self)
{
    // TODO: Testing. Remove.
    for (int i = 0; i < self->num_parent_signals; ++i) {
        if (self->parent_signals[i]->real_time_data.has_update) {
            DPRINTLN("[%s] SIGNAL RAW DATA (CHANNEL%u: %s): ", self->label, i, self->parent_signals[i]->name);
            MEMDUMP(self->parent_signals[i]->real_time_data.data, SIGNAL_REAL_TIME_DATA_BUFFER_SIZE);
            MY_X = *(uint16_t *)&CHANNEL(0).data[0];
        }
    }


    // TODO: Just an example.
    DrawCircle(MY_X, MY_Y, MY_HEIGHT / 2.0f, (Color){ 0xFF, 0x00, 0x00, 0xFF });

    return ERR_OK;
}


static ic_err_t
internal__needle_meter_create(widget_t *self, int argc, char **argv)
{
    self->draw = internal__needle_meter_draw;

    return ERR_OK;
}

func__widget_factory_create needle_meter_create = internal__needle_meter_create;
