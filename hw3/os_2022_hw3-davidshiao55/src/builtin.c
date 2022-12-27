#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include "../include/builtin.h"
#include "../include/command.h"
#include "../include/task.h"

int help(char **args)
{
	int i;
	printf("--------------------------------------------------\n");
	printf("My Little Shell!!\n");
	printf("The following are built in:\n");
	for (i = 0; i < num_builtins(); i++)
	{
		printf("%d: %s\n", i, builtin_str[i]);
	}
	printf("%d: replay\n", i);
	printf("--------------------------------------------------\n");
	return 1;
}

int cd(char **args)
{
	if (args[1] == NULL)
	{
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	}
	else
	{
		if (chdir(args[1]) != 0)
			perror("lsh");
	}
	return 1;
}

int echo(char **args)
{
	bool newline = true;
	for (int i = 1; args[i]; ++i)
	{
		if (i == 1 && strcmp(args[i], "-n") == 0)
		{
			newline = false;
			continue;
		}
		printf("%s", args[i]);
		if (args[i + 1])
			printf(" ");
	}
	if (newline)
		printf("\n");

	return 1;
}

int exit_shell(char **args)
{
	return 0;
}

int record(char **args)
{
	if (history_count < MAX_RECORD_NUM)
	{
		for (int i = 0; i < history_count; ++i)
			printf("%2d: %s\n", i + 1, history[i]);
	}
	else
	{
		for (int i = history_count % MAX_RECORD_NUM; i < history_count % MAX_RECORD_NUM + MAX_RECORD_NUM; ++i)
			printf("%2d: %s\n", i - history_count % MAX_RECORD_NUM + 1, history[i % MAX_RECORD_NUM]);
	}
	return 1;
}

bool isnum(char *str)
{
	for (int i = 0; i < strlen(str); ++i)
	{
		if (str[i] >= 48 && str[i] <= 57)
			continue;
		else
			return false;
	}
	return true;
}

int mypid(char **args)
{
	char fname[BUF_SIZE];
	char buffer[BUF_SIZE];
	if (strcmp(args[1], "-i") == 0)
	{

		pid_t pid = getpid();
		printf("%d\n", pid);
	}
	else if (strcmp(args[1], "-p") == 0)
	{

		if (args[2] == NULL)
		{
			printf("mypid -p: too few argument\n");
			return 1;
		}

		sprintf(fname, "/proc/%s/stat", args[2]);
		int fd = open(fname, O_RDONLY);
		if (fd == -1)
		{
			printf("mypid -p: process id not exist\n");
			return 1;
		}

		read(fd, buffer, BUF_SIZE);
		strtok(buffer, " ");
		strtok(NULL, " ");
		strtok(NULL, " ");
		char *s_ppid = strtok(NULL, " ");
		int ppid = strtol(s_ppid, NULL, 10);
		printf("%d\n", ppid);

		close(fd);
	}
	else if (strcmp(args[1], "-c") == 0)
	{

		if (args[2] == NULL)
		{
			printf("mypid -c: too few argument\n");
			return 1;
		}

		DIR *dirp;
		if ((dirp = opendir("/proc/")) == NULL)
		{
			printf("open directory error!\n");
			return 1;
		}

		struct dirent *direntp;
		while ((direntp = readdir(dirp)) != NULL)
		{
			if (!isnum(direntp->d_name))
			{
				continue;
			}
			else
			{
				sprintf(fname, "/proc/%s/stat", direntp->d_name);
				int fd = open(fname, O_RDONLY);
				if (fd == -1)
				{
					printf("mypid -p: process id not exist\n");
					return 1;
				}

				read(fd, buffer, BUF_SIZE);
				strtok(buffer, " ");
				strtok(NULL, " ");
				strtok(NULL, " ");
				char *s_ppid = strtok(NULL, " ");
				if (strcmp(s_ppid, args[2]) == 0)
					printf("%s\n", direntp->d_name);

				close(fd);
			}
		}

		closedir(dirp);
	}
	else
	{
		printf("wrong type! Please type again!\n");
	}

	return 1;
}

int add(char **args)
{
	int p;
	if (args[3])
		sscanf(args[3], "%d", &p);
	else
		p = 0;
	addTask(args[1], args[2], p);
	return 1;
}

int del(char **args)
{
	deleteTask(args[1]);
	return 1;
}

int ps(char **args)
{
	printTask();
	return 1;
}

int start(char **args)
{
	startSimulation();
	return 1;
}

const char *builtin_str[] = {
	"help",
	"cd",
	"echo",
	"exit",
	"record",
	"mypid",
	"add",
	"del",
	"ps",
	"start"};

const int (*builtin_func[])(char **) = {
	&help,
	&cd,
	&echo,
	&exit_shell,
	&record,
	&mypid,
	&add,
	&del,
	&ps,
	&start};

int num_builtins()
{
	return sizeof(builtin_str) / sizeof(char *);
}
