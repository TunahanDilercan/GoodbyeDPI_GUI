/*
 * GoodbyeDPI GUI implementation
 */

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <time.h>
#include <tlhelp32.h> // For CreateToolhelp32Snapshot and process functions

#include "goodbyedpi.h"
#include "scripts.h"

#include <shlobj.h> // For startup folder
#include <uxtheme.h> // For theme control
#include <vssym32.h> // Theme symbols
#define WM_TRAY   (WM_USER+1)
#define ID_TRAY   1
#define IDM_EXIT  9001
#define IDM_STOP  9002
#define IDM_START 9003
#define IDM_BASE  9100
#define IDM_STATUS     4000
#define IDM_SETTINGS   4100
#define IDM_AUTOSTART  4101
#define IDM_MINIMIZE   4102
#define IDM_ABOUT      4200
#define ID_TRAY_EXIT 1001
#define ID_TRAY_START 1002
#define ID_TRAY_STOP 1003
#define ID_TRAY_SCRIPT_FIRST 2000
#define ID_TRAY_AUTOSTART 3000
#define ID_TRAY_STATUS 3001
#define ID_TRAY_ABOUT 3002
#define ID_TRAY_MENU_START 4001
#define ID_TRAY_MENU_STOP 4002
#define ID_TRAY_MENU_SELECT_FIRST 5000
#define ID_TRAY_MENU_ABOUT 6000
#define ID_TRAY_MENU_EXIT 7000
// Icon IDs updated to match with RC file
#define IDI_ICON_MAIN    101  // Main icon (EXE)
#define IDI_ICON_RUNNING 102  // Running icon
#define IDI_ICON_PAUSED  103  // Paused icon

#define WM_APP_RESTART (WM_APP+100) // Custom message for restarting the application
#define WM_APP_BALLOON_CLICK (WM_APP+101) // When balloon notification is clicked
#define WM_APP_AUTO_START (WM_APP+102) // Custom message for auto-starting after delay

// Watchdog timer ID
#define IDT_WATCHDOG 1001
#define WATCHDOG_INTERVAL 10000 // 10 seconds

// Auto-start timer ID
#define IDT_AUTO_START 2001
#define AUTO_START_DELAY 3000 // 3 seconds delay
#define AUTO_SCRIPT_SWITCH_DELAY 5000 // 5 seconds delay

// Default script name, will be selected automatically
#define DEFAULT_SCRIPT_NAME "[TR] Alternative 4"

static NOTIFYICONDATA nid = {0};
static HMENU hMenu;
static int last_selected_idx = -1; // Last selected script index
static int is_running = 0; // Tracks whether the program is running
static int error_recovery = 0; // Error recovery mode
static HANDLE hMutex = NULL; // Mutex to prevent multiple instances
static int tray_added = 0; // Tracks whether the tray icon is added
static HICON hIconPaused = NULL; // Paused icon
static HICON hIconRunning = NULL; // Running icon
static DWORD start_time = 0; // Start time of operation
static HWND g_hwnd = NULL; // Global window handle
static HMENU hPopupMenu = NULL; // Popup menu handle

// Define menu constants that were referenced but not defined
#define MENU_START ID_TRAY_MENU_START
#define MENU_PAUSE ID_TRAY_MENU_STOP
#define MENU_EXIT ID_TRAY_MENU_EXIT
#define MENU_ABOUT ID_TRAY_MENU_ABOUT
#define MENU_SHOW ID_TRAY_STATUS
#define MENU_SCRIPT_BASE ID_TRAY_SCRIPT_FIRST

// Variables to track process state
static int process_active = 0;
static int selected_script = -1; 
static int scripts_size = 0;
static Script *scripts = NULL;
static int script_count = 0;
static char **script_names = NULL;
static BOOL auto_start_pending = FALSE;  // Flag for auto-start after initial delay
static BOOL script_switch_pending = FALSE;  // Flag for script switching in progress

// Forward declarations of functions to avoid implicit declaration errors
HMENU create_popup_menu(void);
static void handle_start(void);
static void handle_stop(void);
static void update_tray_icon(HWND hwnd);
LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExInfo);

// Function prototypes
static BOOL set_autostart(BOOL enable);
static BOOL get_autostart_enabled(void);
static void show_balloon_notification(const char* title, const char* message, DWORD info_flags);
static void show_status_dialog(HWND hwnd);
static void show_about_dialog(HWND hwnd);
static void terminate_goodbyedpi();
static void save_selected_script(int index);
static void save_selected_script_file(int index);
static int load_selected_script();
static int load_selected_script_file();
static BOOL prompt_iteration_continue(HWND hwnd);
static BOOL check_and_copy_windivert_dll(void);
static BOOL is_admin(void);  // is_admin() prototipi eklendi
static void restart_as_admin(void);  // restart_as_admin() prototipi eklendi
static BOOL install_windivert_driver(void); // install_windivert_driver() prototipi eklendi
static void log_info(const char* format, ...);  // log_info() prototype added
static void log_error(const char* format, ...);  // log_error() prototype added
static BOOL check_goodbyedpi_process(void); // Function to check if GoodbyeDPI process is running

// Helper function to identify Turkey options
int is_turkey_option(const char *name) {
    return (strstr(name, "turkey") != NULL || 
            strstr(name, "Turkey") != NULL || 
            strstr(name, "TURKEY") != NULL ||
            strstr(name, "[TR]") != NULL ||
            strstr(name, "TR ") != NULL);
}

