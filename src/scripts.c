#include "scripts.h"
#include "goodbyedpi.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define MAX_SCRIPTS 16
#define MAX_ARGS 32

static Script scripts[MAX_SCRIPTS];
static int scnt = 0;

// Batch scriptlerden argümanları çıkar
static void add_script(const char *name, const char *args)
{
    if (scnt >= MAX_SCRIPTS) return;
    strncpy(scripts[scnt].name, name, sizeof(scripts[scnt].name)-1);
    strncpy(scripts[scnt].args, args, sizeof(scripts[scnt].args)-1);
    scnt++;
}

void scripts_init(void)
{
    // Scriptler doğrudan kodda tanımlı - İngilizce isimlerle
    add_script("Russia Blacklist DNS Redirect", "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253 --blacklist ..\\russia-blacklist.txt");
    add_script("Any Country DNS Redirect", "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("Any Country Standard", "-5");
    // Türkiye scriptleri alt menü için başında [TR] etiketiyle
    add_script("[TR] DNS Redirect", "-5 --set-ttl 5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 2", "-5");
    add_script("[TR] Alternative 3", "--set-ttl 3 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 4", "-5 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 5", "-9 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253");
    add_script("[TR] Alternative 6", "-9");
    add_script("[TR] TTL 3 Only", "--set-ttl 3");
}

int scripts_count(void) { return scnt; }
const Script* script_get(int i) { return (i >= 0 && i < scnt) ? &scripts[i] : NULL; }

// Argümanları boşluklardan ayırıp argv dizisi oluştur
static int parse_args(const char *args, char **argv)
{
    int argc = 1;
    argv[0] = "goodbyedpi.exe";
    char buf[384];
    strncpy(buf, args, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
    char *token = strtok(buf, " ");
    while (token && argc < MAX_ARGS-1) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;
    return argc;
}

int script_run(int idx)
{
    if (idx < 0 || idx >= scnt) return -1;
    stop_goodbyedpi();
    char *argv[MAX_ARGS];
    int argc = parse_args(scripts[idx].args, argv);
    return start_goodbyedpi(argc, argv);
}

void script_stop(void)
{
    stop_goodbyedpi();
}