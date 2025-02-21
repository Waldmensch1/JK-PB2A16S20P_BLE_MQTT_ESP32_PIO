#ifndef MACROS_H
#define MACROS_H

#define DOUBLEESCAPE(a) #a
#define TEXTIFY(a) DOUBLEESCAPE(a)

// Serial Output configuration
#ifdef SERIAL_OUT
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

#endif //MACROS_H