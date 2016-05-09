#include <pebble.h>


static void window_load(Window* window);
static void window_unload(Window* window);

#define NUM_STATES 8
#define KEY_UP 0
#define KEY_DOWN 1
#define KEY_RIGHT 2
static unsigned char keys[NUM_STATES];
static int8_t        curr_state;
static Window*       s_window;
static Layer         *s_canvas_layer;
static const GPathInfo PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, -50}, {50,50}, {-50, 50}}
};
static GPath         *s_my_path_ptr;

void win_konami_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  // Populate keys to press
  srand(time(NULL));
  curr_state = 0;
  keys[0] = rand()%3;
  for (int8_t i=1; i<NUM_STATES; i++) {
    keys[i] = (keys[i-1]+1+rand()%2)%3;
  }
  
  // Prepare arrow
  s_my_path_ptr= gpath_create(&PATH_INFO);
}

void win_konami_show(void) {
  window_stack_push(s_window, true);
}

////////////////////////////////////////////////////////////////////////////////////
static void update_ui(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_frame(layer);
  // Draw the current arrow
  GPoint selection_center = {
    .x = (int16_t) bounds.size.w/2,
    .y = (int16_t) bounds.size.h/2,
  };
  int32_t rotate_to=0;
  switch (keys[curr_state]) {
    case KEY_DOWN:
      rotate_to=TRIG_MAX_ANGLE/2;
      break;
    case KEY_RIGHT:
      rotate_to=TRIG_MAX_ANGLE/4;
      break;
    // Up does nothing because it is already pointing down
  }
  gpath_rotate_to(s_my_path_ptr, rotate_to);
  gpath_move_to(s_my_path_ptr, selection_center);
  graphics_context_set_fill_color(ctx,PBL_IF_COLOR_ELSE(GColorBlue,GColorBlack));
  gpath_draw_filled(ctx, s_my_path_ptr);

}

static void handle_click(int8_t key) {
  if(key==keys[curr_state]) {
    curr_state++;
  }
  if(curr_state==NUM_STATES) {
    window_stack_pop_all(true); // Gracefully exit the app
  }
  layer_mark_dirty(s_canvas_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  handle_click(KEY_RIGHT);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  curr_state--;
  if (curr_state<0) {
    window_stack_pop(true);
  } else {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  handle_click(KEY_UP);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  handle_click(KEY_DOWN);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 70, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 70, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void window_load(Window* window) {
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);
  window_set_click_config_provider(window, click_config_provider);
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_ui);
  layer_add_child(window_layer, s_canvas_layer);
  
}

static void window_unload(Window* window) {
  
}