// Sanitize name - used for script names displayed in the menu
static void sanitize_name(const char *src, char *dst, size_t dst_size) {
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

// Search for a script name in the script list
static int find_script_by_name(const char *name) {
    if (!name) return -1;
    
    int count = scripts_count();
    for (int i = 0; i < count; i++) {
        const char *script_name = script_get(i)->name;
        if (script_name && strstr(script_name, name)) {
            return i;
        }
    }
    return -1;
}

// Function to find a default script
static int find_default_script(void) {
    // First try to find "[TR] Alternative 4" as the preferred default
    for (int i = 0; i < scripts_size; i++) {
        if (scripts[i].name && strstr(scripts[i].name, "[TR] Alternative 4")) {
            return i;
        }
    }
    
    // If not found, try to find any Turkey script as default
    for (int i = 0; i < scripts_size; i++) {
        if (is_turkey_option(scripts[i].name)) {
            return i;
        }
    }
    
    // If no Turkey script is found, try "Any Country with DNS Redirector"
    for (int i = 0; i < scripts_size; i++) {
        if (scripts[i].name && strstr(scripts[i].name, "Any Country with DNS Redirector")) {
            return i;
        }
    }
    
    // If no preferred script is found, return first available script
    if (scripts_size > 0) {
        return 0;
    }
    
    // No scripts available
    return -1;
}

// Function to get the default script index (option 4 - Any Country with DNS Redirector)
static int get_default_script_index(void) {
    int i;
    
    // First look for "Any Country with DNS Redirector" (option 4)
    for (i = 0; i < scripts_size; i++) {
        if (scripts[i].name && strstr(scripts[i].name, DEFAULT_SCRIPT_NAME)) {
            log_info("Found default script '%s' at index %d", DEFAULT_SCRIPT_NAME, i);
            return i;
        }
    }
    
    // If not found, fall back to find_default_script
    return find_default_script();
}

// Function to log errors with timestamp
static void log_error(const char* format, ...) {
    FILE* file = fopen("goodbyedpi_error.log", "a");
    if (file) {
        time_t now = time(NULL);
        char time_buf[64];
        struct tm *ptm = localtime(&now);
        va_list args;
        if (ptm) {
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", ptm);
        } else {
            strcpy(time_buf, "unknown time");
        }
        fprintf(file, "[%s] ", time_buf);
        va_start(args, format);
        vfprintf(file, format, args);
        va_end(args);
        fprintf(file, "\n");
        fclose(file);
        
        // Ayrıca hataları konsola da yazdır (debug için)
        va_start(args, format);
        fprintf(stderr, "[ERROR] ");
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

// Normal düzeyde log kaydı için yeni bir fonksiyon
static void log_info(const char* format, ...) {
    FILE* file = fopen("goodbyedpi_log.txt", "a");
    if (file) {
        time_t now = time(NULL);
        char time_buf[64];
        struct tm *ptm = localtime(&now);
        va_list args;
        if (ptm) {
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", ptm);
        } else {
            strcpy(time_buf, "unknown time");
        }
        fprintf(file, "[%s] ", time_buf);
        va_start(args, format);
        vfprintf(file, format, args);
        va_end(args);
        fprintf(file, "\n");
        fclose(file);
    }
}

// Let Windows use its own exception handler
static void setup_exception_handler(void) {
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
}

// Exception handler fonksiyonu
LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExInfo) {
    // Exception bilgilerini logla
    FILE* file = fopen("goodbyedpi_error.log", "a");
    if (file) {
        time_t now = time(NULL);
        char time_buf[64];
        struct tm *ptm = localtime(&now);
        
        if (ptm) {
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", ptm);
        } else {
            strcpy(time_buf, "unknown time");
        }
        
        fprintf(file, "[%s] UNHANDLED EXCEPTION: Exception code: 0x%08X at address 0x%p\n", 
                time_buf, ExInfo->ExceptionRecord->ExceptionCode, 
                ExInfo->ExceptionRecord->ExceptionAddress);
        fclose(file);
    }
    
    // Başka exception handler var mı diye kontrol et
    return EXCEPTION_CONTINUE_SEARCH;
}

// Function to create the popup menu
HMENU create_popup_menu(void) {
    HMENU hPopup = CreatePopupMenu();
    if (!hPopup) return NULL;

    // Add Start/Stop buttons at the top
    AppendMenu(hPopup, MF_STRING, ID_TRAY_START, "Start");
    AppendMenu(hPopup, MF_STRING, ID_TRAY_STOP, "Stop");
    AppendMenu(hPopup, MF_SEPARATOR, 0, NULL);

    // Re-initialize scripts to ensure we have the latest data
    log_info("Re-initializing scripts before creating menu");
    scripts_free();  // Free existing scripts if any
    scripts_init();  // Re-initialize scripts

    // Count available scripts
    scripts_size = scripts_count();
    scripts = scripts_get_all();
    log_info("Menu creation - scripts count: %d", scripts_size);

    // Create categories submenu
    HMENU hTurkeyMenu = CreatePopupMenu();
    HMENU hOtherMenu = CreatePopupMenu();

    // Add script options to appropriate submenus
    int turkey_count = 0;
    int other_count = 0;
    
    // Track added script names to avoid duplicates
    char added_scripts[256][256] = {0};
    int added_count = 0;

    // Log the script names for debugging
    log_info("Creating menu with %d scripts", scripts_size);
    
    for (int i = 0; i < scripts_size && i < 256; i++) {
        char sanitized_name[256] = {0};
        
        // Ensure the script name is valid
        if (!scripts[i].name) {
            log_error("Script at index %d has NULL name", i);
            continue;
        }
        
        sanitize_name(scripts[i].name, sanitized_name, sizeof(sanitized_name));
        log_info("Processing script %d: %s", i, sanitized_name);
        
        // Check if this script name has already been added
        int is_duplicate = 0;
        for (int j = 0; j < added_count; j++) {
            if (strcmp(added_scripts[j], sanitized_name) == 0) {
                is_duplicate = 1;
                log_info("Script '%s' is a duplicate, skipping", sanitized_name);
                break;
            }
        }
        
        // Skip if duplicate
        if (is_duplicate) {
            continue;
        }
        
        // Add to the list of added scripts
        if (added_count < 256) {
            strncpy(added_scripts[added_count], sanitized_name, 255);
            added_scripts[added_count][255] = '\0'; // Ensure null termination
            
            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE | MIIM_DATA;
            mii.wID = ID_TRAY_SCRIPT_FIRST + added_count;
            mii.dwTypeData = sanitized_name;
            mii.dwItemData = (ULONG_PTR)i; // Store original script index as item data
            
            // Check if this script is currently selected
            BOOL is_selected = (i == last_selected_idx);
            mii.fState = is_selected ? MFS_CHECKED : MFS_UNCHECKED;
            
            // Add to Turkey menu if it contains TR or Turkey
            if (is_turkey_option(sanitized_name)) {
                InsertMenuItem(hTurkeyMenu, turkey_count++, TRUE, &mii);
                log_info("Added to Turkey menu: %s (ID=%d)", sanitized_name, mii.wID);
            } else {
                InsertMenuItem(hOtherMenu, other_count++, TRUE, &mii);
                log_info("Added to Other menu: %s (ID=%d)", sanitized_name, mii.wID);
            }
            
            added_count++;
        }
    }

    log_info("Menu creation complete - Turkey items: %d, Other items: %d", turkey_count, other_count);

    // Add submenus to main menu
    if (turkey_count > 0) {
        AppendMenu(hPopup, MF_POPUP, (UINT_PTR)hTurkeyMenu, "Turkey Options");
    } else {
        DestroyMenu(hTurkeyMenu);
    }

    if (other_count > 0) {
        AppendMenu(hPopup, MF_POPUP, (UINT_PTR)hOtherMenu, "Other Options");
    } else {
        DestroyMenu(hOtherMenu);
    }

    // Add separator and management options
    AppendMenu(hPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(hPopup, MF_STRING | (get_autostart_enabled() ? MF_CHECKED : MF_UNCHECKED), 
               ID_TRAY_AUTOSTART, "Run at startup");
    AppendMenu(hPopup, MF_STRING, ID_TRAY_STATUS, "Show status");
    AppendMenu(hPopup, MF_STRING, ID_TRAY_ABOUT, "About");
    AppendMenu(hPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(hPopup, MF_STRING, ID_TRAY_EXIT, "Exit");

    // Set proper item states
    if (is_running) {
        EnableMenuItem(hPopup, ID_TRAY_START, MF_GRAYED);
        EnableMenuItem(hPopup, ID_TRAY_STOP, MF_ENABLED);
    } else {
        EnableMenuItem(hPopup, ID_TRAY_START, MF_ENABLED);
        EnableMenuItem(hPopup, ID_TRAY_STOP, MF_GRAYED);
    }
    
    return hPopup;
}

// Function to update tray icon based on application state
static void update_tray_icon(HWND hwnd) {
    // Make sure the icons are loaded
    if (!hIconPaused) {
        hIconPaused = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_PAUSED));
    }
    if (!hIconRunning) {
        hIconRunning = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_RUNNING));
    }
    
    // Update the icon
    nid.hIcon = is_running ? hIconRunning : hIconPaused;
    // Update tooltip
    if (is_running && selected_script >= 0 && selected_script < scripts_size) {
        char buf[128];
        _snprintf(buf, sizeof(buf), "GoodbyeDPI - Running: %s", 
                 scripts[selected_script].name);
        _snprintf(nid.szTip, sizeof(nid.szTip), "%s", buf);
    } else {
        _snprintf(nid.szTip, sizeof(nid.szTip), "GoodbyeDPI - Stopped");
    }
    
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// Start the DPI bypass process
static void handle_start(void) {
    if (is_running) {
        return; // Already running
    }
    
    // Basic admin check
    if (!is_admin()) {
        show_balloon_notification("Error", 
                               "Administrator permissions required", 
                               NIIF_ERROR);
        restart_as_admin();
        return;
    }
    
    // Basic WinDivert check
    if (!check_and_copy_windivert_dll()) {
        show_balloon_notification("Error", 
                               "WinDivert.dll not found", 
                               NIIF_ERROR);
        return;
    }
    
    // Use default script if none selected
    if (selected_script < 0 || selected_script >= scripts_size) {
        selected_script = get_default_script_index();
        if (selected_script < 0) {
            show_balloon_notification("Error", 
                                  "No valid configuration found", 
                                  NIIF_ERROR);
            return;
        }
    }
    
    // Start the script - simple approach
    int result = script_run(selected_script);
    
    if (result == 0) {
        is_running = 1;
        start_time = GetTickCount();
        update_tray_icon(g_hwnd);
        save_selected_script(selected_script);
    }
    else {
        show_balloon_notification("Error", 
                                "Program could not be started", 
                                NIIF_ERROR);
    }
}

// Stop the DPI bypass process
static void handle_stop(void) {
    if (!is_running) {
        return; // Already stopped
    }

    // Simple stop - let script_stop handle everything
    script_stop();
    
    // Brief wait for cleanup
    Sleep(1000);
    
    // Update state
    is_running = 0;
    update_tray_icon(g_hwnd);
}

// Function to get the directory of the executable
static void get_executable_directory(char *path, size_t size) {
    GetModuleFileName(NULL, path, size);
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        *lastSlash = '\0'; // Truncate at last slash to get directory
    }
}

