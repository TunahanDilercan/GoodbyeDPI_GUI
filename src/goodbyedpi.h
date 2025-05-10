#ifndef GOODBYEDPI_H
#define GOODBYEDPI_H

#include <stdio.h>
#include <windivert.h>

#define HOST_MAXLEN 253
#define MAX_PACKET_SIZE 9016

// Error codes for the API functions
#define GDPI_SUCCESS 0
#define GDPI_ERROR_ALREADY_RUNNING -1
#define GDPI_ERROR_WINDIVERT -2
#define GDPI_ERROR_INVALID_ARGS -3
#define GDPI_ERROR_ALLOCATION -4

#ifndef DEBUG
#define debug(...) do {} while (0)
#else
#define debug(...) printf(__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Core DPI logic entrypoint (formerly main)
int goodbyedpi_main(int argc, char *argv[]);

// GUI wrapper API
int  start_goodbyedpi(int argc, char **argv); // GDPI_SUCCESS = OK, <0 = hata
void stop_goodbyedpi(void);                  // Çalışıyorsa durdurur
int  is_goodbyedpi_running(void);            // 1 = Çalışıyor, 0 = Durdu

// Cleanup all filters and resources
void deinit_all(void);

// Get last error message (for UI error reporting)
const char* get_goodbyedpi_error(void);

#ifdef __cplusplus
}
#endif

#endif // GOODBYEDPI_H
