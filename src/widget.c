//
// Created by puhlz on 5/28/25.
//

#include "widget.h"

#include <stdio.h>
#include <string.h>


#define DEFINE_STRTOK(name, target, delims) \
    char *name = strtok(target, delims); \
    if (NULL == name) { \
        fprintf(stderr, "ERROR:  Configuration(line %u): Could not determine the value of '" #name "'.\n", line_num); \
        return ERR_INVALID_CONFIGURATION; \
    } \
    name = strdup(name); \
    if (NULL == name) { \
        fprintf(stderr, "ERROR:  Configuration(line %u): Failed to duplicate value of '" #name "'.\n", line_num); \
        return ERR_INVALID_CONFIGURATION; \
    }
#define DEFINE_STRTOK_CSV(name) DEFINE_STRTOK(name, NULL, ",")
#define CONF_SUMMARIZE(x) DPRINTLN("\t\t'" #x "':  '%s'", x);


widget_t **global_widgets = NULL;


static ic_err_t
get_signal_by_name(const char *name, dbc_signal_t **signal_out)
{
    if (NULL == signal_out || NULL == name || 0 == strlen(name)) return ERR_ARGS;

    for (int i = 0; i < DBC_SIGNALS_LEN; ++i) {
        if (NULL == DBC.signals[i].name) continue;
        if (0 == strcmp(name, DBC.signals[i].name)) {
            *signal_out = &DBC.signals[i];
            return ERR_OK;
        }
    }

    return ERR_NOT_FOUND;
}


ic_err_t
load_widgets(char *mutable_configuration)
{
    ic_err_t status;

    /*
     * Load widgets and init them.
     *  Widgets are specifically tied to signals by name. An example configuration line might show:
     *          Signal_5060_4,Vehicle Speed,needle_meter,20,20,200,200,
     *              180:-50:0:120:20:5:10:MONOSPACE:DIAMOND:GROOVE:80:FD6611AA:DDDDDFF:DD9999CC:mph:100:60
     *      SIGNAL_NAME,WIDGET_LABEL,WIDGET_TYPE,X,Y,W,H,WIDGET_OPTS (specific to each widget)
     *      In the case of the needle meter, we have...
     *          DEGREES:TILT_DEGREES:MIN:MAX:INTERVAL:SUB_INTERVAL:FONT_SIZE:FONT_TYPE:NEEDLE_TYPE:BEZEL_TYPE\
     *              :NEEDLE_PERC:NEEDLE_RGBA:BEZEL_RGBA:NUMBERS_RGBA:UNIT_LABEL:LABEL_OFFSET(X:Y)
     *      These custom properties are documented in the FlexIC project, or in the respective widget source.
     *
     * A widget is uniquely instantiated for each signal that has one to display. That way, when
     *  CAN messages are received with the corresponding signals, data can be modified and signals
     *  can cause a redraw of the component. The logic of the message's `update` hook will control
     *  whether the widget will be forced to fully redraw, go invisible, etc.
     */

    uint32_t line_num = 0;
    uint32_t line_len = 0;
    char *line = strtok(mutable_configuration, "\n");

    do {
        ++line_num;
        line_len = strlen(line);   /* preserve the original line length */

        DEFINE_STRTOK(signal_name, line, ",")
        DEFINE_STRTOK_CSV(widget_label)
        DEFINE_STRTOK_CSV(widget_type)
        DEFINE_STRTOK_CSV(x_pos)
        DEFINE_STRTOK_CSV(y_pos)
        DEFINE_STRTOK_CSV(width)
        DEFINE_STRTOK_CSV(height)
        DEFINE_STRTOK_CSV(widget_opts)

        /* Debugging information only; will not show in non-debug builds. */
        DPRINTLN("Config summary (line %u):", line_num);
        CONF_SUMMARIZE(signal_name);
        CONF_SUMMARIZE(widget_label);
        CONF_SUMMARIZE(widget_type);
        CONF_SUMMARIZE(x_pos);
        CONF_SUMMARIZE(y_pos);
        CONF_SUMMARIZE(width);
        CONF_SUMMARIZE(height);
        CONF_SUMMARIZE(widget_opts);
        DPRINT("\n");

        /* Fetch the related signal by name. */
        /* NOTE: This operation might take a while for many signals, but the mapping only needs to be done once. */
        dbc_signal_t *related_signal = NULL;
        status = get_signal_by_name(signal_name, &related_signal);
        if (ERR_OK != status || NULL == related_signal) {
            fprintf(
                stderr,
                "ERROR:  Configuration(line %u): Could not find signal '%s' (e:%u).\n",
                line_num, signal_name, status
            );
            return ERR_NOT_FOUND;
        }
    } while (NULL != (line = strtok(&line[line_len + 1], "\n")));

    return ERR_OK;
}
