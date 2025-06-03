#include "widget_common.h"

#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <string.h>


typedef enum
{
    NEEDLE_DEFAULT,
    NEEDLE_DIAMOND,
    NEEDLE_GROOVE,
} NeedleStyle;

// !!!!!!!!!!!!!!!!!!!!
// TODO: REMOVE STATIC OBJECTS BECAUSE REFERENCES ARE SHARED ACROSS ALL WIDGET
//    INSTANCES OF THIS TYPE.
struct draw_params
{
    Vector2 center;

    float needle_scale;
    float needle_pivot_radius;

    float inner_radius;
    float outer_radius;
    float start_angle;
    float end_angle;
    float start_angle_ticks;
    float end_angle_ticks;
    int segments;

    float minimum_value;
    float maximum_value;
    float interval;
    float sub_interval;
    float redline_start_value;
    bool redline_fades_in;

    float tick_thickness;
    float sub_tick_thickness;

    int label_text_size;
    bool has_labels;

    char *unit_text;
    int unit_text_size;
    struct { int x; int y; } unit_text_position;

    Color backdrop_color;
    Color border_color;
    Color tick_color;
    Color text_color;
    Color needle_color;
    Color needle_pivot_color;

    NeedleStyle needle_style;
} local_params;

static RenderTexture2D static_texture;
static RenderTexture2D needle_texture;

static float needle_degrees = 0.0f;
static double prev_value = UINT64_MAX - 20;   /* ensuring a first update */


static ic_err_t
needle_meter__default__init(widget_t *self, const renderer_t *renderer)
{
    char label_value[16] = {0};

    static_texture = LoadRenderTexture(MY_WIDTH, MY_HEIGHT);
    needle_texture = LoadRenderTexture(MY_WIDTH, MY_HEIGHT);

    BeginTextureMode(static_texture);

    DrawRing(
        local_params.center,
        local_params.inner_radius,
        local_params.outer_radius,
        local_params.start_angle,
        local_params.end_angle,
        local_params.segments,
        local_params.backdrop_color
    );

    float min_max_diff = local_params.maximum_value - local_params.minimum_value;
    int minor_ticks = floor(min_max_diff / local_params.sub_interval);
    float ticks_angle_span = fabs(local_params.end_angle_ticks - local_params.start_angle_ticks);
    float minor_tick_span = fabs(ticks_angle_span) / minor_ticks;   /* full angle breadth / total minor ticks */

    /* Tick line lengths. */
    float minor_interval_line_length = CLAMP((local_params.outer_radius * 0.030f), 7.0f, 50.0f);
    float major_interval_line_length = CLAMP((local_params.outer_radius * 0.080f), 15.0f, 100.0f);

    for (
        float i = local_params.start_angle_ticks, value_at_tick = local_params.minimum_value;
        i <= local_params.end_angle_ticks;
        i += minor_tick_span, value_at_tick += local_params.sub_interval
    ) {
        float rad = i * DEG2RAD;
        bool is_major_tick = is_multiple(value_at_tick, local_params.interval);
        float interval_line_len = is_major_tick ? major_interval_line_length : minor_interval_line_length;

        float label_x = local_params.center.x + (local_params.outer_radius - interval_line_len - 15) * cosf(rad);
        float label_y = local_params.center.y + (local_params.outer_radius - interval_line_len - 15) * sinf(rad);

        float inner_x = local_params.center.x + (local_params.outer_radius - interval_line_len) * cosf(rad);
        float inner_y = local_params.center.y + (local_params.outer_radius - interval_line_len) * sinf(rad);
        float outer_x = local_params.center.x + local_params.outer_radius * cosf(rad);
        float outer_y = local_params.center.y + local_params.outer_radius * sinf(rad);

        /* The formula here is using a difference between the value to gravitate towards and the percentage
         * of the gap remaining between the current and max meter value. Will need to wrap back and explore simplifying this. */
        Color tick_color;
        if (value_at_tick > local_params.redline_start_value) {
            tick_color = local_params.redline_fades_in
                ? (Color){
                    .r = (local_params.tick_color.r + ((0xFF - local_params.tick_color.r) * ((value_at_tick - local_params.redline_start_value) / (local_params.maximum_value - local_params.redline_start_value)))),
                    .g = (local_params.tick_color.g - ((local_params.tick_color.g) * ((value_at_tick - local_params.redline_start_value) / (local_params.maximum_value - local_params.redline_start_value)))),
                    .b = (local_params.tick_color.b - ((local_params.tick_color.b) * ((value_at_tick - local_params.redline_start_value) / (local_params.maximum_value - local_params.redline_start_value)))),
                    .a = (local_params.tick_color.a + ((0xFF - local_params.tick_color.a) * ((value_at_tick - local_params.redline_start_value) / (local_params.maximum_value - local_params.redline_start_value))))
                }
                : (Color){ .r = 0xFF, .g = 0, .b = 0, .a = 0xFF };
        } else tick_color = local_params.tick_color;

        DrawLineEx(
            (Vector2){inner_x, inner_y},
            (Vector2){outer_x, outer_y},
            is_major_tick ? local_params.tick_thickness : local_params.sub_tick_thickness,
            tick_color
        );

        if (is_major_tick && local_params.has_labels) {
            snprintf(label_value, 16, "%i", (int)floor(value_at_tick));
            DrawText(
                label_value,
                label_x,
                label_y,
                local_params.label_text_size,
                local_params.tick_color
            );
        }
    }

    DrawRingLines(
        local_params.center,
        local_params.inner_radius,
        local_params.outer_radius,
        local_params.start_angle,
        local_params.end_angle,
        local_params.segments,
        local_params.border_color
    );

    DrawText(
        local_params.unit_text,
        local_params.unit_text_position.x,
        local_params.unit_text_position.y,
        local_params.unit_text_size,
        local_params.text_color
    );

    EndTextureMode();


    BeginTextureMode(needle_texture);

    needle_degrees = 0.0f;
    float needle_x = local_params.center.x + (local_params.outer_radius - 10) * cosf(needle_degrees);
    float needle_y = local_params.center.y + (local_params.outer_radius - 10) * sinf(needle_degrees);

    // TODO: This will probably look better as a triangle shape.
    DrawLineEx(
        (Vector2){ local_params.center.x, local_params.center.y },
        (Vector2){ needle_x, needle_y },
        4.0f,   // TODO: Needle styles.
        local_params.needle_color
    );

    /* Needle center pivot overlay. */
    DrawCircleV(
        (Vector2){ local_params.center.x, local_params.center.y },
        MAX(local_params.needle_pivot_radius, 6.0f),
        local_params.needle_pivot_color
    );

    EndTextureMode();

    return ERR_OK;
}


