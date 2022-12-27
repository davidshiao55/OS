#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ucontext.h>
#include <stdbool.h>
#include <pthread.h>
#include "../include/task.h"
#include "../include/function.h"
#include "../include/queue.h"
#include "../include/heap.h"

Queue *ready_queue;
Queue *task_queue;
Queue *waiting_queue;
int algorithm;
int taskCount = 0;
bool sim = false;
Task *curr_task = NULL;
ucontext_t ctx_main;
void (*funcs[9])() = {task1, task2, task3, task4, task5, task6, task7, task8, task9};

void my_alarm_handler(int sig)
{
    static int rr_count = 0;
    if (curr_task)
    {
        curr_task->running_time++;
        if (algorithm == RR)
        {
            rr_count++;
            if (rr_count == 3)
            {
                rr_count = 0;
                curr_task->task_state = READY;
                enQueue(ready_queue, curr_task);
                swapcontext(&(curr_task->task_context->ctx), &ctx_main);
            }
        }
    }

    QNode *rNode = ready_queue->front;
    while (rNode)
    {
        rNode->t->waiting_time++;
        rNode = rNode->next;
    }

    QNode *wNode = waiting_queue->front;
    while (wNode)
    {
        wNode->t->waiting_time++;
        wNode->t->sleep_time--;
        if (wNode->t->sleep_time <= 0)
        {
            // printf("waking up task %s\n", wNode->t->task_name);
            wNode->t->task_state = READY;
            enQueue(ready_queue, wNode->t);
            deleteQueue(waiting_queue, wNode->t);
            break;
        }
        wNode = wNode->next;
    }
}

void my_stp_handler(int sig)
{
    // stop simulation
    sim = false;
}

void task_manager_initialize(const char *inputString)
{
    if (!strcmp(inputString, "FCFS"))
        algorithm = FCFS;
    else if (!strcmp(inputString, "RR"))
        algorithm = RR;
    else if (!strcmp(inputString, "PP"))
        algorithm = PP;

    task_queue = createQueue();
    waiting_queue = createQueue();
    ready_queue = createQueue();

    signal(SIGTSTP, my_stp_handler);
}

void task_manager_destroy()
{
    detroyQueue(ready_queue, false);
    detroyQueue(waiting_queue, false);
    detroyQueue(task_queue, true);
}

void startSimulation()
{
    pthread_t tid;
    sim = true;
    pthread_create(&tid, NULL, simulation, NULL);
}

