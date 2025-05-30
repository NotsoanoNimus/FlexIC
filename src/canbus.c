//
// Created by puhlz on 5/28/25.
//

#include "canbus.h"

/* We assume Linux for this, but this can easily be replaced with your own CAN definitions. */
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>


static void
process_can_frame(struct canfd_frame *frame);


#if IC_OPT_ID_MAPPING==1
typedef void * map_entry_t;
#define I2MM_DIRECTORY_SIZE 256
#define I2MM_DIRECTORY_MASK ((1 << 8) - 1)   /* 0xFF */
#define I2MM_DIRECTORY_SHIFT 24
#define I2MM_TABLE_MASK ((1 << 12) - 1)   /* 0xFFF */
#define I2MM_TABLE_SHIFT 12
#define I2MM_ENTRY_MASK ((1 << 12) - 1)   /* 0xFFF */
#define I2MM_ENTRY_SHIFT 0
static map_entry_t *i2mm_directory[I2MM_DIRECTORY_SIZE] = {0};

static ic_err_t
create_dbc_id_map();

static ic_err_t
lookup_dbc_msg_by_id(canid_t id, const dbc_message_t **out);
#endif   /* IC_OPT_ID_MAPPING */


void *
canbus_listener(void *context)
{
    int s_fd;   /* CAN socket file descriptor. */
    struct sockaddr_can address = {0};
    struct ifreq ifr;
    struct canfd_frame frame = {0};
    ssize_t num_bytes = 0;
    canbus_thread_ctx_t *ctx;
    ic_err_t create_map_response;

    /* Cast incoming structure. */
    ctx = (canbus_thread_ctx_t *)context;
    ctx->thread_status = ERR_OK;   /* assert this condition, even though the caller should set it before start */

    /* Map loaded DBC message pointers to the incoming ID. This should be O(1) rather than O(N). */
#if IC_OPT_ID_MAPPING==1
    if (ERR_OK != (create_map_response = create_dbc_id_map())) {
        fprintf(stderr, "ERROR:  Failed to initialize the ID-to-DBC message map.\n");
        ctx->thread_status = create_map_response;
        return NULL;
    }
#endif   /* IC_OPT_ID_MAPPING */

    /* Create the CAN listener/socket and bind it. */
    s_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s_fd < 0) {
        perror("socket");
        ctx->thread_status = ERR_CAN_SOCKET;
        return NULL;
    }

    /* Use the passed interface name. The system should already have one set up according to the README. */
    strcpy(ifr.ifr_name, ctx->can_if_name);
    if (ioctl(s_fd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        ctx->thread_status = ERR_CAN_IOCTL;
        return NULL;
    }

    /* Bind the socket... */
    address.can_family = AF_CAN;
    address.can_ifindex = ifr.ifr_ifindex;
    if (bind(s_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        ctx->thread_status = ERR_CAN_BIND;
        return NULL;
    }

    /* Indicate everything is ready. */
    ctx->thread_status = ERR_CAN_LISTENING;
    ctx->is_listening = true;

    /* Main recv loop. */
    while (true)
    {
        if (true == ctx->should_close) {
            ctx->thread_status = ERR_CAN_CLOSED;
            break;
        }

        num_bytes = read(s_fd, &frame, sizeof(struct canfd_frame));

        if (!num_bytes) {
            perror("read");
            ctx->thread_status = ERR_CAN_CLOSED;
            break;
        }

        if (
            num_bytes < 9   /* minimum size of at least the prelude of a CAN message, plus 1 byte */
            || num_bytes < 8 + frame.len   /* If we do have a len, needs to match what was received */
        ) {
            fprintf(stderr, "Hmm. Received an incomplete CAN frame. Skipping.\n");
            continue;
        }
#if IC_OPT_DISABLE_CAN_DETAILS!=1
        DPRINTLN("Received CAN frame with ID 0x%X.", frame.can_id);
        DPRINT("Data: "); MEMDUMP(frame.data, frame.len);
#endif   /* IC_OPT_DISABLE_CAN_DETAILS */

        /* Now do something with the CAN frame. */
        process_can_frame(&frame);
    }

    close(s_fd);
    ctx->is_listening = false;

    return ctx;
}


const char *
canbus_status(const ic_err_t status)
{
    switch (status) {
        case ERR_CAN_LISTENING: return "CAN LISTENING";
        case ERR_CAN_BIND: return "CAN BIND";
        case ERR_CAN_CLOSED: return "CAN CLOSED";
        case ERR_CAN_INVALID_CONTEXT: return "CAN INVALID CONTEXT";
        case ERR_CAN_IOCTL: return "CAN IOCTL";
        case ERR_CAN_SOCKET: return "CAN SOCKET";
        default: return "Unknown error";
    }
}


static void
process_can_frame(struct canfd_frame *frame)
{
    const dbc_message_t *message = NULL;

    if (NULL == frame) return;

#if IC_OPT_ID_MAPPING==1
    if (ERR_OK != lookup_dbc_msg_by_id(frame->can_id, &message) || NULL == message) {
#else   /* IC_OPT_ID_MAPPING */
    DBC_MSG_BY_ID(message, frame->can_id);
    if (NULL == message) {
#endif   /* IC_OPT_ID_MAPPING */
#if IC_OPT_DISABLE_CAN_DETAILS!=1
        DPRINTLN(">>> WARNING: Unknown or invalid CAN ID: 0x%X. Skipped.", frame->can_id);
#endif   /* IC_OPT_DISABLE_CAN_DETAILS */
        return;
    }
#if IC_OPT_DISABLE_CAN_DETAILS!=1
    DPRINTLN("INFO:  Received CAN message '%s'", message->name);
#endif   /* IC_OPT_DISABLE_CAN_DETAILS */

    // TODO: Need to detect multiplexor signals that might be part of this message.
    //    This should only update the rtd if the signal is associated with the current multiplexor channel.
    //    Hence, all signals outside the current multiplexor channel must be ignored.
    for (int i = 0; i < message->num_signals; ++i) {
        real_time_data_t *rtd = &(message->signals[i]->real_time_data);

        pthread_mutex_lock(&rtd->lock);
        memset((void *)rtd->data, 0x00, SIGNAL_REAL_TIME_DATA_BUFFER_SIZE);
        memcpy((void *)rtd->data, frame->data, frame->len);

        rtd->has_update = true;
        pthread_mutex_unlock(&rtd->lock);

        /* A global bool prevents the drawing thread from needing to loop signals every pass. */
        pthread_mutex_lock(&CAN.lock);
        CAN.has_update = true;
        pthread_mutex_unlock(&CAN.lock);
    }
}


#if IC_OPT_ID_MAPPING==1
static ic_err_t
create_dbc_id_map()
{
    for (int w = 0; w < DBC_MESSAGES_LEN; ++w) {
        const dbc_message_t *msg = &(DBC.messages[w]);
        canid_t id = (canid_t)msg->id;
        map_entry_t *table_ptr;
        const dbc_message_t **entry_ptr;

    repeat_directory_lookup:
        table_ptr = i2mm_directory[(id >> I2MM_DIRECTORY_SHIFT) & I2MM_DIRECTORY_MASK];
        if (NULL == table_ptr) {
            i2mm_directory[(id >> I2MM_DIRECTORY_SHIFT) & I2MM_DIRECTORY_MASK]
                = (map_entry_t *)malloc(sizeof(map_entry_t) * (I2MM_TABLE_MASK + 1));

            if (NULL == i2mm_directory[(id >> I2MM_DIRECTORY_SHIFT) & I2MM_DIRECTORY_MASK]) return ERR_OUT_OF_RESOURCES;
            goto repeat_directory_lookup;
        }

    repeat_table_lookup:
        entry_ptr = table_ptr[(id >> I2MM_TABLE_SHIFT) & I2MM_TABLE_MASK];
        if (NULL == entry_ptr) {
            table_ptr[(id >> I2MM_TABLE_SHIFT) & I2MM_TABLE_MASK]
                = (dbc_message_t **)malloc(sizeof(dbc_message_t *) * (I2MM_ENTRY_MASK + 1));

            if (NULL == table_ptr[(id >> I2MM_TABLE_SHIFT) & I2MM_TABLE_MASK]) return ERR_OUT_OF_RESOURCES;
            goto repeat_table_lookup;
        }

        entry_ptr[(id >> I2MM_ENTRY_SHIFT) & I2MM_ENTRY_MASK] = msg;
    }

    return ERR_OK;
}


static ic_err_t
lookup_dbc_msg_by_id(canid_t id, const dbc_message_t **out)
{
    if (NULL == out) return ERR_ARGS;

    map_entry_t *table_ptr = i2mm_directory[(id >> I2MM_DIRECTORY_SHIFT) & I2MM_DIRECTORY_MASK];
    if (NULL == table_ptr) return ERR_NOT_FOUND;

    map_entry_t *entry_ptr = table_ptr[(id >> I2MM_TABLE_SHIFT) & I2MM_TABLE_MASK];
    if (NULL == entry_ptr) return ERR_NOT_FOUND;

    dbc_message_t *dbc_msg_ptr = entry_ptr[(id >> I2MM_ENTRY_SHIFT) & I2MM_ENTRY_MASK];
    if (NULL == dbc_msg_ptr) return ERR_NOT_FOUND;

    *out = dbc_msg_ptr;
    return ERR_OK;
}
#endif   /* IC_OPT_ID_MAPPING */
