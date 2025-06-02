#include "widget_common.h"

#include <math.h>
#include <raylib.h>
#include <stdio.h>


int ScreenW = 0;
int ScreenH = 0;
float radius = 200.0f;
float angle = -260.0f;
float smoothing = 0.1f;

static void
internal__render_basic_tachometer()
{
    if (IsKeyDown(KEY_W))
    {
        if (angle < -15.0f) angle += 1.0f; // Limit to 7500 RPM
        if (angle > 10.0f) angle = angle; // Clamp
    }
    if (IsKeyDown(KEY_S))
    {
        if (angle > -270.0f) angle -= 1.0f;
        if (angle < -260.0f) angle = -260.0f;
    }

    // Convert angle to radians
    float radians = angle * DEG2RAD;

    // Calculate tip of line using trig
    float tipX = ScreenW / 2 + radius * cosf(radians);
    float tipY = ScreenH / 2 + radius * sinf(radians);
    Vector2 center = { ScreenW / 2.0f, ScreenH / 2.0f };
    DrawCircleV((Vector2){ScreenW/2, ScreenH/2}, radius, BLACK);
    // rev segments
    for (int i = -270; i <= 0; i += 15)
    {
        float rad = i * DEG2RAD;
        float innerX = center.x + (radius - 20) * cosf(rad);
        float innerY = center.y + (radius - 20) * sinf(rad);
        float outerX = center.x + radius * cosf(rad);
        float outerY = center.y + radius * sinf(rad);

        DrawLineEx((Vector2){innerX, innerY}, (Vector2){outerX, outerY}, 2.0f, WHITE);
    }
    // Optional: colored zones (redline)
    for (float i = -30; i <= 0; i += 5)
    {
        float rad = i * DEG2RAD;
        float x1 = center.x + (radius - 20) * cosf(rad);
        float y1 = center.y + (radius - 20) * sinf(rad);
        float x2 = center.x + radius * cosf(rad);
        float y2 = center.y + radius * sinf(rad);

        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, RED);
    }

    // Draw needle (angle from center)
    float needleRad = angle * DEG2RAD;
    float needleX = center.x + (radius - 30) * cosf(needleRad);
    float needleY = center.y + (radius - 30) * sinf(needleRad);
    DrawLineEx(center, (Vector2){needleX, needleY}, 4.0f, RED);

    // Center dot
    DrawCircleV(center, 6, RED);

    // Optional: Draw labels (1 to 8)
    int labelCount = 10;
    for (int i = 0; i < labelCount; i++)
    {
        float labelAngle = -270 + (270.0f / (labelCount - 1)) * i;
        float rad = labelAngle * DEG2RAD;
        float tx = center.x + (radius - 40) * cosf(rad);
        float ty = center.y + (radius - 40) * sinf(rad);

        char label[3];
        sprintf(label, "%d", i + 1);
        DrawText(label, tx - 10, ty - 10, 20, RAYWHITE);
    }
    //gearing
    char Gear[32] = {0};
    sprintf(Gear, "%f", angle);
    DrawText(Gear, ScreenW/2 + 87, ScreenH/2 + 87, 20, WHITE);
}


static void
needle_meter__default__update(widget_t *self)
{
    // // TODO: Testing. Remove.
    // for (int i = 0; i < self->num_parent_signals; ++i) {
    //     if (self->parent_signals[i]->real_time_data.has_update) {
    //         DPRINTLN("[%s] SIGNAL RAW DATA (CHANNEL%u: %s): ", self->label, i, self->parent_signals[i]->name);
    //         MEMDUMP(&(self->parent_signals[i]->real_time_data.value), 8);
    //         // MY_X = *(uint16_t *)(CHANNEL(0).data);
    //     }
    // }

    // TODO: DELETE
    if (!ScreenW) ScreenW = GetScreenWidth();
    if (!ScreenH) ScreenH = GetScreenHeight();
}


static void
needle_meter__default__draw(widget_t *self, const renderer_t *renderer)
{
    internal__render_basic_tachometer();
}


static ic_err_t
needle_meter__default__parse_args(widget_t *self)
{
    return ERR_OK;
}
