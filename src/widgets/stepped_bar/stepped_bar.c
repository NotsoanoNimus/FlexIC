//
// Created by puhlz on 5/29/25.
//

#include <raylib.h>

#include "widget.h"


static real_time_data_t *x_pos, *y_pos;


static int x = 0, y = 0;
static void
internal__stepped_bar_update(widget_t *self)
{
    MY_X = *(uint16_t *)&(x_pos->data[0]);
    MY_Y = *(uint16_t *)&(x_pos->data[2]);
}


static void
internal__stepped_bar_draw(widget_t *self)
{
    // TODO: Just an example.
    DrawRectangle(MY_X, MY_Y, MY_WIDTH, MY_HEIGHT, (Color){ 0x00, 0x00, 0xFF, 0xFF });
}


static ic_err_t
internal__stepped_bar_create(widget_t *self, int argc, char **argv)
{
    self->update = internal__stepped_bar_update;
    self->draw = internal__stepped_bar_draw;

    init_channel(self , 0, &x_pos);
    init_channel(self , 1, &y_pos);

    return ERR_OK;
}

func__widget_factory_create stepped_bar_create = internal__stepped_bar_create;

