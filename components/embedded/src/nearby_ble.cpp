#include "embedded.hpp"

static espp::Logger logger({.tag = "GFPS BLE", .level = espp::Logger::Verbosity::DEBUG});

/* Attributes State Machine */
enum
  {
    IDX_SVC,
    IDX_CHAR_MODEL_ID,
    IDX_CHAR_VAL_MODEL_ID,

    IDX_CHAR_KB_PAIRING,
    IDX_CHAR_VAL_KB_PAIRING,
    IDX_CHAR_CFG_KB_PAIRING,

    IDX_CHAR_PASSKEY,
    IDX_CHAR_VAL_PASSKEY,
    IDX_CHAR_CFG_PASSKEY,

    IDX_CHAR_ACCOUNT_KEY,
    IDX_CHAR_VAL_ACCOUNT_KEY,

    IDX_CHAR_FW_REVISION,
    IDX_CHAR_VAL_FW_REVISION,

    GFPS_IDX_NB,
  };

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SVC_INST_ID                 0

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
 *  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
 */
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done       = 0;

static uint16_t gfps_handle_table[GFPS_IDX_NB];

typedef struct {
  uint8_t                 *prepare_buf;
  int                     prepare_len;
} prepare_type_env_t;

static SemaphoreHandle_t ble_hidh_cb_semaphore = NULL;
#define WAIT_BLE_CB() xSemaphoreTake(ble_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BLE_CB() xSemaphoreGive(ble_hidh_cb_semaphore)

#define SIZEOF_ARRAY(a) (sizeof(a) / sizeof(*a))

static const nearby_platform_BleInterface *g_ble_interface = nullptr;

esp_bd_addr_t ble_peer_address;

esp_bd_addr_t *get_ble_peer_address(void) { return &ble_peer_address; }

static prepare_type_env_t prepare_write_env;

static uint8_t raw_scan_rsp_data[] = {
  /* flags */
  0x02, 0x01, 0x06,
  /* tx power */
  0x02, 0x0a, 0x00,
  /* service uuid */
  0x03, 0x03, 0x2C, 0xFE,
};
// config scan response data
static esp_ble_adv_data_t scan_rsp_config = {
  .set_scan_rsp = true,
  .include_name = true,
  .appearance = 0x00, // TODO: fix
};

static std::vector<uint8_t> raw_adv_data;

