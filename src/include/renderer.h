//
// Created by puhlz on 5/28/25.
//

#ifndef RENDERER_H
#define RENDERER_H

#include "flex_ic.h"



/* Points and dimensions. */
typedef
struct {
    int32_t x;
    int32_t y;
} vec2_t;


/* Function prototype declarations for renderer. This allows RAYLIB to be swapped out later as desired. */
typedef struct renderer renderer_t;

typedef
ic_err_t (*_func__renderer_init)(
    const renderer_t *self
);

typedef
void (*_func__renderer_loading)(
    const renderer_t *self
);

typedef
void (*_func__renderer_loop)(
    const renderer_t *self
);


/* Display properties (global). */
struct renderer {
    _func__renderer_init    init;
    _func__renderer_loading loading;
    _func__renderer_loop    loop;

    vec2_t resolution;
    uint8_t fps_limit;   /* set to 0 for no limit */
    const char *title;
};


extern const renderer_t *global_renderer;


// TODO! If another Renderer besides Raylib is introduced (e.g. SDL2), we need
//   to be able to abstract all GUI drawing functions out to an interface. This
//   header file is where we can define that 'protocol' with a series of extra
//   function pointers on the 'global_renderer' object. For example, instead of
//   hard-coding 'DrawRectangle' from Raylib, we would attach a _func__draw_rect
//   prototype to the global renderer and use that proxy implementation instead.



#endif   /* RENDERER_H */
