//
// Created by puhlz on 5/29/25.
//

#include <raylib.h>

#include "widget.h"


static ic_err_t
internal__stepped_bar_draw(widget_t *self)
{
    // TODO: Just an example.
    DrawRectangle(MY_X, MY_Y, MY_WIDTH, MY_HEIGHT, (Color){ 0x00, 0x00, 0xFF, 0xFF });

    return ERR_OK;
}


static ic_err_t
internal__stepped_bar_create(
    widget_t *self,
    real_time_data_t *real_time_data_reference,
    uint32_t num_real_time_data_references
) {
    self->draw = internal__stepped_bar_draw;

    return ERR_OK;
}

func__widget_factory_create stepped_bar_create = internal__stepped_bar_create;