// Function to build config path
static void get_config_path(char *path, size_t size) {
    char exePath[MAX_PATH] = {0};
    get_executable_directory(exePath, MAX_PATH);
    _snprintf(path, size, "%s\\last_selected.dat", exePath);
}

// Save selected script to JSON config file (portable and safe)
static void save_selected_script(int index) {
    char configPath[MAX_PATH] = {0};
    char tempPath[MAX_PATH] = {0};
    FILE *logfile, *f;
    
    get_config_path(configPath, MAX_PATH);
    
    // Change extension to .json
    char *dot = strrchr(configPath, '.');
    if (dot) strcpy(dot, ".json");
    else strcat(configPath, ".json");
    
    snprintf(tempPath, MAX_PATH, "%s.tmp", configPath);
    
    // Write JSON to temporary file first
    f = fopen(tempPath, "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"selectedScript\": %d,\n", index);
        fprintf(f, "  \"version\": \"1.0\",\n");
        fprintf(f, "  \"lastUpdated\": \"%ld\"\n", time(NULL));
        fprintf(f, "}\n");
        fclose(f);
        
        // Atomic rename - replace original with temp file
        if (rename(tempPath, configPath) != 0) {
            logfile = fopen("goodbyedpi_error.log", "a");
            if (logfile) {
                fprintf(logfile, "Failed to rename JSON config file from %s to %s\n", tempPath, configPath);
                fclose(logfile);
            }
            remove(tempPath); // Clean up temp file on failure
        } else {
            logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "Saved script index %d to JSON config: %s\n", index, configPath);
                fclose(logfile);
            }
        }
    } else {
        logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "Failed to open JSON config file for writing: %s\n", tempPath);
            fclose(logfile);
        }
        
        // Fallback to simple binary file if JSON fails
        save_selected_script_file(index);
    }
}

