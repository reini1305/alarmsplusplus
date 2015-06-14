#include <pebble.h>
#include "alarms.h"
#include "win-edit.h"
#include "win-snooze.h"
#include "win-about.h"
#include "win-advanced.h"
#include "storage.h"
#include "localize.h"
#include "timeout.h"
#include "pebble_process_info.h"

#define MENU_SECTION_ALARMS   0
#define MENU_SECTION_OTHER   1

#define MENU_ROW_COUNT_OTHER 4
#define MENU_ROW_COUNT_ALARMS NUM_ALARMS

#define MENU_ROW_OTHER_ABOUT         3
#define MENU_ROW_OTHER_SNOOZE        1
#define MENU_ROW_OTHER_HIDE          0
#define MENU_ROW_OTHER_ADVANCED      2

static void window_load(Window* window);
static void window_unload(Window* window);
static void window_appear(Window* window);
static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context);
static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t section_index, void* callback_context);
static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);
static int16_t menu_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context);
static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context);
static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context);
static void menu_draw_row_alarms(GContext* ctx, const Layer* cell_layer, uint16_t row_index);
static void menu_draw_row_other(GContext* ctx, const Layer* cell_layer, uint16_t row_index);
static void menu_select(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context);
static void menu_select_long(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context);
static void menu_select_alarms(uint16_t row_index);
static void menu_select_other(uint16_t row_index);
static void menu_selection_changed(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context);
static void update_id_enabled(void);
static void scroll_timer_callback(void* data);
static void reset_timer_callback(void* data);

static Window*    s_window;
static MenuLayer* s_menu;
#ifdef PBL_SDK_3
static StatusBarLayer *s_status_layer;
static Layer *s_battery_layer;
#endif
static GBitmap* s_statusbar_bitmap;
static Alarm* s_alarms;
static int s_snooze_delay;
static char s_snooze_text[12];
static bool s_hide_unused_alarms;
static char s_id_enabled[NUM_ALARMS];
static char s_num_enabled;

static int8_t s_id_reset = -1;
static bool s_can_be_reset = false;
static AppTimer *s_reset_timer;

extern const PebbleProcessInfo __pbl_app_info;
static char version_text[15];

static int16_t s_scroll_index;
static int16_t s_scroll_row_index;
static AppTimer *s_scroll_timer;

void win_main_init(Alarm* alarms) {
  s_alarms = alarms;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
    .appear = window_appear
  });
  win_edit_init();
  win_snooze_init();
  win_about_init();
  win_advanced_init();
  alarm_set_language();
  s_snooze_delay = load_persistent_storage_int(SNOOZE_KEY,10);
  s_hide_unused_alarms = load_persistent_storage_bool(HIDE_UNUSED_ALARMS_KEY,false);
  update_id_enabled();
  refresh_timeout();
  snprintf(version_text, sizeof(version_text), "Alarms++ v%d.%d",__pbl_app_info.process_version.major,__pbl_app_info.process_version.minor);
  s_scroll_timer = app_timer_register(500,scroll_timer_callback,NULL);
  s_reset_timer = app_timer_register(2000,reset_timer_callback,NULL);
}

void win_main_show(void) {
  window_stack_push(s_window, false);
}

#ifdef PBL_SDK_3
static void battery_proc(Layer *layer, GContext *ctx) {
  // Emulator battery meter on Aplite
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(126, 4, 14, 8));
  graphics_draw_line(ctx, GPoint(140, 6), GPoint(140, 9));
  
  BatteryChargeState state = battery_state_service_peek();
  int width = (int)(float)(((float)state.charge_percent / 100.0F) * 10.0F);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(128, 6, width, 4), 0, GCornerNone);
}
#endif

static void window_load(Window* window) {
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);
  
#ifdef PBL_SDK_3
  s_status_layer = status_bar_layer_create();
  // Change the status bar width to make space for the action bar
  //  GRect frame = GRect(0, 0, width, STATUS_BAR_LAYER_HEIGHT);
  //  layer_set_frame(status_bar_layer_get_layer(status_layer), frame);
  status_bar_layer_set_colors(s_status_layer,GColorBlue,GColorWhite);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_layer));
  // Show legacy battery meter
  s_battery_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
  layer_set_update_proc(s_battery_layer, battery_proc);
  layer_add_child(window_layer, s_battery_layer);
  bounds.size.h-=STATUS_BAR_LAYER_HEIGHT;
  bounds.origin.y+=STATUS_BAR_LAYER_HEIGHT;
