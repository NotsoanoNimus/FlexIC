//
// Created by puhlz on 5/29/25.
//

#include "widget.h"

/* SKINS */
#include "./default.c"
#include "./minimalistic.c"


static ic_err_t
internal__needle_meter_create(widget_t *self, int argc, char **argv)
{
    REGISTER_SKIN(needle_meter, default);
    REGISTER_SKIN(needle_meter, minimalistic);

    return ERR_OK;
}

func__widget_factory_create needle_meter_create = internal__needle_meter_create;