// Fallback file save method
static void save_selected_script_file(int index) {
    char configPath[MAX_PATH] = {0};
    get_config_path(configPath, MAX_PATH);
    
    FILE *f = fopen(configPath, "wb");
    if (f) {
        fwrite(&index, sizeof(int), 1, f);
        fclose(f);
    }
}

// Load selected script from JSON config file (fallback to binary file)
static int load_selected_script() {
    char configPath[MAX_PATH] = {0};
    FILE *logfile, *f;
    int idx = -1;
    char line[256];
    
    get_config_path(configPath, MAX_PATH);
    
    // Change extension to .json
    char *dot = strrchr(configPath, '.');
    if (dot) strcpy(dot, ".json");
    else strcat(configPath, ".json");
    
    // Try to read JSON file
    f = fopen(configPath, "r");
    if (f) {
        // Simple JSON parsing - look for "selectedScript": number
        while (fgets(line, sizeof(line), f)) {
            char *script_pos = strstr(line, "\"selectedScript\":");
            if (script_pos) {
                script_pos += strlen("\"selectedScript\":");
                // Skip whitespace and colon
                while (*script_pos && (*script_pos == ' ' || *script_pos == '\t' || *script_pos == ':')) {
                    script_pos++;
                }
                idx = atoi(script_pos);
                break;
            }
        }
        fclose(f);
        
        // Verify the index is valid
        if (idx >= 0 && idx < scripts_count()) {
            logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "Loaded script index %d from JSON config\n", idx);
                fclose(logfile);
            }
            return idx;
        } else if (idx != -1) {
            logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "Invalid script index %d from JSON config, using default\n", idx);
                fclose(logfile);
            }
        }
    }
    
    // Fallback to binary file method
    return load_selected_script_file();
}

// Fallback file load method  
static int load_selected_script_file() {
    char configPath[MAX_PATH] = {0};
    FILE *logfile;
    int idx = -1;
    
    get_config_path(configPath, MAX_PATH);
    
    FILE *f = fopen(configPath, "rb");
    if (f) {
        size_t read_count = fread(&idx, sizeof(int), 1, f);
        fclose(f);
        
        if (read_count != 1) {
            logfile = fopen("goodbyedpi_error.log", "a");
            if (logfile) {
                fprintf(logfile, "Failed to read config file properly: %s\n", configPath);
                fclose(logfile);
            }
            idx = -1; // Reset to invalid
        }
        
        // Verify the index is valid
        if (idx < 0 || idx >= scripts_count()) {
            logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "Invalid script index %d from config file, using default\n", idx);
                fclose(logfile);
            }
            idx = find_default_script();
        }
    } else {
        // Set a default script if no saved selection
        logfile = fopen("goodbyedpi_log.txt", "a");
        if (logfile) {
            fprintf(logfile, "Config file not found: %s, using default script\n", configPath);
            fclose(logfile);
        }
        idx = find_default_script();
    }
    
    return idx;
}

// Improved function to check and copy WinDivert.dll
static BOOL check_and_copy_windivert_dll(void) {
    char exe_dir[MAX_PATH] = {0};
    char source_path[MAX_PATH] = {0};
    char target_path[MAX_PATH] = {0};
    DWORD attrs;
    FILE *logfile;

    // Log function entry
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "Checking WinDivert.dll...\n");
        fclose(logfile);
    }

    // Get executable directory
    get_executable_directory(exe_dir, MAX_PATH);

    // Check if WinDivert.dll exists next to the executable
    snprintf(target_path, MAX_PATH, "%s\\WinDivert.dll", exe_dir);
    attrs = GetFileAttributesA(target_path);
    
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        // DLL exists next to executable
        logfile = fopen("goodbyedpi_log.txt", "a");
        if (logfile) {
            fprintf(logfile, "WinDivert.dll found next to executable: %s\n", target_path);
            fclose(logfile);
        }
        return TRUE;
    }

    // DLL not found next to executable, try to copy from system architecture appropriate location
#ifdef _WIN64
    snprintf(source_path, MAX_PATH, "%s\\x86_64\\WinDivert.dll", exe_dir);
#else
    snprintf(source_path, MAX_PATH, "%s\\x86\\WinDivert.dll", exe_dir);
#endif

    attrs = GetFileAttributesA(source_path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        // Source DLL not found in subfolder
        logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "WinDivert.dll not found in expected subfolder: %s\n", source_path);
            fclose(logfile);
        }
        
        // Check also in binary folder as a fallback
#ifdef _WIN64
        snprintf(source_path, MAX_PATH, "%s\\..\\binary\\amd64\\WinDivert.dll", exe_dir);
#else
        snprintf(source_path, MAX_PATH, "%s\\..\\binary\\x86\\WinDivert.dll", exe_dir);
#endif
        
        attrs = GetFileAttributesA(source_path);
        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            logfile = fopen("goodbyedpi_error.log", "a");
            if (logfile) {
                fprintf(logfile, "WinDivert.dll not found in binary folder either: %s\n", source_path);
                fclose(logfile);
            }
            return FALSE;
        }
    }

    // Copy the DLL to executable directory
    if (!CopyFileA(source_path, target_path, FALSE)) {
        DWORD error = GetLastError();
        logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "Failed to copy WinDivert.dll from %s to %s, error %lu\n", 
                    source_path, target_path, error);
            fclose(logfile);
        }
        return FALSE;
    }

    // Successfully copied
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "Successfully copied WinDivert.dll from %s to %s\n", source_path, target_path);
        fclose(logfile);
    }
    return TRUE;
}

// Improved function to check for administrator privileges
static BOOL is_admin(void) {
    BOOL is_admin_user = FALSE;
    HANDLE token = NULL;
    TOKEN_ELEVATION elevation;
    DWORD size = sizeof(TOKEN_ELEVATION);
    FILE *logfile;

    // Log function entry
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "Checking administrator privileges...\n");
        fclose(logfile);
    }

    // Get current process token
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        DWORD error = GetLastError();
        logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "OpenProcessToken failed with error %lu\n", error);
            fclose(logfile);
        }
        return FALSE;
    }

    // Check token elevation
    if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
        is_admin_user = elevation.TokenIsElevated;
    }
    else {
        DWORD error = GetLastError();
        logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "GetTokenInformation failed with error %lu\n", error);
            fclose(logfile);
        }
    }

    CloseHandle(token);

    // Log result
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "Administrator check result: %s\n", is_admin_user ? "YES" : "NO");
        fclose(logfile);
    }

    return is_admin_user;
}

