#include <pebble.h>
#include "weather.h"
#include "backgrounds.h"
#include "sprites.h"

// ---- Debug (compile-time; release keeps WF_DEBUG 0 = no debug code) ----
// WF_DEBUG_FORCE_CODE pins a WMO code (0=clear 3=cloudy 61=rain 95=thunder
// 71=snow); -1 = don't pin, and a wrist-shake cycles the scenes instead.
#define WF_DEBUG             0
#define WF_DEBUG_FORCE_CODE  (-1)

static Window *s_window;
static Layer  *s_canvas;
static char    s_time[8];

#define PKEY_TEMP_C   4   // raw °C (key bumped from old unit-specific temp at key 1)
#define PKEY_CODE     2
#define PKEY_UNIT     3
#define PKEY_FOX_MODE 5   // 0 = idle, 1 = walking

static int     s_temp_c       = 20;    // raw °C; converted for display
static int     s_weather_code = 0;     // WMO code; 0 = clear
static uint8_t s_unit         = 0;     // 0 = F, 1 = C
static uint8_t s_fox_mode     = 0;     // 0 = idle, 1 = walking
static uint8_t s_battery      = 100;
static char    s_date[16];
static char    s_status[24];
static char    s_battery_str[8];

static void update_time_strings(struct tm *t) {
  char buf[8];
  if (clock_is_24h_style()) {
    strftime(buf, sizeof(buf), "%H:%M", t);
  } else {
    strftime(buf, sizeof(buf), "%l:%M", t);
  }
  char *p = buf;
  while (*p == ' ') { p++; }            // trim leading space from %l
  strncpy(s_time, p, sizeof(s_time) - 1);
  s_time[sizeof(s_time) - 1] = '\0';

  strftime(s_date, sizeof(s_date), "%a %b %e", t);
}

static int display_temp(void) {
  // s_temp_c is raw °C; convert to °F for display when the unit is Fahrenheit.
  return (s_unit == 1) ? s_temp_c : (s_temp_c * 9 / 5 + 32);
}

static void update_status_strings(void) {
  snprintf(s_status, sizeof(s_status), "%s %d°%c",
           weather_label_for_code(s_weather_code), display_temp(), s_unit == 1 ? 'C' : 'F');
  snprintf(s_battery_str, sizeof(s_battery_str), "%d%%", s_battery);
}

// ---- Debug scene control (no-ops unless WF_DEBUG is set) ----
#if WF_DEBUG && (WF_DEBUG_FORCE_CODE >= 0)
// Pin mode: force a fixed scene; real weather messages cannot override it.
static void debug_apply_pin(void) { s_weather_code = (WF_DEBUG_FORCE_CODE); }
#else
static void debug_apply_pin(void) {}
#endif

#if WF_DEBUG && (WF_DEBUG_FORCE_CODE < 0)
// Cycle mode: step through one representative code per scene on each shake.
static void debug_cycle_code(void) {
  static const int codes[] = {0, 3, 61, 95, 71};  // clear, cloudy, rain, thunder, snow
  static int i = 0;
  i = (i + 1) % (int)(sizeof(codes) / sizeof(codes[0]));
  s_weather_code = codes[i];
  update_status_strings();
}
#else
static void debug_cycle_code(void) {}
#endif

static uint8_t      s_fox_frame = 0;
static uint16_t     s_effect_tick = 0;

#define BURST_TICKS 26          // ~26 * 150ms ≈ 3.9s
static AppTimer    *s_burst_timer = NULL;
static int          s_burst_left  = 0;
static void start_burst(void);     // forward decls (defined below)
static void request_weather(void);

