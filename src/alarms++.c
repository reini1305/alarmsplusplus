#include <pebble.h>
#include "storage.h"
#include "wakeup.h"
#include "localize.h"
#include "timeline.h"
#include "appglance.h"

static Alarm alarms[NUM_ALARMS];
static bool snooze;

void init(void)
{
  setup_communication();
  load_persistent_storage_alarms(alarms);
  perform_wakeup_tasks(alarms,&snooze);
}



void deinit(void)
{
  destroy_communication();
  if(!snooze)
    reschedule_wakeup(alarms);
  write_persistent_storage_alarms(alarms);
  update_app_glance(alarms,true);
}

int main(void) {
  init();
  locale_init();
  app_event_loop();
  deinit();
}
