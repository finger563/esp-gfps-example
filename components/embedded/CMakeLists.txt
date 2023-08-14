idf_component_register(
  INCLUDE_DIRS "../../external/nearby/embedded/client/source" "../../external/nearby/embedded/common/source" "../../external/nearby/embedded/common/target" "include"
  SRC_DIRS "../../external/nearby/embedded/client/source" "../../external/nearby/embedded/common/source" "../../external/nearby/embedded/common/source/mbedtls" "src"
  REQUIRES "bt" "mbedtls" "nvs_flash" "logger" "task" "timer"
)
add_definitions(-DNEARBY_TRACE_LEVEL=2)
add_definitions(-DNEARBY_PLATFORM_USE_MBEDTLS=1)
# add_definitions(-DNEARBY_PLATFORM_HAS_SE)
