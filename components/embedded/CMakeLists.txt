idf_component_register(
  INCLUDE_DIRS "../../external/embedded/client/source" "../../external/embedded/common/source" "../../external/embedded/common/target" "../../external/embedded/common/target/esp32" "include"
  SRC_DIRS "../../external/embedded/client/source" "../../external/embedded/common/source" "src"
  REQUIRES "bt" "logger" "task" "timer"
)
add_definitions(-DNEARBY_TRACE_LEVEL=2)
