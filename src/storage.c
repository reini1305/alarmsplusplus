//
//  storage.c
//  alarms++
//
//  Created by Christian Reinbacher on 02.11.14.
//
//

#include "storage.h"

void load_persistent_storage_alarms(Alarm *alarms)
{
  if(persist_exists(ALARMS_OLD_KEY)) // we have to migrate the old timers
  {
    persist_read_data(ALARMS_OLD_KEY,alarms,5*sizeof(Alarm));
    for (int i=5; i<NUM_ALARMS; i++) {
    alarms[i].hour=0;
    alarms[i].minute=0;
    alarms[i].enabled=false;
    alarms[i].alarm_id=-1;
    for (int weekday=0; weekday<7;weekday++)
      alarms[i].weekdays_active[weekday]=true;
    }
    // delete it from memory
    persist_delete(ALARMS_OLD_KEY);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Migrated Timers");
  }
  else
  {
    if(persist_exists(ALARMS_KEY))
      persist_read_data(ALARMS_KEY,alarms,NUM_ALARMS*sizeof(Alarm));
    else
    {
      for (int i=0; i<NUM_ALARMS; i++) {
        alarms[i].hour=0;
        alarms[i].minute=0;
        alarms[i].enabled=false;
        alarms[i].alarm_id=-1;
        for (int weekday=0; weekday<7;weekday++)
          alarms[i].weekdays_active[weekday]=true;
      }
    }
  }
}

void write_persistent_storage_alarms(Alarm *alarms)
{
  persist_write_data(ALARMS_KEY,alarms,NUM_ALARMS*sizeof(Alarm));
}

void load_persistent_storage_snooze_delay(int *snooze_delay)
{
  *snooze_delay = 10;
  if(persist_exists(SNOOZE_KEY))
    *snooze_delay = persist_read_int(SNOOZE_KEY);
}

void load_persistent_storage_longpress_dismiss(bool *longpress_dismiss)
{
  *longpress_dismiss = false;
  if(persist_exists(LONGPRESS_DISMISS_KEY))
    *longpress_dismiss = persist_read_bool(LONGPRESS_DISMISS_KEY);
}

void load_persistent_storage_hide_unused_alarms(bool *hide_unused_alarms)
{
  *hide_unused_alarms = false;
  if(persist_exists(HIDE_UNUSED_ALARMS_KEY))
    *hide_unused_alarms = persist_read_bool(HIDE_UNUSED_ALARMS_KEY);
}

void load_persistent_storage_vibration_pattern(int *vibration_pattern)
{
  *vibration_pattern = 0;
  if(persist_exists(VIBRATION_PATTERN_KEY))
    *vibration_pattern = persist_read_int(VIBRATION_PATTERN_KEY);
}

void load_persistent_storage_flip_to_snooze(bool *flip_to_snooze)
{
  *flip_to_snooze = false;
  if(persist_exists(FLIP_TO_SNOOZE_KEY))
    *flip_to_snooze = persist_read_bool(FLIP_TO_SNOOZE_KEY);
}