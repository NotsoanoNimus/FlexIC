//
// Created by puhlz on 5/28/25.
//

#include <pthread.h>

#include "renderer.h"
#include "widget.h"

#include <raylib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


/* Raylib-specific rendering methods. */
static ic_err_t
raylib_render_init(const renderer_t *self);

static void
raylib_render_loading(const renderer_t *self);

static void
raylib_render_loop(const renderer_t *self);


/*
 * The primary desktop renderer container.
 *  Controls the rendering of individual widget components.
 */
static renderer_t renderer = {
    .loop = raylib_render_loop,
    .init = raylib_render_init,
};

const renderer_t *global_renderer = (const renderer_t *)&renderer;


static Texture2D background_texture;
static Image background_image;

static ic_err_t
raylib_render_init(const renderer_t *self)
{
    renderer.fps_limit = compile_time_ic_options.window.fps_limit;
    renderer.title = compile_time_ic_options.window.title;
    renderer.resolution = compile_time_ic_options.window.dimensions;

#if IC_OPT_FULL_SCREEN==1
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(GetScreenWidth(), GetScreenHeight(), renderer.title);
    renderer.resolution.x = GetScreenWidth();
    renderer.resolution.y = GetScreenHeight();
#else   /* IC_OPT_FULL_SCREEN */
    InitWindow(renderer.resolution.x, renderer.resolution.y, renderer.title);
#endif   /* IC_OPT_FULL_SCREEN */

    SetTargetFPS(renderer.fps_limit);

    if (ASSET == compile_time_ic_options.background_type) {
        background_image = LoadImageFromMemory(
            compile_time_ic_options.background_asset.file_type,
            compile_time_ic_options.background_asset.image_data,
            compile_time_ic_options.background_asset.image_size
        );

        if (NULL == background_image.data) {
            fprintf(stderr, "FATAL: Failed to load background image asset.\n");
            return ERR_INVALID_CONFIGURATION;
        }

        background_texture = LoadTextureFromImage(background_image);
    }

    /* OK: Everything initialized with no issues. */
    return ERR_OK;
}