static void
needle_meter__default__update(widget_t *self)
{
    // NOTE: Everything from here below MUST BE DRAWN RELATIVE TO THE SCREEN ORIGIN, not the texture origin.

    /* Draw needle (angle from center) */
    float needle_value = (needle_data->value * local_params.needle_scale) - local_params.minimum_value;

    float needle_angle = local_params.start_angle_ticks +
        ((needle_value / (local_params.maximum_value - local_params.minimum_value))
            * (local_params.end_angle_ticks - local_params.start_angle_ticks));

    needle_angle = CLAMP(needle_angle, local_params.start_angle_ticks, local_params.end_angle_ticks);

    needle_degrees = needle_angle;
}


static void
needle_meter__default__draw(widget_t *self, const renderer_t *renderer)
{
    /* Render static needle_meter background content from the preloaded texture. */
    DrawTexturePro(
        static_texture.texture,
        (Rectangle){ 0, 0, MY_WIDTH, -MY_HEIGHT },
        (Rectangle){ MY_X, MY_Y, MY_WIDTH, MY_HEIGHT },
        (Vector2){ 0, 0 },
        MY_ANGLE,
        WHITE
    );

    DrawTexturePro(
        needle_texture.texture,
        (Rectangle){ 0, 0, MY_WIDTH, -MY_HEIGHT },
        (Rectangle){ MY_X + local_params.center.x, MY_Y + local_params.center.y, MY_WIDTH, MY_HEIGHT },
        (Vector2){ local_params.center.x, local_params.center.y },
        // (Vector2){0,0},
        needle_degrees,
        WHITE
    );
}


