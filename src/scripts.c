#include "scripts.h"
#include "goodbyedpi.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define MAX_SCRIPTS 32  // Desteklenen maksimum script sayısını artırdım
#define MAX_ARGS 32
#define DEFAULT_SCRIPT_NAME "Any Country DNS Redirect"  // Varsayılan script adını belirledim

static Script scripts[MAX_SCRIPTS];
static int scnt = 0;
static int default_script_index = -1;  // Varsayılan script indeksi

// Batch scriptlerden argümanları çıkar
static void add_script(const char *name, const char *args)
{
    if (scnt >= MAX_SCRIPTS) return;
    strncpy(scripts[scnt].name, name, sizeof(scripts[scnt].name)-1);
    strncpy(scripts[scnt].args, args, sizeof(scripts[scnt].args)-1);
    
    // Varsayılan scripti işaretle
    if (strcmp(name, DEFAULT_SCRIPT_NAME) == 0) {
        default_script_index = scnt;
    }
    
    scnt++;
}

void scripts_init(void)
{
    // Scriptler .cmd dosyalarından aktarıldı - tam yapılandırmalar
    // Önceki .cmd dosyaları incelenip içeriklerindeki parametreler doğrudan buraya aktarıldı
    
    // Ana scriptler
    add_script("Russia Blacklist DNS Redirect", "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253 --blacklist ..\\russia-blacklist.txt");
    add_script(DEFAULT_SCRIPT_NAME, "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("Any Country Standard", "-5");
    
    // Türkiye scriptleri - Turkey klasöründeki .cmd dosyalarından
    add_script("[TR] DNS Redirect", "-5 --set-ttl 5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 2", "-5");
    add_script("[TR] Alternative 3", "--set-ttl 3 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 4", "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 5", "-9 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 6", "-9");
    add_script("[TR] TTL 3 Only", "--set-ttl 3");
}

// Varsayılan script indeksini döndüren fonksiyon
int get_default_script_index(void) {
    // Eğer default_script_index geçerli değilse, ilk Türkiye scriptini dene
    if (default_script_index < 0) {
        for (int i = 0; i < scnt; i++) {
            if (strstr(scripts[i].name, "[TR]")) {
                return i;
            }
        }
        // Türkiye scripti yoksa ilk scripti kullan
        return (scnt > 0) ? 0 : -1;
    }
    return default_script_index;
}

// Tüm script dizisini döndüren fonksiyon
Script* scripts_get_all(void) {
    return scripts;
}

// Temizleme fonksiyonu (gerçek bir bellek temizliği yapılmıyor çünkü statik dizi)
void scripts_free(void) {
    // Şu anda gerçek bir bellek temizliği gerekmiyor, 
    // ileride dinamik bellek tahsisi yapılırsa kullanılabilir
    scnt = 0;
}

int scripts_count(void) { return scnt; }
const Script* script_get(int i) { return (i >= 0 && i < scnt) ? &scripts[i] : NULL; }

// Argümanları boşluklardan ayırıp argv dizisi oluştur
static int parse_args(const char *args, char **argv)
{
    int argc = 1;
    char *safe_buffer;
    
    // argv[0] her zaman program adı
    argv[0] = _strdup("goodbyedpi.exe");
    
    // Güvenli bir buffer oluştur ve argümanları kopyala
    safe_buffer = _strdup(args); // strdup ile bellek ayrılır
    if (!safe_buffer) return argc; // bellek hatası durumunda geri dön
    
    // Buffer string üzerinde tokenizing işlemi
    char *token = strtok(safe_buffer, " ");
    while (token && argc < MAX_ARGS-1) {
        // Her argüman için yeni bellek ayır
        argv[argc] = _strdup(token);
        if (argv[argc]) {
            argc++;
        }
        token = strtok(NULL, " ");
    }
    
    argv[argc] = NULL;
    
    // safe_buffer'ı serbest bırak, argv içerikleri ayrı bellekte
    free(safe_buffer);
    
    return argc;
}

int script_run(int idx)
{
    if (idx < 0 || idx >= scnt) return -1;
    
    // Önce mevcut işlemi düzgün şekilde durdurduğumuzdan emin olalım
    script_stop();
    
    // İşlemin tamamen durması için biraz bekle
    Sleep(300);
    
    // Temiz bir argv dizisi ayır ve sıfırla
    char *argv[MAX_ARGS] = {0};
    
    // Logla
    FILE *logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] Running script: %s with args: %s\n", 
                __FUNCTION__, scripts[idx].name, scripts[idx].args);
        fclose(logfile);
    }
    
    // Argümanları parse et
    int argc = parse_args(scripts[idx].args, argv);
    
    // Argümanları doğrula
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
    
    // goodbyedpi başlat
    int result = start_goodbyedpi(argc, argv);
    
    // Logla
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] start_goodbyedpi result: %d (%s)\n", 
                __FUNCTION__, result, result == 0 ? "success" : "error");
        fclose(logfile);
    }
    
    // start_goodbyedpi kendi kopyasını oluşturduğu için belleği temizle
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
    
    // Önce normal durdurma dene
    stop_goodbyedpi();
    
    // 300ms bekleyerek durdurmanın tamamlanmasını sağla
    Sleep(300);
    
    // Hala çalışıyor mu kontrol et
    if (is_goodbyedpi_running()) {
        logfile = fopen("goodbyedpi_log.txt", "a");
        if (logfile) {
            fprintf(logfile, "[%s] Normal stop failed, forcing termination...\n", __FUNCTION__);
            fclose(logfile);
        }
        
        // Zorla sonlandır - Doğrudan stop_goodbyedpi tekrar çağır
        stop_goodbyedpi();
    }
    
    logfile = fopen("goodbyedpi_log.txt", "a");
    if (logfile) {
        fprintf(logfile, "[%s] GoodbyeDPI stopped successfully\n", __FUNCTION__);
        fclose(logfile);
    }
}