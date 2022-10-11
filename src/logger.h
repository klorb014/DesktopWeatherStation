#include <string>

#define DEBUG_ACTIVE 1

#define RED_PRE "\e[1;31m"
#define RED_SUF "\e[1;37m"
#define GREY_PRE "\e[33m"
#define GREY_SUF "\e[0m"

#define traceStamp(x)                                  \
        Serial.print("[");                             \
        Serial.print(x);                               \
        Serial.print("]");                             \
        Serial.print("[");                             \
        Serial.print(__FUNCTION__);                    \
        Serial.print(":");                             \
        Serial.print(__LINE__);                        \
        Serial.print("] ");                            

#define LOG_INFO(x)                                    \
        traceStamp("INFO");                            \
        Serial.println(x);                             

#define LOG_ERROR(x)                                   \
        Serial.print(RED_PRE);                         \
        traceStamp("ERROR");                           \
        Serial.print(x);                               \
        Serial.println(RED_SUF);   

#if DEBUG_ACTIVE
#define LOG_DEBUG(x)                                   \
        Serial.print(GREY_PRE);                        \
        traceStamp("DEBUG");                           \
        Serial.print(x);                               \
        Serial.println(GREY_SUF); 
#else
#define LOG_DEBUG(x) 
#endif