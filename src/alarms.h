//
//  alarms.h
//  alarms++
//
//  Created by Christian Reinbacher on 01.11.14.
//
//

#ifndef alarms___alarms_h
#define alarms___alarms_h

#include <pebble.h>
#define NUM_ALARMS 8

typedef struct Alarm{
  unsigned char hour;
  unsigned char minute;
  bool weekdays_active[7];
  bool enabled;
  WakeupId alarm_id;
}Alarm;

void alarm_draw_row(Alarm* alarm, GContext* ctx);
void alarm_toggle_enable(Alarm *alarm);
void alarm_schedule_wakeup(Alarm *alarm);
void alarm_cancel_wakeup(Alarm *alarm);
void reschedule_wakeup(Alarm *alarms);
void convert_24_to_12(int hour_int, int* hour_out, bool* am);
bool alarm_is_one_time(Alarm *alarm);
#endif
