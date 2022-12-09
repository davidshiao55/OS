#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFSIZE 1024
#define MAXTOKENS 64
#define TOK_DELIM " \t\r\n\a"
#define MAXPIPE 5
#define EXCEED_BUFFER 2
#define REPLAYEROOR 3

typedef struct launchType
{
	int pipeNum;
	int pipeLoc[MAXPIPE];
	int builtIn[MAXPIPE + 1]; // command = pipeNum + 1
	bool outredirect[MAXPIPE + 1];
	bool inredirect[MAXPIPE + 1];
	bool background;
} launchType;

typedef struct shellCommand
{
	int numArgs;
	char *lineInput;
	char **parseInput; // parseInput is mapped to the starting char of each args in lineInput
	launchType t;
} shellCommand;

typedef struct element
{
	char command[BUFSIZE];
	struct element *link;
} element;

// cd & exit can't be piped
char *builtin[] = {"help", "cd", "exit", "echo", "record", "replay", "mypid"};

void shellLoop();
void typeReset(launchType *t);
int readInput(char **input);
int parseInput(char *line, char ***args, launchType *t);
void launch(char **arg, launchType t, int i);
void cmdHanler(shellCommand sc);
int singleProcessHandler(char **arg, launchType t);
void multiPipev(char **args, launchType t, int n);
void record();
void replay();
void help();
void echo(char **args, launchType t);
void mypid(char **args, launchType t);

void push(char *input);
shellCommand newCommand(char *input);
element pop();
element *createElement(char *input);

element *front = NULL;
element *rear = NULL;
int queueSize = 0;

pid_t shell_pid;

int main()
{
	shell_pid = getpid();
	printf("=================================================\n");
	printf("*Welcome to my shell                            *\n");
	printf("*                                               *\n");
	printf("*Type \"help\" to see builtin functions.          *\n");
	printf("*                                               *\n");
	printf("*If you want to do thins below:                 *\n");
	printf("+ redirection : \">\" or \"<\"                      *\n");
	printf("+ pipe : \"|\"                                    *\n");
	printf("+ background : \"&\"                              *\n");
	printf("+ Make sure they are seperated by \"(space)\"     *\n");
	printf("*                                               *\n");
	printf("*Have fun!!                                     *\n");
	printf("=================================================\n");

	shellLoop();

	return 0;
}

void shellLoop()
{
	char *input = NULL;

	shellCommand sc;
	while (1)
	{
		printf(">>> $ ");			// print prompt
		if (readInput(&input) == 1) // read input
		{
			push(input);			// record input
			sc = newCommand(input); // contruct struct tpye shellCommand
			cmdHanler(sc);			// execute command
			free(sc.parseInput);
		}
		if (input)
			free(input);
	}
}

void cmdHanler(shellCommand sc)
{
	if (sc.t.pipeNum > 0)
		multiPipev(sc.parseInput, sc.t, sc.t.pipeNum);
	else
		singleProcessHandler(sc.parseInput, sc.t);
}

void multiPipev(char **args, launchType t, int n)
{
	int status;
	int pipes[2 * n];
	pid_t pids[2 * n];
	pid_t pid1, pid2;
	int count = 0;

	for (int i = 0; i < n; i++)
	{
		if (pipe(&pipes[2 * i]) < 0)
		{
			fprintf(stderr, "pipe error\n");
			exit(EXIT_FAILURE);
		}
	}

	// fork the first child
	pid1 = fork();
	if (pid1 < 0)
	{
		fprintf(stderr, "fork error\n");
		exit(EXIT_FAILURE);
	}
	else if (pid1 == 0)
	{
		// replace 1st child stdout with write end of 1st pipe
		dup2(pipes[1], STDOUT_FILENO);

		// close all pipes (very important!); end we're using was safely copied
		for (int j = 0; j < 2 * n; j++)
		{
			close(pipes[j]);
		}
		launch(args, t, 0);
	}
	else
	{
		// record pid to the pidlist
		pids[count++] = pid1;

		for (int i = 0; i < 2 * n; i += 2)
		{
			// fork child to recieve from previous loop
			pid2 = fork();
			if (pid2 < 0)
			{
				fprintf(stderr, "fork error\n");
				exit(EXIT_FAILURE);
			}

			// fork second child (to execute grep)
			else if (pid2 == 0)
			{
				// replace child stdin with previous pipe
				dup2(pipes[i], STDIN_FILENO);

				// if it's not the last child replace child stdout with next pipe
				if (i < 2 * (n - 1))
					dup2(pipes[i + 3], STDOUT_FILENO);

				// close all ends of pipes
				for (int j = 0; j < 2 * n; j++)
				{
					close(pipes[j]);
				}

				launch(&args[t.pipeLoc[i / 2]], t, i / 2 + 1);
			}
			else
			{
				// record pids
				pids[count++] = pid2;
			}
		}
	}

	// only the parent gets here and waits for all children to finish
	for (int i = 0; i < 2 * n; i++)
	{
		close(pipes[i]);
	}

	// if command is in background -> don't wait
	if (t.background)
	{
		printf("[Pid]: %d\n", pids[count - 1]);
		return;
	}

	// wait for all child on the pidlist
	for (int i = 0; i < count; i++)
	{
		waitpid(pids[i], &status, 0);
	}
}

