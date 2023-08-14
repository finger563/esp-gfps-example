#include <chrono>
#include <thread>

#include "logger.hpp"
#include "task.hpp"

#include "embedded.hpp"

using namespace std::chrono_literals;

/**
 * @breif Main application entry point
 */
extern "C" void app_main(void) {
  // Initialize logging
  espp::Logger logger({.tag="main", .level=espp::Logger::Verbosity::DEBUG});

  logger.info("Bootup");

  // Initialize NVS.
  auto ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );

  // Calls into google/nearby/embedded to initialize the nearby framework, using
  // the platform specific implementation of the nearby API which is in the
  // embedded component.
  nearby_fp_client_Init(NULL);
  nearby_fp_client_SetAdvertisement(NEARBY_FP_ADVERTISEMENT_DISCOVERABLE);

  // loop forever
  while (true) {
    std::this_thread::sleep_for(1s);
  }
}
