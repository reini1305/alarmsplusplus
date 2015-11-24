#include <pebble.h>
#include "storage.h"
#include "wakeup.h"
#include "localize.h"
#include "timeline.h"

static Alarm alarms[NUM_ALARMS];
static bool snooze;

void init(void)
{
  setup_communication();
  load_persistent_storage_alarms(alarms);
  //if(load_persistent_storage_bool(BACKGROUND_TRACKING_KEY, false))
  //  app_worker_launch();
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