static void draw_text_outlined(GContext *ctx, const char *text, GFont font, GRect box,
                               GColor fg, GColor outline, GTextAlignment align) {
  graphics_context_set_text_color(ctx, outline);
  static const GPoint k_off[4] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };  // 4-neighbor outline
  for (int i = 0; i < 4; i++) {
    GRect o = box;
    o.origin.x = box.origin.x + k_off[i].x;
    o.origin.y = box.origin.y + k_off[i].y;
    graphics_draw_text(ctx, text, font, o, GTextOverflowModeFill, align, NULL);
  }
  graphics_context_set_text_color(ctx, fg);
  graphics_draw_text(ctx, text, font, box, GTextOverflowModeFill, align, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  WeatherScene scene = weather_scene_for_code(s_weather_code);

  backgrounds_draw(ctx, bounds, scene);

  backgrounds_draw_effects(ctx, bounds, scene,
                           weather_has_lightning(s_weather_code), s_effect_tick);

  GBitmap *fox = sprites_get_fox(s_fox_frame);
  if (fox) {
    GSize fs = gbitmap_get_bounds(fox).size;
    int16_t ground = backgrounds_ground_y(bounds, scene);
    int16_t fx = bounds.origin.x + (bounds.size.w - fs.w) / 2;
    int16_t fy = ground - fs.h;
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, fox, GRect(fx, fy, fs.w, fs.h));
  }

  bool big = bounds.size.w >= 180;
  GColor fg = GColorWhite, ol = GColorBlack;
  GFont f_time = fonts_get_system_font(big ? FONT_KEY_LECO_42_NUMBERS
                                           : FONT_KEY_LECO_36_BOLD_NUMBERS);
  draw_text_outlined(ctx, s_time, f_time,
                     GRect(0, big ? 34 : 26, bounds.size.w, big ? 50 : 42),
                     fg, ol, GTextAlignmentCenter);

  GFont f_text = fonts_get_system_font(big ? FONT_KEY_GOTHIC_24_BOLD
                                           : FONT_KEY_GOTHIC_18_BOLD);
  draw_text_outlined(ctx, s_date, f_text,
                     GRect(0, big ? 84 : 66, bounds.size.w, big ? 28 : 22),
                     fg, ol, GTextAlignmentCenter);
  int16_t row_h  = big ? 24 : 20;
  int16_t batt_w = big ? 54 : 44;   // reserved right-hand width for "100%"
  draw_text_outlined(ctx, s_status, f_text,
                     GRect(4, 0, bounds.size.w - 8 - batt_w, row_h),
                     fg, ol, GTextAlignmentLeft);
  draw_text_outlined(ctx, s_battery_str, f_text,
                     GRect(bounds.size.w - 4 - batt_w, 0, batt_w, row_h),
                     fg, ol, GTextAlignmentRight);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time_strings(tick_time);
  if (tick_time->tm_min % 30 == 0) { request_weather(); }
  start_burst();
  if (s_canvas) { layer_mark_dirty(s_canvas); }
}

static void burst_timer_callback(void *data) {
  s_effect_tick++;
  s_fox_frame = (s_fox_frame + 1) % 4;
  if (s_canvas) { layer_mark_dirty(s_canvas); }
  if (--s_burst_left > 0) {
    s_burst_timer = app_timer_register(150, burst_timer_callback, NULL);
  } else {
    s_burst_timer = NULL;
  }
}

static void start_burst(void) {
  s_burst_left = BURST_TICKS;
  if (!s_burst_timer) {
    s_burst_timer = app_timer_register(150, burst_timer_callback, NULL);
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  debug_cycle_code();
  start_burst();
}

static void request_weather(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *t_temp = dict_find(iter, MESSAGE_KEY_TEMPERATURE);
  Tuple *t_code = dict_find(iter, MESSAGE_KEY_WEATHER_CODE);
  Tuple *t_unit = dict_find(iter, MESSAGE_KEY_UNIT);
  Tuple *t_fox  = dict_find(iter, MESSAGE_KEY_FOX_MODE);
  if (t_temp) { s_temp_c = t_temp->value->int32; }
  if (t_code) { s_weather_code = t_code->value->int32; }
  if (t_unit) { s_unit = t_unit->value->int32 ? 1 : 0; }  // clamp to 0/1
  if (t_fox)  { s_fox_mode = t_fox->value->int32 ? 1 : 0; sprites_set_mode(s_fox_mode); }
  persist_write_int(PKEY_TEMP_C, s_temp_c);
  persist_write_int(PKEY_CODE, s_weather_code);
  persist_write_int(PKEY_UNIT, s_unit);
  persist_write_int(PKEY_FOX_MODE, s_fox_mode);
  debug_apply_pin();
  update_status_strings();
  if (s_canvas) { layer_mark_dirty(s_canvas); }
}

static void battery_handler(BatteryChargeState state) {
  s_battery = state.charge_percent;
  update_status_strings();
  if (s_canvas) { layer_mark_dirty(s_canvas); }
}

static void init(void) {
  if (persist_exists(PKEY_TEMP_C))   { s_temp_c = persist_read_int(PKEY_TEMP_C); }
  if (persist_exists(PKEY_CODE))     { s_weather_code = persist_read_int(PKEY_CODE); }
  if (persist_exists(PKEY_UNIT))     { s_unit = persist_read_int(PKEY_UNIT) ? 1 : 0; }
  if (persist_exists(PKEY_FOX_MODE)) { s_fox_mode = persist_read_int(PKEY_FOX_MODE) ? 1 : 0; }
  debug_apply_pin();

  sprites_init();
  sprites_set_mode(s_fox_mode);
  backgrounds_init();

  s_window = window_create();
  Layer *root = window_get_root_layer(s_window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(root, s_canvas);

  time_t now = time(NULL);
  update_time_strings(localtime(&now));
  s_battery = battery_state_service_peek().charge_percent;
  update_status_strings();

  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(accel_tap_handler);
  battery_state_service_subscribe(battery_handler);
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 128);
}

static void deinit(void) {
  if (s_burst_timer) { app_timer_cancel(s_burst_timer); s_burst_timer = NULL; }
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  battery_state_service_unsubscribe();
  layer_destroy(s_canvas);
  window_destroy(s_window);
  backgrounds_deinit();
  sprites_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
