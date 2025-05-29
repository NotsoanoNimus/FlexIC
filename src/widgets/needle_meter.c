//
// Created by puhlz on 5/29/25.
//

#include <raylib.h>

#include "widget.h"


static ic_err_t
internal__needle_meter_draw(widget_t *self)
{
    // TODO: Just an example.
    DrawCircle(MY_X, MY_Y, MY_HEIGHT / 2.0f, (Color){ 0xFF, 0x00, 0x00, 0xFF });

    return ERR_OK;
}


static ic_err_t
internal__needle_meter_create(
    widget_t *self,
    real_time_data_t *real_time_data_reference,
    uint32_t num_real_time_data_references
) {
    self->draw = internal__needle_meter_draw;

    return ERR_OK;
}

func__widget_factory_create needle_meter_create = internal__needle_meter_create;
