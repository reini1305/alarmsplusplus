//
//  timeline.c
//  alarmsplusplus
//
//  Created by Christian Reinbacher on 24.11.15.
//
//

#include "timeline.h"
#include "debug.h"
//#define KEY_NEXT_ALARM 0
//#define KEY_DESCRIPTION 1
//#define KEY_READY 2

static bool communication_ready;
static Alarm* retry_alarm;
static void prv_inbox_recived(DictionaryIterator *iter, void *context) {
  
  // change to configuring screen
  if (dict_find(iter, MESSAGE_KEY_READY)) {
    communication_ready=true;
    if(retry_alarm)
      alarm_phone_send_pin(retry_alarm);
  }
}


void setup_communication(void) {
  communication_ready=false;
  retry_alarm = NULL;
  app_message_register_inbox_received(prv_inbox_recived);
  app_message_open(10, 50);
}

void destroy_communication(void) {
  communication_ready=false;
  retry_alarm = NULL;
  app_message_deregister_callbacks();
}

void alarm_phone_send_pin(Alarm* alarm) {
  if (communication_ready) {
    time_t now = time(NULL);
    // begin iterator
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    // write data
    dict_write_uint32(iter, MESSAGE_KEY_NEXT_ALARM, alarm_get_time_of_wakeup(alarm)-now);
    dict_write_cstring(iter,MESSAGE_KEY_DESCRIPTION, alarm_has_description(alarm)? alarm->description:"Alarms++");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Pin Sent: %d", (int)ret);
    dict_write_end(iter);
    // send
    int res = app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Pin Sent: %d", res);
    retry_alarm=NULL;
  }
  else
    retry_alarm=alarm;
}
