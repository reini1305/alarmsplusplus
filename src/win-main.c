#include <pebble.h>
#include "alarms.h"
#include "win-edit.h"
#include "win-snooze.h"
#include "win-about.h"
#include "storage.h"
#include "localize.h"
#include "timeout.h"

#define MENU_SECTION_ALARMS   0
#define MENU_SECTION_OTHER   1

#define MENU_ROW_COUNT_OTHER 6
#define MENU_ROW_COUNT_ALARMS NUM_ALARMS

#define MENU_ROW_OTHER_ABOUT         5
#define MENU_ROW_OTHER_LONGPRESS     4
#define MENU_ROW_OTHER_SNOOZE        1
#define MENU_ROW_OTHER_HIDE          0
#define MENU_ROW_OTHER_VIBRATION     3
#define MENU_ROW_OTHER_FLIP          2

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

static Window*    s_window;
static MenuLayer* s_menu;
static Alarm* s_alarms;
static int s_snooze_delay;
static char s_snooze_text[12];
static bool s_longpress_dismiss;
static bool s_hide_unused_alarms;
static char s_id_enabled[NUM_ALARMS];
static char s_num_enabled;
static int s_vibration_pattern;
static bool s_flip_to_snooze;

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
  alarm_set_language();
  s_snooze_delay = load_persistent_storage_int(SNOOZE_KEY,10);
  s_longpress_dismiss = load_persistent_storage_bool(LONGPRESS_DISMISS_KEY,false);
  s_hide_unused_alarms = load_persistent_storage_bool(HIDE_UNUSED_ALARMS_KEY,false);
  s_vibration_pattern = load_persistent_storage_int(VIBRATION_PATTERN_KEY,0);
  s_flip_to_snooze = load_persistent_storage_bool(FLIP_TO_SNOOZE_KEY, false);
  update_id_enabled();
  refresh_timeout();
}

void win_main_show(void) {
  window_stack_push(s_window, false);
}

static void window_load(Window* window) {
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);
  
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
  
  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(s_menu));
}

static void window_unload(Window* window) {
  menu_layer_destroy(s_menu);
}

static void window_appear(Window* window) {
  update_id_enabled();
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
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorBlack);
    
    graphics_draw_text(ctx, _("Options"),
                       fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(3, -2, 144 - 33, 14), GTextOverflowModeWordWrap,
                       GTextAlignmentLeft, NULL);
  }
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
      menu_draw_row_alarms(ctx, cell_layer, s_hide_unused_alarms?s_id_enabled[cell_index->row]:cell_index->row);
      break;
    case MENU_SECTION_OTHER:
      menu_draw_row_other(ctx, cell_layer, cell_index->row);
      break;
  }
}

static void menu_draw_row_alarms(GContext* ctx, const Layer* cell_layer, uint16_t row_index) {
  Alarm* current_alarm = &s_alarms[row_index];
  alarm_draw_row(current_alarm,ctx);
}

static void menu_draw_row_other(GContext* ctx, const Layer* cell_layer, uint16_t row_index) {
  switch (row_index) {
    case MENU_ROW_OTHER_ABOUT:
      // This is a basic menu item with a title and subtitle
      menu_cell_basic_draw(ctx, cell_layer, _("Help"), "Alarms++ v2.9", NULL);
      break;
    case MENU_ROW_OTHER_SNOOZE:
      snprintf(s_snooze_text,sizeof(s_snooze_text),"%02d %s",s_snooze_delay,_("Minutes"));
      menu_cell_basic_draw(ctx, cell_layer, _("Snooze Delay"), s_snooze_text, NULL);
      break;
    case MENU_ROW_OTHER_LONGPRESS:
      menu_cell_basic_draw(ctx, cell_layer, _("Dismiss Alarm"), s_longpress_dismiss?_("Long press"):_("Short press"), NULL);
      break;
    case MENU_ROW_OTHER_HIDE:
      menu_cell_basic_draw(ctx, cell_layer, _("Disabled Alarms"), s_hide_unused_alarms?_("Hide"):_("Show"), NULL);
      break;
    case MENU_ROW_OTHER_FLIP:
      menu_cell_basic_draw(ctx, cell_layer, _("Shake to Snooze"), s_flip_to_snooze?_("Enabled"):_("Disabled"), NULL);
      break;
    case MENU_ROW_OTHER_VIBRATION:
      switch (s_vibration_pattern) {
        case 0:
          menu_cell_basic_draw(ctx, cell_layer, _("Vibration Strength"), _("Constant"), NULL);
          break;
        case 1:
          menu_cell_basic_draw(ctx, cell_layer, _("Vibration Strength"), _("Increasing 10s"), NULL);
          break;
        case 2:
          menu_cell_basic_draw(ctx, cell_layer, _("Vibration Strength"), _("Increasing 20s"), NULL);
          break;
        case 3:
          menu_cell_basic_draw(ctx, cell_layer, _("Vibration Strength"), _("Increasing 30s"), NULL);
          break;
        default:
          break;
      }
      
      break;
  }
}

static void menu_select(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
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

static void menu_select_long(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
      alarm_toggle_enable(&s_alarms[s_hide_unused_alarms?s_id_enabled[cell_index->row]:cell_index->row]);
      update_id_enabled();
      menu_layer_reload_data(menu);
      layer_mark_dirty(menu_layer_get_layer(menu));
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
    case MENU_ROW_OTHER_LONGPRESS:
      s_longpress_dismiss=!s_longpress_dismiss;
      persist_write_bool(LONGPRESS_DISMISS_KEY,s_longpress_dismiss);
      break;
    case MENU_ROW_OTHER_HIDE:
      s_hide_unused_alarms=!s_hide_unused_alarms;
      persist_write_bool(HIDE_UNUSED_ALARMS_KEY,s_hide_unused_alarms);
      break;
    case MENU_ROW_OTHER_FLIP:
      s_flip_to_snooze=!s_flip_to_snooze;
      persist_write_bool(FLIP_TO_SNOOZE_KEY,s_flip_to_snooze);
      break;
    case MENU_ROW_OTHER_VIBRATION:
      s_vibration_pattern++;
      if(s_vibration_pattern>3)
        s_vibration_pattern=0;
      persist_write_int(VIBRATION_PATTERN_KEY,s_vibration_pattern);
      break;
  }
}

static void menu_selection_changed(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context)
{
  refresh_timeout();
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
