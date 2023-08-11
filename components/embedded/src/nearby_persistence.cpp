#include "embedded.hpp"

// Loads stored key
//
// key    - Type of key to fetch.
// output - Buffer to contain retrieved key.
// length - On input, contains the size of the output buffer.
//          On output, contains the Length of key.
nearby_platform_status nearby_platform_LoadValue(nearby_fp_StoredKey key,
                                                 uint8_t* output,
                                                 size_t* length) {
  // TODO: Implement
  return kNearbyStatusOK;
}

// Saves stored key
//
// key    - Type of key to store.
// output - Buffer containing key to store.
// length - Length of key.
nearby_platform_status nearby_platform_SaveValue(nearby_fp_StoredKey key,
                                                 const uint8_t* input,
                                                 size_t length) {
  // TODO: Implement
  return kNearbyStatusOK;
}

// Initializes persistence module
nearby_platform_status nearby_platform_PersistenceInit() {
  // TODO: Implement
  return kNearbyStatusOK;
}
