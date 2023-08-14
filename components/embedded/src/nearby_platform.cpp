#include "embedded.hpp"

static std::chrono::system_clock::time_point s_start_time;

/////////////////PLATFORM///////////////////////

// Gets current time in ms.
unsigned int nearby_platform_GetCurrentTimeMs() {
  auto now = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_start_time);
  return elapsed.count();
}

// Starts a timer. Returns an opaque timer handle or null on error.
//
// callback - Function to call when timer matures.
// delay_ms - Number of milliseconds to run the timer.
void* nearby_platform_StartTimer(void (*callback)(), unsigned int delay_ms) {
  espp::Timer* timer = new espp::Timer(espp::Timer::Config{
      .name = "nearby timer",
      .period = std::chrono::milliseconds(0), // one-shot
      .delay = std::chrono::milliseconds(delay_ms),
      .callback = [callback]() {
        if (callback) callback();
        return true;
      },
    });
  return timer;
}

// Cancels a timer
//
// timer - Timer handle returned by StartTimer.
nearby_platform_status nearby_platform_CancelTimer(void* timer) {
  espp::Timer* t = (espp::Timer*)timer;
  if (!t) return kNearbyStatusError;
  fmt::print("Cancelling timer\n");
  t->cancel();
  delete t;
  return kNearbyStatusOK;
}

// Initializes OS module
nearby_platform_status nearby_platform_OsInit() {
  s_start_time = std::chrono::system_clock::now();
  return kNearbyStatusOK;
}

// Starts ringing
//
// `command` - the requested ringing state as a bitmap:
// Bit 1 (0x01): ring right
// Bit 2 (0x02): ring left
// Bit 3 (0x04): ring case
// Alternatively, `command` hold a special value of 0xFF to ring all
// components that can ring.
// `timeout` - the timeout in deciseconds. The timeout overrides the one already
// in effect if any component of the device is already ringing.
nearby_platform_status nearby_platform_Ring(uint8_t command, uint16_t timeout) {
  // TODO: implement
  return kNearbyStatusOK;
}