static void
raylib_render_loop(const renderer_t *self)
{
    RenderTexture2D outline_texture = LoadRenderTexture(self->resolution.x, self->resolution.y);

    if (0 == num_global_widgets || NULL == global_widgets) {
        fprintf(stderr, "ERROR: Empty or NULL global_widgets.\n");
        return;
    }

    /* Pre-init all widgets (for those that have a hook for it). */
    bool any_outlines = false;

    for (int i = 0; i < num_global_widgets; ++i) {
        if (global_widgets[i]->draw_outline) any_outlines = true;

        if (NULL == global_widgets[i]->init) continue;

        global_widgets[i]->init(global_widgets[i], self);
    }

#if IC_DEBUG==1 && IC_OPT_DISABLE_RENDER_TIME!=1
    double *clock_samples = calloc(1, sizeof(double *) * compile_time_ic_options.window.fps_limit);
    int clock_sample_count = 0;
#endif   /* IC_DEBUG */

    while (!WindowShouldClose())
    {
#if IC_DEBUG==1 && IC_OPT_DISABLE_RENDER_TIME!=1
        clock_t begin = clock();
#endif   /* IC_DEBUG */

        /* Widget updates. */
        for (int i = 0; i < num_global_widgets; ++i)
            global_widgets[i]->update(global_widgets[i]);

        BeginDrawing();

        ClearBackground(ASSET == compile_time_ic_options.background_type
            ? BLACK : compile_time_ic_options.background_color.static_color);

        if (ASSET == compile_time_ic_options.background_type) {
            DrawTexturePro(
                background_texture,
                compile_time_ic_options.background_asset.fit_to_window
                    ? (Rectangle) { 0, 0, background_image.width, background_image.height }
                    : (Rectangle) {
                        0,
                        0,
                        background_image.width - compile_time_ic_options.background_asset.offset_x,
                        background_image.height - compile_time_ic_options.background_asset.offset_y
                    },
                compile_time_ic_options.background_asset.fit_to_window
                    ? (Rectangle) { 0, 0, GetScreenWidth(), GetScreenHeight() }
                    : (Rectangle) {
                        compile_time_ic_options.background_asset.offset_x,
                        compile_time_ic_options.background_asset.offset_y,
                        GetScreenWidth() - compile_time_ic_options.background_asset.offset_x,
                        GetScreenHeight() - compile_time_ic_options.background_asset.offset_y
                    },
                (Vector2) { 0, 0 },
                0.0f,
                compile_time_ic_options.background_asset.tint
            );
        }

        // TODO: Warn many times that rendering these with "draw_boundary_outline" is SLOW!!!
        if (any_outlines) {
            BeginTextureMode(outline_texture);
            ClearBackground((Color){0,0,0,0});

            for (int i = 0; i < num_global_widgets; ++i) {
                if (!global_widgets[i]->draw_outline) continue;

                /* Add a red box around the boundary of the widget to outline it. */
                DrawRectangleLinesEx(
                    (Rectangle){
                        (float)global_widgets[i]->state.position.x,
                        (float)global_widgets[i]->state.position.y,
                        (float)global_widgets[i]->state.resolution.x,
                        (float)global_widgets[i]->state.resolution.y,
                    },
                    3.0f,
                    RED
                );
            }

            EndTextureMode();
        }

#if IC_OPT_BG_STATIC!=1
        DrawRectangleGradientEx(
            (Rectangle){
                .x = 0.0f,
                .y = 0.0f,
                .width = (float)renderer.resolution.x,
                .height = (float)renderer.resolution.y
            },
            compile_time_ic_options.background_color.gradient_top_left,
            compile_time_ic_options.background_color.gradient_bottom_left,
            compile_time_ic_options.background_color.gradient_top_right,
            compile_time_ic_options.background_color.gradient_bottom_right
        );
#endif   /* IC_OPT_BG_STATIC */

        /* Widget render (no value updates). */
        for (int i = 0; i < num_global_widgets; ++i)
            global_widgets[i]->draw(global_widgets[i], self);

        /* After drawing all widgets, clear all 'has_update' flags. This will require an atomic operation... */
        if (CAN.has_update) {
            for (int i = 0; i < DBC_SIGNALS_LEN; ++i) {
                if (!DBC.signals[i].real_time_data.has_update) continue;

                pthread_mutex_lock(&DBC.signals[i].real_time_data.lock);
                DBC.signals[i].real_time_data.has_update = false;
                pthread_mutex_unlock(&DBC.signals[i].real_time_data.lock);
            }

            /* A global bool prevents the drawing thread from needing to loop signals every pass. */
            pthread_mutex_lock(&CAN.lock);
            CAN.has_update = false;
            pthread_mutex_unlock(&CAN.lock);
        }

        if (any_outlines) {
            DrawTexturePro(
                outline_texture.texture,
                (Rectangle){ 0, 0, (float)self->resolution.x, -(float)self->resolution.y },
                (Rectangle){ 0, 0, (float)self->resolution.x, (float)self->resolution.y },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE
            );
        }

#if IC_DEBUG==1 && IC_OPT_DISABLE_RENDER_TIME!=1
        clock_t end = clock();

        clock_samples[clock_sample_count] = (double)(end - begin) / CLOCKS_PER_SEC;
        ++clock_sample_count;

        if (compile_time_ic_options.window.fps_limit == clock_sample_count) {
            double time_avg = 0.0f;
            for (int i = 0; i < compile_time_ic_options.window.fps_limit; ++i) time_avg += clock_samples[i];
            time_avg /= (double)compile_time_ic_options.window.fps_limit;

            DPRINTLN(">>> Average render time per frame over 1s: %f (%f millis)", time_avg, time_avg * 1000.0f);

            clock_sample_count = 0;
        }
#endif   /* IC_DEBUG */

#if IC_DEBUG==1
        DrawFPS(10, 10);
#endif   /* IC_DEBUG */

        EndDrawing();
    }
#if IC_DEBUG==1 && IC_OPT_DISABLE_RENDER_TIME!=1
    free(clock_samples);
#endif   /* IC_OPT_DISABLE_RENDER_TIME */

    CloseWindow();
}

