#include <windows.h>
#include <shellapi.h>
#include "goodbyedpi.h"
#include "scripts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h> // Başlangıç klasörü için
#include <uxtheme.h> // Tema kontrolü için
#include <vssym32.h> // Tema sembolleri
#include <commctrl.h> // Common controls

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
// İkon ID'leri RC dosyası ile uyumlu olarak güncellendi
#define IDI_ICON_MAIN    101  // Ana ikon (EXE)
#define IDI_ICON_RUNNING 102  // Çalışırken ikon
#define IDI_ICON_PAUSED  103  // Durdurulmuşken ikon

#define WM_APP_RESTART (WM_APP+100) // Uygulamanın yeniden başlatılması için kullanılacak özel mesaj
#define WM_APP_BALLOON_CLICK (WM_APP+101) // Balon bildirimine tıklandığında

// Watchdog zamanlayıcısı ID'si
#define IDT_WATCHDOG 1001
#define WATCHDOG_INTERVAL 10000 // 10 saniye

// Varsayılan script ismi, otomatik olarak seçilecek
#define DEFAULT_SCRIPT_NAME "[TR] Alternative1"

static NOTIFYICONDATA nid = {0};
static HMENU hMenu;
static int last_selected_idx = -1; // Son seçilen script'in index'i
static int is_running = 0; // Programın çalışıp çalışmadığını takip eder
static int error_recovery = 0; // Hata kurtarma modu
static HANDLE hMutex = NULL; // Birden fazla kopya çalışmasını engellemek için mutex
static int tray_added = 0; // Tray simgesinin eklenip eklenmediğini takip eder
static HICON hIconPaused = NULL; // Durdurulmuş ikon
static HICON hIconRunning = NULL; // Çalışıyor ikon

// Sanitize name - menüde görünecek script isimleri için kullanılır
static void sanitize_name(const char *src, char *dst, size_t dst_size) {
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

// Bir string'in "Turkey" içerdiğini kontrol et (başlangıç default seçimi için)
static int is_turkey_option(const char *name) {
    return (name && (strstr(name, "Turkey") || strstr(name, "turkey") || 
                     strstr(name, "TR") || strstr(name, "[TR]")));
}

// Belirtilen script adını script listesinde ara
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

// Varsayılan TR Alternatif1 script'ini bul
static int find_default_script(void) {
    int idx = find_script_by_name(DEFAULT_SCRIPT_NAME);
    if (idx >= 0) return idx;
    
    // DEFAULT_SCRIPT_NAME bulunamazsa herhangi bir Turkey seçeneğini ara
    int count = scripts_count();
    for (int i = 0; i < count; i++) {
        const char *name = script_get(i)->name;
        if (is_turkey_option(name)) {
            return i;
        }
    }
    
    // TR seçeneği de bulunamazsa ilk script'i döndür
    return (scripts_count() > 0) ? 0 : -1;
}

// Beklenmedik hatalar için global exception handler
// Windows ile çakışmayı önlemek için isim değiştirildi
LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    // Hata bilgisini loglama
    FILE *f = fopen("goodbyedpi_error.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        fprintf(f, "[%02d/%02d/%04d %02d:%02d:%02d] Exception code: 0x%08X\n", 
                st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond,
                exceptionInfo->ExceptionRecord->ExceptionCode);
        fclose(f);
    }
    
    // GoodbyeDPI servisini durdurma - güvenlik için
    script_stop();
    
    // Yeniden başlatma
    char exe_path[MAX_PATH];
    GetModuleFileName(NULL, exe_path, sizeof(exe_path));
    
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    // Uygulamayı kurtarma moduyla yeniden başlat
    char cmdline[MAX_PATH + 20] = {0};
    sprintf(cmdline, "\"%s\" -recovery", exe_path);
    
    if (CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    
    // Bu process'i sonlandır
    return EXCEPTION_EXECUTE_HANDLER;
}

// Windows kendi exception handler'ını kullansın
static void setup_exception_handler(void) {
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
}

