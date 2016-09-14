
#include "appglance.h"
//#include "debug.h"

#if PBL_API_EXISTS(app_glance_reload)
static char* weekdays[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
static void prv_update_app_glance(AppGlanceReloadSession *session,
                                  size_t limit, void *context) {
  // This should never happen, but developers should always ensure they are
  // not adding more slices than are available
  if (limit < 1) return;

  // Cast the context object to a string
  const char *message = context;

  if(message) {
    time_t expiration_time = time(NULL);
    // Create the AppGlanceSlice
    // NOTE: When .icon_resource_id is not set, the app's default icon is used
    const AppGlanceSlice entry = (AppGlanceSlice) {
      .layout = {
        .icon = APP_GLANCE_SLICE_DEFAULT_ICON,//PUBLISHED_ID_LOGO,
        .subtitle_template_string = message
      },
      .expiration_time = expiration_time+3600*24*7
    };

    // Add the slice, and check the result
    const AppGlanceResult result = app_glance_add_slice(session, entry);

    if (result != APP_GLANCE_RESULT_SUCCESS) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "AppGlance Error: %d", result);
    }
  }
}
#endif

void update_app_glance(Alarm* alarms, bool show_remaining) {
#if PBL_API_EXISTS(app_glance_reload)
  int alarm_id = get_next_alarm(alarms);
  char next_alarm_text[100];
  if(alarm_id>=0)
  {
    time_t timestamp = alarm_get_time_of_wakeup(&alarms[alarm_id]);
    if(show_remaining) {
      snprintf(next_alarm_text,sizeof(next_alarm_text),"{time_until(%d)|format(>0S:'Next in %%Hh %%Mm','')}",(unsigned int)timestamp);
    } else {
      struct tm *t = localtime(&timestamp);
      if(clock_is_24h_style())
        snprintf(next_alarm_text,sizeof(next_alarm_text),"Next %salarm: %s, %02d:%02d",alarms[alarm_id].smart_alarm_minutes>0?"smart ":"", weekdays[t->tm_wday],t->tm_hour,t->tm_min);
      else
      {
        int temp_hour;
        bool is_am;
        convert_24_to_12(t->tm_hour, &temp_hour, &is_am);
        snprintf(next_alarm_text,sizeof(next_alarm_text),"Next alarm: %02d:%02d %s",temp_hour,t->tm_min,is_am?"AM":"PM");
      }
    }
    app_glance_reload(prv_update_app_glance,next_alarm_text);
  } else {
    app_glance_reload(prv_update_app_glance,NULL);
  }
#endif
}
