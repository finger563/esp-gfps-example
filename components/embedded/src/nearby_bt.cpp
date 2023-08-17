#include "embedded.hpp"

#if CONFIG_BT_CLASSIC_ENABLED

static espp::Logger logger({.tag = "GFPS BR/EDR", .level = espp::Logger::Verbosity::DEBUG});

// Contains pointers to callback functions:
// - on_pairing_request(uint64_t peer_address)
// - on_paired(uint64_t peer_address)
// - on_pairing_failed(uint64_t peer_address)
// #if NEARBY_FP_MESSAGE_STREAM
// - on_message_stream_connected(uint64_t peer_address)
// - on_message_stream_disconnected(uint64_t peer_address)
// - on_message_stream_received(uint64_t peer_address, const uint8_t* data, size_t size)
// #endif
static const nearby_platform_BtInterface *g_bt_interface = nullptr;

static SemaphoreHandle_t bt_hidh_cb_semaphore = NULL;
#define WAIT_BT_CB() xSemaphoreTake(bt_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BT_CB() xSemaphoreGive(bt_hidh_cb_semaphore)

#define SIZEOF_ARRAY(a) (sizeof(a) / sizeof(*a))

static const char *bt_gap_evt_names[] = {"DISC_RES",
                                         "DISC_STATE_CHANGED",
                                         "RMT_SRVCS",
                                         "RMT_SRVC_REC",
                                         "AUTH_CMPL",
                                         "PIN_REQ",
                                         "CFM_REQ",
                                         "KEY_NOTIF",
                                         "KEY_REQ",
                                         "READ_RSSI_DELTA"};

const char *bt_gap_evt_str(uint8_t event) {
    if (event >= SIZEOF_ARRAY(bt_gap_evt_names)) {
        return "UNKNOWN";
    }
    return bt_gap_evt_names[event];
}

static void bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        logger.debug("BT GAP DISC_STATE {}",
                 (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) ? "START" : "STOP");
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            SEND_BT_CB();
        }
        break;
    }
    case ESP_BT_GAP_DISC_RES_EVT: {
        break;
    }
    case ESP_BT_GAP_READ_REMOTE_NAME_EVT: {
        logger.info("BT GAP READ_REMOTE_NAME status:{}", (int)param->read_rmt_name.stat);
        logger.info("BT GAP READ_REMOTE_NAME name:{}", param->read_rmt_name.rmt_name);
        break;
    }
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        logger.info("BT GAP KEY_NOTIF passkey:{}", (int)param->key_notif.passkey);
        break;
    case ESP_BT_GAP_MODE_CHG_EVT:
        logger.info("BT GAP MODE_CHG_EVT mode:{}", (int)param->mode_chg.mode);
        esp_bt_gap_read_remote_name(param->mode_chg.bda);
        break;
    default:
        logger.debug("BT GAP EVENT {}", bt_gap_evt_str(event));
        break;
    }
}

/////////////////BLUETOOTH///////////////////////

// Returns Fast Pair Model Id.
uint32_t nearby_platform_GetModelId() {
  return CONFIG_MODEL_ID;
}

// Returns tx power level.
int8_t nearby_platform_GetTxLevel() {
  esp_power_level_t min_tx_power;
  esp_power_level_t max_tx_power;
  auto err = esp_bredr_tx_power_get(&min_tx_power, &max_tx_power);
  return err == ESP_OK ? (int)max_tx_power : 0;
}

// Returns public BR/EDR address.
// On a BLE-only device, return the public identity address.
uint64_t nearby_platform_GetPublicAddress() {
  const uint8_t* addr = esp_bt_dev_get_address();
  return (uint64_t)addr[0] << 40 | (uint64_t)addr[1] << 32 |
         (uint64_t)addr[2] << 24 | (uint64_t)addr[3] << 16 |
         (uint64_t)addr[4] << 8 | (uint64_t)addr[5];
}

// Returns the secondary identity address.
// Some devices, such as ear-buds, can advertise two identity addresses. In this
// case, the Seeker pairs with each address separately but treats them as a
// single logical device set.
// Return 0 if this device does not have a secondary identity address.
uint64_t nearby_platform_GetSecondaryPublicAddress() {
  logger.warn("GetSecondaryPublicAddress not implemented");
  return 0;
}

