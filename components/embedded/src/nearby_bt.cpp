#include "embedded.hpp"

/////////////////BLUETOOTH///////////////////////

// Returns Fast Pair Model Id.
uint32_t nearby_platform_GetModelId() {
  return CONFIG_MODEL_ID;
}

// Returns tx power level.
int8_t nearby_platform_GetTxLevel() {
  // TODO: implement
  return 0;
}

// Returns public BR/EDR address.
// On a BLE-only device, return the public identity address.
uint64_t nearby_platform_GetPublicAddress() {
  // TODO: implement
  return 0;
}

// Returns the secondary identity address.
// Some devices, such as ear-buds, can advertise two identity addresses. In this
// case, the Seeker pairs with each address separately but treats them as a
// single logical device set.
// Return 0 if this device does not have a secondary identity address.
uint64_t nearby_platform_GetSecondaryPublicAddress() {
  // TODO: implement
  return 0;
}

// Returns passkey used during pairing
uint32_t nearby_platfrom_GetPairingPassKey() {
  // TODO: implement
  return 0;
}

// Provides the passkey received from the remote party.
// The system should compare local and remote party and accept/decline pairing
// request accordingly.
//
// passkey - Passkey
void nearby_platform_SetRemotePasskey(uint32_t passkey) {
  // TODO: implement
}

// Sends a pairing request to the Seeker
//
// remote_party_br_edr_address - BT address of peer.
nearby_platform_status nearby_platform_SendPairingRequest(
    uint64_t remote_party_br_edr_address) {
  // TODO: implement
  return kNearbyStatusOK;
}

// Switches the device capabilities field back to default so that new
// pairings continue as expected.
nearby_platform_status nearby_platform_SetDefaultCapabilities() {
  // TODO: implement
  return kNearbyStatusOK;
}

// Switches the device capabilities field to Fast Pair required configuration:
// DisplayYes/No so that `confirm passkey` pairing method will be used.
nearby_platform_status nearby_platform_SetFastPairCapabilities() {
  // TODO: implement
  return kNearbyStatusOK;
}

// Sets null-terminated device name string in UTF-8 encoding
//
// name - Zero terminated string name of device.
nearby_platform_status nearby_platform_SetDeviceName(const char* name) {
  // TODO: implement
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
  // TODO: implement
  return kNearbyStatusOK;
}

// Returns true if the device is in pairing mode (either fast-pair or manual).
bool nearby_platform_IsInPairingMode() {
  // TODO: implement
  return false;
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
  // TODO: implement
  return kNearbyStatusOK;
}

#endif /* NEARBY_FP_MESSAGE_STREAM */
// Initializes BT module
//
// bt_interface - BT callbacks event structure.
nearby_platform_status nearby_platform_BtInit(
    const nearby_platform_BtInterface* bt_interface) {
  // TODO: implement
  return kNearbyStatusOK;
}
