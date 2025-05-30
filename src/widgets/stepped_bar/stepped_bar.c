//
// Created by puhlz on 5/29/25.
//

#include "widget_common.h"

/* SKINS */
#include "./default.c"


static ic_err_t
internal__stepped_bar_create(widget_t *self)
{
    REGISTER_SKIN(stepped_bar, default)

    init_channel(self , 0, &x_pos);
    init_channel(self , 1, &y_pos);

    return ERR_OK;
}

func__widget_factory_create stepped_bar_create = internal__stepped_bar_create;

