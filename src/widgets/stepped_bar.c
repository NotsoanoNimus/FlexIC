//
// Created by puhlz on 5/29/25.
//

#include <raylib.h>

#include "widget.h"


static int x = 0, y = 0;
static ic_err_t
internal__stepped_bar_draw(widget_t *self)
{
    // TODO: Just an example.
    x = (x + 1) % (GetScreenWidth() - MY_X);
    y = (y + 2) % (GetScreenHeight() - MY_Y);
    DrawRectangle(MY_X + x, MY_Y + y, MY_WIDTH, MY_HEIGHT, (Color){ 0x00, 0x00, 0xFF, 0xFF });

    return ERR_OK;
}


static ic_err_t
internal__stepped_bar_create(widget_t *self)
{
    self->draw = internal__stepped_bar_draw;

    return ERR_OK;
}

func__widget_factory_create stepped_bar_create = internal__stepped_bar_create;

