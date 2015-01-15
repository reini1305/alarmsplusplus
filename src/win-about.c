#include <pebble.h>
#include "win-about.h"
#include "localize.h"

static void window_load(Window* window);
static void window_unload(Window* window);
static void layer_header_update(Layer* layer, GContext* ctx);

static Window*          s_window;
static Layer*           s_layer_header;
static TextLayer*       s_layer_text;

void win_about_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_fullscreen(s_window,true);
}

void win_about_show(void) {
  window_stack_push(s_window, true);
}

static void window_load(Window* window) {
  s_layer_header = layer_create(GRect(1, 1, 142, 25));
  layer_set_update_proc(s_layer_header, layer_header_update);
  layer_add_child(window_get_root_layer(window),s_layer_header);

  s_layer_text = text_layer_create(GRect(0, 25, 144, 168 - 24));
  text_layer_set_font(s_layer_text, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_layer_text, "Hints: long-press middle button to toggle alarm on/off. Alarms only go off when the app is closed!");
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(s_layer_text));
}

static void window_unload(Window* window) {
  layer_destroy(s_layer_header);
  text_layer_destroy(s_layer_text);
}

static void layer_header_update(Layer* layer, GContext* ctx) {
  //graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 12, GCornersAll);
  graphics_draw_text(ctx, "Alarms++ v2.9", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(0, -2, 144, 24), GTextOverflowModeFill, GTextAlignmentCenter, 0);
}