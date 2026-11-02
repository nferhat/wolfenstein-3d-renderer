#pragma once

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#define LA_IMPLEMENTATION
#include "la.h"

#define MAP_WIDTH 24
#define MAP_HEIGHT 24

const int WORLD_MAP[MAP_WIDTH][MAP_HEIGHT]
    = { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 3, 0, 3, 0, 3, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 3, 0, 3, 0, 3, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 4, 0, 4, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 4, 0, 0, 0, 0, 5, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 4, 0, 4, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 4, 0, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 } };

typedef struct Raycaster {
    V2d pos, dir, plane;
    // where the user wants us to walk/rotate.
    // x is where we want to talk forward
    // y is where we want to rotate sideways.
    V2i input_dir;
    // the cached input mask
    uint32_t input_mask;

    // FIXME: integrate with wayland side.
    double time, old_time;
} Raycaster;

void raycaster_init(Raycaster* self) {
    self->pos = v2d(22, 12);
    self->dir = v2d(-1, 0);
    self->plane = v2d(0, 2.0 / 3.0);
    self->time = 0, self->old_time = 0;
}

void raycaster_input(Raycaster* self, xkb_keysym_t sym, bool pressed) {
    // char name[32];
    // xkb_keysym_get_name(sym, name, sizeof name);
    // fprintf(stderr, "raycaster_input: sym=%16s pressed=%s\n", name,
    //         pressed ? "down" : "up");

    self->input_dir = (V2i){ .x = 0, .y = 0 };
    if (!pressed) return;

    switch (sym) {
        case XKB_KEY_Up:
            self->input_dir.x += 1.0;
            break;
        case XKB_KEY_Down:
            self->input_dir.x -= 1.0;
            break;
        case XKB_KEY_Left:
            self->input_dir.y += 1.0;
            break;
        case XKB_KEY_Right:
            self->input_dir.y -= 1.0;
            break;
        default:
            break;
    }
}

// FIXME: pass delta time
void raycaster_update(Raycaster* self, double dt) {
    if (WORLD_MAP[(int)(self->pos.x + self->dir.x * self->input_dir.x)][(int)self->pos.y]
        == false) {
        self->pos.x += self->dir.x * self->input_dir.x * 5.0 * dt;
    }
    if (WORLD_MAP[(int)self->pos.x][(int)(self->pos.y + self->dir.y * self->input_dir.x)]
        == false) {
        self->pos.y += self->dir.y * self->input_dir.x * 5.0 * dt;
    }

    // rotate to the right
    if (self->input_dir.y != 0.0) {
        const double rotation_speed = 3.0 * dt * self->input_dir.y;
        // both camera direction and camera plane must be rotated
        V2d old_dir = self->dir;
        self->dir.x = self->dir.x * cos(rotation_speed) - self->dir.y * sin(rotation_speed);
        self->dir.y = old_dir.x * sin(rotation_speed) + self->dir.y * cos(rotation_speed);

        V2d old_plane = self->plane;
        self->plane.x = self->plane.x * cos(rotation_speed) - self->plane.y * sin(rotation_speed);
        self->plane.y = old_plane.x * sin(rotation_speed) + self->plane.y * cos(rotation_speed);
    }
}

void raycaster_draw(Raycaster* self, size_t width, size_t height, uint32_t* pixels) {
    for (int x = 0; x < width; x++) {
        double cam_x = 2.0 * x / (double)width - 1.0;
        V2d ray_dir = v2d_add(self->dir, v2d_mul(self->plane, v2dd(cam_x)));
        V2d delta = v2d(fabs(1 / ray_dir.x), fabs(1 / ray_dir.y));
        V2i map_pos = v2d_2i(self->pos);

        // the next distance to the orthogonal grids point
        double perp_wall_dist;

        // This is the DDA algorithm, in short, instead of ray marching with a
        // constant step, we instead snap our ray to the edges of the orthogonal
        // cell grid, ensuring we always detect a given wall, since the step is
        // adaptive.
        {
            // Based on the ray direction (which is orthogonal to the camera
            // plane with its origin being the player position), we determine
            // the direction raymarch in.
            V2d side_dist;
            V2i step = { 0 };

            if (ray_dir.x < 0) {
                step.x = -1;
                side_dist.x = self->pos.x - map_pos.x;
            } else {
                step.x = 1;
                side_dist.x = map_pos.x + 1.0 - self->pos.x;
            }

            if (ray_dir.y < 0) {
                step.y = -1;
                side_dist.y = self->pos.y - map_pos.y;
            } else {
                step.y = 1;
                side_dist.y = map_pos.y + 1.0 - self->pos.y;
            }

            // Multiply by the delta X/Y from the next cell.
            // If the ray direction is null in that direction this will do
            // nothing which is fine.
            side_dist = v2d_mul(side_dist, delta);

            bool hit = false,  // if we hit a wall
                side = false;  // the side (NS or EW)

            while (!hit) {
                if (side_dist.x < side_dist.y) {
                    side_dist.x += delta.x;
                    map_pos.x += step.x;
                    side = false;
                } else {
                    side_dist.y += delta.y;
                    map_pos.y += step.y;
                    side = true;
                }

                // on each iteration check if we have a cell in the world map
                hit = WORLD_MAP[map_pos.x][map_pos.y];
            }

            perp_wall_dist = side ? (side_dist.y - delta.y) : (side_dist.x - delta.x);
        }

        // Then calculate the line height to be drawn.
        // For now its the inverse of the screen height.
        int line_height = (int)(height / perp_wall_dist);
        int draw_start = maxi(-line_height / 2 + height / 2, 0);
        int draw_end = mini(line_height / 2 + height / 2, height - 2);

        uint32_t color;
        switch (WORLD_MAP[map_pos.x][map_pos.x]) {
            case 1:
                color = 0xFFFF0000;
                break;
            case 2:
                color = 0xFF00FF00;
                break;
            case 3:
                color = 0xFF0000FF;
                break;
            case 4:
                color = 0xFFFF00FF;
                break;
            default:
                color = 0xFFFFFF00;
                break;
        }

        // draw the line
        for (size_t y = draw_start; y < draw_end; y++) {
            pixels[x + (y * width)] = color;
        }
    }
}