// Returns passkey used during pairing
uint32_t nearby_platfrom_GetPairingPassKey() {
  logger.warn("GetPairingPassKey not implemented");
  return 0;
}

// Provides the passkey received from the remote party.
// The system should compare local and remote party and accept/decline pairing
// request accordingly.
//
// passkey - Passkey
void nearby_platform_SetRemotePasskey(uint32_t passkey) {
  logger.warn("SetRemotePasskey not implemented");
  // TODO: implement
}

// Sends a pairing request to the Seeker
//
// remote_party_br_edr_address - BT address of peer.
nearby_platform_status nearby_platform_SendPairingRequest(
    uint64_t remote_party_br_edr_address) {
  logger.warn("SendPairingRequest not implemented");
  // TODO: implement
  return kNearbyStatusOK;
}

// Switches the device capabilities field back to default so that new
// pairings continue as expected.
nearby_platform_status nearby_platform_SetDefaultCapabilities() {
  logger.warn("SetDefaultCapabilities not implemented");
  // TODO: implement
  return kNearbyStatusOK;
}

// Switches the device capabilities field to Fast Pair required configuration:
// DisplayYes/No so that `confirm passkey` pairing method will be used.
nearby_platform_status nearby_platform_SetFastPairCapabilities() {
  logger.warn("SetFastPairCapabilities not implemented");
  // TODO: implement
  return kNearbyStatusOK;
}

// Sets null-terminated device name string in UTF-8 encoding
//
// name - Zero terminated string name of device.
nearby_platform_status nearby_platform_SetDeviceName(const char* name) {
  esp_bt_dev_set_device_name(name);
  return kNearbyStatusOK;
}

// Gets null-terminated device name string in UTF-8 encoding
// pass buffer size in char, and get string length in char.
//
// name   - Buffer to return name string.
// length - On input, the size of the name buffer.
//          On output, returns size of name in buffer.
nearby_platform_status nearby_platform_GetDeviceName(char* name,
                                                     size_t* length) {
  logger.warn("GetDeviceName not implemented");
  // TODO: implement
  return kNearbyStatusOK;
}

// Returns true if the device is in pairing mode (either fast-pair or manual).
bool nearby_platform_IsInPairingMode() {
  // TODO: implement
  return true;
}


#if NEARBY_FP_MESSAGE_STREAM
// Sends message stream through RFCOMM or L2CAP channel initiated by Seeker.
// BT devices should use RFCOMM channel. BLE-only devices should use L2CAP.
//
// peer_address - Peer address.
// message      - Contents of message.
// length       - Length of message
nearby_platform_status nearby_platform_SendMessageStream(uint64_t peer_address,
                                                         const uint8_t* message,
                                                         size_t length) {
  logger.warn("SendMessageStream not implemented");
  // TODO: implement
  return kNearbyStatusOK;
}

#endif /* NEARBY_FP_MESSAGE_STREAM */

// Initializes BT module
//
// bt_interface - BT callbacks event structure.
nearby_platform_status nearby_platform_BtInit(
    const nearby_platform_BtInterface* bt_interface) {

  g_bt_interface = bt_interface;

  logger.info("Initializing Bluetooth");

  esp_err_t ret;
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
  /*
   * Set default parameters for Legacy Pairing
   * Use fixed pin code
   */
  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
  esp_bt_pin_code_t pin_code;
  pin_code[0] = '1';
  pin_code[1] = '2';
  pin_code[2] = '3';
  pin_code[3] = '4';
  esp_bt_gap_set_pin(pin_type, 4, pin_code);

  if ((ret = esp_bt_gap_register_callback(bt_gap_event_handler)) != ESP_OK) {
    logger.error("esp_bt_gap_register_callback failed: {}", ret);
    return kNearbyStatusError;
  }

  // Allow BT devices to connect back to us
  if ((ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE)) != ESP_OK) {
    logger.error("esp_bt_gap_set_scan_mode failed: {}", ret);
    return kNearbyStatusError;
  }

  return kNearbyStatusOK;
}
#endif /* CONFIG_BT_CLASSIC_ENABLED */