void *simulation(void *vargp)
{
    /* set timer */
    struct itimerval tv;
    tv.it_interval.tv_usec = 10000;
    tv.it_interval.tv_sec = 0;
    tv.it_value.tv_usec = 10000;
    tv.it_value.tv_sec = 0;

    if (setitimer(ITIMER_REAL, &tv, NULL) < 0)
    {
        perror("settimer error.\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, my_alarm_handler);

    while (sim)
    {
        if (ready_queue->front)
        {
            curr_task = ready_queue->front->t;
            curr_task->task_state = RUNNING;
            deQueue(ready_queue);
            printf("Task %s is running.\n", curr_task->task_name);
            swapcontext(&ctx_main, &(curr_task->task_context->ctx));
            /* if task is terminated than release context & stack : seg fault*/
            // if (curr_task->task_state == TERMINATED)
            //     deleteContext(curr_task);
            curr_task = NULL;
        }
        else
        {
            if (!waiting_queue->front)
            {
                break;
            }
            /* ready q empty but waiting queue isn't */
            else
            {
                printf("CPU idle\n");
                while (waiting_queue->front)
                {
                }
            }
        }
    }
    printf("SIMULATION END\n");

    // clear timer
    tv.it_value.tv_usec = 0;
    tv.it_value.tv_sec = 0;
    if (setitimer(ITIMER_REAL, &tv, NULL) < 0)
    {
        perror("settimer error.\n");
        exit(EXIT_FAILURE);
    }
    pthread_exit(NULL);
}

void task_sleep(int ms)
{
    printf("Task %s goes to sleep.\n", curr_task->task_name);
    curr_task->task_state = WAITING;
    curr_task->sleep_time = ms / 10;
    enQueue(waiting_queue, curr_task);
    swapcontext(&(curr_task->task_context->ctx), &ctx_main);
}

void task_exit()
{
    printf("Task %s is killed.\n", curr_task->task_name);
    curr_task->task_state = TERMINATED;
    curr_task->turnaround = curr_task->running_time + curr_task->waiting_time;
    swapcontext(&(curr_task->task_context->ctx), &ctx_main);
}

void addTask(char *taskName, char *functionName, int priority)
{
    Task *t = newTask(taskName, functionName, priority);
    switch (algorithm)
    {
    case FCFS:
        enQueue(task_queue, t);
        enQueue(ready_queue, t);
        break;
    case RR:
        enQueue(task_queue, t);
        enQueue(ready_queue, t);
        break;
    case PP:
        enQueue(task_queue, t);
        priorityenQueue(ready_queue, t);
        break;
    }
    printf("Task %s is ready\n", t->task_name);
}

void deleteTask(Task *t)
{
}

void printTask()
{
    printf(" TID|\tname|\t     state|\trunning|\twaiting|\tturnaround|\t  resources|\tpriority\n");
    for (int i = 0; i < 107; i++)
        printf("-");
    printf("\n");
    QNode *tmp = task_queue->front;
    while (tmp)
    {
        taskInfo(tmp->t);
        tmp = tmp->next;
    }
}

void taskInfo(Task *t)
{
    printf("%4d|", t->tid);
    printf("%7s|", t->task_name);
    switch (t->task_state)
    {
    case READY:
        printf("        READY|");
        break;
    case TERMINATED:
        printf("   TERMINATED|");
        break;
    case WAITING:
        printf("      WAITING|");
        break;
    case RUNNING:
        printf("      RUNNING|");
        break;
    }
    printf("\t%7d|", t->running_time);
    printf("\t%7d|", t->waiting_time);
    if (t->turnaround)
        printf("\t%10d|", t->turnaround);
    else
        printf("\t      none|");
    char buffer[18] = "                |";
    int pos = 15;
    for (int i = 0; i < MAX_RESOURCE; i++)
    {
        if (t->resources[i])
        {
            buffer[pos--] = i + '0';
            buffer[pos--] = ' ';
        }
    }
    if (pos == 15)
        strcpy(buffer, "            none|");
    printf("%s", buffer);
    printf("\t%8d\n", t->priority);
}

Task_context *newContext(int f)
{
    Task_context *tc = (Task_context *)malloc(sizeof(Task_context));
    getcontext(&tc->ctx);
    tc->ctx.uc_stack.ss_sp = tc->stack;
    tc->ctx.uc_stack.ss_size = sizeof(tc->stack);
    tc->ctx.uc_stack.ss_flags = 0;
    tc->ctx.uc_link = &ctx_main;
    makecontext(&tc->ctx, funcs[f - 1], 0);
    return tc;
}

void deleteContext(Task *t)
{
    printf("DELETE task context %s\n", t->task_name);
    free(t->task_context);
    t->task_context = NULL;
}

Task *newTask(char *taskName, char *functionName, int priority)
{
    Task *t = (Task *)malloc(sizeof(Task));
    t->tid = ++taskCount;
    t->task_name = malloc(sizeof(char) * strlen(taskName) + 1);
    strcpy(t->task_name, taskName);
    t->function = functionName[4] - '0';
    t->priority = priority;
    t->running_time = 0;
    t->waiting_time = 0;
    t->sleep_time = 0;
    t->turnaround = 0;
    t->task_state = READY;
    t->task_context = newContext(t->function);
    for (int i = 0; i < MAX_RESOURCE; i++)
        t->resources[i] = false;
    return t;
}

void destroyTask(Task *t)
{
    if (t->task_context)
        free(t->task_context);
    free(t->task_name);
    free(t);
}