// Function to restart the application with admin privileges
static void restart_as_admin(void) {
    char exePath[MAX_PATH];
    SHELLEXECUTEINFO sei = {0};
    
    // Get the path of the current executable
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    // Log the restart attempt
    FILE *logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "Attempting to restart with administrator privileges\n");
        fclose(logfile);
    }
    
    // Setup the execution info to run with admin privileges
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpVerb = "runas";  // Request elevated permissions
    sei.lpFile = exePath;
    sei.nShow = SW_NORMAL;
    
    // Execute the process with elevated permissions
    if (!ShellExecuteEx(&sei)) {
        DWORD error = GetLastError();
        
        // If the error is ERROR_CANCELLED, the user declined the UAC prompt
        if (error == ERROR_CANCELLED) {
            MessageBox(NULL, 
                      "GoodbyeDPI requires administrator privileges to run.\n"
                      "Please restart the application and accept the UAC prompt.", 
                      "Administrator Rights Required", 
                      MB_ICONERROR);
        } else {
            char errorMsg[256];
            sprintf(errorMsg, "Failed to restart with administrator privileges.\nError code: %lu", error);
            MessageBox(NULL, errorMsg, "Error", MB_ICONERROR);
        }
        
        // Log the error
        logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "Failed to restart with administrator privileges. Error code: %lu\n", error);
            fclose(logfile);
        }
    }
    
    // Exit the current instance
    exit(1);
}

// Yeni fonksiyon: Yönetici ayrıcalıklarıyla WinDivert sürücüsünü kurma
static BOOL install_windivert_driver(void) {
    char szDriverPath[MAX_PATH];
    char szExePath[MAX_PATH];
    DWORD dwResult;
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    BOOL success = FALSE;
    FILE *logfile = NULL;
    
    // Log bilgisi
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "Attempting to install WinDivert driver...\n");
        fclose(logfile);
    }
    
    // Önce mevcut dizini al
    GetModuleFileName(NULL, szExePath, MAX_PATH);
    char *lastSlash = strrchr(szExePath, '\\');
    if (lastSlash) {
        *lastSlash = '\0'; // Truncate at last slash to get directory
    }
    
    // 32-bit sistem sürücü dosyası
    sprintf(szDriverPath, "%s\\WinDivert32.sys", szExePath);
    
    // Hizmet yöneticisini aç
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL) {
        dwResult = GetLastError();
        logfile = fopen("goodbyedpi_error.log", "a");
        if (logfile) {
            fprintf(logfile, "OpenSCManager failed: %lu\n", dwResult);
            fclose(logfile);
        }
        return FALSE;
    }
    
    // Hizmet zaten var mı kontrol et
    schService = OpenService(schSCManager, "WinDivert", SERVICE_QUERY_STATUS);
    if (schService) {
        // Hizmet zaten var
        SERVICE_STATUS status;
        if (QueryServiceStatus(schService, &status)) {
            if (status.dwCurrentState == SERVICE_RUNNING) {
                // Hizmet zaten çalışıyor
                logfile = fopen("goodbyedpi_log.txt", "a");
                if (logfile) {
                    fprintf(logfile, "WinDivert service is already running\n");
                    fclose(logfile);
                }
                success = TRUE;
            }
            else {
                // Hizmet var ama çalışmıyor, başlat
                if (StartService(schService, 0, NULL)) {
                    logfile = fopen("goodbyedpi_log.txt", "a");
                    if (logfile) {
                        fprintf(logfile, "WinDivert service started\n");
                        fclose(logfile);
                    }
                    success = TRUE;
                }
                else {
                    dwResult = GetLastError();
                    logfile = fopen("goodbyedpi_error.log", "a");
                    if (logfile) {
                        fprintf(logfile, "StartService failed: %lu\n", dwResult);
                        fclose(logfile);
                    }
                }
            }
        }
        CloseServiceHandle(schService);
    }
    else {
        // Hizmet yok, yeni oluştur
        schService = CreateService(
            schSCManager,             // SCManager veri tabanı
            "WinDivert",              // Hizmet adı
            "WinDivert Packet Divert Driver", // Görünen ad
            SERVICE_ALL_ACCESS,       // İstenen erişim
            SERVICE_KERNEL_DRIVER,    // Hizmet türü
            SERVICE_DEMAND_START,     // Başlatma türü
            SERVICE_ERROR_NORMAL,     // Hata kontrolü türü
            szDriverPath,             // Hizmet binary'sinin yolu
            NULL,                     // Yük sırası grubu yok
            NULL,                     // Yük sırası başlangıç değeri yok
            NULL,                     // Bağımlılık yok
            NULL,                     // LocalSystem hesabı
            NULL                      // Şifre yok
        );
        
        if (schService == NULL) {
            dwResult = GetLastError();
            logfile = fopen("goodbyedpi_error.log", "a");
            if (logfile) {
                fprintf(logfile, "CreateService failed: %lu\n", dwResult);
                fprintf(logfile, "Driver path: %s\n", szDriverPath);
                fclose(logfile);
            }
        }
        else {
            // Hizmeti başlat
            if (StartService(schService, 0, NULL)) {
                logfile = fopen("goodbyedpi_log.txt", "a");
                if (logfile) {
                    fprintf(logfile, "WinDivert service installed and started\n");
                    fclose(logfile);
                }
                success = TRUE;
            }
            else {
                dwResult = GetLastError();
                logfile = fopen("goodbyedpi_error.log", "a");
                if (logfile) {
                    fprintf(logfile, "StartService failed: %lu\n", dwResult);
                    fprintf(logfile, "Driver path: %s\n", szDriverPath);
                    fclose(logfile);
                }
            }
            CloseServiceHandle(schService);
        }
    }
    
    CloseServiceHandle(schSCManager);
    return success;
}

// Function to build menu
HMENU build_menu(void) {
    // Try to create the menu without exception handling
    HMENU menu = create_popup_menu();
    return menu;
}

