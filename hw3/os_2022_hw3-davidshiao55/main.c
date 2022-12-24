#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include "include/shell.h"
#include "include/command.h"
#include "include/task.h"

int main(int argc, char *argv[])
{
	task_manager_initialize(argv[1]);
	history_count = 0;
	for (int i = 0; i < MAX_RECORD_NUM; ++i)
		history[i] = (char *)malloc(BUF_SIZE * sizeof(char));

	shell();

	task_manager_destroy();
	for (int i = 0; i < MAX_RECORD_NUM; ++i)
		free(history[i]);

	return 0;
}
