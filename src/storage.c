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
  if(persist_exists(ALARMS_OLD_KEY)) // we have to migrate to the new number of alarms
  {
    persist_read_data(ALARMS_OLD_KEY,alarms,8*sizeof(Alarm));
    for (int i=8; i<NUM_ALARMS; i++) {
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
    for(int i=0;i<NUM_ALARMS/8;i++) { // we can only store 8 alarms in one memory slot
      if (persist_exists(ALARMS_KEY+i)) {
        persist_read_data(ALARMS_KEY+i,&alarms[8*i],8*sizeof(Alarm));
      }
      else {
        for (int j=0; j<8; j++) {
          alarm_reset(&alarms[8*i+j]);
        }
      }
    }
  }
}

void write_persistent_storage_alarms(Alarm *alarms)
{
  for(int i=0;i<NUM_ALARMS/8;i++) // we can only store 8 alarms in one memory slot
    persist_write_data(ALARMS_KEY+i,&alarms[8*i],8*sizeof(Alarm));
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