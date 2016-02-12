//
//  alarms.c
//  alarms++
//
//  Created by Christian Reinbacher on 01.11.14.
//
//
#include "alarms.h"
#include "localize.h"
#include "debug.h"

time_t clock_to_timestamp_precise(WeekDay day, int hour, int minute)
{
  return (clock_to_timestamp(day, hour, minute)/60)*60;
}

bool alarm_is_enabled(Alarm *alarm)
{
  return alarm_weekday_is_active(alarm,7);
}

bool alarm_weekday_is_active(Alarm *alarm,int id)
{
  return alarm->weekday_bitfield & 1<<id;
}

void alarm_set_enabled(Alarm *alarm, bool value)
{
  alarm_set_weekday_active(alarm,7,value);
}

void alarm_set_weekday_active(Alarm *alarm, int id, bool value)
{
  if (value) {
    alarm->weekday_bitfield |= 1<<id;
  }
  else {
    alarm->weekday_bitfield &= 0<<id;
  }
}

void alarm_toggle_enable(Alarm *alarm)
{
  if(alarm_is_enabled(alarm)==false)
  {
    alarm_set_enabled(alarm,true);
  }
  else
  {
    alarm_set_enabled(alarm,false);
    alarm->alarm_id=-1;
  }
}

time_t alarm_get_time_of_wakeup(Alarm *alarm)
{
  //alarm_cancel_wakeup(alarm);
  if(alarm_is_enabled(alarm))
  {
    // Calculate time to wake up
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    time_t timestamp = now + (60*60*24*7);
    time_t temp_timestamp;
    bool some_active=false;
    
    // Check if we may schedule the alarm today
    int current_weekday = t->tm_wday;
    for(int i=0;i<7;i++)
    {
      if(alarm_weekday_is_active(alarm,(i+current_weekday)%7))
      {
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Day %d is active",(i+current_weekday)%7);
        temp_timestamp = clock_to_timestamp_precise(((i+current_weekday)%7)+1,alarm->hour,alarm->minute);
        if(temp_timestamp>(now + (60*60*24*7))) // more than one week away? This is today!
          temp_timestamp-=(60*60*24*7);
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Diff to now: %d",(int)(temp_timestamp-now));
        if(temp_timestamp<timestamp)
        {
          some_active=true;
          timestamp = temp_timestamp;
        }
      }
    }
    if(!some_active) //maybe we have missed a today event. this takes then place next week
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG,"none active");
      if(alarm_weekday_is_active(alarm,current_weekday))
      {
        some_active=true;
        timestamp = clock_to_timestamp_precise(TODAY,alarm->hour,alarm->minute);
      }
      else // one-time alert (no weekdays active)
      {
        timestamp = clock_to_timestamp_precise(TODAY,alarm->hour,alarm->minute);
        if((timestamp-now)>(60*60*24))
        timestamp = clock_to_timestamp_precise(current_weekday+1,alarm->hour,alarm->minute);
      }
    }
    return timestamp;

  }
  return -1;
}

int get_next_alarm(Alarm *alarms)
{
  // search for next alarm
  time_t timestamp = time(NULL)+(60*60*24*8); // now + 1 week
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Now has timestamp %d",(int)timestamp);
  int alarm_id=-1;
  for (int i=0;i<NUM_ALARMS;i++)
  {
    time_t alarm_time = alarm_get_time_of_wakeup(&alarms[i]);
    if(alarm_time<0)
      continue;
    if(alarm_time<timestamp)
    {
      timestamp = alarm_time;
      alarm_id = i;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Alarm %d has timestamp %d",i,(int)alarm_time);
  }
  return alarm_id;
}




void reschedule_wakeup(Alarm *alarms)
{
  wakeup_cancel_all();
  int alarm_id = get_next_alarm(alarms);
    // schedule the winner
  if(alarm_id>=0)
  {
    time_t timestamp = alarm_get_time_of_wakeup(&alarms[alarm_id]);
    alarms[alarm_id].alarm_id = wakeup_schedule(timestamp,0,true);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Scheduling Alarm %d",alarm_id);
    struct tm *t = localtime(&timestamp);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Scheduled at %d.%d %d:%d",t->tm_mday, t->tm_mon+1,t->tm_hour,t->tm_min);
  }
}

bool alarm_is_one_time(Alarm *alarm)
{
  bool onetime=true;
  
  for (int i=0;i<7;i++)
    if(alarm_weekday_is_active(alarm,i))
      onetime=false;
  
  return onetime;
}

void convert_24_to_12(int hour_in, int* hour_out, bool* am)
{
  *hour_out=hour_in%12;
  if (*hour_out==0) {
    *hour_out=12;
  }
  *am=hour_in<12;
}

bool alarm_has_description(Alarm *alarm)
{
  return alarm->description[0];
}

bool alarm_is_set(Alarm *alarm)
{
  // alarm is set if it doesn't have the default settings
  
  return alarm->hour || alarm->minute || alarm_has_description(alarm) || alarm->weekday_bitfield;
}

int8_t get_next_free_slot(Alarm *alarms)
{
  for (int8_t i=0; i<NUM_ALARMS; i++) {
    if (!alarm_is_set(&alarms[i])) {
      return i;
    }
  }
  return -1;
}

void alarm_reset(Alarm *alarm)
{
  alarm->hour=0;
  alarm->minute=0;
  alarm->alarm_id=-1;
  alarm->description[0]=0;
  alarm->weekday_bitfield=0;

}

bool is_24h()
{
#ifdef PBL_PLATFORM_BASALT
  //return false; //remove
#endif
  return clock_is_24h_style();
}
