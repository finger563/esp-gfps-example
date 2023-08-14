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

  logger.info("Device name: '{}'", CONFIG_DEVICE_NAME);
  logger.info("Model ID: 0x{:x}", CONFIG_MODEL_ID);

  // Initialize the bluetooth subsystem
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  bt_cfg.mode = ESP_BT_MODE_BTDM;
  bt_cfg.bt_max_acl_conn = 3;
  bt_cfg.bt_max_sync_conn = 3;

  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    logger.error("{} enable controller failed", __func__);
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
  if (ret) {
    logger.error("{} enable controller failed", __func__);
    return;
  }

  logger.info("{} init bluetooth", __func__);
  ret = esp_bluedroid_init();
  if (ret) {
    logger.error("{} init bluetooth failed", __func__);
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    logger.error("{} enable bluetooth failed", __func__);
    return;
  }

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
