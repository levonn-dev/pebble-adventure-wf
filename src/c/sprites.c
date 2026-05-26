#include "sprites.h"
#include <pebble.h>

#define FOX_FRAMES 4
#define MODE_NONE  0xFF

static GBitmap *s_fox[FOX_FRAMES] = { NULL };
static uint8_t  s_mode = MODE_NONE;   // 0 = idle, 1 = walk, MODE_NONE = nothing loaded

static void unload(void) {
  for (int i = 0; i < FOX_FRAMES; i++) {
    if (s_fox[i]) { gbitmap_destroy(s_fox[i]); s_fox[i] = NULL; }
  }
  s_mode = MODE_NONE;
}

void sprites_init(void) { }   // frames load lazily via sprites_set_mode()

void sprites_deinit(void) { unload(); }

void sprites_set_mode(uint8_t walk) {
  uint8_t mode = walk ? 1 : 0;
  if (mode == s_mode && s_fox[0]) { return; }
  unload();
  if (mode == 1) {
    s_fox[0] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_WALK_0);
    s_fox[1] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_WALK_1);
    s_fox[2] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_WALK_2);
    s_fox[3] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_WALK_3);
  } else {
    s_fox[0] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_IDLE_0);
    s_fox[1] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_IDLE_1);
    s_fox[2] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_IDLE_2);
    s_fox[3] = gbitmap_create_with_resource(RESOURCE_ID_FOX_LG_IDLE_3);
  }
  s_mode = mode;
}

GBitmap *sprites_get_fox(uint8_t frame) {
  return s_fox[frame % FOX_FRAMES];
}
