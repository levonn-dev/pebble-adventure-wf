#pragma once
#include <pebble.h>
#include "weather.h"

void    backgrounds_init(void);
void    backgrounds_deinit(void);

// Draw the scene's background bitmap within `area` (bottom-aligned; sky color
// fills the gap above). Lazy-loads one bitmap at a time.
void    backgrounds_draw(GContext *ctx, GRect area, WeatherScene scene);

// Y coordinate (within `area`) of the ground surface - the fox's paws align here.
int16_t backgrounds_ground_y(GRect area, WeatherScene scene);

// Procedural overlay for the scene. Call AFTER backgrounds_draw and BEFORE the fox.
// `lightning` adds storm bolts (thunderstorm only). `effect_tick` advances motion.
void    backgrounds_draw_effects(GContext *ctx, GRect area, WeatherScene scene,
                                 bool lightning, uint16_t effect_tick);