int singleProcessHandler(char **args, launchType t)
{
	pid_t pid;
	int status;
	pid = fork();
	if (pid == 0)
	{
		launch(args, t, 0);
	}
	else if (pid < 0)
	{
		perror("fork error\n");
	}
	else
	{
		if (!t.background)
			waitpid(pid, &status, 0);
		else
			printf("[Pid]: %d\n", pid);

		// handle command that can't be run on child process
		// these command can't be piped
		if (t.builtIn[0] == 3)
			exit(EXIT_SUCCESS);
		else if (t.builtIn[0] == 2)
			chdir(args[1]);
	}
	return 1;
}

void launch(char **args, launchType t, int i)
{
	// check for redirection
	if (t.inredirect[i])
	{
		// get file name
		char *file;
		for (int i = 0;; i++)
		{
			if (!strcmp(args[i], "<"))
			{
				args[i] = NULL;
				file = args[i + 1];
				break;
			}
		}
		// change process stdin to file
		int fd0 = open(file, O_RDONLY);
		dup2(fd0, STDIN_FILENO);
		close(fd0);
	}

	if (t.outredirect[i])
	{
		// get file name
		char *file = *args;
		for (int i = 0;; i++)
		{
			if (!strcmp(args[i], ">"))
			{
				args[i] = NULL;
				file = args[i + 1];
				break;
			}
		}
		// change process stdout to file
		int fd1 = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		dup2(fd1, STDOUT_FILENO);
		close(fd1);
	}

	if (t.builtIn[i] > 0)
	{
		//{"help", "cd", "exit", "echo", "record", "replay", "mypid"};
		switch (t.builtIn[i])
		{
		case 1:
			help();
			break;
		case 2:
			break;
		case 3:
			printf("GOODBYYYYYYYE!\n");
			break;
		case 4:
			echo(args, t);
			break;
		case 5:
			record();
			break;
		case 6:
			break;
		case 7:
			mypid(args, t);
			break;
		}
		exit(EXIT_SUCCESS);
	}
	else
	{
		if (execvp(args[0], args) == -1)
		{
			perror("launch fail\n");
		}
		exit(EXIT_FAILURE);
	}
}

