#include "slow.h"
#include "pebble.h"

static Window *s_window;
static Layer *s_simple_bg_layer, *s_hands_layer;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static TextLayer *s_time_layer[NUM_CLOCK_TICKS/2];

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
  
  //Draw numbers
  static char *numbuf[12];
  //fill numbuf
  numbuf[0] = "0";
  numbuf[1] = "2";
  numbuf[2] = "4 ";
  numbuf[3] = "6 ";
  numbuf[4] = "8 ";
  numbuf[5] = "10";
  numbuf[6] = "12";
  numbuf[7] = "14";
  numbuf[8] = "16";
  numbuf[9] = "18";
  numbuf[10] = "20";
  numbuf[11] = "22";
  
  int index = 0;
  for (int i = 0; i < NUM_CLOCK_TICKS/2; ++i){
    s_time_layer[i] = text_layer_create(
        GRect((int16_t) (center.x - 13) - (sin_lookup(TRIG_MAX_ANGLE * ((i*2 % NUM_CLOCK_TICKS) * 6) / (NUM_CLOCK_TICKS * 6)) * (int32_t)(hour_hand_length - 2) / TRIG_MAX_RATIO),
              (int16_t) (center.y - 13) - (-cos_lookup(TRIG_MAX_ANGLE * ((i*2 % NUM_CLOCK_TICKS) * 6) / (NUM_CLOCK_TICKS * 6)) * (int32_t)(hour_hand_length - 2) / TRIG_MAX_RATIO), 25, 25));
  
    // Improve the layout to be more like a watchface
    text_layer_set_background_color(s_time_layer[i], GColorClear);
    text_layer_set_text_color(s_time_layer[i], GColorLightGray);
    text_layer_set_text(s_time_layer[i], numbuf[i]);
    text_layer_set_font(s_time_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_time_layer[i], GTextAlignmentCenter);
  
    // Add it as a child layer to the Window's root layer
    layer_add_child(s_simple_bg_layer, text_layer_get_layer(s_time_layer[i]));
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
  graphics_context_set_stroke_width(ctx, 3);

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
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 7, bounds.size.h / 2 - 7, 15, 15), 10, GCornersAll);
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
  for (int i = 0; i < NUM_CLOCK_TICKS/2; ++i){
    text_layer_destroy(s_time_layer[i]);
  }
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