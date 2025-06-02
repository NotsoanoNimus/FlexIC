//
// Created by puhlz on 5/27/25.
//

#include "flex_ic.h"
#include "renderer.h"
#include "canbus.h"
#include "widget.h"

/* Dynamically generated. Should only be included once, since it may contain value assignments. */
#include "vehicle.h"

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>


/*
 * An external reference to the generated widgets-configuration.
 *  See the build process documentation for more details.
 */
extern const char *WIDGETS_CONFIGURATION;


/* Async objects which need to be accessible globally. */
volatile canbus_thread_ctx_t can_bus_ctx;
can_bus_meta_t CAN = {
    .has_update = false,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .can_thread_ctx = &can_bus_ctx
};


int
main(int argc, char **argv)
{
    pthread_t can_bus_thread;
    ic_err_t status;

    /* Check auto-generated vehicle data and values. Make sure the defaults we need are there. */
    init_vehicle_dbc_data();

    /* Populate the CAN bus thread context. */
    can_bus_ctx = (canbus_thread_ctx_t)
    {
        .thread_status = ERR_OK,
        .should_close = false,
        .is_listening = false,
        .can_if_name = IC_OPT_CAN_IF_NAME
    };

    /* Load the widgets configuration. */
    char *conf = strdup(WIDGETS_CONFIGURATION);
    if (NULL == conf) {
        fprintf(stderr, "ERROR:  Failed to load widget config (e:OUT_OF_RESOURCES). Aborting.\n");
        exit(EXIT_FAILURE);
    }

    /* Load all widgets associated with signals. */
    if (ERR_OK != (status = load_widgets(conf))) {
        fprintf(stderr, "ERROR:  Failed to load widgets (e:%u). Aborting.\n", status);
        exit(EXIT_FAILURE);
    }

    free(conf); conf = NULL;   /* can let this go now... */

    /* Spawn the CAN listener thread. Wait for the status to change to ERR_CAN_LISTENING or error. */
    pthread_create(&can_bus_thread, NULL, canbus_listener, (void *)&can_bus_ctx);

    while (ERR_OK == can_bus_ctx.thread_status) usleep(10000);

    if (ERR_CAN_LISTENING != can_bus_ctx.thread_status) {
        fprintf(
            stderr,
            "ERROR: The CAN bus thread failed to enter the LISTENING state.\n>> Reason: %s\n\n",
            canbus_status(can_bus_ctx.thread_status)
        );
        exit(EXIT_FAILURE);
    }

    /* Set up and enter the main rendering loop. */
    if (ERR_OK != global_renderer->init(global_renderer)) {
        fprintf(stderr, "ERROR:  Failed to initialize the IC renderer.\n");
        exit(EXIT_FAILURE);
    }

    global_renderer->loading(global_renderer);   /* flashes an intro sequence from the renderer */
    global_renderer->loop(global_renderer);   /* noreturn; unless application exit condition */

    /* Always wait for the listener to close, if the code reaches these statements. */
    // fprintf(stdout, "Waiting on the CAN socket to close.\n    If this takes too long, just force-close the application...\n");
    // can_bus_ctx.should_close = true;
    // pthread_join(can_bus_thread, NULL);
    /* Just kidding, destroy the thread immediately. We're exiting anyway. */
    pthread_kill(can_bus_thread, SIGKILL);

    /* All done. */
    return 0;
}