// Handle tray events 
static void handle_tray_event(HWND hwnd, LPARAM lParam) {
    if (lParam == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(hwnd);
        if (hPopupMenu) DestroyMenu(hPopupMenu);
        hPopupMenu = create_popup_menu();
        TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        PostMessage(hwnd, WM_NULL, 0, 0);
    }
}

// Window procedure for handling messages
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_hwnd = hwnd;
            
            // Log dosyalarını temizle - her derleme/çalıştırmada sıfırlar
            FILE *clearLog = fopen("goodbyedpi_log.txt", "w");
            if (clearLog) {
                fprintf(clearLog, "=== Log started - %s ===\n", __DATE__);
                fclose(clearLog);
            }
            
            clearLog = fopen("goodbyedpi_error.log", "w");
            if (clearLog) {
                fprintf(clearLog, "=== Error log started - %s ===\n", __DATE__);
                fclose(clearLog);
            }
            
            // Initialize scripts first
            scripts_init();
            
            // Log başlat - debug amaçlı
            FILE *logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "[GUI] WM_CREATE: Initialized scripts, count=%d\n", scripts_count());
                fclose(logfile);
            }
            
            // Scriptleri al ve script sayısını güncelle
            scripts = scripts_get_all();
            scripts_size = scripts_count();
            
            // Debug log
            logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "[GUI] WM_CREATE: Got scripts, scripts_size=%d\n", scripts_size);
                fclose(logfile);
            }
            
            // Load icons
            hIconPaused = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_PAUSED));
            hIconRunning = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_RUNNING));
            
            // Initialize tray icon
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = ID_TRAY;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_TRAY;
            nid.hIcon = hIconPaused;
            strcpy(nid.szTip, "GoodbyeDPI - Stopped");
            
            Shell_NotifyIcon(NIM_ADD, &nid);
            tray_added = 1;
            
            // Önce kaydedilmiş script indeksini yükle
            last_selected_idx = load_selected_script();
            
            // Debug log
            logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "[GUI] WM_CREATE: Loaded selected script index=%d\n", last_selected_idx);
                fclose(logfile);
            }
            
            // Eğer kaydedilmiş script bulunamadıysa varsayılanı kullan
            if (last_selected_idx < 0 || last_selected_idx >= scripts_size) {
                last_selected_idx = get_default_script_index();
                if (last_selected_idx >= 0) {
                    save_selected_script(last_selected_idx);
                    // Debug log
                    logfile = fopen("goodbyedpi_log.txt", "a");
                    if (logfile) {
                        fprintf(logfile, "[GUI] WM_CREATE: Using default script index=%d\n", last_selected_idx);
                        fclose(logfile);
                    }
                }
            }
            
            // selected_script değişkenini güncelle
            selected_script = last_selected_idx;
            
            // Debug log
            logfile = fopen("goodbyedpi_log.txt", "a");
            if (logfile) {
                fprintf(logfile, "[GUI] WM_CREATE: Set selected_script=%d\n", selected_script);
                fclose(logfile);
            }

            // Show initial notification in English instead of Turkish
            nid.uFlags |= NIF_INFO;
            _snprintf(nid.szInfoTitle, sizeof(nid.szInfoTitle), "GoodbyeDPI");
            _snprintf(nid.szInfo, sizeof(nid.szInfo), "Program is running in system tray");
            nid.dwInfoFlags = NIIF_INFO;
            nid.uTimeout = 3000; // 3 seconds
            Shell_NotifyIcon(NIM_MODIFY, &nid);
            
            // Set auto-start flag and start the auto-start timer
            auto_start_pending = TRUE;
            SetTimer(hwnd, IDT_AUTO_START, AUTO_START_DELAY, NULL);
            
            return 0;
            
        case WM_TRAY:
            handle_tray_event(hwnd, lParam);
            return 0;
            
        case WM_COMMAND:
            {
                // Check if command is from script selection
                if (LOWORD(wParam) >= ID_TRAY_SCRIPT_FIRST && 
                    LOWORD(wParam) < ID_TRAY_SCRIPT_FIRST + scripts_size) {
                    
                    int script_idx = LOWORD(wParam) - ID_TRAY_SCRIPT_FIRST;
                    
                    // Update check marks in menu
                    if (last_selected_idx >= 0 && last_selected_idx < scripts_size) {
                        CheckMenuItem(hPopupMenu, ID_TRAY_SCRIPT_FIRST + last_selected_idx, 
                                    MF_BYCOMMAND | MF_UNCHECKED);
                    }
                    
                    CheckMenuItem(hPopupMenu, LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
                    last_selected_idx = script_idx;
                    selected_script = script_idx; // Set selected_script when selecting from menu
                    save_selected_script(script_idx);
                    
                    // If already running, stop first
                    if (is_running) {
                        log_info("Script changed while running. Stopping current script and will restart with new script");
                        
                        // Disable further menu interactions during script switch
                        if (hPopupMenu) {
                            for (int i = 0; i < scripts_size; i++) {
                                EnableMenuItem(hPopupMenu, ID_TRAY_SCRIPT_FIRST + i, MF_GRAYED);
                            }
                            EnableMenuItem(hPopupMenu, ID_TRAY_START, MF_GRAYED);
                            EnableMenuItem(hPopupMenu, ID_TRAY_STOP, MF_GRAYED);
                        }
                        
                        // Stop the current script
                        handle_stop();
                        
                        // Set flag and timer for restart with new script
                        script_switch_pending = TRUE;
                        SetTimer(hwnd, IDT_AUTO_START, AUTO_SCRIPT_SWITCH_DELAY, NULL);
                    } 
                    else {
                        // Not running - start automatically when user selects a script
                        log_info("Script selected while paused, starting automatically");
                        handle_start();
                    }
                } else {
                    switch (LOWORD(wParam)) {
                        case ID_TRAY_START:
                            handle_start();
                            break;
                            
                        case ID_TRAY_STOP:
                            handle_stop();
                            break;
                            
                        case ID_TRAY_EXIT:
                            DestroyWindow(hwnd);
                            break;
                            
                        case ID_TRAY_AUTOSTART:
                            {
                                BOOL current = get_autostart_enabled();
                                set_autostart(!current);
                                CheckMenuItem(hPopupMenu, ID_TRAY_AUTOSTART, 
                                            MF_BYCOMMAND | (get_autostart_enabled() ? MF_CHECKED : MF_UNCHECKED));
                            }
                            break;
                            
                        case ID_TRAY_STATUS:
                            show_status_dialog(hwnd);
                            break;
                            
                        case ID_TRAY_ABOUT:
                            show_about_dialog(hwnd);
                            break;
                    }
                }
                return 0;
            }
            break;
        
        case WM_APP_RESTART:
            // Handle restart
            if (is_running) {
                handle_stop();
                Sleep(1000); // Wait for process to terminate
                handle_start();
            }
            return 0;

        case WM_TIMER:
            if (wParam == IDT_AUTO_START) {
                // Auto-start timer yönetimi
                KillTimer(hwnd, IDT_AUTO_START);
                
                if (auto_start_pending) {
                    auto_start_pending = FALSE;
                    log_info("Auto-start timer fired, starting the program automatically");
                    handle_start();
                }
                else if (script_switch_pending) {
                    script_switch_pending = FALSE;
                    log_info("Script switch timer fired, starting the program with new script");
                    
                    // Extra safety check - ensure we're not already running
                    if (!is_running && !check_goodbyedpi_process()) {
                        handle_start();
                    } else {
                        log_error("Script switch timer fired but process is still running - aborting restart");
                        // Re-enable menu items if restart fails
                        if (hPopupMenu) {
                            for (int i = 0; i < scripts_size; i++) {
                                EnableMenuItem(hPopupMenu, ID_TRAY_SCRIPT_FIRST + i, MF_ENABLED);
                            }
                            EnableMenuItem(hPopupMenu, ID_TRAY_START, MF_ENABLED);
                            EnableMenuItem(hPopupMenu, ID_TRAY_STOP, MF_GRAYED);
                        }
                    }
                }
            }
            else if (wParam == IDT_WATCHDOG) {
                // Watchdog timer yönetimi
                log_info("Watchdog timer fired, checking if the program is still running");
                
                if (is_running && !check_goodbyedpi_process()) {
                    // GoodbyeDPI çalışır görünüyor ancak aslında çalışmıyor
                    log_error("Watchdog detected: GoodbyeDPI is not running but flag is set");
                    is_running = 0;
                    update_tray_icon(hwnd);
                    show_balloon_notification("Error", 
                                           "GoodbyeDPI has stopped unexpectedly", 
                                           NIIF_ERROR);
                }
            }
            return 0;
            
        case WM_DESTROY:
            // Clean up tray icon
            if (tray_added) {
                Shell_NotifyIcon(NIM_DELETE, &nid);
                tray_added = 0;
            }
            
            // Clean up menus
            if (hPopupMenu) {
                DestroyMenu(hPopupMenu);
                hPopupMenu = NULL;
            }
            
            // Stop running process
            if (is_running) {
                handle_stop();
            }
            
            // Clean up icons
            if (hIconPaused) {
                DestroyIcon(hIconPaused);
                hIconPaused = NULL;
            }
            if (hIconRunning) {
                DestroyIcon(hIconRunning);
                hIconRunning = NULL;
            }
            
            // Release mutex
            if (hMutex) {
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
                hMutex = NULL;
            }
            
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Log dosyalarını temizleyen yeni fonksiyon
static void clear_log_files(void) {
    // Log dosyalarını temizle
    FILE *clearLog = fopen("goodbyedpi_log.txt", "w");
    if (clearLog) {
        fprintf(clearLog, "=== Log started - %s ===\n", __DATE__);
        fclose(clearLog);
    }
    
    clearLog = fopen("goodbyedpi_error.log", "w");
    if (clearLog) {
        fprintf(clearLog, "=== Error log started - %s ===\n", __DATE__);
        fclose(clearLog);
    }
}

// Program entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Log başlangıç bilgisi ekle, temizlemeden
    FILE *logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "\n=== Program started - %s ===\n", __DATE__);
        fclose(logfile);
    }
    
    // Check for previous instance
    hMutex = CreateMutex(NULL, TRUE, "GoodbyeDPI_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Silently exit without showing warning message
        return 1;
    }
    
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
    wc.lpszClassName = "GoodbyeDPIClass";
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window registration failed.", "GoodbyeDPI", MB_ICONERROR | MB_OK);
        return 1;
    }
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icc = {0};
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    
    // Create window (hidden)
    HWND hwnd = CreateWindowEx(
        0, "GoodbyeDPIClass", "GoodbyeDPI",
        WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd) {
        MessageBox(NULL, "Window creation failed.", "GoodbyeDPI", MB_ICONERROR | MB_OK);
        return 1;
    }
    
    // Initialize scripts first
    scripts_init();
    
    // Try to load previously selected script index
    last_selected_idx = load_selected_script();
    
    // Auto-enable startup on first run if not already set
    if (!get_autostart_enabled()) {
        set_autostart(TRUE);
        
        // Log this action
        FILE *logfile = fopen("goodbyedpi_log.txt", "a");
        if (logfile) {
            fprintf(logfile, "Auto-enabled 'Run at startup' on first run\n");
            fclose(logfile);
        }
    }
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Clean up scripts
    scripts_free();
    
    return (int)msg.wParam;
}

