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

static void time_window_load(Window* window);
static void time_window_unload(Window* window);
static void click_config_provider(void *context);

static Window *s_time_window;
static Layer *s_canvas_layer;
static TextLayer *s_input_layers[3];

static char s_value_buffers[3][3];
static int s_selection;
static char s_digits[3];
static char s_max[3];
static char s_min[3];
static bool s_withampm;
#ifdef PBL_RECT
#define OFFSET_LEFT 0
#define OFFSET_TOP 0
#define ITEM_HEIGHT 28
#define OFFSET_ITEM_TOP 0
#else
#define OFFSET_LEFT 18
#define OFFSET_TOP 12
#define ITEM_HEIGHT 40
#define OFFSET_ITEM_TOP 6
#endif
#define PIN_WINDOW_SPACING 24
static const GPathInfo PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, -5}, {5,5}, {-5, 5}}
};
static GPath *s_my_path_ptr;

static void window_load(Window* window);
static void window_unload(Window* window);

static Window *s_window;
static MenuLayer* s_menu;
static GBitmap *check_icon,*check_icon_inv;
static bool s_is_am;
static bool s_select_all;

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

#ifndef PBL_PLATFORM_APLITE
static DictationSession *s_dictation_session;
static void dictation_session_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {

  if (status == DictationSessionStatusSuccess) {
    strncpy(temp_alarm.description, transcription, sizeof(temp_alarm.description));
  }
}
#endif

void win_edit_init(void)
{
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  s_time_window = window_create();
  window_set_window_handlers(s_time_window, (WindowHandlers) {
    .load = time_window_load,
    .unload = time_window_unload,
  });
#ifdef PBL_PLATFORM_APLITE
  tertiary_text_init();
#else
  s_dictation_session = dictation_session_create(sizeof(temp_alarm.description),
                                                 dictation_session_callback, NULL);
#endif
#ifdef PBL_COLOR
  check_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK_INV);
  check_icon_inv = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK);
#else
  check_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK);
#endif
  s_my_path_ptr= gpath_create(&PATH_INFO);
}

void win_edit_show(Alarm *alarm){
  memcpy(&temp_alarm,alarm,sizeof(Alarm));
  current_alarm = alarm;
  s_select_all = false;
  s_max[2]=1;s_min[2]=0;
  s_max[1]=59;s_min[1]=0;
  int hour;
  if(clock_is_24h_style())
  {
    s_withampm=false;
    s_max[0]=23;s_min[0]=0;
  }
  else
  {
    s_withampm=true;
    s_max[0]=12;s_min[0]=1;
    convert_24_to_12(temp_alarm.hour, &hour, &s_is_am);
    temp_alarm.hour = hour;
    s_digits[2] = s_is_am;
  }
  s_selection = 0;
  if(temp_alarm.hour==0 && temp_alarm.minute==0)
  {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    convert_24_to_12(t->tm_hour, &hour, &s_is_am);
    s_digits[0] = clock_is_24h_style()?t->tm_hour:hour;
    s_digits[1] = t->tm_min;
    s_digits[2] = s_is_am;
  } else
  {
    s_digits[0] = temp_alarm.hour;
    s_digits[1] = temp_alarm.minute;
  }
  window_stack_push(s_time_window,true);
}

// Time input window stuff