static esp_ble_adv_params_t adv_params = {
  .adv_int_min         = 0x20,
  .adv_int_max         = 0x40,
  .adv_type            = ADV_TYPE_IND,
  .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
  .channel_map         = ADV_CHNL_ALL,
  .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gfps_profile_tab[PROFILE_NUM] = {
  [PROFILE_APP_IDX] = {
    .gatts_cb = gatts_profile_event_handler,
    .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
  },
};

/* Service */
// NOTE: these UUIDs are specified at 16-bit, which means that 1) they are not
// in reverse order, and 2) the rest of their full 128-bit UUIDs are the standard
// Bluetooth SIG base UUID (0x0000XXXX-0000-1000-8000-00805F9B34FB), and the 16-bit
// UUIDs are the last 2 bytes of the first 4 bytes of the full UUIDs (XXXX).
static const uint16_t GATTS_SERVICE_UUID_GFPS            = 0xFE2C;
static const uint16_t GATTS_CHAR_UUID_GFPS_FW_REVISION   = 0x2A26;

// NOTE: the UUIDs are in reverse order
static const uint8_t GATTS_CHAR_UUID_GFPS_MODEL_ID[16]      = {0xEA, 0x0B, 0x10, 0x32,
                                                               0xDE, 0x01, 0xB0, 0x8E,
                                                               0x14, 0x48, 0x66, 0x83,
                                                               0x33, 0x12, 0x2C, 0xFE};
static const uint8_t GATTS_CHAR_UUID_GFPS_KB_PAIRING[16]    = {0xEA, 0x0B, 0x10, 0x32,
                                                               0xDE, 0x01, 0xB0, 0x8E,
                                                               0x14, 0x48, 0x66, 0x83,
                                                               0x34, 0x12, 0x2C, 0xFE};
static const uint8_t GATTS_CHAR_UUID_GFPS_PASSKEY[16]       = {0xEA, 0x0B, 0x10, 0x32,
                                                               0xDE, 0x01, 0xB0, 0x8E,
                                                               0x14, 0x48, 0x66, 0x83,
                                                               0x35, 0x12, 0x2C, 0xFE};
static const uint8_t GATTS_CHAR_UUID_GFPS_ACCOUNT_KEY[16]   = {0xEA, 0x0B, 0x10, 0x32,
                                                               0xDE, 0x01, 0xB0, 0x8E,
                                                               0x14, 0x48, 0x66, 0x83,
                                                               0x36, 0x12, 0x2C, 0xFE};

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

// TODO: update
static const uint8_t ccc[2]           = {0x03, 0x00}; // LSb corresponds to notifications (1 if enabled, 0 if disabled), next bit (bit 1) corresponds to indications - 1 if enabled, 0 if disabled
static const uint32_t model_id        = CONFIG_MODEL_ID;
static const uint8_t fw_revision[4]   = {'1', '.', '0', 0x00};
static const uint8_t char_value[16]    = {0x00};

struct KbPairing {
  uint8_t encrypted_request[16]; //!< 16 byte encrypted request
  uint8_t public_key[64];        //!< 64 byte public key (optional)
} __attribute__((packed));

struct EncryptedRequest {
  uint8_t message_type; //!< One of (0x00 = key based pairing reqeust)
  uint8_t flags;        //!< Bit 0 = deprecated,
                        //   Bit 1 = true if seeker reqeusts provider initiate bonding, and this request contains seekers BR/EDR address
                        //   Bit 2 = true if seeker requests provider notify the existing name
                        //   Bit 3 = true if this is for retroactively writing account key
                        //   Bits 4-7 = reserved
  uint8_t provider_address[6]; //!< Either providers current BLE address or providers public address
  uint8_t seeker_address[6];   //!< Seekers BR/EDR address
  uint8_t remaining_salt_bytes[2]; //!< Remaining bytes of salt
} __attribute__((packed));

// this structure is encrypted and used with the CHAR_UUID_GFPS_PASSKEY
struct Passkey {
  uint8_t message_type; //!< One of (0x02 = seeker's passkey, 0x03 = provider's passkey)
  uint32_t passkey;     //!< 6 digit passkey
  uint8_t random_value[12]; //!< 12 byte random value (salt)
} __attribute__((packed));

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[GFPS_IDX_NB] =
  {
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
                           sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_GFPS), (uint8_t *)&GATTS_SERVICE_UUID_GFPS}},

    /* Characteristic Declaration */
    [IDX_CHAR_MODEL_ID]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_MODEL_ID] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)GATTS_CHAR_UUID_GFPS_MODEL_ID, ESP_GATT_PERM_READ,
                           GATTS_DEMO_CHAR_VAL_LEN_MAX, 3, (uint8_t *)&model_id}},

    /* Characteristic Declaration */
    [IDX_CHAR_KB_PAIRING]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    // TODO: UPDATE
    /* Characteristic Value */
    [IDX_CHAR_VAL_KB_PAIRING]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)GATTS_CHAR_UUID_GFPS_KB_PAIRING, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

    // TODO: UPDATE
    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_KB_PAIRING]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           sizeof(uint16_t), sizeof(ccc), (uint8_t *)ccc}},

    /* Characteristic Declaration */
    [IDX_CHAR_PASSKEY]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    // TODO: UPDATE
    /* Characteristic Value */
    [IDX_CHAR_VAL_PASSKEY]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)GATTS_CHAR_UUID_GFPS_PASSKEY, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

    // TODO: UPDATE
    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_PASSKEY]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           sizeof(uint16_t), sizeof(ccc), (uint8_t *)ccc}},

    /* Characteristic Declaration */
    [IDX_CHAR_ACCOUNT_KEY]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_WRITE,
                           CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_ACCOUNT_KEY]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)GATTS_CHAR_UUID_GFPS_ACCOUNT_KEY, ESP_GATT_PERM_WRITE,
                           GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic Declaration */
    [IDX_CHAR_FW_REVISION]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_FW_REVISION]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_GFPS_FW_REVISION, ESP_GATT_PERM_READ,
                           GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(fw_revision), (uint8_t *)fw_revision}},


  };

static const char *ble_gap_evt_names[] = {
  "ADV_DATA_SET_COMPLETE",
  "SCAN_RSP_DATA_SET_COMPLETE",
  "SCAN_PARAM_SET_COMPLETE",
  "SCAN_RESULT",
  "ADV_DATA_RAW_SET_COMPLETE",
  "SCAN_RSP_DATA_RAW_SET_COMPLETE",
  "ADV_START_COMPLETE",
  "SCAN_START_COMPLETE",
  "AUTH_CMPL",
  "KEY",
  "SEC_REQ",
  "PASSKEY_NOTIF",
  "PASSKEY_REQ",
  "OOB_REQ",
  "LOCAL_IR",
  "LOCAL_ER",
  "NC_REQ",
  "ADV_STOP_COMPLETE",
  "SCAN_STOP_COMPLETE",
  "SET_STATIC_RAND_ADDR",
  "UPDATE_CONN_PARAMS",
  "SET_PKT_LENGTH_COMPLETE",
  "SET_LOCAL_PRIVACY_COMPLETE",
  "REMOVE_BOND_DEV_COMPLETE",
  "CLEAR_BOND_DEV_COMPLETE",
  "GET_BOND_DEV_COMPLETE",
  "READ_RSSI_COMPLETE",
  "UPDATE_WHITELIST_COMPLETE"
};

