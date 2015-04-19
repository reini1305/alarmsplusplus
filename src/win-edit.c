//
//  win-edit.c
//  alarms++
//
//  Created by Christian Reinbacher on 01.11.14.
//
//

#include "win-edit.h"
#include "localize.h"
#include "tertiary_text.h"
#include "debug.h"
#include "timeout.h"

static void window_load(Window* window);
static void window_unload(Window* window);
static void am_pm_window_load(Window* window);
static void am_pm_window_unload(Window* window);

static Window *s_am_pm_window;
static ActionBarLayer *s_am_pm_actionbar;
static TextLayer *s_am_pm_textlayer;
static char s_am_pm_textbuffer[3];

static Window *s_window;
static MenuLayer* s_menu;
static NumberWindow *hour_window,*minute_window;
static GBitmap *check_icon,*up_icon,*down_icon;
static bool s_is_am;
static bool s_select_all;

static void progress_to_minutes(NumberWindow *window,void* context);
static void progress_to_days(NumberWindow *window,void* context);

// Menu stuff
#define MENU_SECTION_WEEKDAYS 1
#define MENU_SECTION_OK 0

static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context);
static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t section_index, void* callback_context);
static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);
static int16_t menu_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context);
static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context);
static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context);
static void menu_select(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context);

static Alarm temp_alarm;
static Alarm *current_alarm;

//static char *english[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//static char *german[7] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
//static char *french[7] = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"};
//static char *spanish[7] = {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"};
//static char *russian[7] = {"Воскресенье","Понедельник","Вторник","Среда","Четверг","Пятница","Суббота"};
//static char **weekday_names=english;


void win_edit_init(void)
{
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  s_am_pm_window = window_create();
  window_set_window_handlers(s_am_pm_window, (WindowHandlers) {
    .load = am_pm_window_load,
    .unload = am_pm_window_unload
  });
  // Use selocale() to obtain the system locale for translation
  char *sys_locale = setlocale(LC_ALL, "");
  if (strcmp("de_DE", sys_locale) == 0) {
    //weekday_names = german;
    hour_window = number_window_create("Stunde",(NumberWindowCallbacks){.selected=progress_to_minutes},NULL);
    minute_window = number_window_create("Minute",(NumberWindowCallbacks){.selected=progress_to_days},NULL);
  } else if (strcmp("fr_FR", sys_locale) == 0) {
    //weekday_names = french;
    hour_window = number_window_create("heure",(NumberWindowCallbacks){.selected=progress_to_minutes},NULL);
    minute_window = number_window_create("minute",(NumberWindowCallbacks){.selected=progress_to_days},NULL);
  } else if (strcmp("es_ES", sys_locale) == 0) {
    //weekday_names = spanish;
    hour_window = number_window_create("hora",(NumberWindowCallbacks){.selected=progress_to_minutes},NULL);
    minute_window = number_window_create("minuto",(NumberWindowCallbacks){.selected=progress_to_days},NULL);
  } else {
    hour_window = number_window_create("Hour",(NumberWindowCallbacks){.selected=progress_to_minutes},NULL);
    minute_window = number_window_create("Minute",(NumberWindowCallbacks){.selected=progress_to_days},NULL);
  }
  
  number_window_set_min(hour_window,0);
  number_window_set_max(hour_window,23);
  number_window_set_step_size(hour_window,1);

  number_window_set_min(minute_window,0);
  number_window_set_max(minute_window,59);
  number_window_set_step_size(minute_window,1);
  
#ifdef PBL_COLOR
  check_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK_INV);
  up_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_UP_INV);
  down_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_DOWN_INV);
#else
  check_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK);
  up_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_UP);
  down_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_DOWN);
#endif
  
  tertiary_text_init();

}

void win_edit_show(Alarm *alarm){
  memcpy(&temp_alarm,alarm,sizeof(Alarm));
  current_alarm = alarm;
  s_select_all = false;
  if(clock_is_24h_style())
  {
    number_window_set_value(hour_window,temp_alarm.hour);
    window_stack_push(number_window_get_window(hour_window),true);
  }
  else
  {
    number_window_set_min(hour_window,1);
    number_window_set_max(hour_window,12);
    int hour;
    convert_24_to_12(temp_alarm.hour, &hour, &s_is_am);
    temp_alarm.hour = hour;
    number_window_set_value(hour_window,temp_alarm.hour);
    snprintf(s_am_pm_textbuffer,sizeof(s_am_pm_textbuffer),s_is_am?"AM":"PM");
    window_stack_push(s_am_pm_window,true);
  }
}

void progress_to_days(NumberWindow *window,void* context)
{
  temp_alarm.minute = number_window_get_value(window);
  window_stack_push(s_window,true);
  refresh_timeout();
}

void progress_to_minutes(NumberWindow *window,void* context)
{
  temp_alarm.hour = number_window_get_value(window);
  number_window_set_value(minute_window,temp_alarm.minute);
  
  window_stack_push(number_window_get_window(minute_window),true);
  refresh_timeout();
}

static void up_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_is_am=!s_is_am;
  snprintf(s_am_pm_textbuffer,sizeof(s_am_pm_textbuffer),s_is_am?"AM":"PM");
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_push(number_window_get_window(hour_window),true);
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, up_down_click_handler);
}

