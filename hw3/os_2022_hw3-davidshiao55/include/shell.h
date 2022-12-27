#ifndef SHELL_H
#define SHELL_H

#include "command.h"

int execute(struct pipes *);
int spawn_proc(int, int, struct cmd *, struct pipes *);
int fork_pipes(struct cmd *);
void shell();

extern char *history[MAX_RECORD_NUM];
extern int history_count;

#endif