static const char *ble_gatts_evt_names[] = {
  "ESP_GATTS_REG_EVT",
  "ESP_GATTS_READ_EVT",
  "ESP_GATTS_WRITE_EVT",
  "ESP_GATTS_EXEC_WRITE_EVT",
  "ESP_GATTS_MTU_EVT",
  "ESP_GATTS_CONF_EVT",
  "ESP_GATTS_UNREG_EVT",
  "ESP_GATTS_CREATE_EVT",
  "ESP_GATTS_ADD_INCL_SRVC_EVT",
  "ESP_GATTS_ADD_CHAR_EVT",
  "ESP_GATTS_ADD_CHAR_DESCR_EVT",
  "ESP_GATTS_DELETE_EVT",
  "ESP_GATTS_START_EVT",
  "ESP_GATTS_STOP_EVT",
  "ESP_GATTS_CONNECT_EVT",
  "ESP_GATTS_DISCONNECT_EVT",
  "ESP_GATTS_OPEN_EVT",
  "ESP_GATTS_CANCEL_OPEN_EVT",
  "ESP_GATTS_CLOSE_EVT",
  "ESP_GATTS_LISTEN_EVT",
  "ESP_GATTS_CONGEST_EVT",
  "ESP_GATTS_RESPONSE_EVT",
  "ESP_GATTS_CREAT_ATTR_TAB_EVT",
  "ESP_GATTS_SET_ATTR_VAL_EVT",
  "ESP_GATTS_SEND_SERVICE_CHANGE_EVT"
};

const char *ble_gap_evt_str(uint8_t event) {
    if (event >= SIZEOF_ARRAY(ble_gap_evt_names)) {
        return "UNKNOWN";
    }
    return ble_gap_evt_names[event];
}

const char *ble_gatts_evt_str(uint8_t event) {
    if (event >= SIZEOF_ARRAY(ble_gatts_evt_names)) {
        return "UNKNOWN";
    }
    return ble_gatts_evt_names[event];
}

