//
// Created by puhlz on 5/28/25.
//

#include "renderer.h"
#include "widget.h"

#include <raylib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


/* Raylib-specific rendering methods. */
static ic_err_t
raylib_render_init(const renderer_t *self, volatile vehicle_data_t *current_data);

static void
raylib_render_loading(const renderer_t *self, volatile vehicle_data_t *current_data);

static void
raylib_render_loop(const renderer_t *self, volatile vehicle_data_t *current_data);


/*
 * The primary desktop renderer container.
 *  Controls the rendering of individual widget components.
 */
static const renderer_t renderer = {
    .fps_limit = IC_OPT_FPS_LIMIT,
    .title = IC_OPT_GUI_TITLE,
    .resolution = { IC_OPT_SCREEN_WIDTH, IC_OPT_SCREEN_HEIGHT },
    .loop = raylib_render_loop,
    .loading = raylib_render_loading,
    .init = raylib_render_init,
};

const renderer_t *global_renderer = &renderer;


static ic_err_t
raylib_render_init(const renderer_t *self, volatile vehicle_data_t *current_data)
{
    ic_err_t config_status = ERR_OK;

#if IC_OPT_FULL_SCREEN==1
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(GetScreenWidth(), GetScreenHeight(), renderer.title);
    renderer.resolution.x = GetScreenWidth();
    renderer.resolution.y = GetScreenHeight();
#else   /* IC_OPT_FULL_SCREEN */
    InitWindow(renderer.resolution.x, renderer.resolution.y, renderer.title);
#endif   /* IC_OPT_FULL_SCREEN */

    SetTargetFPS(renderer.fps_limit);

    /* OK: Everything initialized with no issues. */
    return ERR_OK;
}

static void
raylib_render_loading(const renderer_t *self, volatile vehicle_data_t *current_data)
{
    return;
}


static void
raylib_render_loop(const renderer_t *self, volatile vehicle_data_t *current_data)
{
    while (!WindowShouldClose())
    {
        BeginDrawing();

#if IC_OPT_BG_STATIC==1
        ClearBackground(IC_OPT_BG_TOP_LEFT_RGBA);
#else   /* IC_OPT_BG_STATIC */
        DrawRectangleGradientEx(
            (Rectangle){
                .x = 0.0f,
                .y = 0.0f,
                .width = (float)renderer.resolution.x,
                .height = (float)renderer.resolution.y
            },
            (Color)IC_OPT_BG_TOP_LEFT_RGBA,
            (Color)IC_OPT_BG_BOTTOM_LEFT_RGBA,
            (Color)IC_OPT_BG_TOP_RIGHT_RGBA,
            (Color)IC_OPT_BG_BOTTOM_RIGHT_RGBA
        );
#endif   /* IC_OPT_BG_STATIC */

        if (NULL != global_widgets) {
            for (int i = 0; ; ++i) {
                if (NULL == global_widgets[i]) break;
            }
        }

        EndDrawing();
    }

    CloseWindow();
}

