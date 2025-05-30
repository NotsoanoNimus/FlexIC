//
// Created by puhlz on 5/30/25.
//

#include "widget_common.h"

#include <raylib.h>


static void
stepped_bar__default__update(widget_t *self)
{
    MY_X = *(uint16_t *)&(x_pos->data[0]);
    MY_Y = *(uint16_t *)&(y_pos->data[2]);
}


static void
stepped_bar__default__draw(widget_t *self)
{
    // TODO: Just an example.
    DrawRectangle(MY_X, MY_Y, MY_WIDTH, MY_HEIGHT, (Color){ 0x00, 0x00, 0xFF, 0xFF });
}


static ic_err_t
stepped_bar__default__parse_args(widget_t *self)
{
    return ERR_OK;
}
