#include <pebble_worker.h>


static AppTimer *timeout_timer=NULL;
static time_t last_time;

void reschedule_app_timer_callback(void* data) {
  timeout_timer = app_timer_register(3600000,reschedule_app_timer_callback,NULL);
  time_t current_time = time(NULL);
  time_t diff = current_time-last_time;
  if (abs(diff-3600)>10) {  // time has changed more than 10 seconds in the last hour
    worker_launch_app();
  }
  last_time = current_time;
}

static void init() {
  // Initialize your worker here
  timeout_timer = app_timer_register(3600000,reschedule_app_timer_callback,NULL);
  last_time = time(NULL);
}

static void deinit() {
  // Deinitialize your worker here

}

int main(void) {
  init();
  worker_event_loop();
  deinit();
}
