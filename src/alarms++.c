#include <pebble.h>
#include "storage.h"
#include "wakeup.h"
#include "localize.h"

static Alarm alarms[NUM_ALARMS];
static bool snooze;

void init(void)
{
#ifdef PBL_SDK_3
  app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, 2*APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
#endif
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
