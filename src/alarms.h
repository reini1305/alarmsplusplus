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
#ifdef PBL_PLATFORM_APLITE
#define NUM_ALARMS 24
#define DESCRIPTION_LENGTH 9
typedef struct Alarm{
  unsigned char hour;
  unsigned char minute;
  bool weekdays_active[7];
  bool enabled;
  WakeupId alarm_id;
  char description[DESCRIPTION_LENGTH+1];
}Alarm;

#else
#define NUM_ALARMS 32
#define DESCRIPTION_LENGTH 19
typedef struct Alarm_old{
  unsigned char hour;
  unsigned char minute;
  bool weekdays_active[7];
  bool enabled;
  WakeupId alarm_id;
  char description[10];
}Alarm_old;

typedef struct Alarm{
  unsigned char hour;
  unsigned char minute;
  bool enabled;
  unsigned char smart_alarm_minutes;
  WakeupId alarm_id;
  bool weekdays_active[7];
  char description[DESCRIPTION_LENGTH+1];
}Alarm;
#endif

void alarm_toggle_enable(Alarm *alarm);
time_t alarm_get_time_of_wakeup(Alarm *alarm);
void alarm_reset(Alarm *alarm);
void reschedule_wakeup(Alarm *alarms);
void convert_24_to_12(int hour_int, int* hour_out, bool* am);
bool alarm_is_one_time(Alarm *alarm);
bool alarm_is_set(Alarm *alarm);
bool alarm_has_description(Alarm *alarm);
int8_t get_next_free_slot(Alarm *alarms);
bool is_24h();
int get_next_alarm(Alarm *alarms);

#endif
