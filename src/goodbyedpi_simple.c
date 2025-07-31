/*
 * GoodbyeDPI — Passive DPI blocker and Active DPI circumvention utility.
 * Simplified version for working DPI functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <in6addr.h>
#include <ws2tcpip.h>
#include "windivert.h"
#include "goodbyedpi.h"
#include "utils/repl_str.h"
#include "service.h"
#include "dnsredir.h"
#include "ttltrack.h"
#include "blackwhitelist.h"
#include "fakepackets.h"
#include <windows.h>
#include <process.h>

// Basit state yönetimi
static volatile int exiting = 0;
static volatile int goodbyedpi_running = 0;
static HANDLE goodbyedpi_thread = NULL;

// Basit WinDivert dosya bulma fonksiyonu
static BOOL find_windivert_files(char *dll_path, char *sys_path, size_t path_size) {
    char exe_dir[MAX_PATH] = {0};
    char temp_path[MAX_PATH] = {0};
    
    if (!GetModuleFileNameA(NULL, exe_dir, MAX_PATH)) {
        return FALSE;
    }
    
    char *last_slash = strrchr(exe_dir, '\\');
    if (last_slash) {
        *last_slash = '\0';
    }
    
    // Aynı dizinde ara
    snprintf(temp_path, path_size, "%s\\WinDivert.dll", exe_dir);
    if (GetFileAttributesA(temp_path) != INVALID_FILE_ATTRIBUTES) {
        strncpy(dll_path, temp_path, path_size);
        snprintf(sys_path, path_size, "%s\\WinDivert64.sys", exe_dir);
        return TRUE;
    }
    
    return FALSE;
}

// Basit WinDivert kontrol
BOOL check_windivert_driver(void)
{
    char dll_path[MAX_PATH] = {0};
    char sys_path[MAX_PATH] = {0};
    
    return find_windivert_files(dll_path, sys_path, MAX_PATH);
}

// Basit DPI thread
static unsigned __stdcall dpi_thread(void *arg)
{
    char **argv = (char**)arg;
    int argc = 0;
    
    // Argüman sayısını say
    if (argv) {
        while (argv[argc] && argc < 50) ++argc;
    }
    
    if (argc == 0) {
        goodbyedpi_running = 0;
        return -1;
    }
    
    // Ana DPI fonksiyonunu çalıştır
    goodbyedpi_main(argc, argv);
    
    // Temizlik
    goodbyedpi_running = 0;
    
    // Hafıza temizliği
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            if (argv[i]) free(argv[i]);
        }
        free(argv);
    }
    
    return 0;
}

// Basit başlatma fonksiyonu
int start_goodbyedpi(int argc, char **argv)
{
    if (goodbyedpi_running) {
        return -1; // Zaten çalışıyor
    }
    
    // WinDivert kontrolü
    if (!check_windivert_driver()) {
        MessageBoxA(NULL, "WinDivert başlatılamadı! Lütfen:\n\n1. Programı yönetici olarak çalıştırın\n2. Test modunu etkinleştirin", "Hata", MB_OK | MB_ICONERROR);
        return -4;
    }

    // Argümanları kopyala
    char **copy = malloc((argc + 1) * sizeof(char*));
    if (!copy) return -2;
    
    for (int i = 0; i < argc; ++i) {
        copy[i] = _strdup(argv[i]);
        if (!copy[i]) {
            for (int j = 0; j < i; ++j) free(copy[j]);
            free(copy);
            return -2;
        }
    }
    copy[argc] = NULL;
    
    // Thread başlat
    exiting = 0;
    goodbyedpi_running = 1;
    
    goodbyedpi_thread = (HANDLE)_beginthreadex(NULL, 0, dpi_thread, copy, 0, NULL);
    if (!goodbyedpi_thread) {
        goodbyedpi_running = 0;
        for (int i = 0; i < argc; ++i) free(copy[i]);
        free(copy);
        return -3;
    }
    
    return 0;
}

// Basit durdurma fonksiyonu
void stop_goodbyedpi(void)
{
    if (!goodbyedpi_running || !goodbyedpi_thread) {
        goodbyedpi_running = 0;
        return;
    }

    exiting = 1;
    deinit_all();
    
    // Thread'in bitmesini bekle
    WaitForSingleObject(goodbyedpi_thread, 3000);
    CloseHandle(goodbyedpi_thread);
    
    goodbyedpi_thread = NULL;
    goodbyedpi_running = 0;
    exiting = 0;
}

int is_goodbyedpi_running(void) { 
    return goodbyedpi_running; 
}

// Orijinal goodbyedpi_main fonksiyonu başlangıcı aynı kalacak...
// (Geri kalan kod aynı)
