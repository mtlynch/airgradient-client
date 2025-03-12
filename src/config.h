#ifndef __CONFIG_H
#define __CONFIG_H

// #define ENABLE_DEBUG_MODE
// #define DELAY_INSTEAD_OF_DEEP_SLEEP

#ifdef ENABLE_DEBUG_MODE
#define DBG(...)                     \
    {                                \
        Serial.print("[");           \
        Serial.print(__FUNCTION__);  \
        Serial.print("(): ");        \
        Serial.print(__LINE__);      \
        Serial.print(" ] ");         \
        Serial.println(__VA_ARGS__); \
    }
#else
#define DBG(...)
#endif

#endif