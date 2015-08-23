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
#include "pwm_vibrate.h"
#include "debug.h"

static Window *s_main_window;
static TextLayer *s_output_layer;
static TextLayer *s_description_layer;
#ifdef PBL_SDK_2
  static InverterLayer *s_inverter_layer;
#endif
static ActionBarLayer *action_bar;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_logo;
static char output_text[10];
static bool *s_snooze;
static Alarm *s_alarm;
static uint32_t s_segments[]={600,1};
static VibePatternPWM s_pwmPat = {
  .durations = s_segments,
  .num_segments = 2
};
static int s_vibe_counter = 0;
static int s_vibration_pattern = 0;
static int s_vibration_duration = 0;
static int s_auto_snooze=false;
//static int s_last_z = 10000;

void do_vibrate(void);
void vibe_timer_callback(void* data);
AppTimer* vibe_timer = NULL;
AppTimer* cancel_vibe_timer = NULL;
bool cancel_vibrate=false;
//bool s_flip_to_snooze=false;

static void wakeup_handler(WakeupId id, int32_t reason) {
  // The app has woken!

}

static void dismiss_click_handler(ClickRecognizerRef recognizer, void *context) {
  *s_snooze=false;
  if(alarm_is_one_time(s_alarm))
    s_alarm->enabled=false;
  window_stack_pop(true);
}

static void do_snooze(void)
{
  *s_snooze=true;
  int snooze_delay;
  snooze_delay = load_persistent_storage_int(SNOOZE_KEY,10);
  time_t timestamp = time(NULL) + 60*snooze_delay;
  s_alarm->alarm_id = wakeup_schedule(timestamp,0,true);
  struct tm *t = localtime(&timestamp);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Scheduled snooze at %d.%d %d:%d",t->tm_mday, t->tm_mon+1,t->tm_hour,t->tm_min);
  window_stack_pop(true);
}

static void snooze_click_handler(ClickRecognizerRef recognizer, void *context) {
  do_snooze();
}

static void do_nothing_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, do_nothing_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, do_nothing_click_handler);
  bool longpress_dismiss = load_persistent_storage_bool(LONGPRESS_DISMISS_KEY,false);
#if PBL_SDK_3
  if(longpress_dismiss)
    window_long_click_subscribe(BUTTON_ID_DOWN,1000,dismiss_click_handler,NULL);
  else
    window_single_click_subscribe(BUTTON_ID_DOWN, dismiss_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, snooze_click_handler);
#else
  if(longpress_dismiss)
  window_long_click_subscribe(BUTTON_ID_UP,1000,dismiss_click_handler,NULL);
  else
  window_single_click_subscribe(BUTTON_ID_UP, dismiss_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, snooze_click_handler);
#endif
}

void do_vibrate(void) {
  if(s_vibration_pattern)
  {
    s_pwmPat.durations[1] = (s_vibe_counter/s_vibration_pattern)+1;
    s_vibe_counter++;
    vibes_enqueue_custom_pwm_pattern(&s_pwmPat);
  }
  else
    vibes_long_pulse();
  vibe_timer = app_timer_register(1000,vibe_timer_callback,NULL);
}

void vibe_timer_callback(void* data) {
  if(!cancel_vibrate)
    do_vibrate();
}

void cancel_vibe_timer_callback(void* data) {
  cancel_vibrate = true;
  if(s_auto_snooze)
    do_snooze();
}

/*static void data_handler(AccelData *data, uint32_t num_samples) {
  if(s_last_z>4000) // we are called for the first time
    s_last_z = data[0].z;
  if((s_last_z>0)!=(data[0].z>0)) // sign flip
    do_snooze();
  s_last_z = data[0].z;
}*/

/*static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // Process tap on ACCEL_AXIS_X, ACCEL_AXIS_Y or ACCEL_AXIS_Z
  // Direction is 1 or -1
  if(axis == ACCEL_AXIS_X || axis == ACCEL_AXIS_Z) // flick your wrist
    do_snooze();
}*/

static void update_text(struct tm *t) {
  if(clock_is_24h_style())
    snprintf(output_text, sizeof(output_text), "%02d:%02d",t->tm_hour,t->tm_min);
  else
  {
    int hour;
    bool is_am;
    convert_24_to_12(t->tm_hour, &hour, &is_am);
    snprintf(output_text, sizeof(output_text), "%02d:%02d %s",hour,t->tm_min,is_am?"AM":"PM");
  }
}