// Tray simgesini ekleme fonksiyonu iyileştirelim
BOOL tray_add(HWND hWnd) {
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_PAUSED));
    lstrcpy(nid.szTip, "GoodbyeDPI (Durduruldu)");
    
    BOOL result = Shell_NotifyIcon(NIM_ADD, &nid);
    if (!result) {
        DWORD error = GetLastError();
        char errorMsg[256];
        sprintf(errorMsg, "Sistem tepsisi simgesi eklenirken hata: %lu", error);
        MessageBox(hWnd, errorMsg, "Sistem Tepsisi Hatası", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    tray_added = 1;
    return TRUE;
}

// Tray simgesini kaldıran fonksiyon
BOOL tray_delete() {
    BOOL result = Shell_NotifyIcon(NIM_DELETE, &nid);
    if (!result) {
        // Sessizce başarısız ol - uygulamayı kapatırken önemli değil
        return FALSE;
    }
    tray_added = 0;
    return TRUE;
}

// Tray simgesini güncelleyen fonksiyon
void tray_update(int running) {
    if (running) {
        nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_RUNNING));
        lstrcpy(nid.szTip, "GoodbyeDPI (Çalışıyor)");
    } else {
        nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_PAUSED));
        lstrcpy(nid.szTip, "GoodbyeDPI (Durduruldu)");
    }
    
    BOOL result = Shell_NotifyIcon(NIM_MODIFY, &nid);
    if (!result) {
        // Sistem tepsisi simgesi kaybolduysa, yeniden ekle
        Shell_NotifyIcon(NIM_ADD, &nid);
    }
}

// Windows'la başlatma için Başlangıç klasörüne kısayol oluştur (Registry yerine)
static void set_autostart(void) {
    char exe_path[MAX_PATH];
    char startup_path[MAX_PATH];
    char shortcut_path[MAX_PATH];
    
    // Exe dosyasının tam yolunu al
    GetModuleFileName(NULL, exe_path, sizeof(exe_path));
    
    // Başlangıç klasörünü al
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startup_path))) {
        // Kısayol yolunu oluştur
        sprintf(shortcut_path, "%s\\GoodbyeDPI_GUI.lnk", startup_path);
        
        // Kısayol dosyası oluştur (IShellLink ve IPersistFile kullanarak)
        IShellLink* psl;
        if (SUCCEEDED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                                     &IID_IShellLink, (LPVOID*)&psl))) {
            IPersistFile* ppf;
            
            psl->lpVtbl->SetPath(psl, exe_path);
            psl->lpVtbl->SetDescription(psl, "GoodbyeDPI GUI");
            
            if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf))) {
                WCHAR wsz[MAX_PATH];
                
                // ANSI'den UNICODE'a dönüştür
                MultiByteToWideChar(CP_ACP, 0, shortcut_path, -1, wsz, MAX_PATH);
                
                // Kısayol dosyasını kaydet
                ppf->lpVtbl->Save(ppf, wsz, TRUE);
                
                ppf->lpVtbl->Release(ppf);
            }
            
            psl->lpVtbl->Release(psl);
        }
    }
}

// Otomatik başlatmayı kaldır (kısayolu sil)
static void remove_autostart(void) {
    char startup_path[MAX_PATH];
    char shortcut_path[MAX_PATH];
    
    // Başlangıç klasörünü al
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startup_path))) {
        // Kısayol yolunu oluştur
        sprintf(shortcut_path, "%s\\GoodbyeDPI_GUI.lnk", startup_path);
        
        // Dosya varsa sil
        DeleteFile(shortcut_path);
    }
}

// Service_remove.cmd scriptini yönetici olarak çalıştır
static void run_service_remove_script(void) {
    char script_path[MAX_PATH];
    char exe_path[MAX_PATH];
    char *p;
    
    // Exe dosyasının tam yolunu al
    GetModuleFileName(NULL, exe_path, sizeof(exe_path));
    
    // Exe yolundan .exe kısmını sil
    p = strrchr(exe_path, '\\');
    if (p) {
        *p = '\0';
        
        // Bir üst dizine git (src'den projenin ana dizinine)
        p = strrchr(exe_path, '\\');
        if (p) {
            *p = '\0';
            
            // Service_remove.cmd yolunu oluştur
            snprintf(script_path, sizeof(script_path), "%s\\scrpiler\\service_remove.cmd", exe_path);
            
            // UAC yükseltme ile çalıştır (yönetici haklarıyla)
            ShellExecute(NULL, "runas", script_path, NULL, NULL, SW_HIDE); // SW_HIDE ile gizle
        }
    }
}

