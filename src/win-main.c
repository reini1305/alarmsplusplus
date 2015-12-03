#include <pebble.h>
#include "alarms.h"
#include "win-edit.h"
//#include "win-snooze.h"
//#include "win-about.h"
#include "win-advanced.h"
#include "storage.h"
#include "localize.h"
#include "timeout.h"
#include "timeline.h"
#include "pebble_process_info.h"
#include "debug.h"

#define MENU_SECTION_ALARMS   0
#define MENU_SECTION_OTHER   1

#define MENU_ROW_COUNT_OTHER 4
#define MENU_ROW_COUNT_ALARMS NUM_ALARMS

#ifdef PBL_RECT
#define ALARM_HEIGHT 32
#define DESCRIPTION_HEIGHT 58
#define ALARM_OFFSET_LEFT 0
#define ALARM_OFFSET_RIGHT 0
#define ALARM_OFFSET_TOP 0
#define OTHER_HEIGHT 38
#else
#define ALARM_HEIGHT 64
#define DESCRIPTION_HEIGHT 64
#define ALARM_OFFSET_LEFT 18
#define ALARM_OFFSET_RIGHT 36
#define ALARM_OFFSET_TOP 8
#define OTHER_HEIGHT 42
#endif

#define MENU_ROW_NEXT_ALARM          0
#define MENU_ROW_OTHER_ABOUT         3
#define MENU_ROW_OTHER_SNOOZE        1
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
static void menu_draw_row_add(GContext* ctx, bool selected, int width);
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
static StatusBarLayer *s_status_layer;
static Layer *s_battery_layer;

static Alarm* s_alarms;
static int s_snooze_delay;
static char s_snooze_text[12];
static char s_next_alarm_text[15];
static int8_t s_id_enabled[NUM_ALARMS];
static char s_num_enabled;

static int8_t s_id_reset = -1;
static bool s_can_be_reset = false;
static AppTimer *s_reset_timer;

extern const PebbleProcessInfo __pbl_app_info;
static char version_text[15];

static int16_t s_scroll_index;
static int16_t s_scroll_row_index;
static AppTimer *s_scroll_timer;

static char english[7] = {"SMTWTFS"};
static char german[7] = {"SMDMDFS"};
static char french[7] = {"dlmmjvs"};
static char spanish[7] = {"dlmmjvs"};
static char *weekday_names=english;

// ActionMenu Stuff
static ActionMenu *s_action_menu;
static ActionMenuLevel *s_root_level;
int8_t s_selected_alarm;

static void action_reset_performed_callback(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  alarm_reset(&s_alarms[s_selected_alarm]);
}

static void action_toggle_performed_callback(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  alarm_toggle_enable(&s_alarms[s_selected_alarm]);
}

static void init_action_menu() {
  // Create the root level and secondary custom patterns level
  s_root_level = action_menu_level_create(2);
  
  // Set up the actions for this level, using action context to pass types
  action_menu_level_add_action(s_root_level, "Enable/Disable", action_toggle_performed_callback,
                               NULL);
  action_menu_level_add_action(s_root_level, "Delete", action_reset_performed_callback,
                               NULL);
  
}


void refresh_next_alarm_text(void)
{
  int alarm_id = get_next_alarm(s_alarms);
  // schedule the winner
  if(alarm_id>=0)
  {
    time_t timestamp = alarm_get_time_of_wakeup(&s_alarms[alarm_id]);
    struct tm *t = localtime(&timestamp);
    if(clock_is_24h_style())
      snprintf(s_next_alarm_text,sizeof(s_next_alarm_text),"%02d.%02d %02d:%02d",t->tm_mday, t->tm_mon+1,t->tm_hour,t->tm_min);
    else
    {
      int temp_hour;
      bool is_am;
      convert_24_to_12(t->tm_hour, &temp_hour, &is_am);
      snprintf(s_next_alarm_text,sizeof(s_next_alarm_text),"%02d.%02d %02d:%02d %s",t->tm_mday, t->tm_mon+1,temp_hour,t->tm_min,is_am?"AM":"PM");
    }
    alarm_phone_send_pin(&s_alarms[alarm_id]);
  }
  else
  {
    snprintf(s_next_alarm_text,sizeof(s_next_alarm_text),"---");
  }
}

void win_main_init(Alarm* alarms) {
  s_alarms = alarms;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
    .appear = window_appear
  });
  win_edit_init();
  //win_snooze_init();
  //win_about_init();
  win_advanced_init();
  s_snooze_delay = load_persistent_storage_int(SNOOZE_KEY,10);
  update_id_enabled();
  refresh_timeout();
  refresh_next_alarm_text();
  snprintf(version_text, sizeof(version_text), "Alarms++ v%d.%d",__pbl_app_info.process_version.major,__pbl_app_info.process_version.minor);
  s_scroll_timer = app_timer_register(500,scroll_timer_callback,NULL);
  s_reset_timer = app_timer_register(2000,reset_timer_callback,NULL);
  init_action_menu();
  char *sys_locale = setlocale(LC_ALL, "");
  if (strcmp("de_DE", sys_locale) == 0) {
    weekday_names = german;
  } else if (strcmp("fr_FR", sys_locale) == 0) {
    weekday_names = french;
  } else if (strcmp("es_ES", sys_locale) == 0) {
    weekday_names = spanish;
  }
}

