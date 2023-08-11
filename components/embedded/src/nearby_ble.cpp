#include "embedded.hpp"

/////////////////BLE///////////////////////

// Gets BLE address.
uint64_t nearby_platform_GetBleAddress() {
  // TODO: implement
  return 0;
}

// Sets BLE address. Returns address after change, which may be different than
// requested address.
//
// address - BLE address to set.
uint64_t nearby_platform_SetBleAddress(uint64_t address) {
  // TODO: implement
  return 0;
}

#ifdef NEARBY_FP_HAVE_BLE_ADDRESS_ROTATION
// Rotates BLE address to a random resolvable private address (RPA). Returns
// address after change.
uint64_t nearby_platform_RotateBleAddress() {
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
  // TODO: implement
  return 0;
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
  // TODO: implement
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
  // TODO: implement
  return kNearbyStatusOK;
}

// Initializes BLE
//
// ble_interface - GATT read and write callbacks structure.
nearby_platform_status nearby_platform_BleInit(
    const nearby_platform_BleInterface* ble_interface) {
  // TODO: implement
  return kNearbyStatusOK;
}