static void update_ui(Layer *layer, GContext *ctx) {
  
  for(int i = 0; i < 3; i++) {
#ifdef PBL_COLOR
    text_layer_set_background_color(s_input_layers[i], (i == s_selection) ? GColorBlue : GColorDarkGray);
    if(i==s_selection)
    {
      GPoint selection_center = {
        .x = (int16_t) (s_withampm?23:50) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING) + OFFSET_LEFT,
        .y = (int16_t) 50 + OFFSET_TOP,
      };
      gpath_rotate_to(s_my_path_ptr, 0);
      gpath_move_to(s_my_path_ptr, selection_center);
      graphics_context_set_fill_color(ctx,GColorBlue);
      gpath_draw_filled(ctx, s_my_path_ptr);
      gpath_rotate_to(s_my_path_ptr, TRIG_MAX_ANGLE/2);
      selection_center.y = 110 + OFFSET_TOP;
      gpath_move_to(s_my_path_ptr, selection_center);
      gpath_draw_filled(ctx, s_my_path_ptr);
    }
#else
    text_layer_set_background_color(s_input_layers[i], (i == s_selection) ? GColorBlack : GColorWhite);
    text_layer_set_text_color(s_input_layers[i], (i == s_selection) ? GColorWhite : GColorBlack);
    if(i==s_selection)
    {
      GPoint selection_center = {
        .x = (int16_t) (s_withampm?23:50) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING) + OFFSET_LEFT,
        .y = (int16_t) 50 + OFFSET_TOP,
      };
      gpath_rotate_to(s_my_path_ptr, 0);
      gpath_move_to(s_my_path_ptr, selection_center);
      graphics_context_set_fill_color(ctx,GColorBlack);
      gpath_draw_filled(ctx, s_my_path_ptr);
      gpath_rotate_to(s_my_path_ptr, TRIG_MAX_ANGLE/2);
      selection_center.y = 110 + OFFSET_TOP;
      gpath_move_to(s_my_path_ptr, selection_center);
      gpath_draw_filled(ctx, s_my_path_ptr);
    }
#endif
    if(i<2)
      snprintf(s_value_buffers[i], sizeof("00"), "%02d", s_digits[i]);
    else
      snprintf(s_value_buffers[i], sizeof("AM"), s_digits[i]?"AM":"PM");
    text_layer_set_text(s_input_layers[i], s_value_buffers[i]);
  }
  layer_set_hidden(text_layer_get_layer(s_input_layers[2]),!s_withampm);
  // draw the :
#ifdef PBL_COLOR
  graphics_context_set_text_color(ctx,GColorBlue);
#else
  graphics_context_set_text_color(ctx,GColorBlack);
#endif
  graphics_draw_text(ctx,":",fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),GRect(s_withampm?144/2-27:144/2 + OFFSET_LEFT,58 + OFFSET_TOP,40,20),
                     GTextOverflowModeWordWrap,GTextAlignmentLeft,NULL);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Next column
  s_selection++;
  
  if(s_selection == (s_withampm? 3:2)) {
    temp_alarm.hour = s_digits[0];
    temp_alarm.minute = s_digits[1];
    s_is_am = s_digits[2];
    window_stack_push(s_window,true);
    s_selection--;
  }
  else
    layer_mark_dirty(s_canvas_layer);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Previous column
  s_selection--;
  
  if(s_selection == -1) {
    window_stack_pop(true);
  }
  else
    layer_mark_dirty(s_canvas_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_selection == 0 && s_withampm && s_digits[s_selection] == s_max[s_selection] - 1)
    s_digits[2] = !s_digits[2];
  
  s_digits[s_selection] += s_digits[s_selection] == s_max[s_selection] ? -s_max[s_selection] : 1;

  if(s_selection == 0 && s_withampm && s_digits[0] == 0)
    s_digits[0] = 1;
	
  layer_mark_dirty(s_canvas_layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_selection == 0 && s_withampm && s_digits[s_selection] == s_max[s_selection])
    s_digits[2] = !s_digits[2];
	  
  s_digits[s_selection] -= (s_digits[s_selection] == 0) ? -s_max[s_selection] : 1;
	
  if(s_selection == 0 && s_withampm && s_digits[0] == 0)
    s_digits[0] = s_max[0];
  
  layer_mark_dirty(s_canvas_layer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 70, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 70, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void time_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // init hands
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_ui);
  layer_add_child(window_layer, s_canvas_layer);
  
  for(int i = 0; i < 3; i++) {
    s_input_layers[i] = text_layer_create(GRect((s_withampm?3:30) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING) + OFFSET_LEFT, 60 + OFFSET_TOP, 40, 40));
