idf_component_register(
  INCLUDE_DIRS "../../external/nearby/embedded/client/source" "../../external/nearby/embedded/common/source" "../../external/nearby/embedded/common/target" "include"
  SRC_DIRS "../../external/nearby/embedded/client/source" "../../external/nearby/embedded/common/source" "../../external/nearby/embedded/common/source/mbedtls" "src"
  REQUIRES "bt" "mbedtls" "nvs_flash" "logger" "task" "timer"
)
add_definitions(-DNEARBY_TRACE_LEVEL=1)
add_definitions(-DNEARBY_PLATFORM_USE_MBEDTLS=1)
add_definitions(-DNEARBY_FP_ENABLE_BATTERY_NOTIFICATION=0)
add_definitions(-DNEARBY_FP_ENABLE_ADDITIONAL_DATA=0)
add_definitions(-DNEARBY_FP_MESSAGE_STREAM=0)
# add_definitions(-DNEARBY_PLATFORM_HAS_SE)
# add_definitions(-DNEARBY_FP_HAVE_BLE_ADDRESS_ROTATION=0)
# add_definitions(-DNEARBY_FP_ENABLE_SASS=0) # smart audio source switching
add_definitions(-DNEARBY_FP_RETROACTIVE_PAIRING=1) # not sure what this is...
add_definitions(-DNEARBY_FP_BLE_ONLY=1)
add_definitions(-DNEARBY_FP_PREFER_BLE_BONDING=1)
add_definitions(-DNEARBY_FP_PREFER_LE_TRANSPORT=1)