void win_main_show(void) {
  window_stack_push(s_window, false);
}

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


static void window_load(Window* window) {
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);
  
#if defined(PBL_RECT)
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
}

static void window_unload(Window* window) {
  menu_layer_destroy(s_menu);
}

static void window_appear(Window* window) {
  update_id_enabled();
  refresh_next_alarm_text();
  menu_layer_reload_data(s_menu);
  layer_mark_dirty(menu_layer_get_layer(s_menu));
}

static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context) {
  return 2;
}

static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t section_index, void* callback_context) {
  switch (section_index) {
    case MENU_SECTION_ALARMS:
    return get_next_free_slot(s_alarms)>=0?s_num_enabled+1:s_num_enabled;
    case MENU_SECTION_OTHER:
      return MENU_ROW_COUNT_OTHER;
    default:
      return 0;
  }
}



static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_OTHER:
      return OTHER_HEIGHT;
      break;
    case MENU_SECTION_ALARMS:
      if (get_next_free_slot(s_alarms)>=0) {
        if(cell_index->row==0)
          return ALARM_HEIGHT;
        else
          if(alarm_has_description(&s_alarms[s_id_enabled[cell_index->row-1]]))
            return DESCRIPTION_HEIGHT;
      }
      else
        if(alarm_has_description(&s_alarms[s_id_enabled[cell_index->row]]))
          return DESCRIPTION_HEIGHT;
      break;
  }
  return ALARM_HEIGHT;
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
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack));
    
    GRect layer_size = layer_get_bounds(cell_layer);
    graphics_fill_rect(ctx,GRect(0,1,layer_size.size.w,14),0,GCornerNone);
#ifdef PBL_RECT
    graphics_draw_text(ctx, _("Options"),
                       fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                       GTextAlignmentLeft, NULL);
#else
    graphics_draw_text(ctx, _("Options"),
                       fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                       GTextAlignmentCenter, NULL);
#endif
  }
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
      if(get_next_free_slot(s_alarms)>=0)
      {
        if(cell_index->row==0) {
          GRect layer_size = layer_get_bounds(cell_layer);
          menu_draw_row_add(ctx,menu_cell_layer_is_highlighted(cell_layer),layer_size.size.w);
        }
        else
          menu_draw_row_alarms(ctx, cell_layer, cell_index->row-1);
      }
      else
        menu_draw_row_alarms(ctx, cell_layer, cell_index->row);
      break;
    case MENU_SECTION_OTHER:
      menu_draw_row_other(ctx, cell_layer, cell_index->row);
      break;
  }
}

static void menu_draw_row_add(GContext* ctx, bool selected, int width) {
  // draw a plus sign, consisting of two lines
  int8_t size = 20;
  if(selected)
    graphics_context_set_stroke_color(ctx,GColorWhite);
  else
    graphics_context_set_stroke_color(ctx,GColorBlack);
  graphics_context_set_stroke_width(ctx,2);
  graphics_draw_line(ctx, GPoint(width/2,(ALARM_HEIGHT-size)/2), GPoint(width/2,ALARM_HEIGHT-(ALARM_HEIGHT-size)/2));
  graphics_draw_line(ctx, GPoint((width-size)/2,ALARM_HEIGHT/2), GPoint((width+size)/2,ALARM_HEIGHT/2));

}

