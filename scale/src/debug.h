#ifndef __KONKER_DEBUG__
#define __KONKER_DEBUG__

#ifndef __MODULE_NAME__
#define __MODULE_NAME__ ""
#endif  

#define __LOG_BUFFER_SIZE__ 300

/**
 * macros used for easy of debug and message showing in the code 
 */

#ifdef KONKER_DEBUG
#define LOG(...) { snprintf(__log_buffer__, __LOG_BUFFER_SIZE__, __VA_ARGS__); __log_buffer__[__LOG_BUFFER_SIZE__-1] = '\0'; Serial.print(__MODULE_NAME__); Serial.print(" DEBUG :"); Serial.println(__log_buffer__); }
#else
#define LOG(...) // blank line
#endif

#ifndef DEVELOP
#define LOGDEV(...) { snprintf(__log_buffer__, __LOG_BUFFER_SIZE__, __VA_ARGS__);  __log_buffer__[__LOG_BUFFER_SIZE__-1] = '\0'; Serial.print(__MODULE_NAME__); Serial.print(" DEVELOP: "); Serial.println(__log_buffer__); }
#else
#define LOGDEV(...) // blank line
#endif

#define ERROR(...) { snprintf(__log_buffer__, __LOG_BUFFER_SIZE__, __VA_ARGS__); __log_buffer__[__LOG_BUFFER_SIZE__-1] = '\0'; Serial.print(__MODULE_NAME__); Serial.print(" ERROR: "); Serial.println(__log_buffer__); }
#define WARN(...) { snprintf(__log_buffer__, __LOG_BUFFER_SIZE__, __VA_ARGS__); __log_buffer__[__LOG_BUFFER_SIZE__-1] = '\0'; Serial.print(__MODULE_NAME__); Serial.print(" WARN: "); Serial.println(__log_buffer__); }



extern char __log_buffer__[__LOG_BUFFER_SIZE__];


#endif