// Menü özelleştirme fonksiyonu
static void customize_menu_appearance(HMENU menu) {
    if (!menu) return;
    
    // Burada menü renklerini veya diğer görünüm ayarlarını yapabilirsiniz
    // Windows 10/11'de bu genellikle gerekli değil ama sisteme göre özelleştirilebilir
}

// Hata oluştuğunda loglama yap
static void log_error(const char* message) {
    FILE* f = fopen("goodbyedpi_error.log", "a");
    if (f) {
        time_t now;
        struct tm timeinfo;
        char time_buf[64];
        
        time(&now);
        localtime_s(&timeinfo, &now);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        fprintf(f, "[%s] ERROR: %s\n", time_buf, message);
        fclose(f);
    }
}

// Birleştirilmiş "devam et" mesajı için yardımcı fonksiyon
static int show_continue_dialog(HWND hwnd) {
    return MessageBox(hwnd, 
        "DPI atlatma zaten çalışıyor. Yeni iterasyonla devam edilsin mi?", 
        "GoodbyeDPI", 
        MB_YESNO | MB_ICONQUESTION);
}

static void restart_with_script(int script_idx) {
    if (script_idx < 0 || script_idx >= scripts_count()) return;
    
    // Çalışan işlemi durdur
    script_stop();
    // Küçük bir gecikme ekle
    Sleep(500);
    // Yeni script ile başlat
    script_start(script_idx);
    // UI durumunu güncelle
    is_running = 1;
}

// Balon bildirimi gösterme fonksiyonu
static void show_balloon_tip(HWND hwnd, const char* title, const char* message, DWORD icon_type) {
    if (!tray_added) return;
    
    // Balon bildirimi özelliğini ekle
    nid.uFlags |= NIF_INFO;
    lstrcpy(nid.szInfoTitle, title);
    lstrcpy(nid.szInfo, message);
    nid.dwInfoFlags = icon_type; // NIIF_INFO, NIIF_WARNING, NIIF_ERROR
    nid.uTimeout = 10000; // 10 saniye
    
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    
    // Flags'i eski haline getir
    nid.uFlags &= ~NIF_INFO;
}