// Implementation of auxiliary functions
static void terminate_goodbyedpi() {
    log_info("Force terminating GoodbyeDPI processes");
    
    // Don't call script_stop again - it was already called
    // Just force kill any remaining processes
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32 = {0};
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        if (Process32First(hSnapshot, &pe32)) {
            do {
                // Look for goodbyedpi.exe processes
                if (strcmpi(pe32.szExeFile, "goodbyedpi.exe") == 0) {
                    log_info("Found goodbyedpi.exe process with PID %lu, terminating", pe32.th32ProcessID);
                    
                    // Open the process and terminate it
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess) {
                        if (TerminateProcess(hProcess, 1)) {
                            log_info("Process terminated successfully");
                        } else {
                            log_error("Failed to terminate process, error: %lu", GetLastError());
                        }
                        CloseHandle(hProcess);
                    } else {
                        log_error("Failed to open process for termination, error: %lu", GetLastError());
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    } else {
        log_error("Failed to create process snapshot for termination");
    }
}

// Show status dialog
static void show_status_dialog(HWND hwnd) {
    char message[512];
    if (is_running && selected_script >= 0 && selected_script < scripts_size) {
        DWORD elapsed = GetTickCount() - start_time;
        int hours = elapsed / (1000 * 60 * 60);
        int minutes = (elapsed % (1000 * 60 * 60)) / (1000 * 60);
        int seconds = (elapsed % (1000 * 60)) / 1000;
        
        _snprintf(message, sizeof(message), 
                 "GoodbyeDPI Status\n\n"
                 "Status: Running\n"
                 "Configuration: %s\n"
                 "Running time: %02d:%02d:%02d",
                 scripts[selected_script].name, hours, minutes, seconds);
    } else {
        _snprintf(message, sizeof(message), 
                 "GoodbyeDPI Status\n\n"
                 "Status: Stopped");
    }
    
    MessageBox(hwnd, message, "GoodbyeDPI Status", MB_ICONINFORMATION | MB_OK);
}

// Show about dialog
static void show_about_dialog(HWND hwnd) {
    MessageBox(hwnd, 
               "GoodbyeDPI GUI\n"
               "Version 2.3.14 - Working WinDivert Link\n\n"
               "GitHub/TunahanDilercan\n\n"
               "GitHub/ValdikSS/GoodbyeDPI\n\n"
               "DPI circumvention utility\n\n"
               "Fixed WinDivert library linking for DPI bypass",
               "About GoodbyeDPI", MB_ICONINFORMATION | MB_OK);
}

// Implementation of prompt_iteration_continue function
BOOL prompt_iteration_continue(HWND hwnd) {
    // Safety check for hwnd
    if (!hwnd || !IsWindow(hwnd)) {
        // Use main window handle if available, or NULL as fallback
        hwnd = g_hwnd ? g_hwnd : NULL;
    }
    
    int result = MessageBox(hwnd, 
                  "The current operation has completed one iteration.\n\nContinue to iterate?",
                  "GoodbyeDPI", 
                  MB_YESNO | MB_ICONQUESTION);
    
    // Return TRUE if Yes was clicked
    return (result == IDYES);
}

// Show balloon notification
static void show_balloon_notification(const char* title, const char* message, DWORD info_flags) {
    // Bildirim gösterme süresi 5 saniye olarak ayarlandı (daha önce 10 saniye idi)
    static DWORD lastNotificationTime = 0;
    DWORD currentTime = GetTickCount();
    
    // En az 3 saniye geçmediyse yeni bildirim gösterme
    if (lastNotificationTime != 0 && (currentTime - lastNotificationTime < 3000)) {
        // Çok sık bildirim göstermekten kaçınmak için atla
        return;
    }
    
    nid.uFlags |= NIF_INFO;
    _snprintf(nid.szInfoTitle, sizeof(nid.szInfoTitle), "%s", title);
    _snprintf(nid.szInfo, sizeof(nid.szInfo), "%s", message);
    nid.dwInfoFlags = info_flags;
    nid.uTimeout = 5000;  // 5 saniye
    
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    lastNotificationTime = currentTime;
}

// Function to check if application is set to run at startup
static BOOL get_autostart_enabled(void) {
    HKEY hKey;
    LONG result;
    char registryPath[MAX_PATH] = {0};
    char currentPath[MAX_PATH] = {0};
    DWORD type, size = sizeof(registryPath);
    BOOL enabled = FALSE;
    
    // Get current executable path
    GetModuleFileName(NULL, currentPath, MAX_PATH);
    
    // Check if registry key exists
    result = RegOpenKeyEx(HKEY_CURRENT_USER, 
                           "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                           0, KEY_READ | KEY_WRITE, &hKey);
    
    if (result == ERROR_SUCCESS) {
        // Try to get the value
        result = RegQueryValueEx(hKey, "GoodbyeDPI", NULL, &type, 
                                (BYTE*)registryPath, &size);
        
        // If value exists and is a string
        if (result == ERROR_SUCCESS && type == REG_SZ) {
            enabled = TRUE;
            
            // Check if the registered path is different from current path
            if (strcmp(registryPath, currentPath) != 0) {
                // Update registry with current path
                result = RegSetValueEx(hKey, "GoodbyeDPI", 0, REG_SZ, 
                                      (BYTE*)currentPath, strlen(currentPath) + 1);
                
                // Log the path update
                FILE *logfile = fopen("goodbyedpi_log.txt", "a");
                if (logfile) {
                    fprintf(logfile, "Updated autostart path from '%s' to '%s'\n", 
                            registryPath, currentPath);
                    fclose(logfile);
                }
            }
        }
        
        RegCloseKey(hKey);
    }
    
    return enabled;
}

// Function to set or remove autostart setting
static BOOL set_autostart(BOOL enable) {
    HKEY hKey;
    LONG result;
    char exePath[MAX_PATH] = {0};
    
    // Open the registry key
    result = RegOpenKeyEx(HKEY_CURRENT_USER, 
                           "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                           0, KEY_WRITE, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return FALSE;
    }
    
    if (enable) {
        // Get path of current executable
        GetModuleFileName(NULL, exePath, MAX_PATH);
        
        // Add application to autostart by creating registry value
        result = RegSetValueEx(hKey, "GoodbyeDPI", 0, REG_SZ, 
                              (BYTE*)exePath, strlen(exePath) + 1);
    }
    else {
        // Remove application from autostart by deleting registry value
        result = RegDeleteValue(hKey, "GoodbyeDPI");
    }
    
    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

// Function to check if GoodbyeDPI process is still running
static BOOL check_goodbyedpi_process(void) {
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    BOOL found = FALSE;
    
    // Take a snapshot of all processes
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        log_error("Failed to create process snapshot");
        return FALSE;
    }
    
    // Set the size of the structure before using it
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    // Get the first process
    if (Process32First(hSnapshot, &pe32)) {
        do {
            // Check if this is the GoodbyeDPI process - could be named differently based on compilation
            // This checks for both the main executable and any child processes
            if (strcmpi(pe32.szExeFile, "goodbyedpi.exe") == 0) {
                found = TRUE;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return found;
}