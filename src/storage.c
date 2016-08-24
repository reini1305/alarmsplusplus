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
  // Perform migration
  if(persist_exists(ALARMS_OLD_KEY)) {
    Alarm_old temp_alarms[8];
    for(int i=0;i<NUM_ALARMS/8;i++) { // we can only store 8 alarms in one memory slot on aplite
      persist_read_data(ALARMS_OLD_KEY+i,temp_alarms,8*sizeof(Alarm_old));
      persist_delete(ALARMS_OLD_KEY+i);
      for (int j=0; j<8; j++) {
        alarms[8*i+j].alarm_id = temp_alarms[j].alarm_id;
        alarms[8*i+j].enabled = temp_alarms[j].enabled;
        alarms[8*i+j].hour = temp_alarms[j].hour;
        alarms[8*i+j].minute = temp_alarms[j].minute;
        alarms[8*i+j].smart_alarm_minutes = 0;
        for (int w=0; w<7; w++) {
          alarms[8*i+j].weekdays_active[w] = temp_alarms[j].weekdays_active[w];
        }
        strncpy(temp_alarms[j].description,alarms[8*i].description,8);
      }
    }
  } else {
    for(int i=0;i<NUM_ALARMS/4;i++) { // we can only store 4 alarms in one memory slot on other platforms
      if (persist_exists(ALARMS_KEY+i)) {
        persist_read_data(ALARMS_KEY+i,&alarms[4*i],4*sizeof(Alarm));
      }
      else {
        for (int j=0; j<4; j++) {
          alarm_reset(&alarms[4*i+j]);
        }
      }
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Size of one alarm struct: %d",sizeof(Alarm));
}

void write_persistent_storage_alarms(Alarm *alarms)
{
  for(int i=0;i<NUM_ALARMS/4;i++) // we can only store 4 alarms in one memory slot on other platforms
  persist_write_data(ALARMS_KEY+i,&alarms[4*i],4*sizeof(Alarm));
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