void am_pm_window_load(Window* window)
{
  GRect bounds = layer_get_bounds(window_get_root_layer(window));

  s_am_pm_actionbar = action_bar_layer_create();
  action_bar_layer_set_click_config_provider(s_am_pm_actionbar, click_config_provider);
  action_bar_layer_set_icon(s_am_pm_actionbar,BUTTON_ID_UP,up_icon);
  action_bar_layer_set_icon(s_am_pm_actionbar,BUTTON_ID_DOWN,down_icon);
  action_bar_layer_set_icon(s_am_pm_actionbar,BUTTON_ID_SELECT,check_icon);
  action_bar_layer_add_to_window(s_am_pm_actionbar,window);

  s_am_pm_textlayer = text_layer_create(GRect(0, bounds.size.h/2-21, bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
  text_layer_set_text_alignment(s_am_pm_textlayer, GTextAlignmentCenter);
  text_layer_set_font(s_am_pm_textlayer,fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text(s_am_pm_textlayer,s_am_pm_textbuffer);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_am_pm_textlayer));
}

void am_pm_window_unload(Window *window)
{
  action_bar_layer_destroy(s_am_pm_actionbar);
  text_layer_destroy(s_am_pm_textlayer);
  
}

void window_load(Window* window)
{
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  s_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_num_sections,
    .get_num_rows = menu_num_rows,
    .get_cell_height = menu_cell_height,
    .get_header_height = menu_header_height,
    .draw_header = menu_draw_header,
    .draw_row = menu_draw_row,
    .select_click = menu_select,
  });
  
  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu, s_window);
  
  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(s_menu));
}

void window_unload(Window* window)
{
  menu_layer_destroy(s_menu);
  //gbitmap_destroy(check_icon);
}

static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context) {
  return 2;
}

static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t section_index, void* callback_context) {
  switch (section_index) {
    case MENU_SECTION_WEEKDAYS:
      return 8;
      break;
    case MENU_SECTION_OK:
      return 2;
    default:
      break;
  }
  return 7;
}

static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context) {
  return 28;
}

static int16_t menu_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context) {
  return 16;
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context) {
  graphics_context_set_text_color(ctx, GColorWhite);
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorDukeBlue);
  #else
    graphics_context_set_fill_color(ctx, GColorBlack);
  #endif
  graphics_fill_rect(ctx,GRect(0,1,144,14),0,GCornerNone);
  
  graphics_draw_text(ctx, section_index==MENU_SECTION_OK?_("Update Alarm"):_("Weekdays"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, 144 - 33, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
#ifdef PBL_COLOR
  if(menu_cell_layer_is_highlighted(cell_layer))
  {
    graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_fill_rect(ctx,GRect(0,0,144,28),0,GCornerNone);
    
  }
  else{
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  }
#else
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
#endif
  char* text = NULL;
  GFont font = NULL;
  bool draw_checkmark=false;
  
  if(cell_index->section == MENU_SECTION_OK)
  {
    if(cell_index->row==0) // OK
    {
      text = _("OK"),
      font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    }
    if(cell_index->row==1) // set text
    {
      text = _("Description");
      font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    }
  }
  else
  {
    if(cell_index->row==0) // select all-none
    {
      text = _("Select All/None");
      font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
      // draw checkmark if enabled
      draw_checkmark = s_select_all;
    }
    else
    {
      switch (cell_index->row) {
        case 1:
          text = _("Sunday");
          break;
        case 2:
          text = _("Monday");
          break;
        case 3:
          text = _("Tuesday");
          break;
        case 4:
          text = _("Wednesday");
          break;
        case 5:
          text = _("Thursday");
          break;
        case 6:
          text = _("Friday");
          break;
        case 7:
          text = _("Saturday");
          break;
          
        default:
          break;
      }
      //text = weekday_names[cell_index->row-1];
      font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
      // draw checkmark if enabled
      draw_checkmark = temp_alarm.weekdays_active[cell_index->row-1];
    }
  }
  graphics_draw_text(ctx, text,
                     font,
                     GRect(3, -3, 144 - 33, 28), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
  if(draw_checkmark)
    graphics_draw_bitmap_in_rect(ctx, check_icon, GRect(144 - 3 - 16, 6, 16, 16));
}

static void menu_select(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context) {
  switch (cell_index->section) {
    case MENU_SECTION_OK:
      if(cell_index->row==0)
      {
        // update timer, destroy windows
        temp_alarm.enabled=true;
        if(clock_is_24h_style())
        {
          memcpy(current_alarm,&temp_alarm,sizeof(Alarm));
          window_stack_pop(true);
          window_stack_pop(false);
          window_stack_pop(false);
        }
        else
        {
          // convert hours and am/pm back
          APP_LOG(APP_LOG_LEVEL_DEBUG,"Hour before conversion is %d",temp_alarm.hour);
          if(s_is_am)
          {
            int hour = temp_alarm.hour;
            hour -= 12;
            if(hour<0) hour+=12;
            APP_LOG(APP_LOG_LEVEL_DEBUG,"Hour after conversion is %d",hour);
            temp_alarm.hour = hour;
          }
          else
            temp_alarm.hour = ((temp_alarm.hour+12)%12) + 12;
          memcpy(current_alarm,&temp_alarm,sizeof(Alarm));
          window_stack_pop(true);
          window_stack_pop(false);
          window_stack_pop(false);
          window_stack_pop(false);
        }
      }
      else
        tertiary_text_show(temp_alarm.description);
      break;
    case MENU_SECTION_WEEKDAYS:
      if(cell_index->row==0)
      {
        s_select_all=!s_select_all;
        for(int i=0;i<7;temp_alarm.weekdays_active[i++]=s_select_all);
      }
      else {
        temp_alarm.weekdays_active[cell_index->row-1]=!temp_alarm.weekdays_active[cell_index->row-1];
        menu_layer_set_selected_next(menu,false,MenuRowAlignCenter,true);
      }
      layer_mark_dirty(menu_layer_get_layer(menu));
      break;
      
    default:
      break;
  }
  refresh_timeout();
}
