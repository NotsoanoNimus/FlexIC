//
// Created by puhlz on 5/28/25.
//

#include "widget.h"

#include <ctype.h>
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

#define CHECK_STRTOL(property) \
    if (*endptr) { \
        fprintf(stderr, "ERROR:  Configuration(line %u): Invalid integer value of '" #property " (value '%s')'.\n", line_num, property); \
        return ERR_INVALID_CONFIGURATION; \
    }


widget_t **global_widgets = NULL;
uint32_t num_global_widgets = 0;

/* Local only to this source file. */
#define MAX_REGISTRATIONS       32

/* Name-to-create associations. */
typedef
struct {
    const char *name;
    func__widget_factory_create create;
} registered_widget_factory_t;

/* An array of registered factory associations for different widget types.
    Populated at runtime by the IC_WIDGETS preprocessor definition. */
static registered_widget_factory_t *widget_factories;

static inline bool assign_factory_methods(void);
uint32_t num_widget_factories;


static inline void
reorder_widgets_map(void);


static ic_err_t
get_signal_by_name(const char *name, dbc_signal_t **signal_out)
{
    if (NULL == signal_out || NULL == name || 0 == strlen(name)) return ERR_ARGS;

    *signal_out = NULL;

    for (int i = 0; i < DBC_SIGNALS_LEN; ++i) {
        if (NULL == DBC.signals[i].name) continue;
        if (0 == strcmp(name, DBC.signals[i].name)) {
            *signal_out = &DBC.signals[i];
            return ERR_OK;
        }
    }

    return ERR_NOT_FOUND;
}


static ic_err_t
update_signal_and_create_widget(uint32_t line_num, char *rel_signal_name, widget_t **widget_ref)
{
    if (NULL == rel_signal_name || NULL == widget_ref) return ERR_ARGS;

    dbc_signal_t *related_signal = NULL;
    ic_err_t status = get_signal_by_name(rel_signal_name, &related_signal);
    if (ERR_OK != status || NULL == related_signal) {
        fprintf(
            stderr,
            "ERROR:  Configuration(line %u): Could not find signal '%s' (e:%u).\n",
            line_num, rel_signal_name, status
        );
        return ERR_NOT_FOUND;
    }
    DPRINTLN("Located signal '%s' at %p.", rel_signal_name, related_signal);

    /* If the widget ref is currently empty, create a new widget. */
    if (NULL == *widget_ref) {
        *widget_ref = calloc(1, sizeof(widget_t));
        if (NULL == *widget_ref) return ERR_OUT_OF_RESOURCES;
    }

    /* Prepare to attach the widget reference to the signal. */
    ++related_signal->num_widget_instances;

    /* Widget already has an assigned widget (or more than one). */
    if (related_signal->num_widget_instances > 1) {
        if (NULL == related_signal->widget_instances) {
            fprintf(
                stderr,
                "ERROR:  Configuration (line %u, signal %s): There are somehow multiple widget instances, but no buffer to contain them.\n",
                line_num, rel_signal_name
            );
        }

        void **new_instance_list = realloc(
            related_signal->widget_instances, related_signal->num_widget_instances * sizeof(widget_t *)
        );
        related_signal->widget_instances = new_instance_list;

        if (NULL == related_signal->widget_instances) {
            fprintf(
                stderr,
                "ERROR:  Configuration(line %u, signal %s): Could not append widget ref to signal - OOM.\n",
                line_num, rel_signal_name
            );
            return ERR_OUT_OF_RESOURCES;
        }

        related_signal->widget_instances[related_signal->num_widget_instances - 1] = *widget_ref;
    } else {
        related_signal->widget_instances = malloc(sizeof(widget_t *));
        related_signal->widget_instances[0] = *widget_ref;
    }

    /* Now, do the inverse for the widget instance. */
    widget_t *the_widget = *widget_ref;
    ++the_widget->num_parent_signals;

    /* Widget already has an assigned widget (or more than one). */
    if (the_widget->num_parent_signals > 1) {
        if (NULL == the_widget->parent_signals) {
            fprintf(
                stderr,
                "ERROR:  Configuration (line %u, signal %s): There are somehow multiple signal instances, but no buffer to contain them.\n",
                line_num, rel_signal_name
            );
        }

        void **new_instance_list = realloc(
            the_widget->parent_signals, the_widget->num_parent_signals * sizeof(dbc_signal_t *)
        );
        the_widget->parent_signals = (dbc_signal_t **)new_instance_list;

        if (NULL == the_widget->parent_signals) {
            fprintf(
                stderr,
                "ERROR:  Configuration(line %u, signal %s): Could not append signal ref to widget - OOM.\n",
                line_num, rel_signal_name
            );
            return ERR_OUT_OF_RESOURCES;
        }

        the_widget->parent_signals[the_widget->num_parent_signals - 1] = related_signal;
    } else {
        the_widget->parent_signals = malloc(sizeof(widget_t *));
        the_widget->parent_signals[0] = related_signal;
    }

    return ERR_OK;
}


ic_err_t
load_widgets(char *mutable_configuration)
{
    ic_err_t status;

    char *loaded_widgets = strdup(IC_WIDGETS);
    DPRINTLN("loaded_widgets:  '%s'", loaded_widgets);

    char *str_walk = strtok(loaded_widgets, ",");
    if (NULL == str_walk) {
        fprintf(stderr, "ERROR: No widget types are defined in the Makefile. FlexIC is useless without widgets.\n");
        return ERR_NO_WIDGET_TYPES;
    }

    num_widget_factories = 0;
    widget_factories = malloc(1);   /* placeholder for initialization */

    do {
        ++num_widget_factories;

        registered_widget_factory_t *new_widget_factories
            = realloc(widget_factories, num_widget_factories * sizeof(registered_widget_factory_t));
        if (NULL == new_widget_factories) {
            fprintf(stderr, "ERROR: Failed to reallocate widget-type registrations - OOM.\n");
            free(loaded_widgets); free(widget_factories);
            return ERR_OUT_OF_RESOURCES;
        }
        widget_factories = new_widget_factories;

        widget_factories[num_widget_factories - 1].name = strdup(str_walk);
    } while (NULL != (str_walk = strtok(NULL, ",")));

    if (MAX_REGISTRATIONS <= num_widget_factories) {
        fprintf(
            stderr,
            "ERROR: Too many widget registrations. Max supported is %u, but you have %u.\n",
            MAX_REGISTRATIONS, num_widget_factories
        );
        return ERR_TOO_MANY_WIDGET_TYPES;
    }

    /* Put into its own area; too verbose and annoying. */
    if (!assign_factory_methods()) {
        fprintf(stderr, "ERROR: Could not assign factory methods.\n");
        return ERR_TOO_MANY_WIDGET_TYPES;
    }

    /*
     * Load widgets and init them.
     *  Widgets are specifically tied to signals by name. An example configuration line might show:
     *          Signal_5060_4,Vehicle Speed,needle_meter,true,20,20,200,200,0,
     *              180:-50:0:120:20:5:10:MONOSPACE:DIAMOND:GROOVE:80:FD6611AA:DDDDDFF:DD9999CC:mph:100:60
     *      SIGNAL_NAME,WIDGET_LABEL,WIDGET_TYPE,VISIBLE,X,Y,W,H,Z-INDEX,WIDGET_OPTS (specific to each widget)
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
    global_widgets = malloc(sizeof(widget_t *));
    num_global_widgets = 0;

    uint32_t line_num = 0;
    uint32_t line_len = 0;
    char *line = strtok(mutable_configuration, "\n");

    do {
        ++line_num;
        line_len = strlen(line);   /* preserve the original line length */

        DEFINE_STRTOK(signal_name, line, ",");
        DEFINE_STRTOK_CSV(widget_label);
        DEFINE_STRTOK_CSV(widget_type);
        DEFINE_STRTOK_CSV(visible);
        DEFINE_STRTOK_CSV(x_pos);
        DEFINE_STRTOK_CSV(y_pos);
        DEFINE_STRTOK_CSV(width);
        DEFINE_STRTOK_CSV(height);
        DEFINE_STRTOK_CSV(z_index);
        DEFINE_STRTOK_CSV(widget_opts);

        /* Debugging information only; will not show in non-debug builds. */
        DPRINTLN("Config summary (line %u):", line_num);
        CONF_SUMMARIZE(signal_name);
        CONF_SUMMARIZE(widget_label);
        CONF_SUMMARIZE(widget_type);
        CONF_SUMMARIZE(visible);
        CONF_SUMMARIZE(x_pos);
        CONF_SUMMARIZE(y_pos);
        CONF_SUMMARIZE(width);
        CONF_SUMMARIZE(height);
        CONF_SUMMARIZE(z_index);
        CONF_SUMMARIZE(widget_opts);
        DPRINT("\n");

        /* Fetch the related signal by name. */
        /* NOTE: This operation might take a while for many signals, but the mapping only needs to be done once. */
        char *rel_signal_name = strtok(signal_name, ":");
        if (NULL == rel_signal_name) {
            fprintf(stderr, "ERROR:  Configuration(line %u): Failed to get a valid signal name.\n", line_num);
            return ERR_INVALID_CONFIGURATION;
        }

        /* Correlate the widget to its specified signals. Also returns a new widget. */
        widget_t *new_widget = NULL;
        do {
            status = update_signal_and_create_widget(line_num, rel_signal_name, &new_widget);
            if (ERR_OK != status) return status;
        } while (NULL != (rel_signal_name = strtok(NULL, ":")));

        if (NULL == new_widget) {
            fprintf(stderr, "ERROR:  Configuration(line %u): NULL widgets are not permitted.\n", line_num);
            return ERR_NOT_FOUND;
        }
        DPRINTLN("widget:  Loaded %u signal references.", new_widget->num_parent_signals);

        /* Add the new (shared) widget instance reference to the flat global list. */
        ++num_global_widgets;
        widget_t **new_global_widgets = realloc(global_widgets, num_global_widgets * sizeof(widget_t *));
        global_widgets = new_global_widgets;
        if (NULL == global_widgets) {
            fprintf(stderr, "ERROR:  Failed to reallocate the flat list of global widgets.\n");
            return ERR_OUT_OF_RESOURCES;
        }
        DPRINTLN("global_widgets: Update index %u", (num_global_widgets - 1));
        global_widgets[num_global_widgets - 1] = new_widget;

        /* Initialize as much as we can before handing control to the factory method for the specific widget type. */
        char *endptr = NULL;
        new_widget->param_string = strdup(widget_opts);
        new_widget->type = strdup(widget_type);
        new_widget->label = strdup(widget_label);
        new_widget->state.resolution.x = (int)strtol(width, &endptr, 10);   CHECK_STRTOL(width);
        new_widget->state.resolution.y = (int)strtol(height, &endptr, 10);  CHECK_STRTOL(height);
        new_widget->state.position.x = (int)strtol(x_pos, &endptr, 10);     CHECK_STRTOL(x_pos);
        new_widget->state.position.y = (int)strtol(y_pos, &endptr, 10);     CHECK_STRTOL(y_pos);
        new_widget->state.z_index = (int)strtol(z_index, &endptr, 10);      CHECK_STRTOL(z_index);

        /* Locate the factory method for the widget_type. */
        bool created = false;
        for (int i = 0; i < num_widget_factories; ++i) {
            if (NULL == widget_factories[i].name || NULL == widget_factories[i].create) continue;

            if (__builtin_expect(0 == strcmp(widget_type, widget_factories[i].name), false)) {
                if (ERR_OK != (status = widget_factories[i].create(new_widget))) {
                    fprintf(
                        stderr,
                        "ERROR:  Failed to instantiate widget type '%s' for signal '%s' (e:%u).\n",
                        widget_type, signal_name, status
                    );
                    return status;
                }
                created = true;
                break;
            }
        }

        if (!created) {
            fprintf(
                stderr,
                "ERROR:  Configuration (line %u): Invalid 'widget_type' name '%s'.\n\t\tDid you forget to add the widget type to the compiler options?\n",
                line_num, widget_type
            );
            return ERR_INVALID_WIDGET_TYPE;
        }

        /* Free up variables. */
        free(signal_name);
        free(widget_label);
        free(widget_type);
        free(visible);
        free(x_pos);
        free(y_pos);
        free(width);
        free(height);
        free(z_index);
        free(widget_opts);
    } while (NULL != (line = strtok(&line[line_len + 1], "\n")));

    DPRINTLN("global_widgets: Created and populated a flat list of %u widget references.", num_global_widgets);

    reorder_widgets_map();
    DPRINTLN("global_widgets: Re-ordered based on descending Z-INDEX.");

    return ERR_OK;
}


