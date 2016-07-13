#include "slow.h"
#include <math.h>
#include "pebble.h"

static Window *s_window;
static Layer *s_simple_bg_layer, *s_hands_layer;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];

static void bg_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  const int16_t hour_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, (bounds.size.w / 2) - 15);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorLightGray);
  
  // draw the hour markers  
  graphics_context_set_fill_color(ctx, GColorLightGray);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    graphics_fill_rect(ctx, GRect((int16_t) (center.x - 3) - (sin_lookup(TRIG_MAX_ANGLE * ((i % NUM_CLOCK_TICKS) * 6) / (NUM_CLOCK_TICKS * 6)) * (int32_t)(hour_hand_length + 10) / TRIG_MAX_RATIO),
                                  (int16_t) (center.y - 3) - (-cos_lookup(TRIG_MAX_ANGLE * ((i % NUM_CLOCK_TICKS) * 6) / (NUM_CLOCK_TICKS * 6)) * (int32_t)(hour_hand_length + 10) / TRIG_MAX_RATIO), 
                                  5, 5),
                       2, GCornersAll);
  }
  
  // draw the 15-minute markers
  graphics_context_set_fill_color(ctx, GColorLightGray);
  for (int i = 0; i < NUM_SUB_TICKS; ++i) {
    if (i % 4){
      graphics_fill_rect(ctx, GRect((int16_t) (center.x - 1) - (sin_lookup(TRIG_MAX_ANGLE * ((i % NUM_SUB_TICKS) * 6) / (NUM_SUB_TICKS * 6)) * (int32_t)(hour_hand_length + 10) / TRIG_MAX_RATIO),
                                    (int16_t) (center.y - 1) - (-cos_lookup(TRIG_MAX_ANGLE * ((i % NUM_SUB_TICKS) * 6) / (NUM_SUB_TICKS * 6)) * (int32_t)(hour_hand_length + 10) / TRIG_MAX_RATIO),
                                    1, 1),
                         1, GCornersAll);
    }
  }

}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  const int16_t hour_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, (bounds.size.w / 2) - 15);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // hour hand
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 5);

  int32_t hour_angle = TRIG_MAX_ANGLE * (((t->tm_hour % 24) * 6) + (t->tm_min / 10)) / (24 * 6);
  GPoint hour_hand_tip = {
    .x = (int16_t) center.x - (sin_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO),
    .y = (int16_t) center.y - (-cos_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO),
  };
  
   GPoint hour_hand_end = {
    .x = (int16_t) center.x + 0.3 * (sin_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO),
    .y = (int16_t) center.y + 0.3 * (-cos_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO),
  };
  
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_line(ctx, hour_hand_tip, hour_hand_end);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 10, bounds.size.h / 2 - 10, 20, 20), 10, GCornersAll);
}


static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);

  layer_destroy(s_hands_layer);
}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // init hand path

  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);


  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit() {

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
