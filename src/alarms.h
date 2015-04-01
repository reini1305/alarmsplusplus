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
#define DESCRIPTION_LENGTH 9

typedef struct Alarm{
  unsigned char hour;
  unsigned char minute;
  bool weekdays_active[7];
  bool enabled;
  WakeupId alarm_id;
  char description[DESCRIPTION_LENGTH+1];
}Alarm;

typedef struct AlarmOld{
  unsigned char hour;
  unsigned char minute;
  bool weekdays_active[7];
  bool enabled;
  WakeupId alarm_id;
}AlarmOld;

void alarm_draw_row(Alarm* alarm, GContext* ctx, bool selected, bool reset);
void alarm_toggle_enable(Alarm *alarm);
void alarm_schedule_wakeup(Alarm *alarm);
void alarm_cancel_wakeup(Alarm *alarm);
void alarm_reset(Alarm *alarm);
void reschedule_wakeup(Alarm *alarms);
void convert_24_to_12(int hour_int, int* hour_out, bool* am);
bool alarm_is_one_time(Alarm *alarm);
void alarm_set_language(void);
bool alarm_has_description(Alarm *alarm);
#endif
