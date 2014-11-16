//
//  storage.h
//  alarms++
//
//  Created by Christian Reinbacher on 01.11.14.
//
//

#ifndef alarms___storage_h
#define alarms___storage_h

#include "alarms.h"

#define ALARMS_OLD_KEY 1
#define SNOOZE_KEY 2
#define LONGPRESS_DISMISS_KEY 3
#define ALARMS_KEY 4
#define HIDE_UNUSED_ALARMS_KEY 5
#define VIBRATION_PATTERN_KEY 6

void load_persistent_storage_alarms(Alarm *alarms);
void load_persistent_storage_snooze_delay(int *snooze_delay);
void load_persistent_storage_longpress_dismiss(bool *longpress_dismiss);
void load_persistent_storage_hide_unused_alarms(bool *hide_unused_alarms);
void load_persistent_storage_vibration_pattern(int *vibration_pattern);

void write_persistent_storage_alarms(Alarm *alarms);
//void write_persistent_storage_snooze_delay(int snooze_delay);

#endif
