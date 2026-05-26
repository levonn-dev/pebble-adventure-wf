#include "backgrounds.h"
#include <pebble.h>

// Deterministic hash - same input always yields the same output.
// Used to pick stable particle positions from (tick, index).
static inline uint16_t bg_hash(uint16_t a, uint16_t b) {
  uint32_t h = ((uint32_t)a * 73856093u) ^ ((uint32_t)b * 19349663u);
  return (uint16_t)(h ^ (h >> 16));
}

// --- ported verbatim from pebble-adventure: drifting clouds + pollen ---
static void effects_plains(GContext *ctx, GRect area, uint16_t tick) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (uint8_t i = 0; i < 2; i++) {
    uint16_t x_seed = bg_hash(i, 0);
    int16_t cx = (int16_t)(((uint32_t)x_seed + (uint32_t)area.size.w * 256
                            - (uint32_t)(tick)) % area.size.w) + area.origin.x;
    int16_t cy = area.origin.y + 18 + (int16_t)(bg_hash(i, 1) % 8);
    graphics_fill_circle(ctx, GPoint(cx,     cy),     4);
    graphics_fill_circle(ctx, GPoint(cx - 5, cy + 1), 3);
    graphics_fill_circle(ctx, GPoint(cx + 5, cy + 1), 3);
    graphics_fill_circle(ctx, GPoint(cx - 2, cy - 2), 3);
    graphics_fill_circle(ctx, GPoint(cx + 2, cy - 2), 3);
  }

  if (area.size.h > 60) {
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t x_seed = bg_hash(i, 2);
      int16_t px = (int16_t)(((uint32_t)x_seed + (uint32_t)area.size.w * 256
                              - (uint32_t)(tick * 2)) % area.size.w) + area.origin.x;
      int16_t py = area.origin.y + 60 + (int16_t)(bg_hash(i, 3) % (area.size.h / 3));
      graphics_fill_circle(ctx, GPoint(px, py), 1);
    }
  }
}

// --- ported verbatim from pebble-adventure: falling leaves ---
static void effects_forest(GContext *ctx, GRect area, uint16_t tick) {
  for (uint8_t i = 0; i < 6; i++) {
    uint16_t x_range = (i == 0) ? (area.size.w * 2 / 5) : area.size.w;
    int16_t x0 = (int16_t)(((uint32_t)bg_hash(i, 10) + (uint32_t)area.size.w * 256
                            - (uint32_t)(tick)) % x_range) + area.origin.x;
    int16_t sway = (int16_t)((tick + i * 3) % 12) - 6;
    int16_t y = (int16_t)((bg_hash(i, 11) + tick * 2) % area.size.h) + area.origin.y;
    int16_t lx = x0 + sway;

    GColor leaf_color = (i % 2 == 0)
      ? PBL_IF_COLOR_ELSE(GColorOrange, GColorLightGray)
      : PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite);
    graphics_context_set_stroke_color(ctx, leaf_color);
    graphics_context_set_fill_color(ctx, leaf_color);

    int16_t tilt = (sway > 0) ? 1 : -1;
    graphics_fill_circle(ctx, GPoint(lx, y), 1);
    graphics_draw_pixel(ctx, GPoint(lx + tilt * 2, y - 1));
    graphics_draw_pixel(ctx, GPoint(lx - tilt * 2, y + 1));
  }
}

// --- ported verbatim from pebble-adventure: drifting snowflakes ---
static void effects_mountain(GContext *ctx, GRect area, uint16_t tick) {
  for (uint8_t i = 0; i < 8; i++) {
    int16_t sx = (int16_t)(((uint32_t)bg_hash(i, 30) + (uint32_t)area.size.w * 256
                            - (uint32_t)(tick)) % area.size.w) + area.origin.x;
    int16_t sy = (int16_t)((bg_hash(i, 31) + tick * 2) % area.size.h) + area.origin.y;
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_circle(ctx, GPoint(sx, sy), 1);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, GPoint(sx, sy), 0);
  }
}

// --- ported from pebble-adventure with an added `lightning` flag ---
static void effects_storm(GContext *ctx, GRect area, uint16_t tick, bool lightning) {
  // Dense diagonal rain streaks drifting right-to-left (wind from the right).
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  graphics_context_set_stroke_width(ctx, 1);
  for (uint8_t i = 0; i < 30; i++) {
    int16_t rx = (int16_t)(((uint32_t)bg_hash(i, 50) + (uint32_t)area.size.w * 256
                            - (uint32_t)(tick * 3)) % area.size.w) + area.origin.x;
    int16_t ry = (int16_t)((bg_hash(i, 51) + tick * 6) % area.size.h) + area.origin.y;
    graphics_draw_line(ctx, GPoint(rx, ry), GPoint(rx - 2, ry + 5));
  }

  if (lightning) {
    uint8_t cycle = tick % 25;
    if (cycle < 2) {
      uint16_t bolt_seed = bg_hash((uint16_t)(tick / 25), 55);
      int16_t bx = area.origin.x + 20 + (int16_t)(bolt_seed % (area.size.w - 40));
      int16_t by = area.origin.y;
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_context_set_stroke_width(ctx, 1);
      int16_t x = bx, y = by;
      for (int seg = 0; seg < 5; seg++) {
        int16_t dx = (int16_t)((bg_hash(bolt_seed + seg, 56) % 11) - 5);
        int16_t dy = (int16_t)(8 + bg_hash(bolt_seed + seg, 57) % 6);
        int16_t nx = x + dx, ny = y + dy;
        graphics_draw_line(ctx, GPoint(x, y), GPoint(nx, ny));
        x = nx; y = ny;
      }
      if (cycle == 0) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_circle(ctx, GPoint(bx, by + 4), 3);
      }
    }
  }
  graphics_context_set_stroke_width(ctx, 1);
}

void backgrounds_draw_effects(GContext *ctx, GRect area, WeatherScene scene,
                              bool lightning, uint16_t effect_tick) {
  switch (scene) {
    case SCENE_CLEAR:  effects_plains(ctx, area, effect_tick);            break;
    case SCENE_CLOUDY: effects_forest(ctx, area, effect_tick);            break;
    case SCENE_RAIN:   effects_storm(ctx, area, effect_tick, lightning);  break;
    case SCENE_SNOW:   effects_mountain(ctx, area, effect_tick);          break;
    default: break;
  }
}
