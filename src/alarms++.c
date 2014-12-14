#include <pebble.h>
#include "storage.h"
#include "wakeup.h"
#include "localize.h"

static Alarm alarms[NUM_ALARMS];
static bool snooze;

void init(void)
{
  load_persistent_storage_alarms(alarms);
  perform_wakeup_tasks(alarms,&snooze);
}

void deinit(void)
{
  if(!snooze)
    reschedule_wakeup(alarms);
  write_persistent_storage_alarms(alarms);
}

int main(void) {
  init();

 // APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  locale_init();
  app_event_loop();
  deinit();
}
