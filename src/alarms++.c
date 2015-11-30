#include <pebble.h>
#include "storage.h"
#include "wakeup.h"
#include "localize.h"
#ifdef PBL_SDK_3
#include "timeline.h"
#endif

static Alarm alarms[NUM_ALARMS];
static bool snooze;

void init(void)
{
#ifdef PBL_SDK_3
  setup_communication();
#endif
  load_persistent_storage_alarms(alarms);
  //if(load_persistent_storage_bool(BACKGROUND_TRACKING_KEY, false))
  //  app_worker_launch();
  perform_wakeup_tasks(alarms,&snooze);
}

void deinit(void)
{
#ifdef PBL_SDK_3
  destroy_communication();
#endif
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
