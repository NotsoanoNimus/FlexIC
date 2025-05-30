
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

#if IC_OPT_CAN_FD_EXTENDED==1
#   define SIGNAL_REAL_TIME_DATA_BUFFER_SIZE    64   /* CAN_FD_MAX_DLEN */
#else   /* IC_OPT_CAN_FD_EXTENDED */
#   define SIGNAL_REAL_TIME_DATA_BUFFER_SIZE    8    /* CAN_MAX_DLEN */
#endif   /* IC_OPT_CAN_FD_EXTENDED */


#if IC_DEBUG==1
#define DPRINTLN(x, ...) \
    fprintf(stdout, "DEBUG:  " x "\n", ##__VA_ARGS__);
#define DPRINT(x, ...) \
    fprintf(stdout, "DEBUG:  " x, ##__VA_ARGS__);
#define MEMDUMP(ptr, len) \
    for (unsigned int memdump_idx = 0; memdump_idx < (len); ++memdump_idx) \
        fprintf(stdout, "%02X%c", ((unsigned char *)(ptr))[memdump_idx], ((memdump_idx + 1) % 16) ? ' ' : '\n'); \
    printf("\n");
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


/* Forward declarations. */
typedef struct dbc_signal dbc_signal_t;
typedef struct dbc_message dbc_message_t;

/* Real-time data attachment for signals. */
typedef
struct {
    volatile uint8_t data[SIGNAL_REAL_TIME_DATA_BUFFER_SIZE];
    volatile bool has_update;
    pthread_mutex_t lock;
} real_time_data_t;

/* A structure holding a DBC message. */
// TODO: Organize fields.
struct dbc_message {
    uint32_t id;
    const char *name;
    dbc_signal_t **signals;
    uint32_t num_signals;
};

/* A structure holding a DBC signal type. */
struct dbc_signal {
    dbc_message_t *parent_message;
    void **widget_instances;   /* This is VOID because of circular dependencies which I don't feel like resolving atm */
    uint8_t num_widget_instances;
    real_time_data_t real_time_data;
    char *name;
    uint32_t start_bit;
    uint32_t signal_size;
    bool is_little_endian;
    bool is_unsigned;
    float factor;
    float offset;
    uint32_t minimum_value;
    uint32_t maximum_value;
    bool is_multiplexed;
    uint8_t multiplexor;
    char *unit_text;
    unit_type_t parsed_unit_type;
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