#ifdef PBL_COLOR
    text_layer_set_text_color(s_input_layers[i], GColorWhite);
    text_layer_set_background_color(s_input_layers[i], GColorDarkGray);
#else
    text_layer_set_text_color(s_input_layers[i], GColorBlack);
    text_layer_set_background_color(s_input_layers[i], GColorWhite);
#endif
    text_layer_set_text(s_input_layers[i], "00");
    text_layer_set_font(s_input_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(s_input_layers[i], GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_input_layers[i]));
  }
  window_set_click_config_provider(window, click_config_provider);
  layer_mark_dirty(s_canvas_layer);
}

static void time_window_unload(Window *window) {
  for(int i = 0; i < 3; i++) {
    text_layer_destroy(s_input_layers[i]);
  }
  layer_destroy(s_canvas_layer);
  //window_destroy(window);
}


// Alarm options window stuff
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
  
#ifdef PBL_COLOR
  menu_layer_set_highlight_colors(s_menu,GColorBlue,GColorWhite);
  menu_layer_pad_bottom_enable(s_menu,false);
#endif
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
  return ITEM_HEIGHT;
}

static int16_t menu_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context) {
#ifdef PBL_RECT
  return 16;
#else
  if(section_index == MENU_SECTION_WEEKDAYS)
  return 16;
  else return 0;
#endif
}

static void menu_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context) {
  graphics_context_set_text_color(ctx, GColorWhite);
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorBlue);
  #else
    graphics_context_set_fill_color(ctx, GColorBlack);
  #endif
  GRect layer_size = layer_get_bounds(cell_layer);
  graphics_fill_rect(ctx,GRect(0,1,layer_size.size.w,14),0,GCornerNone);
  
#ifdef PBL_RECT
  graphics_draw_text(ctx, section_index==MENU_SECTION_OK?_("Update Alarm"):_("Weekdays"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
#else
  if (section_index==MENU_SECTION_WEEKDAYS)
  graphics_draw_text(ctx, _("Weekdays"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, layer_size.size.w - 3, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
#endif
}

static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
#ifdef PBL_COLOR
//  if(menu_cell_layer_is_highlighted(cell_layer))
//  {
//    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
////    graphics_context_set_fill_color(ctx, GColorBlack);
////    graphics_context_set_text_color(ctx, GColorWhite);
////    graphics_fill_rect(ctx,GRect(0,0,144,28),0,GCornerNone);
//    
//  }
//  else{
//    graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
//  }
#else
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
#endif
  char* text = NULL;
  GFont font = NULL;
  bool draw_checkmark=false;
  GRect layer_size = layer_get_bounds(cell_layer);
  
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
#ifdef PBL_RECT
  graphics_draw_text(ctx, text,
                     font,
                     GRect(3, -3 + OFFSET_ITEM_TOP, layer_size.size.w - 3, 28), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
#else
  graphics_draw_text(ctx, text,
                     font,
                     GRect(3, -3 + OFFSET_ITEM_TOP, layer_size.size.w - 3, 28), GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
#endif
  if(draw_checkmark)
  {
#if PBL_COLOR
  if(menu_cell_layer_is_highlighted(cell_layer))
    graphics_draw_bitmap_in_rect(ctx, check_icon, GRect(layer_size.size.w - 3 - 16 - OFFSET_LEFT, 6+ OFFSET_ITEM_TOP, 16, 16));
  else
    graphics_draw_bitmap_in_rect(ctx, check_icon_inv, GRect(layer_size.size.w - 3 - 16 - OFFSET_LEFT, 6+ OFFSET_ITEM_TOP, 16, 16));
#else
    graphics_draw_bitmap_in_rect(ctx, check_icon, GRect(layer_size.size.w - 3 - 16 - OFFSET_LEFT, 6+ OFFSET_ITEM_TOP, 16, 16));
#endif
  }
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
        }
      }
      else
      {
#ifdef PBL_PLATFORM_APLITE
        tertiary_text_show(temp_alarm.description);
#else
        dictation_session_start(s_dictation_session);
#endif
      }
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
