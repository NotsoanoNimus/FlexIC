//
// Created by puhlz on 5/29/25.
//

#include "widget.h"

/* SKINS */
#include "./default.c"
#include "./minimalistic.c"


static ic_err_t
internal__needle_meter_create(widget_t *self)
{
    REGISTER_SKIN(needle_meter, default);
    REGISTER_SKIN(needle_meter, minimalistic);

    init_channel(self, 0, &needle_data);

    return ERR_OK;
}

func__widget_factory_create needle_meter_create = internal__needle_meter_create;
