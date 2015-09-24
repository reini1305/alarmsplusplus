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

void alarm_toggle_enable(Alarm *alarm)
{
  if(alarm->enabled==false)
  {
    alarm->enabled=true;
  }
  else
  {
    alarm->enabled=false;
    alarm->alarm_id=-1;
  }
}

time_t alarm_get_time_of_wakeup(Alarm *alarm)
{
  //alarm_cancel_wakeup(alarm);
  if(alarm->enabled)
  {
    // Calculate time to wake up
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    time_t timestamp = clock_to_timestamp_precise(TODAY,alarm->hour,alarm->minute);
    bool some_active=false;
    
    // Check if we may schedule the alarm today
    int current_weekday = t->tm_wday;
    for(int i=0;i<7;i++)
    {
      if(alarm->weekdays_active[(i+current_weekday)%7])
      {
        if(i==0)
          timestamp = clock_to_timestamp_precise(TODAY,alarm->hour,alarm->minute);
        else
          timestamp = clock_to_timestamp_precise(((i+current_weekday)%7)+1,alarm->hour,alarm->minute);
        if((now-timestamp)>(60*60*24*7)) // we can not be more than a week away?
          continue;
        else
        {
          some_active=true;
          break;
        }
      }
    }
    if(!some_active) //maybe we have missed a today event. this takes then place next week
    {
      if(alarm->weekdays_active[current_weekday])
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


void reschedule_wakeup(Alarm *alarms)
{
  wakeup_cancel_all();
  
  // search for next alarm
  time_t timestamp = time(NULL)+(60*60*24*7); // now + 1 week
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
  // schedule the winner
  if(alarm_id>=0)
  {
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
    if(alarm->weekdays_active[i])
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
  bool retval=alarm->enabled || alarm->hour || alarm->minute || alarm_has_description(alarm);
  for (int i=0; i<7; i++) {
    retval = retval || !alarm->weekdays_active[i];
  }
  return retval;
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
  alarm->enabled=false;
  alarm->alarm_id=-1;
  alarm->description[0]=0;
  for (int weekday=0; weekday<7;weekday++)
    alarm->weekdays_active[weekday]=true;

}
