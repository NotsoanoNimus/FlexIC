
// Created by puhlz on 5/27/25.
//

#ifndef FLEX_IC_H
#define FLEX_IC_H

#include <stdint.h>
#include <stdbool.h>

#include "flex_ic_opts.h"
#include "unit_types.h"


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

    /* CAN thread-specific error statuses. */
    ERR_CAN_LISTENING,
    ERR_CAN_INVALID_CONTEXT,
    ERR_CAN_SOCKET,
    ERR_CAN_IOCTL,
    ERR_CAN_BIND,
    ERR_CAN_CLOSED,
} ic_err_t;


/* Control 'malloc' and other stdlib definitions from here. */
#if IC_OPT_NOSTDLIB==0
#   include <stdlib.h>
#endif   /* IC_OPT_NOSTDLIB */


/* Door status array positions. Some cars won't have 'rear' doors. */
typedef
enum {
    DRIVER_FRONT = 0,
    PASSENGER_FRONT,
    DRIVER_REAR,
    PASSENGER_REAR,
    TRUNK,
    HOOD,
    FUEL_CAP,
} door_position_t;


/* Forward declarations. */
typedef struct vehicle_data vehicle_data_t;
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
    _func__update_vehicle_data update;
};

/* A structure holding a DBC signal type. */
struct dbc_signal {
    dbc_message_t *parent_message;
    void *widget_instance;
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


/* Finally, a value from the CAN bus should hold a reference to the parsed Signal,
 * along with the actual value of the signal in real-time. */
typedef
struct {
    void *value;   /* a malloc'd value which should be updated in-place during lifetime */
    // dbc_signal_t *signal;
    unit_type_t output_unit_type;   /* another malloc'd value, conversion mask over input data (when not 0). */
} real_time_data_t;

inline ic_err_t
init_real_time_data(real_time_data_t *real_time_data, const int sizeof_type)
{
    real_time_data->value = malloc(sizeof_type);
    return NULL == real_time_data->value ? ERR_OUT_OF_RESOURCES : ERR_OK;
}


/* References to external variables that should be defined only in vehicle.c. */
extern const char *VEHICLE_NAME;
extern const char *VEHICLE_VERSION;
extern const unsigned int BUS_SPEED;
extern const dbc_t DBC;


/* The implied struct is defined in the generated 'vehicle.h' header, based on DBC fields. */
typedef struct specific_data vehicle_specific_data_t;
#include "vehicle.h"

/* A structure holding current real-time vehicle data. Updated by CAN bus messages.
 * This list is a set of basic properties which should be used to construct most basic ICs. */
struct vehicle_data {
    real_time_data_t rpm;
    real_time_data_t speed;
    real_time_data_t engine_load;
    real_time_data_t coolant_temperature;
    real_time_data_t oil_temperature;
    real_time_data_t intake_air_temperature;
    real_time_data_t fuel_pressure_gauge;
    real_time_data_t fuel_consumption_rate;
    real_time_data_t oil_pressure;
    real_time_data_t mass_airflow_rate;
    real_time_data_t throttle_position;
    real_time_data_t engine_run_time_s;
    real_time_data_t ambient_temperature;
    real_time_data_t engine_torque;
    real_time_data_t odometer;
    real_time_data_t fuel_level;
    bool doors_open[7];   /* see typedef door_position_t above. 'true' means open */
    bool trouble;   /* whether any DTCs are set */
    bool parking_brake;   /* whether the parking brake is enabled */
    vehicle_specific_data_t custom_data;   /* type should be defined in gen'd vehicle.h */
};


/* Some macros to help with data identification. */
#define DBC_MSG_BY_ID(assignee, frame_id) \
    assignee = NULL; \
    for (int dbc_msg_idx = 0; dbc_msg_idx < DBC_MESSAGES_LEN; ++dbc_msg_idx) { \
        if ((frame_id) == DBC.messages[dbc_msg_idx].id) { \
            assignee = &(DBC.messages[dbc_msg_idx]); \
            break; \
        } \
    }



#endif   /* FLEX_IC_H */
