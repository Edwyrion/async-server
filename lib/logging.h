#ifndef LOGGING_H_
#define LOGGINH_H_

    #include <stdio.h>

    #ifdef DEBUG
        /// @brief Log an informational message.
        /// @param message The informational message to log.
        #define LOG_INFO(formatted_message, ...) \
        do { \
            (void) fprintf(stdout, formatted_message, ##__VA_ARGS__); \
        } while (0)

        /// @brief Log an error message.
        /// @param message The error message to log.
        #define LOG_ERROR(message) \
        do { \
            perror(message); \
        } while (0)
    #else
        /// @brief Log an informational message.
        /// @param message The informational message to log.
        #define LOG_INFO(formatted_message, ...) \
        do { \
            (void) formatted_message; \
        } while (0)

        /// @brief Log an error message.
        /// @param message The error message to log.
        #define LOG_ERROR(message) \
        do { \
            (void) message; \
        } while (0)
    #endif // DEBUG

#endif // LOGGING_H_