const char *esp_ble_key_type_str(esp_ble_key_type_t key_type) {
    const char *key_str = NULL;
    switch (key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;
    }
    return key_str;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  switch (event) {
    /*
     * SCAN
     * */
  case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
    logger.debug("BLE GAP EVENT SCAN_PARAM_SET_COMPLETE");
    SEND_BLE_CB();
    break;
  }
  case ESP_GAP_BLE_SCAN_RESULT_EVT: {
    logger.info("BLE GAP EVENT SCAN_RESULT");
    break;
  }
  case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
    logger.debug("BLE GAP EVENT SCAN CANCELED");
    break;
  }

    /*
     * ADVERTISEMENT
     * */
  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    adv_config_done &= (~ADV_CONFIG_FLAG);
    if (adv_config_done == 0){
      esp_ble_gap_start_advertising(&adv_params);
    }
    break;
  case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
    adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
    if (adv_config_done == 0){
      // esp_ble_gap_start_advertising(&adv_params);
    }
    break;
  case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
    if (adv_config_done == 0){
      // esp_ble_gap_start_advertising(&adv_params);
    }
    break;
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    /* advertising start complete event to indicate advertising start successfully or failed */
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      logger.error("advertising start failed");
    }else{
      logger.info("advertising start successfully");
    }
    break;
  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      logger.error("Advertising stop failed");
    }
    else {
      logger.info("Stop adv successfully");
    }
    break;
  case ESP_GAP_BLE_ADV_TERMINATED_EVT:
    logger.info("BLE GAP ADV_TERMINATED");
    break;

    /*
     * CONNECTION
     * */
  case ESP_GAP_BLE_GET_DEV_NAME_COMPLETE_EVT:
    logger.debug("BLE GAP GET_DEV_NAME_COMPLETE");
    // print the name
    logger.info("BLE GAP DEVICE NAME: {}", param->get_dev_name_cmpl.name);
    break;

    /*
     * BOND
     * */
  case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
    logger.debug("BLE GAP REMOVE_BOND_DEV_COMPLETE");
    // log the bond that was removed
    // esp_log_buffer_hex(TAG, param->remove_bond_dev_cmpl.bd_addr, ESP_BD_ADDR_LEN);
    break;

  case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT:
    logger.debug("BLE GAP CLEAR_BOND_DEV_COMPLETE");
    break;

  case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT:
    logger.debug("BLE GAP GET_BOND_DEV_COMPLETE");
    break;

    /*
     * AUTHENTICATION
     * */
  case ESP_GAP_BLE_AUTH_CMPL_EVT:
    if (!param->ble_security.auth_cmpl.success) {
      logger.error("BLE GAP AUTH ERROR: {:#x}", param->ble_security.auth_cmpl.fail_reason);
    } else {
      logger.info("BLE GAP AUTH SUCCESS");
      // log the address of the peer device
      // esp_log_buffer_hex(TAG, param->ble_security.auth_cmpl.bd_addr, ESP_BD_ADDR_LEN);
      // save the address of the peer device
      memcpy(ble_peer_address, param->ble_security.auth_cmpl.bd_addr, ESP_BD_ADDR_LEN);
    }
    break;

  case ESP_GAP_BLE_KEY_EVT: // shows the ble key info share with peer device to the user.
    logger.info("BLE GAP KEY type = {}",
                esp_ble_key_type_str(param->ble_security.ble_key.key_type));
    break;

  case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: // ESP_IO_CAP_OUT
    // The app will receive this evt when the IO has Output capability and the peer device IO
    // has Input capability. Show the passkey number to the user to input it in the peer device.
    logger.info("BLE GAP PASSKEY_NOTIF passkey: {}", (int)param->ble_security.key_notif.passkey);
    break;

  case ESP_GAP_BLE_NC_REQ_EVT: // ESP_IO_CAP_IO
    // The app will receive this event when the IO has DisplayYesNO capability and the peer
    // device IO also has DisplayYesNo capability. show the passkey number to the user to
    // confirm it with the number displayed by peer device.
    logger.info("BLE GAP NC_REQ passkey: {}", (int)param->ble_security.key_notif.passkey);
    esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
    break;

  case ESP_GAP_BLE_PASSKEY_REQ_EVT: // ESP_IO_CAP_IN
    // The app will receive this evt when the IO has Input capability and the peer device IO has
    // Output capability. See the passkey number on the peer device and send it back.
    logger.info("BLE GAP PASSKEY_REQ");
    // esp_ble_passkey_reply(param->ble_security.ble_req.bd_addr, true, 1234);
    break;

  case ESP_GAP_BLE_OOB_REQ_EVT:
    // OOB request event
    logger.info("BLE GAP OOB_REQ");
    // esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, TK, sizeof(TK));
    break;

  case ESP_GAP_BLE_SC_OOB_REQ_EVT:
    // secure connection oob request event
    logger.info("BLE GAP SC_OOB_REQ");
    // esp_ble_sc_oob_req_reply(param->ble_security.ble_req.bd_addr, ble_oob_sec_data.oob_c, ble_oob_sec_data.oob_r);
    break;

  case ESP_GAP_BLE_SC_CR_LOC_OOB_EVT:
    // secure connection create oob data complete event
    logger.info("BLE GAP SC_CR_LOC_OOB");
    // retrieve and store the local oob data
    // memcpy(ble_oob_sec_data.oob_c, param->ble_security.oob_data.oob_c, ESP_BT_OCTET16_LEN);
    // memcpy(ble_oob_sec_data.oob_r, param->ble_security.oob_data.oob_r, ESP_BT_OCTET16_LEN);
    // save that we've received the local oob data
    // created_ble_oob_data = true;
    break;

  case ESP_GAP_BLE_SEC_REQ_EVT:
    logger.info("BLE GAP SEC_REQ");
    // Send the positive(true) security response to the peer device to accept the security
    // request. If not accept the security request, should send the security response with
    // negative(false) accept value.
    esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
    break;

  case ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT:
    logger.info("BLE GAP PHY_UPDATE_COMPLETE");
    break;

  case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    logger.debug("update connection params status = {}, min_int = {}, max_int = {},conn_int = {},latency = {}, timeout = {}",
                (int)param->update_conn_params.status,
                (int)param->update_conn_params.min_int,
                (int)param->update_conn_params.max_int,
                (int)param->update_conn_params.conn_int,
                (int)param->update_conn_params.latency,
                (int)param->update_conn_params.timeout);
    break;
  default:
    break;
  }
}

void prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
  logger.info("prepare write, handle = {}, value len = {}", param->write.handle, param->write.len);
  esp_gatt_status_t status = ESP_GATT_OK;
  if (prepare_write_env->prepare_buf == NULL) {
    prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
    prepare_write_env->prepare_len = 0;
    if (prepare_write_env->prepare_buf == NULL) {
      logger.error("{}, Gatt_server prep no mem", __func__);
      status = ESP_GATT_NO_RESOURCES;
    }
  } else {
    if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
      status = ESP_GATT_INVALID_OFFSET;
    } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
      status = ESP_GATT_INVALID_ATTR_LEN;
    }
  }
  /*send response when param->write.need_rsp is true */
  if (param->write.need_rsp){
    esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
    if (gatt_rsp != NULL){
      gatt_rsp->attr_value.len = param->write.len;
      gatt_rsp->attr_value.handle = param->write.handle;
      gatt_rsp->attr_value.offset = param->write.offset;
      gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
      memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
      esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
      if (response_err != ESP_OK){
        logger.error("Send response error");
      }
      free(gatt_rsp);
    }else{
      logger.error("{}, malloc failed", __func__);
    }
  }
  if (status != ESP_GATT_OK){
    return;
  }
  memcpy(prepare_write_env->prepare_buf + param->write.offset,
         param->write.value,
         param->write.len);
  prepare_write_env->prepare_len += param->write.len;

}

