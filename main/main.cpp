#include <chrono>
#include <thread>

#include "logger.hpp"
#include "task.hpp"

#include "embedded.hpp"

using namespace std::chrono_literals;

void on_fp_event(nearby_event_Event* event) {
  switch (event->event_type) {
  case kNearbyEventMessageStreamConnected: {
    fmt::print("on_fp_event: kNearbyEventMessageStreamConnected\n");
    // cast event->payload to nearby_event_MessageStreamConnected which has a uint64_t peer_address
    nearby_event_MessageStreamConnected* connected = (nearby_event_MessageStreamConnected*)event->payload;
    fmt::print("peer_address: 0x{:x}\n", connected->peer_address);
  }
    break;
  case kNearbyEventMessageStreamDisconnected: {
    fmt::print("on_fp_event: kNearbyEventMessageStreamDisconnected\n");
    // cast to nearby_event_MessageStreamDisconnected which has a uint64_t peer_address
    nearby_event_MessageStreamDisconnected* disconnected = (nearby_event_MessageStreamDisconnected*)event->payload;
    fmt::print("peer_address: 0x{:x}\n", disconnected->peer_address);
  }
    break;
  case kNearbyEventMessageStreamReceived: {
    fmt::print("on_fp_event: kNearbyEventMessageStreamReceived\n");
    // cast to nearby_event_MessageStreamReceived which has:
    //  - uint64_t peer_address
    //  - uint8_t message_group
    //  - uint8_t message_code
    //  - size_t length
    //  - uint8_t* data
    nearby_event_MessageStreamReceived* received = (nearby_event_MessageStreamReceived*)event->payload;
    fmt::print("peer_address: 0x{:x}\n", received->peer_address);
    fmt::print("message_group: {}\n", received->message_group);
    fmt::print("message_code: {}\n", received->message_code);
    fmt::print("length: {}\n", received->length);
    std::vector<uint8_t> data(received->data, received->data + received->length);
    fmt::print("data: {::#x}\n", data);
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
  const nearby_fp_client_Callbacks callbacks = {
    .on_event = on_fp_event,
  };
  nearby_fp_client_Init(&callbacks);
  nearby_fp_client_SetAdvertisement(NEARBY_FP_ADVERTISEMENT_DISCOVERABLE);

  // loop forever
  while (true) {
    std::this_thread::sleep_for(1s);
  }
}