static void handle_tick(struct tm *t, TimeUnits units_changed) {
  update_text(t);
  layer_mark_dirty(text_layer_get_layer(s_output_layer));
  if(alarm_has_description(s_alarm))
    layer_mark_dirty(text_layer_get_layer(s_description_layer));
}


static void main_window_load(Window *window) {
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  
  action_bar = action_bar_layer_create();
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
#ifdef PBL_SDK_3
  action_bar_layer_set_icon_animated(action_bar,BUTTON_ID_DOWN,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CROSS_INV),true);
  action_bar_layer_set_icon_animated(action_bar,BUTTON_ID_UP,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_ZZ_INV),true);
#else
  action_bar_layer_set_icon(action_bar,BUTTON_ID_UP,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CROSS));
  action_bar_layer_set_icon(action_bar,BUTTON_ID_DOWN,gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_ZZ));
#endif
  action_bar_layer_add_to_window(action_bar,window);
  
  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(0, bounds.size.h/2-21-(clock_is_24h_style()?0:21), bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
#ifdef PBL_SDK_2
  text_layer_set_font(s_output_layer,fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
#else
  text_layer_set_font(s_output_layer,fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
#endif
  //snprintf(output_text, sizeof(output_text), "00:00");
  text_layer_set_text(s_output_layer, output_text);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_output_layer));
  
  //snprintf(output_text, sizeof(output_text), "00:00");
  if(alarm_has_description(s_alarm))
  {
    // Create Description
    s_description_layer = text_layer_create(GRect(0, 6, bounds.size.w-ACTION_BAR_WIDTH, 36));
    text_layer_set_text_alignment(s_description_layer, GTextAlignmentCenter);
    text_layer_set_font(s_description_layer,fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text(s_description_layer, s_alarm->description);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_description_layer));
  }
  else
  {
    // Create Bitmap
    s_bitmap_layer = bitmap_layer_create(GRect(0,10,bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
    s_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);
    bitmap_layer_set_bitmap(s_bitmap_layer,s_logo);
    bitmap_layer_set_alignment(s_bitmap_layer,GAlignTop);
    layer_add_child(window_get_root_layer(window),bitmap_layer_get_layer(s_bitmap_layer));
  }
#ifdef PBL_SDK_2
  s_inverter_layer = inverter_layer_create(GRect(0,0,bounds.size.w,bounds.size.h));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(s_inverter_layer));
#endif
  s_vibration_pattern = load_persistent_storage_int(VIBRATION_PATTERN_KEY,0);
  s_vibration_duration = load_persistent_storage_int(VIBRATION_DURATION_KEY, 2);
  s_auto_snooze = load_persistent_storage_bool(AUTO_SNOOZE_KEY, true);
  do_vibrate();
  // switch off vibration after x minutes
  switch (s_vibration_duration) {
    case 0:
      s_vibration_duration = 30;
      break;
    case 1:
    s_vibration_duration = 60;
    break;
    case 2:
    s_vibration_duration = 120;
    break;
    case 3:
    s_vibration_duration = 300;
    break;
    case 4:
    s_vibration_duration = 2;
    break;
    case 5:
    s_vibration_duration = 10;
    break;
    default:
    break;
  }
  cancel_vibe_timer = app_timer_register(1000*s_vibration_duration,cancel_vibe_timer_callback,NULL);
  
  // test snoozing with the accelerometer
  /*if(s_flip_to_snooze)
  {
    accel_tap_service_subscribe(&accel_tap_handler);
  }*/
}

static void main_window_unload(Window *window) {
  app_timer_cancel(vibe_timer);
  //if(s_flip_to_snooze)
  //  accel_tap_service_unsubscribe();
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
#ifndef PBL_COLOR
  window_set_fullscreen(s_main_window,true);
#endif
  
  //s_flip_to_snooze = load_persistent_storage_bool(FLIP_TO_SNOOZE_KEY, false);
  
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
    
    // Get the current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    update_text(t);
    
    //handle_tick(t, MINUTE_UNIT);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
    light_enable_interaction();
    window_stack_push(s_main_window, true);
    //wakeup_handler(id, (int32_t)alarms);
  }
  else if(launch_reason() == APP_LAUNCH_WORKER)  // worker woke us because something is with the current time
  {
    reschedule_wakeup(alarms);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Worker woke me up!");
    // app should end here without a window
  }
  else{
    *snooze=false;
    win_main_init(alarms);
    win_main_show();
  }
}