void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
  if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
    std::vector<uint8_t> data(prepare_write_env->prepare_buf, prepare_write_env->prepare_buf + prepare_write_env->prepare_len);
    logger.debug("ESP_GATT_PREP_WRITE_EXEC: data = {::#x}", data);
  }else{
    logger.info("ESP_GATT_PREP_WRITE_CANCEL");
  }
  if (prepare_write_env->prepare_buf) {
    free(prepare_write_env->prepare_buf);
    prepare_write_env->prepare_buf = NULL;
  }
  prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
  uint64_t peer_address = 0;
  logger.debug("gatts_profile_event_handler: event = {}, gatts_if = {}",
               ble_gatts_evt_str(event),
               (int)gatts_if);
  switch (event) {
  case ESP_GATTS_REG_EVT:{
    logger.info("ESP_GATTS_REG_EVT");
    esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(CONFIG_DEVICE_NAME);
    if (set_dev_name_ret){
      logger.error("set device name failed, error code = {:#x}", set_dev_name_ret);
    }
    esp_err_t adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data.data(), raw_adv_data.size());
    esp_err_t scan_ret = esp_ble_gap_config_adv_data(&scan_rsp_config);
    // esp_err_t scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
    esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, GFPS_IDX_NB, SVC_INST_ID);
    if (adv_ret){
      logger.error("config adv data failed, error code = {:#x}", adv_ret);
    }
    if (scan_ret){
      logger.error("config scan response data failed, error code = {:#x}", scan_ret);
    }
    if (create_attr_ret){
      logger.error("create attr table failed, error code = {:#x}", create_attr_ret);
    }
    adv_config_done |= ADV_CONFIG_FLAG;
    adv_config_done |= SCAN_RSP_CONFIG_FLAG;
  }
    break;
  case ESP_GATTS_READ_EVT:
    for (int i = 0; i < 6; i++) {
      peer_address += ((uint64_t)param->read.bda[i]) << (i * 8);
    }
    logger.debug("ESP_GATTS_READ_EVT, peer_address: {:#x}", peer_address);
    // use the g_ble_interface on_gatt_read callback to handle this
    if (g_ble_interface != nullptr){
      // get the characteristic handle
      nearby_fp_Characteristic characteristic;
      if (param->read.handle ==gfps_handle_table[IDX_CHAR_VAL_MODEL_ID]) {
        logger.debug("read model id");
        characteristic = kModelId;
      } else if (param->read.handle ==gfps_handle_table[IDX_CHAR_VAL_KB_PAIRING]) {
        logger.debug("read kb pairing");
        characteristic = kKeyBasedPairing;
      } else if (param->read.handle ==gfps_handle_table[IDX_CHAR_VAL_PASSKEY]) {
        logger.debug("read passkey");
        characteristic = kPasskey;
      } else if (param->read.handle ==gfps_handle_table[IDX_CHAR_VAL_ACCOUNT_KEY]) {
        logger.debug("read account key");
        characteristic = kAccountKey;
      } else if (param->read.handle ==gfps_handle_table[IDX_CHAR_VAL_FW_REVISION]) {
        logger.debug("read firmware revision");
        characteristic = kFirmwareRevision;
      } else {
        logger.error("Unknown characteristic handle: {}", param->read.handle);
        break;
      }
      // make some memory for the output
      size_t output_size = 32;
      uint8_t *output = (uint8_t *)malloc(sizeof(uint8_t) * output_size);
      // now actually call the callback
      logger.debug("Calling on_gatt_read with peer_address = {:#x}, characteristic = {}", peer_address, (int)characteristic);
      auto status = g_ble_interface->on_gatt_read(peer_address, characteristic, output, &output_size);
      // if the status was good, send the response
      if (status == kNearbyStatusOK) {
        // write the response to the characteristic
        esp_ble_gatts_set_attr_value(param->read.handle, output_size, output);
      } else {
        logger.error("on_gatt_read returned status {}", (int)status);
      }
      // free the memory
      free(output);
    }
    break;
  case ESP_GATTS_WRITE_EVT:
    for (int i = 0; i < 6; i++) {
      peer_address += ((uint64_t)param->write.bda[i]) << (i * 8);
    }
    logger.debug("ESP_GATTS_WRITE_EVT, peer_address: {:#x}", peer_address);
    if (!param->write.is_prep){
      // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
      logger.info("GATT_WRITE_EVT, handle = {}, value len = {}", param->write.handle, param->write.len);

      if (gfps_handle_table[IDX_CHAR_CFG_KB_PAIRING] == param->write.handle && param->write.len == 2){
        logger.info("write to IDX_CHAR_CFG_KB_PAIRING");
        uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
        if (descr_value == 0x0001){
          logger.info("notify enable");
          uint8_t notify_data[15];
          for (int i = 0; i < sizeof(notify_data); ++i)
            {
              notify_data[i] = i % 0xff;
            }
          //the size of notify_data[] need less than MTU size
          esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gfps_handle_table[IDX_CHAR_VAL_KB_PAIRING],
                                      sizeof(notify_data), notify_data, false);
        }else if (descr_value == 0x0002){
          logger.info("indicate enable");
          uint8_t indicate_data[15];
          for (int i = 0; i < sizeof(indicate_data); ++i)
            {
              indicate_data[i] = i % 0xff;
            }
          //the size of indicate_data[] need less than MTU size
          esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gfps_handle_table[IDX_CHAR_VAL_KB_PAIRING],
                                      sizeof(indicate_data), indicate_data, true);
        }
        else if (descr_value == 0x0000){
          logger.info("notify/indicate disable ");
        }else{
          logger.error("unknown descr value");
          // esp_log_buffer_hex("", param->write.value, param->write.len);
        }
      }
      else {
        // use the g_ble_interface on_gatt_write callback to handle this
        if (g_ble_interface != nullptr){
          // get the characteristic handle
          nearby_fp_Characteristic characteristic;
          if (param->write.handle ==gfps_handle_table[IDX_CHAR_VAL_KB_PAIRING]) {
            logger.debug("write to IDX_CHAR_VAL_KB_PAIRING");
            characteristic = kKeyBasedPairing;
          } else if (param->write.handle ==gfps_handle_table[IDX_CHAR_VAL_PASSKEY]) {
            logger.debug("write to IDX_CHAR_VAL_PASSKEY");
            characteristic = kPasskey;
          } else if (param->write.handle ==gfps_handle_table[IDX_CHAR_VAL_ACCOUNT_KEY]) {
            logger.debug("write to IDX_CHAR_VAL_ACCOUNT_KEY");
            characteristic = kAccountKey;
          } else {
            logger.error("Unknown characteristic handle: {}", param->write.handle);
            break;
          }
          // now actually call the callback
          logger.debug("Calling on_gatt_write with peer_address = {:#x}, characteristic = {}", peer_address, (int)characteristic);
          auto status = g_ble_interface->on_gatt_write(peer_address, characteristic, param->write.value, param->write.len);
          // if the status was good, send the response
          if (status == kNearbyStatusOK) {
            logger.debug("on_gatt_write returned status OK");
          } else {
            logger.error("Error: on_gatt_write returned status {}", (int)status);
          }
        }
      }
      /* send response when param->write.need_rsp is true*/
      if (param->write.need_rsp){
        logger.info("send response");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
      }
    }else{
      /* handle prepare write */
      logger.info("ESP_GATTS_PREP_WRITE_EVT");
      prepare_write_event_env(gatts_if, &prepare_write_env, param);
    }
    break;
  case ESP_GATTS_EXEC_WRITE_EVT:
    // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
    logger.info("ESP_GATTS_EXEC_WRITE_EVT");
    exec_write_event_env(&prepare_write_env, param);
    break;
  case ESP_GATTS_SET_ATTR_VAL_EVT:
    logger.info("ESP_GATTS_SET_ATTR_VAL_EVT, attr_handle {}, srvc_handle {}, status {}",
                (int)param->set_attr_val.attr_handle,
                (int)param->set_attr_val.srvc_handle,
                (int)param->set_attr_val.status);
    break;
  case ESP_GATTS_MTU_EVT:
    logger.info("ESP_GATTS_MTU_EVT, MTU {}", (int)param->mtu.mtu);
    break;
  case ESP_GATTS_CONF_EVT:
    logger.info("ESP_GATTS_CONF_EVT, status = {}, attr_handle {}", (int)param->conf.status, (int)param->conf.handle);
    break;
  case ESP_GATTS_START_EVT:
    logger.info("SERVICE_START_EVT, status {}, service_handle {}", (int)param->start.status, (int)param->start.service_handle);
    break;
  case ESP_GATTS_CONNECT_EVT: {
    logger.info("ESP_GATTS_CONNECT_EVT, conn_id = {}", (int)param->connect.conn_id);
    // update the connection id to each profile table
    gfps_profile_tab[PROFILE_APP_IDX].conn_id = param->connect.conn_id;
    // esp_log_buffer_hex("", param->connect.remote_bda, 6);
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
    conn_params.latency = 0;
    conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
    conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
    conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
    //start sent the update connection parameters to the peer device.
    esp_ble_gap_update_conn_params(&conn_params);
  }
    break;
  case ESP_GATTS_DISCONNECT_EVT:
    logger.info("ESP_GATTS_DISCONNECT_EVT, reason = {:#x}", (int)param->disconnect.reason);
    esp_ble_gap_start_advertising(&adv_params);
    break;
  case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
    if (param->add_attr_tab.status != ESP_GATT_OK){
      logger.error("create attribute table failed, error code={:#x}", (int)param->add_attr_tab.status);
    }
    else if (param->add_attr_tab.num_handle != GFPS_IDX_NB){
      logger.error("create attribute table abnormally, num_handle ({}) \
                        doesn't equal to GFPS_IDX_NB({})", (int)param->add_attr_tab.num_handle, (int)GFPS_IDX_NB);
    }
    else {
      logger.info("create attribute table successfully, the number handle = {}", (int)param->add_attr_tab.num_handle);
      memcpy(gfps_handle_table, param->add_attr_tab.handles, sizeof(gfps_handle_table));
      esp_ble_gatts_start_service(gfps_handle_table[IDX_SVC]);
    }
    break;
  }
  case ESP_GATTS_STOP_EVT:
  case ESP_GATTS_OPEN_EVT:
  case ESP_GATTS_CANCEL_OPEN_EVT:
  case ESP_GATTS_CLOSE_EVT:
  case ESP_GATTS_LISTEN_EVT:
  case ESP_GATTS_CONGEST_EVT:
  case ESP_GATTS_UNREG_EVT:
  case ESP_GATTS_DELETE_EVT:
  default:
    logger.debug("gatts_event_handler: unhandled event {}", (int)event);
    break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

  /* If event is register event, store the gatts_if for each profile */
  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      gfps_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
    } else {
      logger.error("reg app failed, app_id {:04x}, status {}",
                   (int)param->reg.app_id,
                   (int)param->reg.status);
      return;
    }
  }
  do {
    int idx;
    for (idx = 0; idx < PROFILE_NUM; idx++) {
      /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
      if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gfps_profile_tab[idx].gatts_if) {
        if (gfps_profile_tab[idx].gatts_cb) {
          gfps_profile_tab[idx].gatts_cb(event, gatts_if, param);
        }
      }
    }
  } while (0);
}


