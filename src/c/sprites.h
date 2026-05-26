#pragma once
#include <pebble.h>

void     sprites_init(void);
void     sprites_deinit(void);

// Load the active fox set into memory: 0 = idle, 1 = walk. Frees the other set,
// so only one 4-frame set is resident at a time (keeps the heap small). Safe to
// call repeatedly; a no-op if the requested set is already loaded.
void     sprites_set_mode(uint8_t walk);

// Active set's frame, 0..3 (wraps via modulo). NULL if not loaded.
GBitmap *sprites_get_fox(uint8_t frame);
