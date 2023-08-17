#include <chrono>
#include <thread>

#include "logger.hpp"
#include "task.hpp"

#include "embedded.hpp"

static espp::Logger logger({.tag="main", .level=espp::Logger::Verbosity::DEBUG});

using namespace std::chrono_literals;

void on_fp_event(nearby_event_Event* event) {
  switch (event->event_type) {
  case kNearbyEventMessageStreamConnected: {
    logger.info("on_fp_event: kNearbyEventMessageStreamConnected");
    // cast event->payload to nearby_event_MessageStreamConnected which has a uint64_t peer_address
    nearby_event_MessageStreamConnected* connected = (nearby_event_MessageStreamConnected*)event->payload;
    logger.info("peer_address: 0x{:x}", connected->peer_address);
  }
    break;
  case kNearbyEventMessageStreamDisconnected: {
    logger.info("on_fp_event: kNearbyEventMessageStreamDisconnected");
    // cast to nearby_event_MessageStreamDisconnected which has a uint64_t peer_address
    nearby_event_MessageStreamDisconnected* disconnected = (nearby_event_MessageStreamDisconnected*)event->payload;
    logger.info("peer_address: 0x{:x}", disconnected->peer_address);
  }
    break;
  case kNearbyEventMessageStreamReceived: {
    logger.info("on_fp_event: kNearbyEventMessageStreamReceived");
    // cast to nearby_event_MessageStreamReceived which has:
    //  - uint64_t peer_address
    //  - uint8_t message_group
    //  - uint8_t message_code
    //  - size_t length
    //  - uint8_t* data
    nearby_event_MessageStreamReceived* received = (nearby_event_MessageStreamReceived*)event->payload;
    logger.debug("peer_address: 0x{:x}", received->peer_address);
    logger.debug("message_group: {}", received->message_group);
    logger.debug("message_code: {}", received->message_code);
    logger.debug("length: {}", received->length);
    std::vector<uint8_t> data(received->data, received->data + received->length);
    logger.debug("data: {::#x}", data);
  }
    break;
  default:
    break;
  }
}

/**
 * @breif Main application entry point
 */
extern "C" void app_main(void) {
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

  #if CONFIG_BT_CLASSIC_ENABLED
  esp_bt_mode_t mode = ESP_BT_MODE_BTDM;
  logger.info("BT mode");
  #else
  esp_bt_mode_t mode = ESP_BT_MODE_BLE;
  logger.info("BLE mode");
  #endif

  // Initialize the bluetooth subsystem
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  bt_cfg.mode = mode;
#if CONFIG_BT_CLASSIC_ENABLED
  if (mode & ESP_BT_MODE_CLASSIC_BT) {
    bt_cfg.bt_max_acl_conn = 3;
    bt_cfg.bt_max_sync_conn = 3;
  } else
#endif
    {
      ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
      if (ret) {
        logger.error("esp_bt_controller_mem_release failed: {}", ret);
        return;
      }
    }

  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    logger.error("enable controller failed");
    return;
  }

  ret = esp_bt_controller_enable(mode);
  if (ret) {
    logger.error("enable controller failed");
    return;
  }

  logger.info("init bluetooth");
  ret = esp_bluedroid_init();
  if (ret) {
    logger.error("init bluetooth failed");
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    logger.error("enable bluetooth failed");
    return;
  }

  // Calls into google/nearby/embedded to initialize the nearby framework, using
  // the platform specific implementation of the nearby API which is in the
  // embedded component.
  const nearby_fp_client_Callbacks callbacks = {
    .on_event = on_fp_event,
  };
  nearby_fp_client_Init(&callbacks);
  int advertisement_mode =
    NEARBY_FP_ADVERTISEMENT_DISCOVERABLE |
    NEARBY_FP_ADVERTISEMENT_PAIRING_UI_INDICATOR;
  nearby_fp_client_SetAdvertisement(advertisement_mode);

  // loop forever
  while (true) {
    std::this_thread::sleep_for(1s);
  }
}