/////////////////BLE///////////////////////

// Gets BLE address.
uint64_t nearby_platform_GetBleAddress() {
  // get the mac address of the radio
  const uint8_t* point = esp_bt_dev_get_address();
  uint64_t radio_mac_addr = 0;
  if (point == nullptr) {
    return 0;
  } else {
    // convert the 6 byte mac address to a 48 bit integer
    for (int i = 0; i < 6; i++) {
      radio_mac_addr |= (uint64_t)point[5-i] << (i * 8);
    }
  }
  logger.debug("radio mac address: {:#x}", radio_mac_addr);
  return radio_mac_addr;
}

// Sets BLE address. Returns address after change, which may be different than
// requested address.
//
// address - BLE address to set.
uint64_t nearby_platform_SetBleAddress(uint64_t address) {
  logger.warn("SetBleAddress not implemented, trying to set to {:#x}", address);
  // TODO: implement, possibly with esp_base_mac_addr_set()
  return 0;
}

#ifdef NEARBY_FP_HAVE_BLE_ADDRESS_ROTATION
// Rotates BLE address to a random resolvable private address (RPA). Returns
// address after change.
uint64_t nearby_platform_RotateBleAddress() {
  logger.warn("RotateBleAddress not implemented");
  // TODO: implement
  return 0;
}
#endif /* NEARBY_FP_HAVE_BLE_ADDRESS_ROTATION */