// replay is handle here
int readInput(char **input)
{
	int pos = 0;
	int accept = 0;
	char localBuffer[10];
	int prevSpace = 0;
	*input = (char *)malloc(sizeof(char) * BUFSIZE);

	if (!input)
	{
		fprintf(stderr, "allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (pos < BUFSIZE)
	{
		int c = getchar();

		if (c == '\n' || c == EOF)
		{
			(*input)[pos] = '\0';
			return accept;
		}
		else if (c == ' ')
		{
			prevSpace = pos;
		}
		(*input)[pos++] = c;

		// check for replay every input
		strncpy(localBuffer, *input + prevSpace, 6);
		localBuffer[6] = '\0';
		if (!strcmp(localBuffer, "replay"))
		{
			int replayParam = 0, l;
			char c;
			element *tmp = front;
			getchar(); // get next space

			// record replay param
			while ((c = getchar()) != ' ' && (c != '\n'))
			{
				replayParam = replayParam * 10 + c - '0';
			}

			// get corresponding command in queue
			if (replayParam > queueSize || replayParam < 1)
			{
				fprintf(stderr, "replay: wrong args\n");
				return REPLAYEROOR;
			}
			for (int i = 1; i < replayParam; i++)
				tmp = tmp->link;

			// replace replay command with the correspong command in queue
			l = strlen(tmp->command);
			strncpy(*input + prevSpace, tmp->command, l);
			pos = prevSpace + l;

			// if it isn't a piped command the return
			if (c == '\n')
			{
				(*input)[pos] = '\0';
				return accept;
			}
			(*input)[pos++] = ' ';
		}

		// check for empty input
		if (!accept)
		{
			if (c != ' ' && c != '\t')
				accept = 1;
		}
	}
	fprintf(stderr, "exceed buffer size\n");
	return EXCEED_BUFFER;
}

int parseInput(char *input, char ***args, launchType *t)
{
	int pos = 0;
	char **tokens = malloc(sizeof(char *) * MAXTOKENS);
	char *token;

	if (!tokens)
	{
		fprintf(stderr, "allocation error\n");
		exit(EXIT_FAILURE);
	}

	*args = tokens;
	token = strtok(input, TOK_DELIM);

	// maps args[i] to start of each argument in input
	while (token != NULL)
	{
		tokens[pos++] = token;

		// check for special character
		if (!strcmp(token, "|"))
		{
			t->pipeLoc[t->pipeNum++] = pos;
		}
		else if (!strcmp(token, ">"))
		{
			t->outredirect[t->pipeNum] = true;
		}
		else if (!strcmp(token, "<"))
		{
			t->inredirect[t->pipeNum] = true;
		}

		token = strtok(NULL, TOK_DELIM);
	}

	if (!strcmp(tokens[pos - 1], "&"))
	{
		t->background = true;
		pos--; // set the & arg to NULL
	}

	// check whether command is built-in
	for (int i = 0; i < 7; i++)
	{
		if (!strcmp((*args)[0], builtin[i]))
		{
			t->builtIn[0] = i + 1;
			break;
		}
	}
	for (int i = 0; i < t->pipeNum; i++)
	{
		tokens[t->pipeLoc[i] - 1] = NULL;
		for (int j = 0; j < 7; j++)
		{
			if (!strcmp((*args)[t->pipeLoc[i]], builtin[j]))
			{
				t->builtIn[i + 1] = i + 1;
				break;
			}
		}
	}
	tokens[pos] = NULL;

	return pos;
}

void record()
{
	// print queue
	int i = 0;
	for (element *tmp = front; tmp; tmp = tmp->link)
	{
		printf("%2d: %s\n", i + 1, tmp->command);
		i++;
	}
}

void help()
{
	printf("------------------------------------------------------------\n");
	printf("my shell\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("\n");
	printf("The following are built in :\n");
	printf("1: help:   show all buid-in function info\n");
	printf("2: cd:     change directory\n");
	printf("3: echo:   echo the strings to standard output\n");
	printf("4: record: show last-16 cmds you typed in\n");
	printf("5: replay: re-singleProcessHandler the cmd showed in record\n");
	printf("6: mypid:  findand print process-ids\n");
	printf("7: exit:   exit shell\n\n");
	printf("Use the \"man\" command for information on other programs\n");
	printf("------------------------------------------------------------\n");
}

void echo(char **args, launchType t)
{
	bool newline = true;
	int index = 1;

	// check first args
	if (!strcmp(args[1], "-n"))
	{
		newline = false;
		index++;
	}

	while (args[index])
	{
		printf("%s ", args[index++]);
	}

	if (newline)
		printf("\n");
}

void mypid(char **args, launchType t)
{
	if (!strcmp(args[1], "-i"))
	{
		if (!t.background)
			printf("%d\n", shell_pid);
		else
			printf("%d\n", getpid());
	}
	else if (!strcmp(args[1], "-p"))
	{
		char loc[6 + 7 + 1 + strlen(args[2])];
		strcpy(loc, "/proc/");
		strncpy(loc + 6, args[2], strlen(args[2]));
		strncpy(loc + 6 + strlen(args[2]), "/status\0", 8);

		char *com[] = {"grep", "PPid", loc, NULL, "cut", "-f2", NULL};
		launchType _t;
		_t.pipeNum = 1;
		_t.pipeLoc[0] = 4;
		_t.background = false;
		for (int i = 0; i <= _t.pipeNum; i++)
		{
			_t.builtIn[i] = 0;
			_t.inredirect[i] = false;
			_t.outredirect[i] = false;
		}
		multiPipev(com, _t, 1);
	}
	else if (!strcmp(args[1], "-c"))
	{
		char loc[23 + strlen(args[2]) * 2];
		strcpy(loc, "/proc/");
		strncpy(loc + 6, args[2], strlen(args[2]));
		strncpy(loc + 6 + strlen(args[2]), "/task/", 7);
		strncpy(loc + 12 + strlen(args[2]), args[2], strlen(args[2]));
		strncpy(loc + 12 + strlen(args[2]) * 2, "/children\0", 10);
		execlp("cat", "cat", loc, NULL);
	}
}

void typeReset(launchType *t)
{
	t->background = false;
	for (int i = 0; i < MAXPIPE + 1; i++)
	{
		t->builtIn[i] = 0;
		t->outredirect[i] = false;
		t->inredirect[i] = false;
		if (i < MAXPIPE)
			t->pipeLoc[i] = 0;
	}
	t->pipeNum = 0;
}

void push(char *command)
{
	element *e = createElement(command);
	if (!rear)
	{
		front = e;
		rear = e;
	}
	else
	{
		rear->link = e;
		rear = e;
	}
	queueSize++;
	if (queueSize > 16)
		pop();
}

element pop()
{
	if (!front)
	{
		fprintf(stderr, "queue empty\n");
		exit(EXIT_FAILURE);
	}
	element *deleteElement = front;
	element returnElement = *front;
	front = front->link;
	queueSize--;

	free(deleteElement);
	return returnElement;
}

element *createElement(char *input)
{
	element *n = (element *)malloc(sizeof(element));
	strcpy(n->command, input);

	n->link = NULL;
	return n;
}

shellCommand newCommand(char *input)
{
	shellCommand ns;
	ns.lineInput = input;
	typeReset(&ns.t);
	ns.numArgs = parseInput(ns.lineInput, &(ns.parseInput), &(ns.t));
	return ns;
}