static ic_err_t
needle_meter__default__parse_args(widget_t *self)
{
    char *option = NULL, *name = NULL;

    local_params.center = (Vector2){
        .x = self->state.resolution.x / 2,
        .y = self->state.resolution.y / 2
    };

    /* Fixed outer radius. */
    local_params.outer_radius = MIN(self->state.resolution.x / 2, self->state.resolution.y / 2);

    OPTION_OR_DIE("inner_radius");
    local_params.inner_radius = atof(option);
    if (local_params.inner_radius >= local_params.outer_radius) {
        fprintf(stderr, "FATAL:  needle_meter cannot have an inner_radius >= its outer_radius.\n");
        return ERR_ARGS;
    }
    DPRINTLN("inner_radius(%u)", local_params.inner_radius);

    OPTION_OR_DIE("needle_scale");
    local_params.needle_scale = atof(option);
    DPRINTLN("needle_scale(%f)", local_params.needle_scale);

    OPTION_OR_DIE("segments");
    local_params.segments = atoi(option);
    DPRINTLN("segments(%u)", local_params.segments);

    OPTION_OR_DIE("start_angle");
    local_params.start_angle = atof(option);
    DPRINTLN("start_angle(%f)", local_params.start_angle);

    OPTION_OR_DIE("end_angle");
    local_params.end_angle = atof(option);
    DPRINTLN("end_angle(%f)", local_params.end_angle);

    OPTION_OR_DIE("start_angle_ticks");
    local_params.start_angle_ticks = atof(option);
    DPRINTLN("start_angle_ticks(%f)", local_params.start_angle_ticks);

    OPTION_OR_DIE("end_angle_ticks");
    local_params.end_angle_ticks = atof(option);
    DPRINTLN("end_angle_ticks(%f)", local_params.end_angle_ticks);

    OPTION_OR_DIE("minimum_value");
    local_params.minimum_value = atof(option);
    DPRINTLN("minimum_value(%f)", local_params.minimum_value);

    OPTION_OR_DIE("maximum_value");
    local_params.maximum_value = atof(option);
    DPRINTLN("maximum_value(%f)", local_params.maximum_value);

    OPTION_OR_DIE("interval");
    local_params.interval = atof(option);
    DPRINTLN("interval(%f)", local_params.interval);

    OPTION_OR_DIE("sub_interval");
    local_params.sub_interval = atof(option);
    DPRINTLN("sub_interval(%f)", local_params.sub_interval);

    OPTION_OR_DIE("redline_start_value");
    local_params.redline_start_value = atof(option);
    DPRINTLN("redline_start_value(%f)", local_params.redline_start_value);

    OPTION_OR_DIE("redline_fades_in");
    local_params.redline_fades_in = (0 == strcmp(option, "True")) ? true : false;
    DPRINTLN("redline_fades_in(%u)", local_params.redline_fades_in);

    OPTION_OR_DIE("tick_thickness");
    local_params.tick_thickness = atof(option);
    DPRINTLN("tick_thickness(%f)", local_params.tick_thickness);

    OPTION_OR_DIE("sub_tick_thickness");
    local_params.sub_tick_thickness = atof(option);
    DPRINTLN("sub_tick_thickness(%f)", local_params.sub_tick_thickness);

    OPTION_OR_DIE("needle_pivot_radius");
    local_params.needle_pivot_radius = atof(option);
    DPRINTLN("needle_pivot_radius(%f)", local_params.needle_pivot_radius);

    OPTION_OR_DIE("label_text_size");
    local_params.label_text_size = atoi(option);
    DPRINTLN("label_text_size(%u)", local_params.label_text_size);

    OPTION_OR_DIE("has_labels");
    local_params.has_labels = 0 == strcasecmp(option, "True");
    DPRINTLN("has_labels(%u)", local_params.has_labels);

    OPTION_OR_DIE("unit_text");
    local_params.unit_text = option;
    DPRINTLN("unit_text(%s)", local_params.unit_text);

    OPTION_OR_DIE("unit_text_size");
    local_params.unit_text_size = atoi(option);
    DPRINTLN("unit_text_size(%u)", local_params.unit_text_size);

    OPTION_OR_DIE("unit_text_position");
    char *x = strtok(option, ":");
    char *y = strtok(NULL, ":");
    if (x[strlen(x)-1] == '%') {
        x[strlen(x)-1] = '\0';
        local_params.unit_text_position.x = (atof(x) / 100.0f) * MY_WIDTH;
    } else local_params.unit_text_position.x = atoi(x);
    if (y[strlen(y)-1] == '%') {
        y[strlen(y)-1] = '\0';
        local_params.unit_text_position.y = (atof(y) / 100.0f) * MY_HEIGHT;
    } else local_params.unit_text_position.y = atof(y);
    if (NULL == x || NULL == y) goto param_error;
    DPRINTLN("unit_text_position(%u,%u)", local_params.unit_text_position.x, local_params.unit_text_position.y);

    OPTION_OR_DIE("backdrop_color");
    if (8 != strlen(option)) goto param_error;
    option = strdup(option);
    local_params.backdrop_color.a = hex_to_value(&option[6]); option[6] = '\0';
    local_params.backdrop_color.b = hex_to_value(&option[4]); option[4] = '\0';
    local_params.backdrop_color.g = hex_to_value(&option[2]); option[2] = '\0';
    local_params.backdrop_color.r = hex_to_value(&option[0]);
    free(option);
    DPRINT("backdrop_color "); MEMDUMP(&local_params.backdrop_color, sizeof(Color));

    OPTION_OR_DIE("border_color");
    if (8 != strlen(option)) goto param_error;
    option = strdup(option);
    local_params.border_color.a = hex_to_value(&option[6]); option[6] = '\0';
    local_params.border_color.b = hex_to_value(&option[4]); option[4] = '\0';
    local_params.border_color.g = hex_to_value(&option[2]); option[2] = '\0';
    local_params.border_color.r = hex_to_value(&option[0]);
    free(option);
    DPRINT("border_color "); MEMDUMP(&local_params.border_color, sizeof(Color));

    OPTION_OR_DIE("tick_color");
    if (8 != strlen(option)) goto param_error;
    option = strdup(option);
    local_params.tick_color.a = hex_to_value(&option[6]); option[6] = '\0';
    local_params.tick_color.b = hex_to_value(&option[4]); option[4] = '\0';
    local_params.tick_color.g = hex_to_value(&option[2]); option[2] = '\0';
    local_params.tick_color.r = hex_to_value(&option[0]);
    free(option);
    DPRINT("tick_color "); MEMDUMP(&local_params.tick_color, sizeof(Color));

    OPTION_OR_DIE("text_color");
    if (8 != strlen(option)) goto param_error;
    option = strdup(option);
    local_params.text_color.a = hex_to_value(&option[6]); option[6] = '\0';
    local_params.text_color.b = hex_to_value(&option[4]); option[4] = '\0';
    local_params.text_color.g = hex_to_value(&option[2]); option[2] = '\0';
    local_params.text_color.r = hex_to_value(&option[0]);
    free(option);
    DPRINT("text_color "); MEMDUMP(&local_params.text_color, sizeof(Color));

    OPTION_OR_DIE("needle_color");
    if (8 != strlen(option)) goto param_error;
    option = strdup(option);
    local_params.needle_color.a = hex_to_value(&option[6]); option[6] = '\0';
    local_params.needle_color.b = hex_to_value(&option[4]); option[4] = '\0';
    local_params.needle_color.g = hex_to_value(&option[2]); option[2] = '\0';
    local_params.needle_color.r = hex_to_value(&option[0]);
    free(option);
    DPRINT("needle_color "); MEMDUMP(&local_params.needle_color, sizeof(Color));

    OPTION_OR_DIE("needle_pivot_color");
    if (8 != strlen(option)) goto param_error;
    option = strdup(option);
    local_params.needle_pivot_color.a = hex_to_value(&option[6]); option[6] = '\0';
    local_params.needle_pivot_color.b = hex_to_value(&option[4]); option[4] = '\0';
    local_params.needle_pivot_color.g = hex_to_value(&option[2]); option[2] = '\0';
    local_params.needle_pivot_color.r = hex_to_value(&option[0]);
    free(option);
    DPRINT("needle_pivot_color "); MEMDUMP(&local_params.needle_pivot_color, sizeof(Color));

    return ERR_OK;

param_error:
    fprintf(stderr, "FATAL: Widget option/argument '%s' was not found or is not valid for its type.\n", name);
    return ERR_ARGS;
}