// Gets the PSM - Protocol and Service Mulitplexor - assigned to Fast Pair's
// Message Stream.
// To support Message Stream for BLE devices, Fast Pair will build and maintain
// a BLE L2CAP channel for sending and receiving messages. The PSM can be
// dynamic or fixed.
// Returns a 16 bit PSM number or a negative value on error. When a valid PSM
// number is returned, the device must be ready to accept L2CAP connections.
int32_t nearby_platform_GetMessageStreamPsm() {
  logger.warn("GetMessageStreamPsm not implemented");
  // TODO: implement
  return -1;
}

// Sends a notification to the connected GATT client.
//
// peer_address   - Address of peer.
// characteristic - Characteristic UUID
// message        - Message buffer to transmit.
// length         - Length of message in buffer.
nearby_platform_status nearby_platform_GattNotify(
    uint64_t peer_address, nearby_fp_Characteristic characteristic,
    const uint8_t* message, size_t length) {
  logger.debug("GattNotify: peer_address={:#x}, characteristic={}, length={}",
               peer_address, (int)characteristic, length);
  // look up the attribute handle for the characteristic from the gfp_handle_table
  uint16_t attr_handle = 0;
  switch (characteristic) {
    case kModelId:
      logger.debug("Notifying characteristic: kModelId");
      attr_handle = gfps_handle_table[IDX_CHAR_VAL_MODEL_ID];
      break;
    case kKeyBasedPairing:
      logger.debug("Notifying characteristic: kKeyBasedPairing");
      attr_handle = gfps_handle_table[IDX_CHAR_VAL_KB_PAIRING];
      break;
    case kPasskey:
      logger.debug("Notifying characteristic: kPasskey");
      attr_handle = gfps_handle_table[IDX_CHAR_VAL_PASSKEY];
      break;
    case kAccountKey:
      logger.debug("Notifying characteristic: kAccountKey");
      attr_handle = gfps_handle_table[IDX_CHAR_VAL_ACCOUNT_KEY];
      break;
    case kFirmwareRevision:
      logger.debug("Notifying characteristic: kFirmwareRevision");
      attr_handle = gfps_handle_table[IDX_CHAR_VAL_FW_REVISION];
      break;
      // TODO: add support for kAdditionalData
      // TODO: add support for kMessageStreamPsm
    default:
      logger.error("[{}] Unknown/unsupported characteristic: {}", __func__, (int)characteristic);
      return kNearbyStatusError;
  }
  // now actually send the notification
  uint16_t gatts_if = gfps_profile_tab[PROFILE_APP_IDX].gatts_if;
  uint16_t conn_id = gfps_profile_tab[PROFILE_APP_IDX].conn_id;
  logger.debug("Sending notification: gatts_if={}, conn_id={}, attr_handle={}, length={}",
               gatts_if, conn_id, attr_handle, length);
  esp_err_t err = esp_ble_gatts_send_indicate(gatts_if,
                                              conn_id,
                                              attr_handle,
                                              length,
                                              (uint8_t*)message,
                                              true); // false = notification, true = indication
  // auto err = esp_ble_gatts_set_attr_value(attr_handle, length, (uint8_t*)message);
  // auto err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
  if (err != ESP_OK) {
    logger.error("esp_ble_gatts_send_indicate failed: {}", err);
    return kNearbyStatusError;
  }
  return kNearbyStatusOK;
}

