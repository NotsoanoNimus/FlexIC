
// Created by puhlz on 5/27/25.
//

#ifndef FLEX_IC_H
#define FLEX_IC_H

#include <stdint.h>
#include <stdbool.h>

#include "flex_ic_opts.h"
#include "unit_types.h"

/* Control 'malloc' and other stdlib definitions from here. */
#if IC_OPT_NOSTDLIB==0
#   include <stdlib.h>
#endif   /* IC_OPT_NOSTDLIB */

#include "vehicle.h"


#define STRINGIFY(x) #x
#define AS_LITERAL(x) STRINGIFY(x)


#if IC_DEBUG==1
#define DPRINTLN(x, ...) \
    fprintf(stdout, "DEBUG:  " x "\n", ##__VA_ARGS__);
#define DPRINT(x, ...) \
    fprintf(stdout, "DEBUG:  " x, ##__VA_ARGS__);
#define MEMDUMP(ptr, len) \
    for (unsigned int i = 0; i < len; ++i) \
        fprintf(stdout, "%02X%c", ((unsigned char *)ptr)[i], ((i+1) % 16) ? ' ' : '\n');
#else   /* IC_DEBUG */
#define DPRINTLN(x, ...)
#define DPRINT(x, ...)
#define MEMDUMP(ptr, len)
#endif   /* IC_DEBUG */


/* Error definitions. */
typedef
enum {
    ERR_OK = 0,

    ERR_ARGS,
    ERR_OUT_OF_RESOURCES,
    ERR_NOT_FOUND,
    ERR_CONFIG_MISSING,
    ERR_CONFIG_LOAD,
    ERR_CONFIG_READ,
    ERR_INVALID_CONFIGURATION,
    ERR_NO_WIDGET_TYPES,
    ERR_TOO_MANY_WIDGET_TYPES,
    ERR_INVALID_WIDGET_TYPE,

    /* CAN thread-specific error statuses. */
    ERR_CAN_LISTENING,
    ERR_CAN_INVALID_CONTEXT,
    ERR_CAN_SOCKET,
    ERR_CAN_IOCTL,
    ERR_CAN_BIND,
    ERR_CAN_CLOSED,
} ic_err_t;


inline ic_err_t
init_real_time_data(real_time_data_t *real_time_data, const int sizeof_type)
{
    real_time_data->value = malloc(sizeof_type);
    return NULL == real_time_data->value ? ERR_OUT_OF_RESOURCES : ERR_OK;
}


/* Forward declarations. */
typedef struct dbc_signal dbc_signal_t;
typedef struct dbc_message dbc_message_t;

/* Vehicle update hook. */
typedef
ic_err_t (*_func__update_vehicle_data)(
    const dbc_message_t *self,
    volatile vehicle_data_t *vehicle_data
);

/* A structure holding a DBC message. */
struct dbc_message {
    uint32_t id;
    const char *name;
    dbc_signal_t **signals;
    uint32_t num_signals;
    _func__update_vehicle_data update;
};

/* A structure holding a DBC signal type. */
struct dbc_signal {
    dbc_message_t *parent_message;
    void **widget_instances;
    uint8_t num_widget_instances;
    uint32_t vehicle_real_time_data_offset;
    char *name;
    // TODO!
    unit_type_t unit_type;
};

/* Finally, a structure which encapsulates all DBC data. */
typedef
struct {
    const dbc_message_t *messages;
    dbc_signal_t *signals;
} dbc_t;

/* References to external variables that should be defined only in vehicle.c. */
extern const char *VEHICLE_NAME;
extern const char *VEHICLE_VERSION;
extern const unsigned int BUS_SPEED;
extern dbc_t DBC;


/* Macro to help with DBC Message identification by ID number. */
#define DBC_MSG_BY_ID(assignee, frame_id) \
    assignee = NULL; \
    for (int dbc_msg_idx = 0; dbc_msg_idx < DBC_MESSAGES_LEN; ++dbc_msg_idx) { \
        if ((frame_id) == DBC.messages[dbc_msg_idx].id) { \
            assignee = &(DBC.messages[dbc_msg_idx]); \
            break; \
        } \
    }



#endif   /* FLEX_IC_H */
