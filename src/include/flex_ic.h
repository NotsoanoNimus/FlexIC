
// Created by puhlz on 5/27/25.
//

#ifndef FLEX_IC_H
#define FLEX_IC_H

#include <stdint.h>
#include <stdbool.h>

#include <raylib.h>


/* --- configuration types (sourced in auto-gen files too) --- */
/* Non-fine (float) points and dimensions. */
typedef
struct {
    int32_t x;
    int32_t y;
} vec2_t;

/* One-time splash screen hook on start-up. */
typedef
void (*_func__render_splash)(
    const void *renderer
);

/* BGCOLOR types. */
typedef
enum {
    COLOR = 1,
    ASSET
} ic_background_type;


/* Compile-time options structure. */
typedef
struct {
    struct {
        int fps_limit;
        vec2_t dimensions;
        bool full_screen;
        const char *title;
    } window;

    _func__render_splash splash_hook_func;

    const char **pages;
    int num_pages;
    int default_page_number;

    struct {
        const char *interface_name;
        bool enable_fd;
    } can;

    ic_background_type background_type;
    union {
        struct {
            bool is_gradient;
            Color static_color;
            Color gradient_top_left;
            Color gradient_top_right;
            Color gradient_bottom_left;
            Color gradient_bottom_right;
        } background_color;

        struct {
            uint8_t *image_data;
            int image_size;
            const char *file_type;
            bool fit_to_window;
            int offset_x;
            int offset_y;
            Color tint;
        } background_asset;
    };
} ic_opts_t;



/* --- only the auto-generated files should have this set --- */
#ifndef CONFIG_TYPES_ONLY
#include <pthread.h>

#include "flex_ic_opts.h"
#include "units.h"

/* Control 'malloc' and other stdlib definitions from here. */
#if IC_OPT_NOSTDLIB==0
#   include <stdlib.h>
#endif   /* IC_OPT_NOSTDLIB */

#include <float.h>
#include <math.h>

#include "vehicle.h"


#define STRINGIFY(x) #x
#define AS_LITERAL(x) STRINGIFY(x)

#define CLAMP(val, min, max) \
    ((val) > (max) ? (max) : ((val) < (min) ? (min) : (val)))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


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


inline unsigned char hex_to_value(char *str)
{
    return (unsigned char)((str[0] > '9' ? (str[0] - 'a' + 10) : (str[0] - 0x30)) << 4)
        | ((str[1] > '9' ? (str[1] - 'a' + 10) : (str[1] - 0x30)) & 0x0f);
}


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


/* Track CAN-bus items globally where desired. */
typedef
struct {
    volatile bool has_update;
    pthread_mutex_t lock;
    volatile void *can_thread_ctx;
} can_bus_meta_t;

extern can_bus_meta_t CAN;

/* Forward declarations. */
typedef struct dbc_signal dbc_signal_t;
typedef struct dbc_message dbc_message_t;

/* Real-time data attachment for signals. */
typedef
struct {
    volatile bool has_update;
    volatile double value;
    // int value_width;
    pthread_mutex_t lock;
} real_time_data_t;

/* A structure holding a DBC message. */
// TODO: Organize fields.
struct dbc_message {
    uint32_t id;
    uint32_t expected_length;
    const char *name;
    dbc_signal_t **signals;
    uint32_t num_signals;
};

/* Valid signal multiplex types. */
typedef
enum {
    Plain = 0,
    Multiplexor,
    MultiplexedSignal,
    MultiplexorAndMultiplexedSignal
} multiplex_type_t;

/* A structure holding a DBC signal type. */
struct dbc_signal {
    char *name;
    dbc_message_t *parent_message;
    void **widget_instances;   /* This is VOID because of circular dependencies which I don't feel like resolving atm */
    uint8_t num_widget_instances;
    real_time_data_t real_time_data;
    uint32_t start_bit;
    uint32_t signal_size;
    bool is_little_endian;
    bool is_unsigned;
    multiplex_type_t multiplex_type;
    uint8_t multiplexor;
    double factor;
    double offset;
    double minimum_value;
    double maximum_value;
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
extern const char *IC_VEHICLE_DBC_COMPILED_BY;
extern const char *IC_VEHICLE_NAME;
extern const char *IC_VEHICLE_SHORT_NAME;
extern const char *IC_VEHICLE_VERSION;
extern const char *IC_VEHICLE_OWNER_NAME;
extern const char *IC_VEHICLE_OWNER_PHONE;
extern const char *IC_VEHICLE_OWNER_EMAIL;
extern const char *IC_VEHICLE_EXTRA_INFO;
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


inline bool is_multiple(float numerator, float divisor)
{
    if (divisor == 0) return false;

    float tolerance = FLT_EPSILON * fmax(fabs(numerator), fabs(divisor));
    return fabs(fmod(numerator, divisor)) <= tolerance;
}


#endif   /* CONFIG_TYPES_ONLY */
#endif   /* FLEX_IC_H */
