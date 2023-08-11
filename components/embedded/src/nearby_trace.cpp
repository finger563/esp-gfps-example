#include "embedded.hpp"

static espp::Logger logger({.tag="embedded", .level=espp::Logger::Verbosity::DEBUG});

// Generates conditional trace line. This is usually wrapped in a macro to
// provide compiler parameters.
//
// level    - Debug level, higher is less messages.
// filename - Name of file calling trace.
// lineno   - Source line of trace call.
// fmt      - printf() style format string (%).
// ...      - A series of parameters indicated by the fmt string.
void nearby_platform_Trace(nearby_platform_TraceLevel level,
                           const char *filename, int lineno, const char *fmt,
                           ...) {
  // TODO: implement
}

// Processes assert. Processes a failed assertion.
//
// filename - Name of file calling assert.
// lineno   - Source line of assert call
// reason   - String message indicating reason for assert.
void nearby_platfrom_CrashOnAssert(const char *filename, int lineno,
                                   const char *reason) {
  logger.error("Assert failed: {}", reason);
  logger.error("File: {}, line: {}", filename, lineno);
  abort();
}

// Initializes trace module.
void nearby_platform_TraceInit(void) {

}
