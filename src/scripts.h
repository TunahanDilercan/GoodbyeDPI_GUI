#ifndef SCRIPTS_H
#define SCRIPTS_H

typedef struct {
    char name[128];
    char args[256];
} Script;

void scripts_init(void);
int  scripts_count(void);
const Script* script_get(int);
int  script_run(int idx);
void script_stop(void);
Script* scripts_get_all(void);
void scripts_free(void);

#endif