// Sets the Fast Pair advertisement payload and starts advertising at a given
// interval.
//
// payload  - Advertising data.
// length   - Length of data.
// interval - Advertising interval code.
nearby_platform_status nearby_platform_SetAdvertisement(
    const uint8_t* payload, size_t length,
    nearby_fp_AvertisementInterval interval) {
  static bool is_advertising = false;
  if (is_advertising) {
    logger.warn("Already advertising");
    return kNearbyStatusError;
  }
  if (length > 31) {
    logger.error("Advertisement payload too long");
    return kNearbyStatusError;
  }
  if (length == 0) {
    logger.error("Advertisement payload empty");
    return kNearbyStatusError;
  }
  logger.info("Setting advertisement");
  raw_adv_data.assign(payload, payload + length);
  logger.debug("Payload: {::#x}", raw_adv_data);
  esp_err_t adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data.data(), raw_adv_data.size());
  esp_err_t scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
  if (adv_ret){
    logger.error("config adv data failed, error code = {:#x} ", (int)adv_ret);
    return kNearbyStatusError;
  }
  if (scan_ret){
    logger.error("config scan response data failed, error code = {:#x}", (int)scan_ret);
    return kNearbyStatusError;
  }
  // TODO: set the advertising interval
  // update the state
  is_advertising = true;
  return kNearbyStatusOK;
}

// Initializes BLE
//
// ble_interface - GATT read and write callbacks structure.
nearby_platform_status nearby_platform_BleInit(
    const nearby_platform_BleInterface* ble_interface) {

  logger.info("Initializing BLE");

  // save the ble_interface (on_gatt_write and on_gatt_read callbacks) for later
  g_ble_interface = ble_interface;

  esp_err_t ret;

  // TODO: this auth mode works, but requires many prompts
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;

  // TODO: this auth mode times out or blocks after getting the SC_OOB_REQ event
  // esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;

  if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, 1)) !=
      ESP_OK) {
    logger.error("GAP set_security_param AUTHEN_REQ_MODE failed: {}", ret);
    return kNearbyStatusError;
  }

  esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE; // device is not capable of input or output, unsecure
  if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 1)) != ESP_OK) {
    logger.error("GAP set_security_param IOCAP_MODE failed: {}", ret);
    return kNearbyStatusError;
  }

  // uint8_t oob_support = ESP_BLE_OOB_ENABLE;
  uint8_t oob_support = ESP_BLE_OOB_DISABLE;
  if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t))) != ESP_OK) {
    logger.error("GAP set_security_param OOB_SUPPORT failed: {}", ret);
    return kNearbyStatusError;
  }

  uint8_t spec_auth = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
  if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &spec_auth, sizeof(uint8_t))) != ESP_OK) {
    logger.error("GAP set_security_param ONLY_ACCEPT_SPECIFIED_SEC_AUTH failed: {}", ret);
    return kNearbyStatusError;
  }

  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t key_size = 16; //the key size should be 7~16 bytes
  uint32_t passkey = 1234;//ESP_IO_CAP_OUT
  if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, 1)) != ESP_OK) {
    logger.error("GAP set_security_param SET_INIT_KEY failed: {}", ret);
    return kNearbyStatusError;
  }

  if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, 1)) != ESP_OK) {
    logger.error("GAP set_security_param SET_RSP_KEY failed: {}", ret);
    return kNearbyStatusError;
  }

  if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, 1)) != ESP_OK) {
    logger.error("GAP set_security_param MAX_KEY_SIZE failed: {}", ret);
    return kNearbyStatusError;
  }

  esp_ble_gatts_register_callback(gatts_event_handler);
  esp_ble_gap_register_callback(gap_event_handler);
  esp_ble_gatts_app_register(ESP_APP_ID);
  // esp_ble_gatt_set_local_mtu(500);

  return kNearbyStatusOK;
}
