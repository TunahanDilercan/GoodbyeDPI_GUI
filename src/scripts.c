#include "scripts.h"
#include "goodbyedpi.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define MAX_SCRIPTS 32  // Increase the maximum number of supported scripts
#define MAX_ARGS 32
#define DEFAULT_SCRIPT_NAME "Any Country DNS Redirect"  // Set the default script name

static Script scripts[MAX_SCRIPTS];
static int scnt = 0;
static int default_script_index = -1;  // Default script index

// Extract arguments from batch scripts
static void add_script(const char *name, const char *args)
{
    if (scnt >= MAX_SCRIPTS) return;
    
    // Check if script with same name already exists to avoid duplicates
    for (int i = 0; i < scnt; i++) {
        if (strcmp(scripts[i].name, name) == 0) {
            // Script with same name already exists, don't add it again
            FILE *logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "[scripts] Duplicate script detected: %s (skipping)\n", name);
                fclose(logfile);
            }
            return;
        }
    }
    
    // Clear memory before setting
    memset(scripts[scnt].name, 0, sizeof(scripts[scnt].name));
    memset(scripts[scnt].args, 0, sizeof(scripts[scnt].args));
    
    // Copy with safe length limits
    strncpy(scripts[scnt].name, name, sizeof(scripts[scnt].name)-1);
    strncpy(scripts[scnt].args, args, sizeof(scripts[scnt].args)-1);
    
    // Ensure null termination
    scripts[scnt].name[sizeof(scripts[scnt].name)-1] = '\0';
    scripts[scnt].args[sizeof(scripts[scnt].args)-1] = '\0';
    
    // Mark the default script
    if (strcmp(name, DEFAULT_SCRIPT_NAME) == 0) {
        default_script_index = scnt;
    }
    
    // Log successful addition
    FILE *logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[scripts] Added script #%d: %s\n", scnt, name);
        fclose(logfile);
    }
    
    scnt++;
}

void scripts_init(void)
{
    // Scripts transferred from .cmd files - full configurations
    // Previous .cmd files were reviewed and their parameters were directly transferred here
    
    // Main scripts
    add_script("Russia Blacklist DNS Redirect", "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253 --blacklist ..\\russia-blacklist.txt");
    add_script(DEFAULT_SCRIPT_NAME, "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("Any Country Standard", "-5");
    
    // Turkey scripts - From .cmd files in the Turkey folder
    add_script("[TR] DNS Redirect", "-5 --set-ttl 5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 2", "-5");
    add_script("[TR] Alternative 3", "--set-ttl 3 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 4", "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 5", "-9 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 6", "-9");
    add_script("[TR] TTL 3 Only", "--set-ttl 3");
}

// Function to return the default script index
int get_default_script_index(void) {
    // If default_script_index is invalid, try the first Turkey script
    if (default_script_index < 0) {
        for (int i = 0; i < scnt; i++) {
            if (strstr(scripts[i].name, "[TR]")) {
                return i;
            }
        }
        // If no Turkey script exists, use the first script
        return (scnt > 0) ? 0 : -1;
    }
    return default_script_index;
}

// Function to return the entire script array
Script* scripts_get_all(void) {
    return scripts;
}

// Cleanup function (no actual memory cleanup since static array is used)
void scripts_free(void) {
    // No actual memory cleanup needed for now, 
    // can be used if dynamic memory allocation is implemented in the future
    scnt = 0;
}

int scripts_count(void) { return scnt; }
const Script* script_get(int i) { return (i >= 0 && i < scnt) ? &scripts[i] : NULL; }

// Split arguments by spaces and create argv array
static int parse_args(const char *args, char **argv)
{
    int argc = 1;
    char *safe_buffer;
    
    // argv[0] is always the program name
    argv[0] = _strdup("goodbyedpi.exe");
    
    // Create a safe buffer and copy arguments
    safe_buffer = _strdup(args); // Memory allocated with strdup
    if (!safe_buffer) return argc; // Return in case of memory error
    
    // Tokenizing the buffer string
    char *token = strtok(safe_buffer, " ");
    while (token && argc < MAX_ARGS-1) {
        // Allocate new memory for each argument
        argv[argc] = _strdup(token);
        if (argv[argc]) {
            argc++;
        }
        token = strtok(NULL, " ");
    }
    
    argv[argc] = NULL;
    
    // Free safe_buffer, argv contents are in separate memory
    free(safe_buffer);
    
    return argc;
}

int script_run(int idx)
{
    if (idx < 0 || idx >= scnt) return -1;
    
    // Safety check - don't start if already running
    if (is_goodbyedpi_running()) {
        FILE *logfile = fopen("goodbyedpi_log.txt", "a");
        if (logfile) {
            fprintf(logfile, "[%s] WARNING: Attempted to start script while another is running\n", __FUNCTION__);
            fclose(logfile);
        }
        return -2;
    }
    
    // Ensure the current process is properly stopped first
    script_stop();
    
    // Wait a bit to ensure the process is completely stopped
    Sleep(300);
    
    // Double check it's really stopped
    if (is_goodbyedpi_running()) {
        FILE *logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "[%s] ERROR: Previous process still running after stop attempt\n", __FUNCTION__);
            fclose(logfile);
        }
        return -3;
    }
    
    // Allocate and initialize a clean argv array
    char *argv[MAX_ARGS] = {0};
    
    // Log
    FILE *logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] Running script: %s with args: %s\n", 
                __FUNCTION__, scripts[idx].name, scripts[idx].args);
        fclose(logfile);
    }
    
    // Parse arguments
    int argc = parse_args(scripts[idx].args, argv);
    
    // Validate arguments
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] Script %s parsed: %d arguments\n", 
                __FUNCTION__, scripts[idx].name, argc);
        for (int i = 0; i < argc; i++) {
            if (argv[i] == NULL) {
                fprintf(logfile, "  argv[%d]: NULL! - Memory error\n", i);
            }
            else if (argv[i][0] == '\0') {
                fprintf(logfile, "  argv[%d]: empty string\n", i);
            }
            else {
                fprintf(logfile, "  argv[%d]: '%s'\n", i, argv[i]);
            }
        }
        fclose(logfile);
    }
    
    // Start goodbyedpi
    int result = start_goodbyedpi(argc, argv);
    
    // Log
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] start_goodbyedpi result: %d (%s)\n", 
                __FUNCTION__, result, result == 0 ? "success" : "error");
        fclose(logfile);
    }
    
    // Free memory allocated by start_goodbyedpi
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            free(argv[i]);
            argv[i] = NULL;
        }
    }
    
    return result;
}

void script_stop(void)
{
    FILE *logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] Stopping GoodbyeDPI...\n", __FUNCTION__);
        fclose(logfile);
    }
    
    // Only call stop once and trust it to handle timeout properly
    stop_goodbyedpi();
    
    // Give it a moment to complete
    Sleep(500);
    
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] Stop completed, running status: %d\n", 
                __FUNCTION__, is_goodbyedpi_running());
        fclose(logfile);
    }
}