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