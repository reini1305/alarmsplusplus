//
//  storage.c
//  alarms++
//
//  Created by Christian Reinbacher on 02.11.14.
//
//

#include "storage.h"
#include "debug.h"

void load_persistent_storage_alarms(Alarm *alarms)
{
  /*if(persist_exists(ALARMS_OLD_OLD_KEY)) // we have to migrate the old timers
  {
    AlarmOld alarms_old[5];
    persist_read_data(ALARMS_OLD_KEY,alarms_old,5*(sizeof(AlarmOld)));
    for (int i=0; i<5; i++) {
      alarms[i].hour=alarms_old[i].hour;
      alarms[i].minute=alarms_old[i].minute;
      alarms[i].enabled=alarms_old[i].enabled;
      alarms[i].alarm_id=alarms_old[i].alarm_id;
      for (int weekday=0; weekday<7;weekday++)
        alarms[i].weekdays_active[weekday]=alarms_old[i].weekdays_active[weekday];
    }
    for (int i=5; i<NUM_ALARMS; i++) {
      alarms[i].hour=0;
      alarms[i].minute=0;
      alarms[i].enabled=false;
      alarms[i].alarm_id=-1;
      for (int weekday=0; weekday<7;weekday++)
        alarms[i].weekdays_active[weekday]=true;
    }
    // delete it from memory
    persist_delete(ALARMS_OLD_OLD_KEY);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Migrated Timers");
  } else */if(persist_exists(ALARMS_OLD_KEY)) // we have to migrate the old timers
  {
    AlarmOld alarms_old[8];
    persist_read_data(ALARMS_OLD_KEY,&alarms_old,8*(sizeof(AlarmOld)));
    for (int i=0; i<NUM_ALARMS; i++) {
      alarms[i].hour=alarms_old[i].hour;
      alarms[i].minute=alarms_old[i].minute;
      alarms[i].enabled=alarms_old[i].enabled;
      alarms[i].alarm_id=alarms_old[i].alarm_id;
      for (int weekday=0; weekday<7;weekday++)
        alarms[i].weekdays_active[weekday]=alarms_old[i].weekdays_active[weekday];
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

bool load_persistent_storage_bool(int key, bool default_val)
{
  bool temp = default_val;
  if(persist_exists(key))
    temp = persist_read_bool(key);
  return temp;
}
int load_persistent_storage_int(int key, int default_val)
{
  int temp = default_val;
  if(persist_exists(key))
    temp = persist_read_bool(key);
  return temp;
}