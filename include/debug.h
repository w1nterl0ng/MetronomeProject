#pragma once

#ifdef DEBUG_OUTPUT
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)

#if DEBUG_LEVEL >= 2
#define DEBUG_VERBOSE(x) Serial.println(x)
#else
#define DEBUG_VERBOSE(x)
#endif
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(x, ...)
#define DEBUG_VERBOSE(x)
#endif