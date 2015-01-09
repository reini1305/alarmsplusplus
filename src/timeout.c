//
//  timeout.c
//  alarmsplusplus
//
//  Created by Christian Reinbacher on 09.01.15.
//
//

#include <pebble.h>
#include "timeout.h"
#include "debug.h"

static AppTimer *timeout_timer=NULL;

void kill_app_timer_callback(void* data) {
  window_stack_pop_all(true);
}
void refresh_timeout(void)
{
  // set a timeout for 600 seconds = 10 minutes. After that, the programm ends gracefully.
  if(!timeout_timer)
    timeout_timer = app_timer_register(600000,kill_app_timer_callback,NULL);
  else
    app_timer_reschedule(timeout_timer,600000);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Rescheduling Timer");
}