static void menu_draw_row_alarms(GContext* ctx, const Layer* cell_layer, uint16_t row_index) {
  Alarm* alarm = &s_alarms[s_id_enabled[row_index]];
  bool reset = s_id_reset==s_id_enabled[row_index];
  
  if(menu_cell_layer_is_highlighted(cell_layer))
    graphics_context_set_text_color(ctx,GColorWhite);
  else
    graphics_context_set_text_color(ctx,GColorBlack);

  GFont font = fonts_get_system_font(alarm->enabled?FONT_KEY_GOTHIC_28_BOLD:FONT_KEY_GOTHIC_28);
  
  GRect layer_size = layer_get_bounds(cell_layer);
  char alarm_time[6];
  int hour_out = alarm->hour;
  bool is_am = false;
#ifdef PBL_RECT
  int offset = ALARM_OFFSET_TOP;
#else
  int offset = ALARM_OFFSET_TOP+(alarm->enabled?0:8);
#endif
  if (!clock_is_24h_style())
  {
    convert_24_to_12(alarm->hour, &hour_out, &is_am);
    graphics_draw_text(ctx, is_am?"A\nM":"P\nM",
                       fonts_get_system_font(alarm->enabled?FONT_KEY_GOTHIC_14_BOLD:FONT_KEY_GOTHIC_14),
                       GRect(57-(alarm->enabled?3:0)+ALARM_OFFSET_LEFT, -1+offset, layer_size.size.w - 33 - ALARM_OFFSET_RIGHT, 30), GTextOverflowModeWordWrap,
                       GTextAlignmentLeft, NULL);
  }
  
  snprintf(alarm_time,sizeof(alarm_time),"%02d:%02d",hour_out,alarm->minute);
  graphics_draw_text(ctx, alarm_time,font,
                     GRect(3+ALARM_OFFSET_LEFT, -3+offset, layer_size.size.w - 33- ALARM_OFFSET_RIGHT, 28), GTextOverflowModeFill,
                     GTextAlignmentLeft, NULL);
  
  // draw activity state
  char state[]={"RST"};
  if(!reset)
  snprintf(state, sizeof(state), "%s",alarm->enabled? _("ON"):_("OFF"));
  graphics_draw_text(ctx, state,font,
                     GRect(3+ALARM_OFFSET_LEFT, alarm_has_description(alarm)?7:-3+offset, layer_size.size.w - 5 - ALARM_OFFSET_RIGHT, 28), GTextOverflowModeFill,
                     GTextAlignmentRight, NULL);
  
  // draw active weekdays
  char weekday_state[10];
  snprintf(weekday_state,sizeof(weekday_state),"%c%c%c%c%c\n%c%c",
           alarm->weekdays_active[1]?weekday_names[1]:'_',
           alarm->weekdays_active[2]?weekday_names[2]:'_',
           alarm->weekdays_active[3]?weekday_names[3]:'_',
           alarm->weekdays_active[4]?weekday_names[4]:'_',
           alarm->weekdays_active[5]?weekday_names[5]:'_',
           alarm->weekdays_active[6]?weekday_names[6]:'_',
           alarm->weekdays_active[0]?weekday_names[0]:'_');
  
  graphics_draw_text(ctx, weekday_state,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(70+ALARM_OFFSET_LEFT, -1+offset, layer_size.size.w - 33 - ALARM_OFFSET_RIGHT, 30), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
  
  // draw description
  if(alarm_has_description(alarm))
  {
    graphics_draw_text(ctx, alarm->description,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(10+ALARM_OFFSET_LEFT, 22+offset, layer_size.size.w - 3 - ALARM_OFFSET_RIGHT, 30), GTextOverflowModeWordWrap,
                       GTextAlignmentLeft, NULL);
  }
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
    case MENU_ROW_NEXT_ALARM:
      text = _("Next Alarm");
      subtext = s_next_alarm_text;
      break;
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
  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
        menu_select_alarms(cell_index->row);
      break;
    case MENU_SECTION_OTHER:
      menu_select_other(cell_index->row);
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
  // Configure the ActionMenu Window about to be shown
  ActionMenuConfig config = (ActionMenuConfig) {
    .root_level = s_root_level,
    .colors = {
      .background = GColorBlue,
      .foreground = GColorWhite,
    },
    .align = ActionMenuAlignCenter
  };

  switch (cell_index->section) {
    case MENU_SECTION_ALARMS:
      if(get_next_free_slot(s_alarms)>=0)
        if(cell_index->row==0)
          return;
        else
          s_selected_alarm=s_id_enabled[cell_index->row-1];
      else
        s_selected_alarm=s_id_enabled[cell_index->row];
      // Show the settings dialog
      
           // Show the ActionMenu
      s_action_menu = action_menu_open(&config);
      break;
    case MENU_SECTION_OTHER:
      if(cell_index->row == MENU_ROW_OTHER_SNOOZE)
      {
        s_snooze_delay--;
        if(s_snooze_delay<0)
          s_snooze_delay=30;
        persist_write_int(SNOOZE_KEY,s_snooze_delay);
        layer_mark_dirty(menu_layer_get_layer(menu));
      }
      break;
  }
}


static void menu_select_alarms(uint16_t row_index) {
  // Show the settings dialog
  int8_t next_slot = get_next_free_slot(s_alarms);
  if(next_slot>=0)
  {
    if (row_index==0)
      win_edit_show(&s_alarms[next_slot]);
    else
      win_edit_show(&s_alarms[s_id_enabled[row_index-1]]);
  }
  else
    win_edit_show(&s_alarms[s_id_enabled[row_index]]);
}

static void menu_select_other(uint16_t row_index) {
  switch (row_index) {
    case MENU_ROW_OTHER_SNOOZE:
      //win_snooze_show(&s_snooze_delay);
      s_snooze_delay++;
      if(s_snooze_delay>30)
        s_snooze_delay=0;
      persist_write_int(SNOOZE_KEY,s_snooze_delay);
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
    if(alarm_is_set(&s_alarms[i]))
    {
      s_id_enabled[id++]=i;
      s_num_enabled++;
    }
  }
}