// Durum gösterme fonksiyonu
static void show_status_window(HWND parent) {
    // Basit bir durum penceresi oluştur
    HWND status_window = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "STATIC",
        "GoodbyeDPI Durum Bilgisi",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        100, 100, 350, 200,
        parent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!status_window) return;
    
    // Çalışma durumu metni
    char status_text[512];
    sprintf(status_text, "DPI Bypass Durumu: %s\n\n", 
            is_running ? "ÇALIŞIYOR" : "DURDURULDU");
    
    // Eğer çalışıyorsa, hangi mod kullanıldığını ekle
    if (is_running && last_selected_idx >= 0) {
        const char* script_name = script_get(last_selected_idx)->name;
        char display_name[128];
        sanitize_name(script_name, display_name, sizeof(display_name));
        
        strcat(status_text, "Aktif Mod: ");
        strcat(status_text, display_name);
        strcat(status_text, "\n\n");
        
        // Çalışma süresi eklenebilir (gelecek sürüm için)
        // ...
        
        // İstatistikler eklenebilir (gelecek sürüm için)
        strcat(status_text, "İstatistikler gelecek sürümde eklenecektir.");
    }
    
    // Metin kontrolü ekle
    HWND status_text_ctrl = CreateWindowEx(
        0,
        "STATIC",
        status_text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, 330, 150,
        status_window,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Tamam butonu ekle
    HWND ok_button = CreateWindowEx(
        0,
        "BUTTON",
        "Tamam",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        140, 160, 70, 25,
        status_window,
        (HMENU)IDOK,
        GetModuleHandle(NULL),
        NULL
    );
    
    ShowWindow(status_window, SW_SHOW);
    
    // Mesaj döngüsü
    MSG msg;
    BOOL ret;
    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) break;
        
        if (msg.message == WM_COMMAND && LOWORD(msg.wParam) == IDOK) {
            DestroyWindow(status_window);
            break;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static HMENU create_popup_menu() {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return NULL;
    
    // Durum menü öğesi
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_STATUS, "Durum Bilgisi");
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    
    // Scriptler için alt menü
    HMENU hScriptsMenu = CreatePopupMenu();
    if (!hScriptsMenu) {
        DestroyMenu(hMenu);
        return NULL;
    }
    
    // Scriptleri ekle
    for (unsigned int i = 0; i < scripts_count(); i++) {
        const char *name = script_get(i)->name;
        char display_name[128];
        sanitize_name(name, display_name, sizeof(display_name));
        
        UINT flags = MF_BYPOSITION | MF_STRING;
        if (i == last_selected_idx)
            flags |= MF_CHECKED;
            
        InsertMenu(hScriptsMenu, -1, flags, i + IDM_BASE, display_name);
    }
    
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hScriptsMenu, "Mod Seç");
    
    // Çalıştır/Durdur
    if (is_running) {
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_STOP, "Durdur");
    }
    else {
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_START, "Başlat");
    }
    
    // Ayarlar alt menüsü
    HMENU hSettingsMenu = CreatePopupMenu();
    if (hSettingsMenu) {
        InsertMenu(hSettingsMenu, -1, MF_BYPOSITION | MF_STRING | 
                   (get_autostart_enabled() ? MF_CHECKED : MF_UNCHECKED), 
                   IDM_AUTOSTART, "Windows ile Otomatik Başlat");
        InsertMenu(hSettingsMenu, -1, MF_BYPOSITION | MF_STRING, 
                   IDM_MINIMIZE, "Başlangıçta Simge Durumuna Küçült");
                   
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_POPUP | MF_STRING, 
                   (UINT_PTR)hSettingsMenu, "Ayarlar");
    }
    
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    
    // Hakkında ve Çıkış
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_ABOUT, "Hakkında");
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_EXIT, "Çıkış");
    
    return hMenu;
}

