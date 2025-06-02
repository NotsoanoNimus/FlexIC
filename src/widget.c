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

const char *widget_default_skin_name = "default";

/* Local only to this source file. */
#define MAX_REGISTRATIONS       128

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
     *      SIGNAL_NAME(S),WIDGET_LABEL,WIDGET_TYPE[:SKIN],VISIBLE,X,Y,W,H,Z-INDEX,WIDGET_OPTS (specific to each widget)
     *      In the case of the needle meter's default skin, we have...
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

        /* Quickly get the skin name, if defined. */
        char *skin_name = strtok(widget_type, ":");
        if (NULL == (skin_name = strtok(NULL, ":"))) {
            skin_name = (char *)DEFAULT_SKIN;
        }
        CONF_SUMMARIZE(skin_name);
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
        new_widget->type = strdup(widget_type);
        new_widget->label = strdup(widget_label);
        new_widget->skin_name = strdup(skin_name);
        new_widget->state.visible = 0 == strcasecmp(visible, "yes");
        new_widget->state.resolution.x = (int)strtol(width, &endptr, 10);   CHECK_STRTOL(width);
        new_widget->state.resolution.y = (int)strtol(height, &endptr, 10);  CHECK_STRTOL(height);
        new_widget->state.position.x = (int)strtol(x_pos, &endptr, 10);     CHECK_STRTOL(x_pos);
        new_widget->state.position.y = (int)strtol(y_pos, &endptr, 10);     CHECK_STRTOL(y_pos);
        new_widget->state.z_index = (int)strtol(z_index, &endptr, 10);      CHECK_STRTOL(z_index);

        /* Locate the factory method for the widget_type. */
        bool created = false;
        for (int i = 0; i < num_widget_factories; ++i) {
            if (__builtin_expect(NULL == widget_factories[i].name || NULL == widget_factories[i].create, false)) continue;

            if (__builtin_expect(0 != strcmp(widget_type, widget_factories[i].name), true)) continue;

            /* Create the params/args details for the new widget from the options string (delimited by ':'). */
            int argc = 0;
            char **argv = NULL;

            char *token = strtok(widget_opts, ":");
            if (NULL != token) {
                argv = malloc(1);   /* placeholder */
                if (NULL == argv) return ERR_OUT_OF_RESOURCES;

                do {
                    char **temp_argv = realloc(argv, sizeof(char *) * (argc + 1));
                    if (NULL == temp_argv) return ERR_OUT_OF_RESOURCES;
                    argv = temp_argv;

                    argv[argc] = strdup(token);
                    ++argc;
                } while (NULL != (token = strtok(NULL, ":")));
            }

            new_widget->argc = argc;
            new_widget->argv = argv;

#if IC_DEBUG==1
            DPRINTLN("Line %u parameters (%u):", line_num, argc);
            for (int i = 0; i < argc; i++) {
                DPRINTLN("\t>>> arg[%02u]:  '%s'", i, argv[i]);
            }
#endif   /* IC_DEBUG */

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
    DPRINTLN("global_widgets: Re-ordered based on ascending Z-INDEX.");

    return ERR_OK;
}


void
init_channel(
    widget_t *self,
    int channel_number,
    real_time_data_t **out
) {
    if (self->num_parent_signals <= channel_number) {
        fprintf(
            stderr,
            "\n\nFATAL:  Init'd widget '%s' (type '%s') does not have channel '%u' defined."
                "\n\tThis indicates a configuration problem. Make sure you've assigned all the right signals"
                "\n\t from your vehicle in the order specified by the '%s' widget's documentation.\n\n",
        self->label, self->type, channel_number, self->type);
        exit(EXIT_FAILURE);
    }
    *out = &CHANNEL(channel_number);
    DPRINTLN(
        "[%s] Initialized CHANNEL%u on signal '%s'.",
        self->label, channel_number, self->parent_signals[channel_number]->name
    );
}


