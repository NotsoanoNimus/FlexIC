//
// Created by puhlz on 5/28/25.
//

#ifndef IC_CAN_BUS_H
#define IC_CAN_BUS_H

#include <stdint.h>
#include <stdbool.h>

#include "flex_ic.h"


typedef
struct {
    const char *can_if_name;
    volatile ic_err_t thread_status;
    volatile bool is_listening;
    volatile bool should_close;
} canbus_thread_ctx_t;


void *canbus_listener(void *context);

const char *canbus_status(ic_err_t status);



#endif   /* IC_CAN_BUS_H */
