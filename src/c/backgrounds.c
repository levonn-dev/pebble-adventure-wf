#include "backgrounds.h"
#include <pebble.h>

#define BG_NONE 0xFF

static GBitmap *s_current_bg    = NULL;
static GBitmap *s_rain_toprow   = NULL;  // cached 1px top-row strip for rain gap extrusion
static uint8_t  s_current_scene = BG_NONE;

static uint32_t bg_resource_id(WeatherScene scene) {
  switch (scene) {
    case SCENE_CLEAR:  return RESOURCE_ID_BG_PLAINS;
    case SCENE_CLOUDY: return RESOURCE_ID_BG_FOREST;
    case SCENE_RAIN:   return RESOURCE_ID_BG_STORM;
    case SCENE_SNOW:   return RESOURCE_ID_BG_MOUNTAIN;
    default:           return 0;
  }
}

static void ensure_loaded(WeatherScene scene) {
  if ((uint8_t)scene == s_current_scene && s_current_bg) { return; }
  if (s_rain_toprow) { gbitmap_destroy(s_rain_toprow); s_rain_toprow = NULL; }
  if (s_current_bg) { gbitmap_destroy(s_current_bg); s_current_bg = NULL; }
  s_current_scene = BG_NONE;
  uint32_t rid = bg_resource_id(scene);
  if (rid == 0) { return; }
  s_current_bg = gbitmap_create_with_resource(rid);
  if (s_current_bg) {
    s_current_scene = (uint8_t)scene;
    if (scene == SCENE_RAIN) {  // cache the 1px top row once for sky-gap extrusion
      GSize img = gbitmap_get_bounds(s_current_bg).size;
      s_rain_toprow = gbitmap_create_as_sub_bitmap(s_current_bg, GRect(0, 0, img.w, 1));
    }
  }
}

void backgrounds_init(void) { }

void backgrounds_deinit(void) {
  if (s_rain_toprow) { gbitmap_destroy(s_rain_toprow); s_rain_toprow = NULL; }
  if (s_current_bg) { gbitmap_destroy(s_current_bg); s_current_bg = NULL; }
  s_current_scene = BG_NONE;
}

static void draw_fallback(GContext *ctx, GRect area, WeatherScene scene) {
  GColor c;
  // emery (color) sky matches each image; flint (B&W) art uses a BLACK sky, so the
  // gap fill must be black there - otherwise a white band sits above the scene and
  // makes it look shoved down.
  switch (scene) {
    case SCENE_CLEAR:  c = PBL_IF_COLOR_ELSE(GColorCeleste,  GColorBlack); break;
    case SCENE_CLOUDY: c = PBL_IF_COLOR_ELSE(GColorWhite,    GColorBlack); break;
    case SCENE_RAIN:   c = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack); break;
    case SCENE_SNOW:   c = PBL_IF_COLOR_ELSE(GColorWhite,    GColorBlack); break;
    default:           c = GColorBlack; break;
  }
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_rect(ctx, area, 0, GCornerNone);
}

void backgrounds_draw(GContext *ctx, GRect area, WeatherScene scene) {
  ensure_loaded(scene);
  if (!s_current_bg) { draw_fallback(ctx, area, scene); return; }

  GSize img = gbitmap_get_bounds(s_current_bg).size;
  int16_t draw_w = img.w < area.size.w ? img.w : area.size.w;

  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  if (img.h >= area.size.h) {
    int16_t trim = img.h - area.size.h;
    GBitmap *sub = gbitmap_create_as_sub_bitmap(s_current_bg,
        GRect(0, trim, draw_w, area.size.h));
    if (sub) {
      graphics_draw_bitmap_in_rect(ctx, sub, area);
      gbitmap_destroy(sub);
    } else {
      draw_fallback(ctx, area, scene);
    }
  } else {
    int16_t gap = area.size.h - img.h;
    draw_fallback(ctx, GRect(area.origin.x, area.origin.y, area.size.w, gap), scene);
    GRect dest = GRect(area.origin.x, area.origin.y + gap, draw_w, img.h);
    graphics_draw_bitmap_in_rect(ctx, s_current_bg, dest);

    // Rain: extend the storm image's vertical streaks up through the sky gap by
    // extruding its (cached) top row, so the blue lines reach the top of the screen.
    if (scene == SCENE_RAIN && gap > 0 && s_rain_toprow) {
      for (int16_t y = area.origin.y; y < area.origin.y + gap; y++) {
        graphics_draw_bitmap_in_rect(ctx, s_rain_toprow, GRect(area.origin.x, y, draw_w, 1));
      }
    }
  }
}

int16_t backgrounds_ground_y(GRect area, WeatherScene scene) {
#ifdef PBL_COLOR
  int16_t offset;
  switch (scene) {
    case SCENE_CLEAR:  offset = 20; break;  // plains
    case SCENE_CLOUDY: offset = 22; break;  // forest
    case SCENE_RAIN:   offset = 18; break;  // storm
    case SCENE_SNOW:   offset = 20; break;  // mountain
    default:           offset = 20; break;
  }
#else
  int16_t offset;
  switch (scene) {
    case SCENE_CLEAR:  offset = 8; break;  // plains
    case SCENE_CLOUDY: offset = 8; break;  // forest
    case SCENE_RAIN:   offset = 8; break;  // storm
    case SCENE_SNOW:   offset = 6; break;  // mountain
    default:           offset = 10; break;
  }
#endif
  return area.origin.y + area.size.h - offset;
}
