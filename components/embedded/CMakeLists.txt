idf_component_register(
  INCLUDE_DIRS "../../external/nearby/embedded/client/source" "../../external/nearby/embedded/common/source" "../../external/nearby/embedded/common/target" "include"
  SRC_DIRS "../../external/nearby/embedded/client/source" "../../external/nearby/embedded/common/source" "src"
  REQUIRES "bt" "nvs_flash" "logger" "task" "timer"
)
add_definitions(-DNEARBY_TRACE_LEVEL=2)