static void build_menu(void)
{
    __try {
        // Önceki menüyü temizle
        if (hMenu) {
            DestroyMenu(hMenu);
        }
        
        hMenu = create_popup_menu();
        if (!hMenu) {
            log_error("Failed to create popup menu");
            return;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        log_error("Exception in build_menu function");
        hMenu = NULL;
    }
}

// Tray simge olaylarını işleme
static void handle_tray_event(HWND hwnd, LPARAM lParam) {
    __try {
        switch(lParam) {
            case WM_LBUTTONDOWN:
                // Sol tıklamada program durumunu değiştir
                if (is_running) {
                    script_stop();
                    is_running = 0;
                }
                else if (last_selected_idx >= 0) {
                    script_start(last_selected_idx);
                    is_running = 1;
                }
                tray_update(is_running);
                break;
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                {
                    POINT pt;
                    
                    // Menüyü yeniden oluştur (her zaman güncel olması için)
                    build_menu();
                    
                    // İmleç konumunu al
                    if (!GetCursorPos(&pt)) {
                        log_error("GetCursorPos failed");
                        pt.x = 0;
                        pt.y = 0;
                    }
                    
                    // Menü gösterilmeden önce pencereyi aktif hale getir
                    // Bu, menünün düzgün kapanmasını garantiler
                    SetForegroundWindow(hwnd);
                    
                    if (hMenu) {
                        // Menüyü göster ve işlem yap
                        UINT flags = TPM_RIGHTBUTTON | TPM_RETURNCMD;
                        int cmd = TrackPopupMenu(hMenu, flags, pt.x, pt.y, 0, hwnd, NULL);
                        
                        // Komut işleme
                        if (cmd) {
                            PostMessage(hwnd, WM_COMMAND, cmd, 0);
                        }
                    }
                    else {
                        log_error("Menu handle is NULL during right click");
                    }
                    
                    // Hayalet tık sorunu için
                    PostMessage(hwnd, WM_NULL, 0, 0);
                }
                break;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        log_error("Exception in handle_tray_event function");
        // Çökme sonrası kurtarma girişimi
        if (hMenu) {
            DestroyMenu(hMenu);
            hMenu = NULL;
            build_menu(); // Menüyü yeniden oluştur
        }
    }
}

static LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l)
{
    // Hata yakalama için try-except bloğu ekleyelim
    __try {
        switch(m)
        {
        case WM_TRAY:
            handle_tray_event(h, l);
            break;
        case WM_COMMAND:
            switch (LOWORD(w)) {
                case IDM_EXIT: 
                    // Programı tamamen kapat ve tüm kalıntıları temizle
                    script_stop(); 
                    tray_update(0); 
                    
                    // Windows otomatik başlatma ayarını kaldır
                    remove_autostart();
                    
                    // Service_remove.cmd scriptini yönetici olarak çalıştır
                    run_service_remove_script();
                    
                    // "last_selected.dat" dosyasını sil
                    remove("last_selected.dat");
                    
                    // Uygulamayı kapat
                    DestroyWindow(h); 
                    break;
                case IDM_STOP: 
                    script_stop(); 
                    tray_update(0); 
                    break;
                case IDM_START:
                    {
                        // Önceki bir işlem zaten çalışıyorsa
                        if (is_running) {
                            int result = show_continue_dialog(h);
                            
                            if (result == IDYES) {
                                restart_with_script(last_selected_idx);
                                tray_update(1); // UI durumunu güncelle
                            }
                            // IDNO durumunda hiçbir şey yapma
                            break;
                        }
                        
                        // Önceden seçilmiş bir script varsa
                        if (last_selected_idx >= 0) {
                            script_start(last_selected_idx);
                            is_running = 1;
                            tray_update(1);
                        } 
                        else {
                            MessageBox(h, "Lütfen önce bir DPI atlatma yöntemi seçin.", 
                                      "GoodbyeDPI", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    break;
                case IDM_STATUS:
                    show_status_window(h); // Durum penceresini göster
                    break;
                default:
                    if (LOWORD(w) >= IDM_BASE) {
                        int idx = LOWORD(w) - IDM_BASE;
                        
                        // Eğer halihazırda DPI atlatma çalışıyorsa kullanıcıya sor
                        if (is_running) {
                            int result = show_continue_dialog(h);
                                
                            if (result == IDYES) {
                                restart_with_script(idx);
                                
                                // Seçilen son indeksi güncelle
                                last_selected_idx = idx;
                                
                                // Her menü öğesini işaretle/işareti kaldır
                                build_menu();
                                tray_update(1);
                            }
                            break;
                        }
                        
                        // Değilse, seçilen scriptleri başlat
                        script_start(idx);
                        is_running = 1;
                        
                        // Seçilen son indeksi güncelle
                        last_selected_idx = idx;
                        
                        // Her menü öğesini işaretle/işareti kaldır
                        build_menu();
                        
                        // Simge durumunu güncelle
                        tray_update(1);
                    }
                    break;
            }
            break;
            
        // Özel yeniden başlatma mesajı
        case WM_APP_RESTART:
            if (is_running) {
                script_stop();
            }
            if (last_selected_idx >= 0) {
                script_run(last_selected_idx);
                tray_update(1);
            } else {
                tray_update(0);
            }
            break;
            
        // Watchdog zamanlayıcısı - uygulama çalışmasını düzenli kontrol et
        case WM_TIMER:
            if (w == IDT_WATCHDOG) {
                // Sistem tepsi ikonunun hala aktif olup olmadığını kontrol et
                // Shell_NotifyIcon başarısız olursa ikon kaybolmuş demektir
                if (!Shell_NotifyIcon(NIM_MODIFY, &nid)) {
                    // Sistem tepsisinden ikon kaybolmuş, tekrar ekle
                    tray_add(h);
                    
                    // DPI çalışır durumdaysa ikon rengini güncelle
                    if (is_running) {
                        tray_update(1);
                    }
                }
            }
            break;
            
        case WM_DESTROY:
            tray_delete();
            if (hMutex) CloseHandle(hMutex); // Mutex'i temizle
            PostQuitMessage(0); 
            break;
            
        default: 
            return DefWindowProc(h,m,w,l);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        log_error("Exception in WndProc function");
        // Daha sağlam hata kurtarma - menüyü yeniden oluştur ve tray simgesini kontrol et
        if (hMenu) {
            DestroyMenu(hMenu);
            hMenu = NULL;
            build_menu();
        }
        
        // Tray simgesini yeniden ekle (kaybolmuşsa)
        if (!tray_added) {
            tray_add(h);
            tray_update(is_running);
        }
    }
    
    return DefWindowProc(h, m, w, l);
}

// Function to save last selected script index when exiting
static void save_last_selected(void) {
    if (last_selected_idx >= 0) {
        FILE *f = fopen("last_selected.dat", "wb");
        if (f) {
            fwrite(&last_selected_idx, sizeof(last_selected_idx), 1, f);
            fclose(f);
        }
    }
}

int WINAPI WinMain(HINSTANCE hI,HINSTANCE,LPSTR lpCmdLine,INT)
{
    // Birden fazla kopya çalışmasını engelle
    hMutex = CreateMutex(NULL, TRUE, "GoodbyeDPI_Singleton_Mutex");
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        // Uygulama zaten çalışıyor, çık
        CloseHandle(hMutex);
        return 0;
    }
    
    // Global exception handler kur
    setup_exception_handler();
    
    // Windows XP ve sonrası için tema desteğini etkinleştir
    InitCommonControls();
    
    // Kurtarma modu kontrolü
    if (lpCmdLine && strstr(lpCmdLine, "-recovery")) {
        error_recovery = 1;
        // Uygulama çöktükten sonra yeniden başlarken biraz bekle
        Sleep(2000);
    }
    
    // COM kütüphanesini başlat (kısayol oluşturmak için gerekli)
    CoInitialize(NULL);
    
    // Common Controls'ü başlat (Modern UI görünümü için)
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX) };
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);
    
    scripts_init();

    // Program açılışta otomatik başlama ayarını etkinleştir
    set_autostart();
    
    // Last selected index'i son seçimi hatırlamak için dosyadan oku
    // (Uygulama kapanıp açıldığında seçim hatırlanacak)
    FILE *f = fopen("last_selected.dat", "rb");
    if (f) {
        fread(&last_selected_idx, sizeof(last_selected_idx), 1, f);
        fclose(f);
        // Geçersiz index kontrolü
        if (last_selected_idx < 0 || last_selected_idx >= scripts_count())
            last_selected_idx = -1;
    }
    
    // Eğer hiç seçim yapılmamışsa ya da geçersiz bir seçim varsa varsayılan script'i seç
    if (last_selected_idx < 0) {
        last_selected_idx = find_default_script();
    }

    WNDCLASS wc={0}; 
    wc.lpfnWndProc=WndProc; 
    wc.hInstance=hI;
    wc.lpszClassName="GoodbyeDPI_GUI";
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_MAIN)); // Ana ikon ata
    RegisterClass(&wc);
    HWND hWnd=CreateWindow("GoodbyeDPI_GUI","",WS_OVERLAPPEDWINDOW,
                           0,0,0,0,NULL,NULL,hI,NULL);

    tray_add(hWnd);
    build_menu();

    // Uygulama kapandığında son seçimi kaydet
    atexit(save_last_selected);
    
    // Watchdog zamanlayıcısı kur - 10 saniyede bir servis durumunu kontrol et
    SetTimer(hWnd, IDT_WATCHDOG, WATCHDOG_INTERVAL, NULL);

    MSG msg; 
    while(GetMessage(&msg,NULL,0,0)) { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }
    
    // COM kütüphanesini kapat
    CoUninitialize();
    
    return 0;
}