#endif
  
  // Create the menu layer
  s_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_num_sections,
    .get_num_rows = menu_num_rows,
    .get_cell_height = menu_cell_height,
    .draw_row = menu_draw_row,
    .select_click = menu_select,
    .select_long_click = menu_select_long,
    .draw_header = menu_draw_header,
    .get_header_height = menu_header_height,
    .selection_changed = menu_selection_changed,
  });
  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu, s_window);
#ifdef PBL_COLOR
  menu_layer_pad_bottom_enable(s_menu,false);
  menu_layer_set_highlight_colors(s_menu,GColorBlue,GColorWhite);
#endif
  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(s_menu));
  
  s_statusbar_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATUS);
  window_set_status_bar_icon(window,s_statusbar_bitmap);
}

static void window_unload(Window* window) {
  menu_layer_destroy(s_menu);
}

static void window_appear(Window* window) {
  update_id_enabled();
  menu_layer_reload_data(s_menu);
  layer_mark_dirty(menu_layer_get_layer(s_menu));
}

static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context) {
  return 2;
}

static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t section_index, void* callback_context) {
  switch (section_index) {
    case MENU_SECTION_ALARMS:
      if(s_hide_unused_alarms)
        return s_num_enabled;
      else
        return MENU_ROW_COUNT_ALARMS;
    case MENU_SECTION_OTHER:
      return MENU_ROW_COUNT_OTHER;
    default:
      return 0;
  }
}

static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_OTHER:
      return 38;
      break;
    case MENU_SECTION_ALARMS:
      if(alarm_has_description(&s_alarms[s_hide_unused_alarms?s_id_enabled[cell_index->row]:cell_index->row]))
        return 52;
      break;
  }
  return 32;
}

static int16_t menu_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context) {
  if(section_index==MENU_SECTION_OTHER)
    return 16;
  else
    return 0;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context) {
  if(section_index==MENU_SECTION_OTHER)
  {
    graphics_context_set_text_color(ctx, GColorWhite);
    #ifdef PBL_COLOR
        graphics_context_set_fill_color(ctx, GColorBlue);
    #else
        graphics_context_set_fill_color(ctx, GColorBlack);
    #endif
    
    graphics_fill_rect(ctx,GRect(0,1,144,14),0,GCornerNone);
    
    graphics_draw_text(ctx, _("Options"),
                       fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(3, -2, 144 - 33, 14), GTextOverflowModeWordWrap,
                       GTextAlignmentLeft, NULL);
  }
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
      menu_draw_row_alarms(ctx, cell_layer, cell_index->row);
      break;
    case MENU_SECTION_OTHER:
      menu_draw_row_other(ctx, cell_layer, cell_index->row);
      break;
  }
}

static void menu_draw_row_alarms(GContext* ctx, const Layer* cell_layer, uint16_t row_index) {
  int8_t current_id = s_hide_unused_alarms?s_id_enabled[row_index]:row_index;
#ifdef PBL_COLOR
  alarm_draw_row(&s_alarms[current_id],ctx,
                 menu_cell_layer_is_highlighted(cell_layer),
                 s_id_reset==current_id);
#else
  alarm_draw_row(&s_alarms[current_id],ctx,true,s_id_reset==current_id);
#endif
}

static void menu_cell_animated_draw(GContext* ctx, const Layer* cell_layer, char* text, char* subtext, bool animate)
{
  if(animate && s_scroll_index>0)
  {
    if(((int16_t)strlen(text)-15-s_scroll_index)>0)
      text+=s_scroll_index;
  }
  menu_cell_basic_draw(ctx,cell_layer,text,subtext,NULL);
}

static void scroll_timer_callback(void *data)
{
  s_scroll_index+=3;
  if(s_scroll_row_index>0)
  layer_mark_dirty(menu_layer_get_layer(s_menu));
  s_scroll_timer = app_timer_register(1000,scroll_timer_callback,NULL);
}

