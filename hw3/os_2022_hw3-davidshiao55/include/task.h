#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <ucontext.h>
#include "resource.h"

#define FCFS 0
#define RR 1
#define PP 2

#define READY 0
#define WAITING 1
#define RUNNING 2
#define TERMINATED 3

typedef struct Task_context
{
    ucontext_t ctx;
    int stack[1024 * 128];
} Task_context;

typedef struct Task
{
    int tid;
    char *task_name;
    int function;
    int task_state;
    int running_time;
    int waiting_time;
    int turnaround;
    bool resources[MAX_RESOURCE];
    int priority;
    Task_context *task_context;
    int sleep_time;
    int *waiting_resources;
    int waiting_resources_count;
} Task;

void task_manager_initialize(const char *inputString);
void task_manager_destroy();

void task_sleep(int);
void task_exit();
void task_wait_resource(int count, int *resources);
void addTask(char *taskName, char *functionName, int priority);
Task *newTask(char *taskName, char *functionName, int priority);
void deleteTask(char *taskName);
void destroyTask(Task *t);
void printTask();
void taskInfo(Task *t);
void startSimulation();
void *simulation(void *vargp);

Task_context *newContext(int f);
void deleteContext(Task *t);

void my_alarm_handler(int sig);
void my_stp_handler(int sig);

extern Task *curr_task;
#endif
