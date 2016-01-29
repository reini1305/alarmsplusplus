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

#define SNOOZE_KEY 2
#define LONGPRESS_DISMISS_KEY 3
#define HIDE_UNUSED_ALARMS_KEY 5
#define VIBRATION_PATTERN_KEY 6
#define FLIP_TO_SNOOZE_KEY 7
#define VIBRATION_DURATION_KEY 9
#define AUTO_SNOOZE_KEY 10
#define BACKGROUND_TRACKING_KEY 11
#define ALARMS_KEY 12
#define TOP_BUTTON_DISMISS_KEY 20

void load_persistent_storage_alarms(Alarm *alarms);
bool load_persistent_storage_bool(int key, bool default_val);
int load_persistent_storage_int(int key, int default_val);

void write_persistent_storage_alarms(Alarm *alarms);

#endif