static void menu_draw_row_other(GContext* ctx, const Layer* cell_layer, uint16_t row_index) {
  bool animate = row_index==s_scroll_row_index;
  char *text = NULL;
  char *subtext = NULL;
  switch (row_index) {
    case MENU_ROW_OTHER_ABOUT:
      // This is a basic menu item with a title and subtitle
    text = _("Help");
    subtext = version_text;
      break;
    case MENU_ROW_OTHER_SNOOZE:
      snprintf(s_snooze_text,sizeof(s_snooze_text),"%02d %s",s_snooze_delay,_("Minutes"));
    text = _("Snooze Delay");
    subtext = s_snooze_text;
      break;
    case MENU_ROW_OTHER_HIDE:
    text = _("Disabled Alarms");
    subtext = s_hide_unused_alarms?_("Hide"):_("Show");
      break;
    case MENU_ROW_OTHER_ADVANCED:
    text = _("Advanced Options");
    subtext = _("Click");
      break;
  }
  menu_cell_animated_draw(ctx, cell_layer, text, subtext, animate);
}

static void menu_selection_changed(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context)
{
  s_scroll_index=0;
  if(new_index.section==MENU_SECTION_OTHER)
    s_scroll_row_index = new_index.row;
  else
    s_scroll_row_index = -1; // disable scrolling
  app_timer_reschedule(s_scroll_timer,1000);
  //s_id_reset = -1;
  refresh_timeout();
}

static void menu_select(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context) {
  int8_t current_id = s_hide_unused_alarms?s_id_enabled[cell_index->row]:cell_index->row;
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
      if(s_id_reset==current_id)  // the state is already reset
      {
        alarm_reset(&s_alarms[current_id]);
        s_id_reset = -1;
        update_id_enabled();
        menu_layer_reload_data(s_menu);
        layer_mark_dirty(menu_layer_get_layer(s_menu));
      }
      else
        menu_select_alarms(s_hide_unused_alarms?s_id_enabled[cell_index->row]:cell_index->row);
      break;
    case MENU_SECTION_OTHER:
      menu_select_other(cell_index->row);
      if(cell_index->row==MENU_ROW_OTHER_HIDE)
      {
        menu_layer_reload_data(menu);
        menu_layer_set_selected_index(menu,(MenuIndex){.row=0,.section=1},MenuRowAlignCenter,false);
      }
      layer_mark_dirty(menu_layer_get_layer(menu));
      break;
  }
  refresh_timeout();
}

static void reset_timer_callback(void *data)
{
  s_can_be_reset=false;
}

static void menu_select_long(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context) {
  int8_t current_id = s_hide_unused_alarms?s_id_enabled[cell_index->row]:cell_index->row;
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
      if(!s_can_be_reset)
        alarm_toggle_enable(&s_alarms[current_id]);
      else
      {
        if(s_id_reset==current_id)  // the state is already reset
        {
          alarm_toggle_enable(&s_alarms[current_id]);
          s_id_reset = -1;
        }
        else
        {
          if(s_alarms[current_id].enabled)
          {
            alarm_toggle_enable(&s_alarms[current_id]);
          }
          else
          {
            s_id_reset=current_id;
          }
        }
        //alarm_toggle_enable(&s_alarms[current_id]);
      }
      update_id_enabled();
      menu_layer_reload_data(menu);
      layer_mark_dirty(menu_layer_get_layer(menu));
      if(!app_timer_reschedule(s_reset_timer,2000))
        s_reset_timer=app_timer_register(2000,reset_timer_callback,NULL);
      s_can_be_reset=true;
      break;
    case MENU_SECTION_OTHER:
      break;
  }
}

static void menu_select_alarms(uint16_t row_index) {
  // Show the settings dialog
  win_edit_show(&s_alarms[row_index]);
}

static void menu_select_other(uint16_t row_index) {
  switch (row_index) {
    case MENU_ROW_OTHER_SNOOZE:
      win_snooze_show(&s_snooze_delay);
      break;
    case MENU_ROW_OTHER_ABOUT:
      win_about_show();
      break;
    case MENU_ROW_OTHER_HIDE:
      s_hide_unused_alarms=!s_hide_unused_alarms;
      persist_write_bool(HIDE_UNUSED_ALARMS_KEY,s_hide_unused_alarms);
      break;
    case MENU_ROW_OTHER_ADVANCED:
      win_advanced_show();
      break;
  }
}

static void update_id_enabled(void)
{
  int id=0;
  s_num_enabled=0;
  for (int i=0; i<NUM_ALARMS; i++) {
    if(s_alarms[i].enabled)
    {
      s_id_enabled[id++]=i;
      s_num_enabled++;
    }
  }
}