void
set_hooks_for_skin(
    widget_t *self,
    const char *skin_name,
    _func__widget_update update_hook,
    _func__widget_draw draw_hook,
    _func__widget_parse_args parse_args_hook
) {
    if (0 != strcasecmp(skin_name, self->skin_name)) return;

    self->draw = draw_hook;
    self->update = update_hook;

    /* Parse skin-specific arguments/options. */
    ic_err_t args_status = ERR_OK;
    if (ERR_OK != (args_status = parse_args_hook(self))) {
        fprintf(
            stderr,
            "FATAL: widget(%s, %s): Failed to process widget_opts string according to skin requirements (e:%u).\n",
            self->label, self->type, args_status
        );
        exit(EXIT_FAILURE);
    }
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
    printf("\n");
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
#ifdef WIDGET_FACTORY_32
    if (32 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_32;
    widget_factories[32].create = WIDGET_FACTORY_32;
#endif   /* WIDGET_FACTORY_32 */
#ifdef WIDGET_FACTORY_33
    if (33 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_33;
    widget_factories[33].create = WIDGET_FACTORY_33;
#endif   /* WIDGET_FACTORY_33 */
#ifdef WIDGET_FACTORY_34
    if (34 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_34;
    widget_factories[34].create = WIDGET_FACTORY_34;
#endif   /* WIDGET_FACTORY_34 */
#ifdef WIDGET_FACTORY_35
    if (35 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_35;
    widget_factories[35].create = WIDGET_FACTORY_35;
#endif   /* WIDGET_FACTORY_35 */
#ifdef WIDGET_FACTORY_36
    if (36 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_36;
    widget_factories[36].create = WIDGET_FACTORY_36;
#endif   /* WIDGET_FACTORY_36 */
#ifdef WIDGET_FACTORY_37
    if (37 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_37;
    widget_factories[37].create = WIDGET_FACTORY_37;
#endif   /* WIDGET_FACTORY_37 */
#ifdef WIDGET_FACTORY_38
    if (38 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_38;
    widget_factories[38].create = WIDGET_FACTORY_38;
#endif   /* WIDGET_FACTORY_38 */
#ifdef WIDGET_FACTORY_39
    if (39 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_39;
    widget_factories[39].create = WIDGET_FACTORY_39;
#endif   /* WIDGET_FACTORY_39 */
#ifdef WIDGET_FACTORY_40
    if (40 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_40;
    widget_factories[40].create = WIDGET_FACTORY_40;
#endif   /* WIDGET_FACTORY_40 */
#ifdef WIDGET_FACTORY_41
    if (41 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_41;
    widget_factories[41].create = WIDGET_FACTORY_41;
#endif   /* WIDGET_FACTORY_41 */
#ifdef WIDGET_FACTORY_42
    if (42 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_42;
    widget_factories[42].create = WIDGET_FACTORY_42;
#endif   /* WIDGET_FACTORY_42 */
#ifdef WIDGET_FACTORY_43
    if (43 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_43;
    widget_factories[43].create = WIDGET_FACTORY_43;
#endif   /* WIDGET_FACTORY_43 */
#ifdef WIDGET_FACTORY_44
    if (44 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_44;
    widget_factories[44].create = WIDGET_FACTORY_44;
#endif   /* WIDGET_FACTORY_44 */
#ifdef WIDGET_FACTORY_45
    if (45 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_45;
    widget_factories[45].create = WIDGET_FACTORY_45;
#endif   /* WIDGET_FACTORY_45 */
#ifdef WIDGET_FACTORY_46
    if (46 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_46;
    widget_factories[46].create = WIDGET_FACTORY_46;
#endif   /* WIDGET_FACTORY_46 */
#ifdef WIDGET_FACTORY_47
    if (47 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_47;
    widget_factories[47].create = WIDGET_FACTORY_47;
#endif   /* WIDGET_FACTORY_47 */
#ifdef WIDGET_FACTORY_48
    if (48 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_48;
    widget_factories[48].create = WIDGET_FACTORY_48;
#endif   /* WIDGET_FACTORY_48 */
#ifdef WIDGET_FACTORY_49
    if (49 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_49;
    widget_factories[49].create = WIDGET_FACTORY_49;
#endif   /* WIDGET_FACTORY_49 */
#ifdef WIDGET_FACTORY_50
    if (50 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_50;
    widget_factories[50].create = WIDGET_FACTORY_50;
#endif   /* WIDGET_FACTORY_50 */
#ifdef WIDGET_FACTORY_51
    if (51 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_51;
    widget_factories[51].create = WIDGET_FACTORY_51;
#endif   /* WIDGET_FACTORY_51 */
#ifdef WIDGET_FACTORY_52
    if (52 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_52;
    widget_factories[52].create = WIDGET_FACTORY_52;
#endif   /* WIDGET_FACTORY_52 */
#ifdef WIDGET_FACTORY_53
    if (53 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_53;
    widget_factories[53].create = WIDGET_FACTORY_53;
#endif   /* WIDGET_FACTORY_53 */
#ifdef WIDGET_FACTORY_54
    if (54 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_54;
    widget_factories[54].create = WIDGET_FACTORY_54;
#endif   /* WIDGET_FACTORY_54 */
#ifdef WIDGET_FACTORY_55
    if (55 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_55;
    widget_factories[55].create = WIDGET_FACTORY_55;
#endif   /* WIDGET_FACTORY_55 */
#ifdef WIDGET_FACTORY_56
    if (56 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_56;
    widget_factories[56].create = WIDGET_FACTORY_56;
#endif   /* WIDGET_FACTORY_56 */
#ifdef WIDGET_FACTORY_57
    if (57 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_57;
    widget_factories[57].create = WIDGET_FACTORY_57;
#endif   /* WIDGET_FACTORY_57 */
#ifdef WIDGET_FACTORY_58
    if (58 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_58;
    widget_factories[58].create = WIDGET_FACTORY_58;
#endif   /* WIDGET_FACTORY_58 */
#ifdef WIDGET_FACTORY_59
    if (59 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_59;
    widget_factories[59].create = WIDGET_FACTORY_59;
#endif   /* WIDGET_FACTORY_59 */
#ifdef WIDGET_FACTORY_60
    if (60 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_60;
    widget_factories[60].create = WIDGET_FACTORY_60;
#endif   /* WIDGET_FACTORY_60 */
#ifdef WIDGET_FACTORY_61
    if (61 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_61;
    widget_factories[61].create = WIDGET_FACTORY_61;
#endif   /* WIDGET_FACTORY_61 */
#ifdef WIDGET_FACTORY_62
    if (62 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_62;
    widget_factories[62].create = WIDGET_FACTORY_62;
#endif   /* WIDGET_FACTORY_62 */
#ifdef WIDGET_FACTORY_63
    if (63 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_63;
    widget_factories[63].create = WIDGET_FACTORY_63;
#endif   /* WIDGET_FACTORY_63 */
#ifdef WIDGET_FACTORY_64
    if (64 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_64;
    widget_factories[64].create = WIDGET_FACTORY_64;
#endif   /* WIDGET_FACTORY_64 */
#ifdef WIDGET_FACTORY_65
    if (65 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_65;
    widget_factories[65].create = WIDGET_FACTORY_65;
#endif   /* WIDGET_FACTORY_65 */
#ifdef WIDGET_FACTORY_66
    if (66 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_66;
    widget_factories[66].create = WIDGET_FACTORY_66;
#endif   /* WIDGET_FACTORY_66 */
#ifdef WIDGET_FACTORY_67
    if (67 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_67;
    widget_factories[67].create = WIDGET_FACTORY_67;
#endif   /* WIDGET_FACTORY_67 */
#ifdef WIDGET_FACTORY_68
    if (68 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_68;
    widget_factories[68].create = WIDGET_FACTORY_68;
#endif   /* WIDGET_FACTORY_68 */
#ifdef WIDGET_FACTORY_69
    if (69 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_69;
    widget_factories[69].create = WIDGET_FACTORY_69;
#endif   /* WIDGET_FACTORY_69 */
#ifdef WIDGET_FACTORY_70
    if (70 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_70;
    widget_factories[70].create = WIDGET_FACTORY_70;
#endif   /* WIDGET_FACTORY_70 */
#ifdef WIDGET_FACTORY_71
    if (71 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_71;
    widget_factories[71].create = WIDGET_FACTORY_71;
#endif   /* WIDGET_FACTORY_71 */
#ifdef WIDGET_FACTORY_72
    if (72 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_72;
    widget_factories[72].create = WIDGET_FACTORY_72;
#endif   /* WIDGET_FACTORY_72 */
#ifdef WIDGET_FACTORY_73
    if (73 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_73;
    widget_factories[73].create = WIDGET_FACTORY_73;
#endif   /* WIDGET_FACTORY_73 */
#ifdef WIDGET_FACTORY_74
    if (74 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_74;
    widget_factories[74].create = WIDGET_FACTORY_74;
#endif   /* WIDGET_FACTORY_74 */
#ifdef WIDGET_FACTORY_75
    if (75 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_75;
    widget_factories[75].create = WIDGET_FACTORY_75;
#endif   /* WIDGET_FACTORY_75 */
#ifdef WIDGET_FACTORY_76
    if (76 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_76;
    widget_factories[76].create = WIDGET_FACTORY_76;
#endif   /* WIDGET_FACTORY_76 */
#ifdef WIDGET_FACTORY_77
    if (77 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_77;
    widget_factories[77].create = WIDGET_FACTORY_77;
#endif   /* WIDGET_FACTORY_77 */
#ifdef WIDGET_FACTORY_78
    if (78 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_78;
    widget_factories[78].create = WIDGET_FACTORY_78;
#endif   /* WIDGET_FACTORY_78 */
#ifdef WIDGET_FACTORY_79
    if (79 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_79;
    widget_factories[79].create = WIDGET_FACTORY_79;
#endif   /* WIDGET_FACTORY_79 */
#ifdef WIDGET_FACTORY_80
    if (80 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_80;
    widget_factories[80].create = WIDGET_FACTORY_80;
#endif   /* WIDGET_FACTORY_80 */
#ifdef WIDGET_FACTORY_81
    if (81 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_81;
    widget_factories[81].create = WIDGET_FACTORY_81;
#endif   /* WIDGET_FACTORY_81 */
#ifdef WIDGET_FACTORY_82
    if (82 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_82;
    widget_factories[82].create = WIDGET_FACTORY_82;
#endif   /* WIDGET_FACTORY_82 */
#ifdef WIDGET_FACTORY_83
    if (83 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_83;
    widget_factories[83].create = WIDGET_FACTORY_83;
#endif   /* WIDGET_FACTORY_83 */
#ifdef WIDGET_FACTORY_84
    if (84 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_84;
    widget_factories[84].create = WIDGET_FACTORY_84;
#endif   /* WIDGET_FACTORY_84 */
#ifdef WIDGET_FACTORY_85
    if (85 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_85;
    widget_factories[85].create = WIDGET_FACTORY_85;
#endif   /* WIDGET_FACTORY_85 */
#ifdef WIDGET_FACTORY_86
    if (86 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_86;
    widget_factories[86].create = WIDGET_FACTORY_86;
#endif   /* WIDGET_FACTORY_86 */
#ifdef WIDGET_FACTORY_87
    if (87 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_87;
    widget_factories[87].create = WIDGET_FACTORY_87;
#endif   /* WIDGET_FACTORY_87 */
#ifdef WIDGET_FACTORY_88
    if (88 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_88;
    widget_factories[88].create = WIDGET_FACTORY_88;
#endif   /* WIDGET_FACTORY_88 */
#ifdef WIDGET_FACTORY_89
    if (89 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_89;
    widget_factories[89].create = WIDGET_FACTORY_89;
#endif   /* WIDGET_FACTORY_89 */
#ifdef WIDGET_FACTORY_90
    if (90 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_90;
    widget_factories[90].create = WIDGET_FACTORY_90;
#endif   /* WIDGET_FACTORY_90 */
#ifdef WIDGET_FACTORY_91
    if (91 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_91;
    widget_factories[91].create = WIDGET_FACTORY_91;
#endif   /* WIDGET_FACTORY_91 */
#ifdef WIDGET_FACTORY_92
    if (92 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_92;
    widget_factories[92].create = WIDGET_FACTORY_92;
#endif   /* WIDGET_FACTORY_92 */
#ifdef WIDGET_FACTORY_93
    if (93 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_93;
    widget_factories[93].create = WIDGET_FACTORY_93;
#endif   /* WIDGET_FACTORY_93 */
#ifdef WIDGET_FACTORY_94
    if (94 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_94;
    widget_factories[94].create = WIDGET_FACTORY_94;
#endif   /* WIDGET_FACTORY_94 */
#ifdef WIDGET_FACTORY_95
    if (95 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_95;
    widget_factories[95].create = WIDGET_FACTORY_95;
#endif   /* WIDGET_FACTORY_95 */
#ifdef WIDGET_FACTORY_96
    if (96 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_96;
    widget_factories[96].create = WIDGET_FACTORY_96;
#endif   /* WIDGET_FACTORY_96 */
#ifdef WIDGET_FACTORY_97
    if (97 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_97;
    widget_factories[97].create = WIDGET_FACTORY_97;
#endif   /* WIDGET_FACTORY_97 */
#ifdef WIDGET_FACTORY_98
    if (98 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_98;
    widget_factories[98].create = WIDGET_FACTORY_98;
#endif   /* WIDGET_FACTORY_98 */
#ifdef WIDGET_FACTORY_99
    if (99 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_99;
    widget_factories[99].create = WIDGET_FACTORY_99;
#endif   /* WIDGET_FACTORY_99 */
#ifdef WIDGET_FACTORY_100
    if (100 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_100;
    widget_factories[100].create = WIDGET_FACTORY_100;
#endif   /* WIDGET_FACTORY_100 */
#ifdef WIDGET_FACTORY_101
    if (101 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_101;
    widget_factories[101].create = WIDGET_FACTORY_101;
#endif   /* WIDGET_FACTORY_101 */
#ifdef WIDGET_FACTORY_102
    if (102 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_102;
    widget_factories[102].create = WIDGET_FACTORY_102;
#endif   /* WIDGET_FACTORY_102 */
#ifdef WIDGET_FACTORY_103
    if (103 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_103;
    widget_factories[103].create = WIDGET_FACTORY_103;
#endif   /* WIDGET_FACTORY_103 */
#ifdef WIDGET_FACTORY_104
    if (104 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_104;
    widget_factories[104].create = WIDGET_FACTORY_104;
#endif   /* WIDGET_FACTORY_104 */
#ifdef WIDGET_FACTORY_105
    if (105 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_105;
    widget_factories[105].create = WIDGET_FACTORY_105;
#endif   /* WIDGET_FACTORY_105 */
#ifdef WIDGET_FACTORY_106
    if (106 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_106;
    widget_factories[106].create = WIDGET_FACTORY_106;
#endif   /* WIDGET_FACTORY_106 */
#ifdef WIDGET_FACTORY_107
    if (107 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_107;
    widget_factories[107].create = WIDGET_FACTORY_107;
#endif   /* WIDGET_FACTORY_107 */
#ifdef WIDGET_FACTORY_108
    if (108 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_108;
    widget_factories[108].create = WIDGET_FACTORY_108;
#endif   /* WIDGET_FACTORY_108 */
#ifdef WIDGET_FACTORY_109
    if (109 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_109;
    widget_factories[109].create = WIDGET_FACTORY_109;
#endif   /* WIDGET_FACTORY_109 */
#ifdef WIDGET_FACTORY_110
    if (110 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_110;
    widget_factories[110].create = WIDGET_FACTORY_110;
#endif   /* WIDGET_FACTORY_110 */
#ifdef WIDGET_FACTORY_111
    if (111 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_111;
    widget_factories[111].create = WIDGET_FACTORY_111;
#endif   /* WIDGET_FACTORY_111 */
#ifdef WIDGET_FACTORY_112
    if (112 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_112;
    widget_factories[112].create = WIDGET_FACTORY_112;
#endif   /* WIDGET_FACTORY_112 */
#ifdef WIDGET_FACTORY_113
    if (113 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_113;
    widget_factories[113].create = WIDGET_FACTORY_113;
#endif   /* WIDGET_FACTORY_113 */
#ifdef WIDGET_FACTORY_114
    if (114 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_114;
    widget_factories[114].create = WIDGET_FACTORY_114;
#endif   /* WIDGET_FACTORY_114 */
#ifdef WIDGET_FACTORY_115
    if (115 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_115;
    widget_factories[115].create = WIDGET_FACTORY_115;
#endif   /* WIDGET_FACTORY_115 */
#ifdef WIDGET_FACTORY_116
    if (116 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_116;
    widget_factories[116].create = WIDGET_FACTORY_116;
#endif   /* WIDGET_FACTORY_116 */
#ifdef WIDGET_FACTORY_117
    if (117 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_117;
    widget_factories[117].create = WIDGET_FACTORY_117;
#endif   /* WIDGET_FACTORY_117 */
#ifdef WIDGET_FACTORY_118
    if (118 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_118;
    widget_factories[118].create = WIDGET_FACTORY_118;
#endif   /* WIDGET_FACTORY_118 */
#ifdef WIDGET_FACTORY_119
    if (119 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_119;
    widget_factories[119].create = WIDGET_FACTORY_119;
#endif   /* WIDGET_FACTORY_119 */
#ifdef WIDGET_FACTORY_120
    if (120 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_120;
    widget_factories[120].create = WIDGET_FACTORY_120;
#endif   /* WIDGET_FACTORY_120 */
#ifdef WIDGET_FACTORY_121
    if (121 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_121;
    widget_factories[121].create = WIDGET_FACTORY_121;
#endif   /* WIDGET_FACTORY_121 */
#ifdef WIDGET_FACTORY_122
    if (122 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_122;
    widget_factories[122].create = WIDGET_FACTORY_122;
#endif   /* WIDGET_FACTORY_122 */
#ifdef WIDGET_FACTORY_123
    if (123 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_123;
    widget_factories[123].create = WIDGET_FACTORY_123;
#endif   /* WIDGET_FACTORY_123 */
#ifdef WIDGET_FACTORY_124
    if (124 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_124;
    widget_factories[124].create = WIDGET_FACTORY_124;
#endif   /* WIDGET_FACTORY_124 */
#ifdef WIDGET_FACTORY_125
    if (125 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_125;
    widget_factories[125].create = WIDGET_FACTORY_125;
#endif   /* WIDGET_FACTORY_125 */
#ifdef WIDGET_FACTORY_126
    if (126 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_126;
    widget_factories[126].create = WIDGET_FACTORY_126;
#endif   /* WIDGET_FACTORY_126 */
#ifdef WIDGET_FACTORY_127
    if (127 >= num_widget_factories) return false;
    extern func__widget_factory_create WIDGET_FACTORY_127;
    widget_factories[127].create = WIDGET_FACTORY_127;
#endif   /* WIDGET_FACTORY_127 */

    return true;
}
