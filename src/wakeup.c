//
//  wakeup.c
//  alarms++
//
//  Created by Christian Reinbacher on 01.11.14.
//
//

#include "wakeup.h"
#include "win-main.h"
#include "storage.h"

static Window *s_main_window;
static TextLayer *s_output_layer;
static InverterLayer *s_inverter_layer;
static ActionBarLayer *action_bar;
static char output_text[10];
static bool *s_snooze;
static Alarm *s_alarm;

void do_vibrate(void);
void vibe_timer_callback(void* data);
AppTimer* vibe_timer = NULL;
AppTimer* cancel_vibe_timer = NULL;
bool cancel_vibrate=false;

static void wakeup_handler(WakeupId id, int32_t reason) {
  // The app has woken!

}

static void dismiss_click_handler(ClickRecognizerRef recognizer, void *context) {
  *s_snooze=false;
  if(alarm_is_one_time(s_alarm))
    s_alarm->enabled=false;
  window_stack_pop(true);
}

static void snooze_click_handler(ClickRecognizerRef recognizer, void *context) {
  *s_snooze=true;
  int snooze_delay;
  load_persistent_storage_snooze_delay(&snooze_delay);
  time_t timestamp = time(NULL) + 60*snooze_delay;
  s_alarm->alarm_id = wakeup_schedule(timestamp,0,true);
  struct tm *t = localtime(&timestamp);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Scheduled snooze at %d.%d %d:%d",t->tm_mday, t->tm_mon+1,t->tm_hour,t->tm_min);
  window_stack_pop(true);
}

static void do_nothing_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, do_nothing_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, do_nothing_click_handler);
  bool longpress_dismiss;
  load_persistent_storage_longpress_dismiss(&longpress_dismiss);
  if(longpress_dismiss)
    window_long_click_subscribe(BUTTON_ID_UP,1000,dismiss_click_handler,NULL);
  else
    window_single_click_subscribe(BUTTON_ID_UP, dismiss_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, snooze_click_handler);
}

void do_vibrate(void) {
  vibes_long_pulse();
  vibe_timer = app_timer_register(1000,vibe_timer_callback,NULL);
}

void vibe_timer_callback(void* data) {
  //vibe_timer = NULL;
  if(!cancel_vibrate)
    do_vibrate();
}

void cancel_vibe_timer_callback(void* data) {
  //vibe_timer = NULL;
  cancel_vibrate = true;
}

static void main_window_load(Window *window) {
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  
  action_bar = action_bar_layer_create();
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
  action_bar_layer_set_icon(action_bar,BUTTON_ID_UP,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CROSS));
  action_bar_layer_set_icon(action_bar,BUTTON_ID_DOWN,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_ZZ));
  action_bar_layer_add_to_window(action_bar,window);
  
  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(0, bounds.size.h/2-21-(clock_is_24h_style()?0:21), bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
  text_layer_set_font(s_output_layer,fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  //snprintf(output_text, sizeof(output_text), "00:00");
  text_layer_set_text(s_output_layer, output_text);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_output_layer));
  s_inverter_layer = inverter_layer_create(GRect(0,0,bounds.size.w,bounds.size.h));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(s_inverter_layer));
  do_vibrate();
  // switch off vibration after 3 minutes
  cancel_vibe_timer = app_timer_register(1000*60*3,cancel_vibe_timer_callback,NULL);
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
  inverter_layer_destroy(s_inverter_layer);
  app_timer_cancel(vibe_timer);
}

void perform_wakeup_tasks(Alarm* alarms, bool *snooze)
{
  s_snooze=snooze;
  // Create main Window
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_set_fullscreen(s_main_window,true);
  
  // Subscribe to Wakeup API
  wakeup_service_subscribe(wakeup_handler);
  
  // Was this a wakeup?
  if(launch_reason() == APP_LAUNCH_WAKEUP) {
    // The app was started by a wakeup
    WakeupId id = 0;
    int32_t reason = 0;
    
    // Get details and handle the wakeup
    wakeup_get_launch_event(&id, &reason);
    // search for alarm which caused the wakeup
    s_alarm = alarms;
    while(s_alarm->alarm_id!=id)
      s_alarm++;
    if(clock_is_24h_style())
      snprintf(output_text, sizeof(output_text), "%02d:%02d",s_alarm->hour,s_alarm->minute);
    else
    {
      int hour;
      bool is_am;
      convert_24_to_12(s_alarm->hour, &hour, &is_am);
      snprintf(output_text, sizeof(output_text), "%02d:%02d %s",hour,s_alarm->minute,is_am?"AM":"PM");
    }
    light_enable_interaction();
    window_stack_push(s_main_window, true);
    //wakeup_handler(id, (int32_t)alarms);
  }
  else{
    *snooze=false;
    win_main_init(alarms);
    win_main_show();
  }
}