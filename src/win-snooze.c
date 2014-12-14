//
//  win-snooze.c
//  alarms++
//
//  Created by Christian Reinbacher on 02.11.14.
//
//

#include "win-snooze.h"
#include "storage.h"
#include <pebble.h>
#include "localize.h"

static NumberWindow *s_window;
static int *s_snooze_delay;

void store_snooze(NumberWindow *window,void* context)
{
  *s_snooze_delay = number_window_get_value(s_window);
  persist_write_int(SNOOZE_KEY,*s_snooze_delay);
  window_stack_pop(true);
}
void win_snooze_init(void)
{
  s_window = number_window_create(_("Snooze Delay"),(NumberWindowCallbacks){.selected=store_snooze},NULL);
  number_window_set_min(s_window,1);
  number_window_set_max(s_window,60);
  number_window_set_step_size(s_window,1);
}
void win_snooze_show(int *snooze_delay)
{
  s_snooze_delay = snooze_delay;
  *s_snooze_delay = load_persistent_storage_int(SNOOZE_KEY, 10);
  
  number_window_set_value(s_window,*s_snooze_delay);
  window_stack_push(number_window_get_window(s_window),true);
}

