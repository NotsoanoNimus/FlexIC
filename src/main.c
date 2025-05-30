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
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * HEY! The configuration parser is quite sensitive.
 * Want to make sure your configuration is correct? Please check the repository's 'docs/widget_conf.md' instructions.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
static char *WIDGET_CONFIGURATION =
#if IC_OPT_STATIC_CONFIG==1
    (char *)
    "ICSim_Speed,Vehicle Speed,needle_meter,yes,20,20,200,200,2,"
        "240:0:120:20:5:10:MONOSPACE:DIAMOND:GROOVE:80:FD6611AA:DDDDDFF:DD9999CC:mph:100:60\n"
    // "Signal_5060_4:Signal_5060_3,Vehicle Speed 2,needle_meter,yes,600,100,200,200,1,"
    //     "240:0:120:20:5:10:MONOSPACE:DIAMOND:GROOVE:80:FD6611AA:DDDDDFF:DD9999CC:mph:100:60\n"
    "Signal_7_xpos:Signal_7_ypos,le metric,stepped_bar,yes,320,100,200,200,3,"
        "240:0:120:20:5:10:MONOSPACE:DIAMOND:GROOVE:80:FD6611AA:DDDDDFF:DD9999CC:mph:100:60"
#else   /* IC_OPT_STATIC_CONFIG */
    NULL
#endif   /* IC_OPT_STATIC_CONFIG */
;


static ic_err_t load_config(char **into);


int
main(int argc, char **argv)
{
    // for (int i = 0; i < 32; ++i)
    //     printf("#ifdef WIDGET_FACTORY_%u\n    if (%u >= num_widget_factories) return false;\n    extern func__widget_factory_create WIDGET_FACTORY_%u;\n    widget_factories[%u].create = WIDGET_FACTORY_%u;\n#endif   /* WIDGET_FACTORY_%u */\n", i, i, i, i, i, i);
    // return 0;
    pthread_t can_bus_thread;
    volatile canbus_thread_ctx_t can_bus_ctx;
    ic_err_t status;

    /* Check auto-generated vehicle data and values. Make sure the defaults we need are there. */
    // TODO!
    init_vehicle();

    /* Populate the CAN bus thread context. */
    can_bus_ctx = (canbus_thread_ctx_t)
    {
        .thread_status = ERR_OK,
        .should_close = false,
        .is_listening = false,
        .can_if_name = "vcan0"  // TODO! params
    };

    /* Load the widgets configuration. */
    char *conf = NULL;
    if (ERR_OK != (status = load_config(&conf))) {
        fprintf(stderr, "ERROR:  Failed to load widget config (e:%u). Aborting.\n", status);
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


static ic_err_t
load_config(char **into)
{
    if (NULL == into) return ERR_ARGS;

#if IC_OPT_STATIC_CONFIG!=1
    if (0 != access(IC_OPT_CONFIG_PATH, F_OK)) {
        return ERR_CONFIG_MISSING;
    }
    FILE *config_file = fopen(IC_OPT_CONFIG_PATH, "rb");  // open in binary mode
    if (NULL == config_file) {
        perror("fopen");
        return ERR_CONFIG_LOAD;
    }
    if (fseek(config_file, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(config_file);
        return ERR_CONFIG_LOAD;
    }
    long file_size = ftell(config_file);
    if (file_size < 0) {
        perror("ftell");
        fclose(config_file);
        return ERR_CONFIG_LOAD;
    }

    rewind(config_file);

    uint8_t *buffer = calloc(1, file_size + 5);
    if (NULL == buffer) {
        perror("malloc");
        fclose(config_file);
        return ERR_OUT_OF_RESOURCES;
    }

    size_t bytes_read = fread(buffer, 1, file_size, config_file);
    if (bytes_read != file_size) {
        perror("fread");
        free(buffer);
        fclose(config_file);
        return ERR_CONFIG_READ;
    }

    fclose(config_file);

    *into = (const char *)buffer;
#else   /* IC_OPT_STATIC_CONFIG */
    if (NULL == WIDGET_CONFIGURATION || strlen(WIDGET_CONFIGURATION) <= 0) {
        return ERR_CONFIG_MISSING;
    }

    *into = calloc(1, strlen(WIDGET_CONFIGURATION) + 5);
    strcpy(*into, WIDGET_CONFIGURATION);
#endif   /* IC_OPT_STATIC_CONFIG */

    return ERR_OK;
}