static inline void
reorder_widgets_map(void)
{
    /* Chad-grade Bubblesort because I'm lazy. */
    uint32_t comparisons = num_global_widgets;
    while (comparisons > 0) {
        for (int i = 0; i < comparisons - 1; ++i) {
            if (global_widgets[i]->state.z_index > global_widgets[i + 1]->state.z_index) {
                widget_t *tmp = global_widgets[i];
                global_widgets[i] = global_widgets[i + 1];
                global_widgets[i + 1] = tmp;
            }
        }
        --comparisons;;
    }

#if IC_DEBUG==1
    DPRINTLN("Sorted z-index values:"); DPRINT("\t\t");
    for (int i = 0; i < num_global_widgets; ++i) {
        printf("%u ", global_widgets[i]->state.z_index);
    }
    DPRINT("\n");
#endif   /* IC_DEBUG */
}


static inline bool
assign_factory_methods(void)
{
    DPRINTLN("Widget types count: %u", num_widget_factories);
    if (0 == num_widget_factories) return false;

    /* Yeah... C's limited preprocessor support requires this to be unrolled like this.
        Unfortunately, if there is a way to roll this up into a loop, I'm not skilled enough
        with preprocessor macros to do so. :) */
#ifdef WIDGET_FACTORY_0
    if (0 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_0;
    widget_factories[0].create = WIDGET_FACTORY_0;
#endif   /* WIDGET_FACTORY_0 */
#ifdef WIDGET_FACTORY_1
    if (1 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_1;
    widget_factories[1].create = WIDGET_FACTORY_1;
#endif   /* WIDGET_FACTORY_1 */
#ifdef WIDGET_FACTORY_2
    if (2 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_2;
    widget_factories[2].create = WIDGET_FACTORY_2;
#endif   /* WIDGET_FACTORY_2 */
#ifdef WIDGET_FACTORY_3
    if (3 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_3;
    widget_factories[3].create = WIDGET_FACTORY_3;
#endif   /* WIDGET_FACTORY_3 */
#ifdef WIDGET_FACTORY_4
    if (4 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_4;
    widget_factories[4].create = WIDGET_FACTORY_4;
#endif   /* WIDGET_FACTORY_4 */
#ifdef WIDGET_FACTORY_5
    if (5 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_5;
    widget_factories[5].create = WIDGET_FACTORY_5;
#endif   /* WIDGET_FACTORY_5 */
#ifdef WIDGET_FACTORY_6
    if (6 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_6;
    widget_factories[6].create = WIDGET_FACTORY_6;
#endif   /* WIDGET_FACTORY_6 */
#ifdef WIDGET_FACTORY_7
    if (7 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_7;
    widget_factories[7].create = WIDGET_FACTORY_7;
#endif   /* WIDGET_FACTORY_7 */
#ifdef WIDGET_FACTORY_8
    if (8 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_8;
    widget_factories[8].create = WIDGET_FACTORY_8;
#endif   /* WIDGET_FACTORY_8 */
#ifdef WIDGET_FACTORY_9
    if (9 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_9;
    widget_factories[9].create = WIDGET_FACTORY_9;
#endif   /* WIDGET_FACTORY_9 */
#ifdef WIDGET_FACTORY_10
    if (10 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_10;
    widget_factories[10].create = WIDGET_FACTORY_10;
#endif   /* WIDGET_FACTORY_10 */
#ifdef WIDGET_FACTORY_11
    if (11 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_11;
    widget_factories[11].create = WIDGET_FACTORY_11;
#endif   /* WIDGET_FACTORY_11 */
#ifdef WIDGET_FACTORY_12
    if (12 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_12;
    widget_factories[12].create = WIDGET_FACTORY_12;
#endif   /* WIDGET_FACTORY_12 */
#ifdef WIDGET_FACTORY_13
    if (13 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_13;
    widget_factories[13].create = WIDGET_FACTORY_13;
#endif   /* WIDGET_FACTORY_13 */
#ifdef WIDGET_FACTORY_14
    if (14 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_14;
    widget_factories[14].create = WIDGET_FACTORY_14;
#endif   /* WIDGET_FACTORY_14 */
#ifdef WIDGET_FACTORY_15
    if (15 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_15;
    widget_factories[15].create = WIDGET_FACTORY_15;
#endif   /* WIDGET_FACTORY_15 */
#ifdef WIDGET_FACTORY_16
    if (16 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_16;
    widget_factories[16].create = WIDGET_FACTORY_16;
#endif   /* WIDGET_FACTORY_16 */
#ifdef WIDGET_FACTORY_17
    if (17 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_17;
    widget_factories[17].create = WIDGET_FACTORY_17;
#endif   /* WIDGET_FACTORY_17 */
#ifdef WIDGET_FACTORY_18
    if (18 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_18;
    widget_factories[18].create = WIDGET_FACTORY_18;
#endif   /* WIDGET_FACTORY_18 */
#ifdef WIDGET_FACTORY_19
    if (19 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_19;
    widget_factories[19].create = WIDGET_FACTORY_19;
#endif   /* WIDGET_FACTORY_19 */
#ifdef WIDGET_FACTORY_20
    if (20 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_20;
    widget_factories[20].create = WIDGET_FACTORY_20;
#endif   /* WIDGET_FACTORY_20 */
#ifdef WIDGET_FACTORY_21
    if (21 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_21;
    widget_factories[21].create = WIDGET_FACTORY_21;
#endif   /* WIDGET_FACTORY_21 */
#ifdef WIDGET_FACTORY_22
    if (22 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_22;
    widget_factories[22].create = WIDGET_FACTORY_22;
#endif   /* WIDGET_FACTORY_22 */
#ifdef WIDGET_FACTORY_23
    if (23 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_23;
    widget_factories[23].create = WIDGET_FACTORY_23;
#endif   /* WIDGET_FACTORY_23 */
#ifdef WIDGET_FACTORY_24
    if (24 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_24;
    widget_factories[24].create = WIDGET_FACTORY_24;
#endif   /* WIDGET_FACTORY_24 */
#ifdef WIDGET_FACTORY_25
    if (25 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_25;
    widget_factories[25].create = WIDGET_FACTORY_25;
#endif   /* WIDGET_FACTORY_25 */
#ifdef WIDGET_FACTORY_26
    if (26 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_26;
    widget_factories[26].create = WIDGET_FACTORY_26;
#endif   /* WIDGET_FACTORY_26 */
#ifdef WIDGET_FACTORY_27
    if (27 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_27;
    widget_factories[27].create = WIDGET_FACTORY_27;
#endif   /* WIDGET_FACTORY_27 */
#ifdef WIDGET_FACTORY_28
    if (28 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_28;
    widget_factories[28].create = WIDGET_FACTORY_28;
#endif   /* WIDGET_FACTORY_28 */
#ifdef WIDGET_FACTORY_29
    if (29 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_29;
    widget_factories[29].create = WIDGET_FACTORY_29;
#endif   /* WIDGET_FACTORY_29 */
#ifdef WIDGET_FACTORY_30
    if (30 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_30;
    widget_factories[30].create = WIDGET_FACTORY_30;
#endif   /* WIDGET_FACTORY_30 */
#ifdef WIDGET_FACTORY_31
    if (31 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_31;
    widget_factories[31].create = WIDGET_FACTORY_31;
#endif   /* WIDGET_FACTORY_31 */


    return true;
}
